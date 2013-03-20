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

#define RADIX_SORT
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

	typedef std::vector< Label > LabelVec;

	typedef typename LabelVec::iterator label_iter;
	typedef typename LabelVec::const_iterator const_label_iter;
	typedef typename std::vector<NodeLabel>::iterator nodelabel_iter;

	struct LabelSetStruct {
		#ifdef BUCKET_SORT
			LabelVec candidates;
		#endif
		LabelVec labels;
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

	struct GroupNodeLabelsByNodeComp {
		inline bool operator() (const NodeLabel& i, const NodeLabel& j) const {
			if (i.node == j.node) {
				if (i.first_weight == j.first_weight) {
					return i.second_weight < j.second_weight;
				}
				return i.first_weight < j.first_weight;
			}
			return i.node < j.node;
		}
	} groupCandidates;

	struct GroupLabelsByNodeComp {
		inline bool operator() (const Label& i, const Label& j) const {
			if (i.first_weight == j.first_weight) {
				return i.second_weight < j.second_weight;
			}
			return i.first_weight < j.first_weight;
		}
	} groupLabels;

	struct GroupByWeightComp {
		inline bool operator() (const Operation<NodeLabel>& i, const Operation<NodeLabel>& j) const {
			if (i.data.first_weight == j.data.first_weight) {
				if (i.data.second_weight == j.data.second_weight) {
					return i.data.node < j.data.node;
				}
				return i.data.second_weight < j.data.second_weight;
			}
			return i.data.first_weight < j.data.first_weight;
		}
	} groupByWeight;

	static inline Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

	static struct WeightLessComp {
		inline bool operator() (const Label& i, const Label& j) const {
			return i.first_weight < j.first_weight;
		}
	} firstWeightLess;

	static inline bool secondWeightGreaterOrEquals(const Label& i, const Label& j) {
		return i.second_weight >= j.second_weight;
	}

	/** First label where the x-coord is truly smaller */
	static inline label_iter x_predecessor(label_iter begin, label_iter end, const Label& new_label) {
		return --std::lower_bound(begin, end, new_label, firstWeightLess);
	}

	/** First label where the y-coord is truly smaller */
	static inline label_iter y_predecessor(const label_iter& begin, const Label& new_label) {
		label_iter i = begin;
		while (secondWeightGreaterOrEquals(*i, new_label)) {
			++i;
		}
		return i;
	}

	static inline bool isDominated(label_iter begin, label_iter end, const Label& new_label, label_iter& iter) {
		iter = x_predecessor(begin, end, new_label);

		if (iter->second_weight <= new_label.second_weight) {
			return true; // new label is dominated
		}
		++iter; // move to element with greater or equal x-coordinate

		if (iter->first_weight == new_label.first_weight && 
			iter->second_weight <= new_label.second_weight) {
			// new label is dominated by the (single) pre-existing element with the same
			// x-coordinate.
			return true; 
		}
		return false;
	}

	template<class candidates_iter_type>
	void updateLabelSet(const NodeID node, std::vector<Label>& labelset, const candidates_iter_type start, const candidates_iter_type end, std::vector<Operation<NodeLabel>>& updates) {
		typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

		label_iter labelset_iter = labelset.begin();
		for (candidates_iter_type candidate = start; candidate != end; ++candidate) {
			Label new_label = *candidate;
			// short cut dominated check among candidates
			if (new_label.second_weight >= min) { 
				stats.report(LABEL_DOMINATED);
				stats.report(DOMINATION_SHORTCUT);
				continue; 
			}
			label_iter iter;
			if (isDominated(labelset_iter, labelset.end(), new_label, iter)) {
				stats.report(LABEL_DOMINATED);
				min = iter->second_weight; 
				#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
					const double size = labelset.size()-2; // without both sentinals
					const double position = iter - (labelset.begin() + 1); // without leading sentinal 
					if (size > 0) set_dominations[(int) (100.0 * position / size)]++;
				#endif	
				continue;
			}
			min = new_label.second_weight;
			stats.report(LABEL_NONDOMINATED);
			updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, new_label)});

			label_iter first_nondominated = y_predecessor(iter, new_label);

			#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
				const double size = labelset.size()-2; // without both sentinals
				const double position = iter - (labelset.begin() + 1); // without leading sentinal 
				if (size > 0) set_insertions[(int) (100.0 * position / size)]++;
			#endif	

			if (iter == first_nondominated) {
				// delete range is empty, so just insert
				labelset_iter = labelset.insert(first_nondominated, new_label);
			} else {
				// schedule deletion of dominated labels
				for (label_iter i = iter; i != first_nondominated; ++i) {
					updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, *i)});
				}
				// replace first dominated label and remove the rest
				*iter = new_label;
				labelset_iter = labelset.erase(++iter, first_nondominated);
			}
		}
	}

public:
	ParetoSearch(const graph_slot& graph_):
		labels(graph_.numberOfNodes()), 
		graph(graph_)
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_insertions(101)
			,set_dominations(101)
		#endif
	{
		const typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::min();
		const typename Label::weight_type max = std::numeric_limits<typename Label::weight_type>::max();

		// add sentinals
		for (size_t i=0; i<labels.size(); ++i) {
			auto& l = labels[i].labels;
			l.insert(l.begin(), Label(min, max));
			l.insert(l.end(), Label(max, min));
		}
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
					updateLabelSet(node, ls.labels, ls.candidates.cbegin(), ls.candidates.cend(), updates);
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
	 				#ifdef RADIX_SORT
						std::sort(range_start, cand_iter, groupLabels);
					#endif
					updateLabelSet(range_start->node, ls.labels, range_start, cand_iter, updates);
				}
				#ifdef GATHER_SUBCOMPNENT_TIMING
					stop = tbb::tick_count::now();
					timings[UPDATE_LABELSETS] += (stop-start).seconds();
					start = stop;
				#endif
			#endif

			// Sort sequence for batch update
			std::sort(updates.begin()+minima_count, updates.end(), groupByWeight);
			std::inplace_merge(updates.begin(), updates.begin()+minima_count, updates.end(), groupByWeight);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[UPDATES_SORT] += (stop-start).seconds();
				start = stop;
			#endif

			pq.applyUpdates(updates);
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
			std::cout << "# LabelSet Modifications: insertion/deletion, dominance position" << std::endl;
			for (size_t i=0; i < 101; ++i) {
				std::cout << i << " " << set_insertions[i] << " " << set_dominations[i] << std::endl;
			}
		#endif
		std::cout << stats.toString(labels) << std::endl;
		#ifdef GATHER_SUBCOMPNENT_TIMING
			std::cout << "Subcomponent Timings:" << std::endl;
			std::cout << "  " << timings[FIND_PARETO_MIN]  << " Find Pareto Min" << std::endl;
			std::cout << "  " << timings[CANDIDATE_SORT]  << " Group Candidates" << std::endl;
			std::cout << "  " << timings[UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			std::cout << "  " << timings[UPDATES_SORT] << " Sort Updates"  << std::endl;
			std::cout << "  " << timings[PQ_UPDATE] << " Update PQ " << std::endl;
		#endif
		pq.printStatistics();
	}

	// Subtraction / addition used to hide the sentinals
	size_t size(NodeID node) {return labels[node].labels.size()-2; }

	label_iter begin(NodeID node) { return ++labels[node].labels.begin(); }
	const_label_iter begin(NodeID node) const { return ++labels[node].labels.begin(); }
	label_iter end(NodeID node) { return --labels[node].labels.end(); }
	const_label_iter end(NodeID node) const { return --labels[node].labels.end(); }
};


#endif 
