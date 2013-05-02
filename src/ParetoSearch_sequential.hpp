/*
 * A implementation of a bi-objective shortest path label setting algorithm
 * that always relaxes _all_ globaly pareto optimal labels instead of just one.
 *  
 * Author: Stephan Erb
 */
#ifndef SEQUENTIAL_PARETO_SEARCH_H_
#define SEQUENTIAL_PARETO_SEARCH_H_

#include "utility/datastructure/graph/GraphMacros.h"
#include "options.hpp"

#include "ParetoQueue_sequential.hpp"
#include "ParetoSearchStatistics.hpp"
#include <algorithm>
#include "Label.hpp"

#include "ParetoLabelSet_sequential.hpp"
#include "radix_sort.hpp"

#ifdef GATHER_SUBCOMPNENT_TIMING
#include "tbb/tick_count.h"
#endif

template<typename graph_slot>
class ParetoSearch {
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;

	struct GroupLabelsByWeightComp {
		inline bool operator() (const Label& i, const Label& j) const {
			return i.combined() < j.combined(); 
		}
	} groupLabels;
	

	#ifdef BTREE_PARETO_LABELSET
		typedef BtreeParetoLabelSet<Label, GroupLabelsByWeightComp, std::allocator<Label>> LabelSet;
	#else
		typedef VectorParetoLabelSet<std::allocator<Label>> LabelSet;
	#endif

	struct LabelSetStruct {
		#ifdef BUCKET_SORT
			LabelVec candidates;
		#endif
		LabelSet labels;
	};
	std::vector<LabelSetStruct> labels;

	const graph_slot& graph;
	ParetoQueue pq;
	ParetoSearchStatistics<Label> stats;

	#ifdef GATHER_SUBCOMPNENT_TIMING
		enum Component {FIND_PARETO_MIN=0, CANDIDATE_SORT=1, UPDATE_LABELSETS=2, UPDATES_SORT=3, PQ_UPDATE=4};
		double timings[5] = {0, 0, 0, 0, 0};
	#endif

	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> set_insertions;
		std::vector<unsigned long> set_dominations;
	#endif

	#ifdef BTREE_PARETO_LABELSET
		typename LabelSet::ThreadLocalLSData labelset_data;
	#endif

public:
	ParetoSearch(const graph_slot& graph_):
		labels(graph_.numberOfNodes()), 
		graph(graph_)
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_insertions(101)
			,set_dominations(101)
		#endif
	{
		#ifdef BTREE_PARETO_LABELSET
			labelset_data.spare_leaf = labels[0].labels.allocate_leaf_without_count();
		#endif
	 }

	~ParetoSearch() {
		#ifdef BTREE_PARETO_LABELSET
			labels[0].labels.free_node_without_count(labelset_data.spare_leaf);
		#endif
	}

	void run(const NodeID node) {
		std::vector<Operation<NodeLabel> > updates;
		std::vector<NodeID> affected_nodes;
		std::vector<NodeLabel> candidates;
		pq.init(NodeLabel(node, Label(0,0)));
		#ifdef BTREE_PARETO_LABELSET
			labels[node].labels.init(Label(0,0), labelset_data);
		#else
			labels[node].labels.init(Label(0,0));
		#endif

		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif

		while (!pq.empty()) {
			stats.report(ITERATION, pq.size());

			pq.findParetoMinima(updates, candidates, graph);
			const size_t minima_count = updates.size();
			stats.report(MINIMA_COUNT, minima_count);

			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[FIND_PARETO_MIN] += (stop-start).seconds();
				start = stop;
			#endif

			#ifdef RADIX_SORT
				radix_sort(candidates.data(), candidates.size(), [](const NodeLabel& x) { return x.node; });
			#else 
				std::sort(candidates.begin(), candidates.end(), groupCandidates);
			#endif
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[CANDIDATE_SORT] += (stop-start).seconds();
				start = stop;
			#endif

			const auto cand_end = candidates.end();
			auto cand_iter = candidates.begin();
			while (cand_iter != cand_end) {
				// find all labels belonging to the same target node
				auto range_start = cand_iter;
				auto& ls = labels[range_start->node];
				ls.labels.prefetch();
				while (cand_iter != cand_end && range_start->node == cand_iter->node) {
					++cand_iter;
				}
				std::sort(range_start, cand_iter, groupLabels);
				#ifdef BTREE_PARETO_LABELSET
					ls.labels.updateLabelSet(range_start->node, range_start, cand_iter, updates, labelset_data);
				#else
					ls.labels.updateLabelSet(range_start->node, range_start, cand_iter, updates, stats);
				#endif
			}
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[UPDATE_LABELSETS] += (stop-start).seconds();
				start = stop;
			#endif

			// Sort sequence for batch update
			std::sort(updates.begin()+minima_count, updates.end(), groupOpsByWeight);
			std::inplace_merge(updates.begin(), updates.begin()+minima_count, updates.end(), groupOpsByWeight);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[UPDATES_SORT] += (stop-start).seconds();
				start = stop;
			#endif

			const size_t pre_update_size = pq.size();
			stats.report(UPDATE_COUNT, updates.size());
			pq.applyUpdates(updates);
			stats.report(PQ_SIZE_DELTA, std::abs((signed long)pq.size()-(signed long)pre_update_size));

			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[PQ_UPDATE] += (stop-start).seconds();
				start = stop;
			#endif

			updates.clear();
			candidates.clear();
			affected_nodes.clear();
		}		
	}
	
	void printStatistics() {
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			for (auto& ls : labels) {
				for (size_t i=0; i < 101; ++i) {
					set_insertions[i] += ls.labels.set_insertions[i];
					set_dominations[i] += ls.labels.set_dominations[i];
				}
			}
			std::cout << "# LabelSet Modifications: insertion/deletion, dominance position" << std::endl;
			for (size_t i=0; i < 101; ++i) {
				std::cout << i << " " << set_insertions[i] << " " << set_dominations[i] << std::endl;
			}
		#endif
		std::cout << stats.toString(labels) << std::endl;
		#ifdef GATHER_SUBCOMPNENT_TIMING
			std::cout << "# Subcomponent Timings:" << std::endl;
			std::cout << "#   " << timings[FIND_PARETO_MIN]  << " Find Pareto Min" << std::endl;
			std::cout << "#   " << timings[CANDIDATE_SORT]  << " Group Candidates" << std::endl;
			std::cout << "#   " << timings[UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			std::cout << "#   " << timings[UPDATES_SORT] << " Sort Updates"  << std::endl;
			std::cout << "#   " << timings[PQ_UPDATE] << " Update PQ " << std::endl;
		#endif
		pq.printStatistics();
	}

	size_t size(NodeID node) const { return labels[node].labels.size(); }
	std::vector<Label>::iterator begin(NodeID node) { return labels[node].labels.begin(); }
	std::vector<Label>::const_iterator begin(NodeID node) const { return labels[node].labels.begin(); }
	std::vector<Label>::iterator end(NodeID node) { return labels[node].labels.end(); }
	std::vector<Label>::const_iterator end(NodeID node) const { return labels[node].labels.end(); }


private:

	struct GroupNodeLabelsByNodeComp {
		inline bool operator() (const NodeLabel& i, const NodeLabel& j) const {
			return i.node < j.node;
		}
	} groupCandidates;

	struct GroupOpsByWeightComp {
		inline bool operator() (const Operation<NodeLabel>& i, const Operation<NodeLabel>& j) const {
			if (i.data.first_weight == j.data.first_weight) {
				if (i.data.second_weight == j.data.second_weight) {
					return i.data.node < j.data.node;
				}
				return i.data.second_weight < j.data.second_weight;
			}
			return i.data.first_weight < j.data.first_weight;
		}
	} groupOpsByWeight;

	static inline Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}
};


#endif 
