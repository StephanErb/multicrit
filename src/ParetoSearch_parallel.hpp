/*
 * A implementation of a bi-objective shortest path label setting algorithm
 * that always relaxes _all_ globaly pareto optimal labels instead of just one.
 *  
 * Author: Stephan Erb
 */
#ifndef PARALLEL_PARETO_SEARCH_H_
#define PARALLEL_PARETO_SEARCH_H_

#include "utility/datastructure/graph/GraphMacros.h"
#include "options.hpp"
#include "ParetoQueue.hpp"
#include "ParetoSearchStatistics.hpp"

#include "tbb/parallel_sort.h"
#include "tbb/concurrent_vector.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/enumerable_thread_specific.h"


#include <deque>

template<typename graph_slot>
class ParetoSearch {
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Label;

	struct Data : public Label {
		NodeID node;
  		typedef Label label_type;
 		Data(const NodeID& x, const Label& y) : Label(y), node(x) {}
 		Data() : Label(0,0), node(0) {}
 		Data(const Data& data) : Label(data), node(data.node) {}

 		friend std::ostream& operator<<(std::ostream& os, const Data& data) {
			os << " (" << data.node << ": " << data.first_weight << "," << data.second_weight << ")";
			return os;
		}
	};

	typedef typename std::vector<Label>::iterator label_iter;
	typedef typename std::vector<Label>::iterator const_label_iter;
	typedef typename std::vector<Label>::iterator const_cand_iter;
	typedef typename std::vector<Data>::iterator pareto_iter;
	typedef typename std::vector<Data>::const_iterator const_pareto_iter;

    // Permanent and temporary labels per node.
	std::vector<std::vector<Label>> labels;
	std::vector<std::vector<Label>> candidates;

	typedef tbb::enumerable_thread_specific< std::vector<Operation<Data>> > UpdateListType; 
	UpdateListType tls_local_updates;

	graph_slot graph;
	ParetoQueue<Data> pq;
	ParetoSearchStatistics<Label> stats;

	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> set_changes;
	#endif

	struct GroupByNodeComp {
		bool operator() (const Label& i, const Label& j) const {
			if (i.first_weight == j.first_weight) {
					return i.second_weight < j.second_weight;
				}
			return i.first_weight < j.first_weight;
		}
	} groupLabels;

	struct GroupByWeightComp {
		bool operator() (const Operation<Data>& i, const Operation<Data>& j) const {
			if (i.data.first_weight == j.data.first_weight) {
				if (i.data.second_weight == j.data.second_weight) {
					return i.data.node < j.data.node;
				}
				return i.data.second_weight < j.data.second_weight;
			}
			return i.data.first_weight < j.data.first_weight;
		}
	} groupByWeight;

	static Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

	static struct WeightLessComp {
		bool operator() (const Label& i, const Label& j) const {
			return i.first_weight < j.first_weight;
		}
	} firstWeightLess;

	static bool secondWeightGreaterOrEquals(const Label& i, const Label& j) {
		return i.second_weight >= j.second_weight;
	}

	/** First label where the x-coord is truly smaller */
	static label_iter x_predecessor(label_iter begin, label_iter end, const Label& new_label) {
		return --std::lower_bound(begin, end, new_label, firstWeightLess);
	}

	/** First label where the y-coord is truly smaller */
	static label_iter y_predecessor(const label_iter& begin, const Label& new_label) {
		label_iter i = begin;
		while (secondWeightGreaterOrEquals(*i, new_label)) {
			++i;
		}
		return i;
	}

	static bool isDominated(label_iter begin, label_iter end, const Label& new_label, label_iter& iter) {
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

	void updateLabelSet(const NodeID node, std::vector<Label>& labelset, const const_cand_iter start, const const_cand_iter end, std::vector<Operation<Data> >& updates) {
		typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

		label_iter labelset_iter = labelset.begin();
		for (const_cand_iter candidate = start; candidate != end; ++candidate) {
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
				continue;
			}
			min = new_label.second_weight;
			stats.report(LABEL_NONDOMINATED);
			updates.push_back(Operation<Data>(Operation<Data>::INSERT, Data(node, new_label)));

			label_iter first_nondominated = y_predecessor(iter, new_label);
			if (iter == first_nondominated) {

				#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
					set_changes[(int)(100*((first_nondominated - labelset.begin())/(double)labelset.size()) + 0.5)]++;
				#endif

				// delete range is empty, so just insert
				labelset_iter = labelset.insert(first_nondominated, new_label);
			} else {
				// schedule deletion of dominated labels
				for (label_iter i = iter; i != first_nondominated; ++i) {

					#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
						if (i != iter) set_changes[(int)(100*((first_nondominated - labelset.begin())/(double)labelset.size()) + 0.5)]++;
					#endif

					updates.push_back(Operation<Data>(Operation<Data>::DELETE, Data(node, *i)));
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
		candidates(graph_.numberOfNodes()),
		graph(graph_),
		pq(graph_.numberOfNodes())
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_changes(101)
		#endif
	{
		const typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::min();
		const typename Label::weight_type max = std::numeric_limits<typename Label::weight_type>::max();

		// add sentinals
		for (size_t i=0; i<labels.size(); ++i) {
			labels[i].insert(labels[i].begin(), Label(min, max));
			labels[i].insert(labels[i].end(), Label(max, min));		
		}
	}

	void run(const NodeID node) {
		std::vector<Data> globalMinima;
		std::vector<Operation<Data>> updates;
		std::vector<NodeID> affected_nodes;
		pq.init(Data(node, Label(0,0)));

		while (!pq.empty()) {
			stats.report(ITERATION, pq.size());

			pq.findParetoMinima(globalMinima);
			stats.report(MINIMA_COUNT, globalMinima.size());

			for (size_t i = 0; i != globalMinima.size(); ++i) {
				FORALL_EDGES(graph, globalMinima[i].node, eid) {
					const Edge& edge = graph.getEdge(eid);
					if (candidates[edge.target].empty()) {
						affected_nodes.push_back(edge.target);
					}
					candidates[edge.target].push_back(createNewLabel(globalMinima[i], edge));
				}
			}
				
			tbb::parallel_for(tbb::blocked_range<size_t>(0, affected_nodes.size()),
				[&, this](const tbb::blocked_range<size_t>& r) {

				typename UpdateListType::reference local_updates = tls_local_updates.local();

				for (size_t i = r.begin(); i != r.end(); ++i) {
						auto node = affected_nodes[i];
						std::sort(candidates[node].begin(), candidates[node].end(), groupLabels);
						stats.report(IDENTICAL_TARGET_NODE, candidates[node].size());

						// batch process labels belonging to the same target node
						this->updateLabelSet(node, labels[node], candidates[node].begin(), candidates[node].end(), local_updates);
						candidates[node].clear();
				}

			});
			for (typename UpdateListType::reference local_updates : tls_local_updates) {

				updates.reserve(updates.size() + local_updates.size());
				std::copy(local_updates.begin(), local_updates.end(), std::back_insert_iterator<std::vector<Operation<Data>>>(updates));
				local_updates.clear();
			}
			tbb::parallel_sort(updates.begin(), updates.end(), groupByWeight);

			size_t update_size = updates.size();
			// Schedule optima for deletion
			for (const_pareto_iter i = globalMinima.begin(); i != globalMinima.end(); ++i) {
				updates.push_back(Operation<Data>(Operation<Data>::DELETE, *i));
			}
			// Merge the sorted minima
			std::inplace_merge(updates.begin(), updates.begin()+update_size, updates.end(), groupByWeight);


			pq.applyUpdates(updates);

			globalMinima.clear();
			updates.clear();
			affected_nodes.clear();
		}		
	}
	
	void printStatistics() {
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			std::cout << "# LabelSet Modifications" << std::endl;
			for (size_t i=0; i < 101; ++i) {
				std::cout << i << " " << set_changes[i] << std::endl;
			}
		#endif
		pq.printStatistics();
		std::cout << stats.toString(labels) << std::endl;
	}

	// Subtraction / addition used to hide the sentinals
	size_t size(NodeID node) {return labels[node].size()-2; }

	label_iter begin(NodeID node) { return ++labels[node].begin(); }
	const_label_iter begin(NodeID node) const { return ++labels[node].begin(); }
	label_iter end(NodeID node) { return --labels[node].end(); }
	const_label_iter end(NodeID node) const { return --labels[node].end(); }
};


#endif 
