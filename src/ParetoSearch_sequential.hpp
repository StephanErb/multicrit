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
#include "Graph.hpp"

#include "ParetoLabelSet_sequential.hpp"
#include "radix_sort.hpp"

#ifdef GATHER_SUBCOMPNENT_TIMING
	#include "tbb/tick_count.h"
	#define TIME_COMPONENT(target) do { stop = tbb::tick_count::now(); target += (stop-start).seconds(); start = stop; } while(0)
#else
	#define TIME_COMPONENT(target) do { } while(0)
#endif

class ParetoSearch {
private:
	GroupOperationsByWeightAndNodeComperator<Operation<NodeLabel>> groupOpsByWeight;
	GroupNodeLabelsByNodeComperator groupCandidates;
	GroupLabelsByWeightComperator groupLabels;
	

	#ifdef BTREE_PARETO_LABELSET
		typedef BtreeParetoLabelSet<Label, GroupLabelsByWeightComperator, std::allocator<Label>> LabelSet;
	#else
		typedef VectorParetoLabelSet<std::allocator<Label>> LabelSet;
	#endif

	std::vector<LabelSet> labels;

	std::vector<Operation<NodeLabel> > updates;
	std::vector<NodeID> affected_nodes;
	std::vector<NodeLabel> candidates;

	const Graph& graph;
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

	typename LabelSet::ThreadLocalLSData labelset_data;


public:
	ParetoSearch(const Graph& graph_):
		labels(graph_.numberOfNodes()), 
		graph(graph_),
		labelset_data(labels[0])
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_insertions(101)
			,set_dominations(101)
		#endif
	{
		updates.reserve(LARGE_ENOUGH_FOR_MOST);
		candidates.reserve(LARGE_ENOUGH_FOR_MOST);
	 }

	void run(const NodeID node) {
		pq.init(NodeLabel(node, Label(0,0)));
		labels[node].init(Label(0,0), labelset_data);

		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif

		while (!pq.empty()) {
			stats.report(ITERATION, pq.size());

			pq.findParetoMinima(updates, candidates, graph);
			const size_t minima_count = updates.size();
			stats.report(MINIMA_COUNT, minima_count);
			TIME_COMPONENT(timings[FIND_PARETO_MIN]);

			sort(candidates);
			TIME_COMPONENT(timings[CANDIDATE_SORT]);

			const auto cand_end = candidates.end();
			auto cand_iter = candidates.begin();
			while (cand_iter != cand_end) {
				// find all labels belonging to the same target node
				auto range_start = cand_iter;
				auto& ls = labels[range_start->node];
				ls.prefetch();
				while (cand_iter != cand_end && range_start->node == cand_iter->node) {
					++cand_iter;
				}
				std::sort(range_start, cand_iter, groupLabels);
				ls.updateLabelSet(range_start->node, range_start, cand_iter, updates, labelset_data, stats);
			}
			TIME_COMPONENT(timings[UPDATE_LABELSETS]);

			// Sort sequence for batch update
			std::sort(updates.begin()+minima_count, updates.end(), groupOpsByWeight);
			std::inplace_merge(updates.begin(), updates.begin()+minima_count, updates.end(), groupOpsByWeight);
			TIME_COMPONENT(timings[UPDATES_SORT]);

			const size_t pre_update_size = pq.size();
			pq.applyUpdates(updates);
			stats.report(UPDATE_COUNT, updates.size());
			stats.report(PQ_SIZE_DELTA, std::abs((signed long)pq.size()-(signed long)pre_update_size));
			TIME_COMPONENT(timings[PQ_UPDATE]);

			updates.clear();
			candidates.clear();
		}		
	}

	inline void sort(std::vector<NodeLabel>& candidates) {
		#ifdef RADIX_SORT
			radix_sort(candidates.data(), candidates.size(), [](const NodeLabel& x) { return x.node; });
		#else 
			std::sort(candidates.begin(), candidates.end(), groupCandidates);
		#endif
	}
	
	void printStatistics() {
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			for (auto& ls : labels) {
				for (size_t i=0; i < 101; ++i) {
					set_insertions[i] += ls.set_insertions[i];
					set_dominations[i] += ls.set_dominations[i];
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

	void printComponentTimings() const {
		#ifdef GATHER_SUBCOMPNENT_TIMING
			std::cout << "sequ" << " " << 0 << " " << timings[FIND_PARETO_MIN] << " " << timings[CANDIDATE_SORT] << " " << timings[UPDATE_LABELSETS] << " " << timings[UPDATES_SORT] << " " << timings[PQ_UPDATE];
		#endif
	}

	size_t size(NodeID node) const { return labels[node].size(); }
	std::vector<Label>::iterator begin(NodeID node) { return labels[node].begin(); }
	std::vector<Label>::const_iterator begin(NodeID node) const { return labels[node].begin(); }
	std::vector<Label>::iterator end(NodeID node) { return labels[node].end(); }
	std::vector<Label>::const_iterator end(NodeID node) const { return labels[node].end(); }


};


#endif 
