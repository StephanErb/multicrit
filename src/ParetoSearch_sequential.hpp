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
#include "ParetoLabelSet_sequential.hpp"
#include "ParetoSearchStatistics.hpp"
#include <algorithm>
#include "Label.hpp"

#define BTREE_PARETO_LABELSET

//#define RADIX_SORT
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
			if (i.first_weight == j.first_weight) {
				return i.second_weight < j.second_weight;
			}
			return i.first_weight < j.first_weight;
		}
	} groupLabels;
	

	#ifdef BTREE_PARETO_LABELSET
		typedef BtreeParetoLabelSet<Label, GroupLabelsByWeightComp> LabelSet;
	#else
		typedef VectorParetoLabelSet LabelSet;
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
			labelset_data.weightdelta.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
			labelset_data.local_updates.reserve(LARGE_ENOUGH_FOR_MOST);
			labelset_data.leaves.reserve(LARGE_ENOUGH_FOR_MOST);
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

		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif

		while (!pq.empty()) {
			stats.report(ITERATION, pq.size());

			pq.findParetoMinima(updates);
			const size_t minima_count = updates.size();
			stats.report(MINIMA_COUNT, minima_count);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[FIND_PARETO_MIN] += (stop-start).seconds();
				start = stop;
			#endif

			#ifdef BUCKET_SORT
				for (const auto& op : updates) {
					const NodeLabel& min = op.data;
					FORALL_EDGES(graph, min.node, eid) {
						const Edge& edge = graph.getEdge(eid);
						auto& c = labels[edge.target].candidates;
						if (c.empty()) {
							affected_nodes.push_back(edge.target);
						}
						c.push_back(createNewLabel(min, edge));
					}
				}
				#ifdef GATHER_SUBCOMPNENT_TIMING
					stop = tbb::tick_count::now();
					timings[CANDIDATE_SORT] += (stop-start).seconds();
					start = stop;
				#endif
				for (size_t i=0; i<affected_nodes.size(); ++i) {
					// batch process labels belonging to the same target node
					const NodeID node = affected_nodes[i];
					auto& ls = labels[node];
					std::sort(ls.candidates.begin(), ls.candidates.end(), groupLabels);
					#ifdef BTREE_PARETO_LABELSET
    					ls.labels.setup(labelset_data);
					#endif
					ls.labels.updateLabelSet(node, ls.candidates.cbegin(), ls.candidates.cend(), updates, stats);
					ls.candidates.clear();
				}
				#ifdef GATHER_SUBCOMPNENT_TIMING
					stop = tbb::tick_count::now();
					timings[UPDATE_LABELSETS] += (stop-start).seconds();
					start = stop;
				#endif

			#else 
				// Comparision-based sort of candidates to group them by their target node
				for (const auto& op : updates) {
					const NodeLabel& min = op.data;
					FORALL_EDGES(graph, min.node, eid) {
						const Edge& edge = graph.getEdge(eid);
						candidates.push_back(NodeLabel(edge.target, createNewLabel(min, edge)));
					}
 				}
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

				auto cand_iter = candidates.begin();
				while (cand_iter != candidates.end()) {
					// find all labels belonging to the same target node
					nodelabel_iter range_start = cand_iter;
					while (cand_iter != candidates.end() && range_start->node == cand_iter->node) {
						++cand_iter;
					}
					auto& ls = labels[range_start->node];
					std::sort(range_start, cand_iter, groupLabels);
					#ifdef BTREE_PARETO_LABELSET
    					ls.labels.setup(labelset_data);
					#endif
					ls.labels.updateLabelSet(range_start->node, range_start, cand_iter, updates, stats);
				}
				#ifdef GATHER_SUBCOMPNENT_TIMING
					stop = tbb::tick_count::now();
					timings[UPDATE_LABELSETS] += (stop-start).seconds();
					start = stop;
				#endif
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
				set_insertions += ls.labels.set_insertions;
				set_dominations += ls.labels.set_dominations;
			}
			std::cout << "# LabelSet Modifications: insertion/deletion, dominance position" << std::endl;
			for (size_t i=0; i < 101; ++i) {
				std::cout << i << " " << set_insertions[i] << " " << set_dominations[i] << std::endl;
			}
		#endif
		pq.printStatistics();
		std::cout << stats.toString(labels) << std::endl;
		#ifdef GATHER_SUBCOMPNENT_TIMING
			std::cout << "Subcomponent Timings:" << std::endl;
			std::cout << "  " << timings[FIND_PARETO_MIN]  << " Find Pareto Min" << std::endl;
			std::cout << "  " << timings[CANDIDATE_SORT]  << " Group Candidates" << std::endl;
			std::cout << "  " << timings[UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			std::cout << "  " << timings[UPDATES_SORT] << " Sort Updates"  << std::endl;
			std::cout << "  " << timings[PQ_UPDATE] << " Update PQ " << std::endl;
		#endif
	}

	size_t size(NodeID node) const { return labels[node].labels.size(); }
	label_iter begin(NodeID node) { return labels[node].labels.begin(); }
	const_label_iter begin(NodeID node) const { return labels[node].labels.begin(); }
	label_iter end(NodeID node) { return labels[node].labels.end(); }
	const_label_iter end(NodeID node) const { return labels[node].labels.end(); }


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
