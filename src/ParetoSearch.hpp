/*
 * A implementation of a bi-objective shortest path label setting algorithm
 * that always relaxes _all_ globaly pareto optimal labels instead of just one.
 *  
 * Author: Stephan Erb
 */
#ifndef PARETO_SEARCH_H_
#define PARETO_SEARCH_H_

#include "utility/datastructure/graph/GraphMacros.h"

#include "ParetoQueue.hpp"
#include "ParetoSearchStatistics.hpp"



template<typename graph_slot>
class SequentialParetoSearch {
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Label;

	struct Data : public Label {
		NodeID node;
  		typedef Label label_type;
 		explicit Data(const NodeID& x, const Label& y) : Label(y), node(x) {}
 		Data() : Label(0,0), node(0) {}
 		Data(const Data& data) : Label(data), node(data.node) {}
	};

	typedef typename std::vector<Label>::iterator label_iter;
	typedef typename std::vector<Label>::iterator const_label_iter;
	typedef typename std::vector<Data>::iterator pareto_iter;
	typedef typename std::vector<Data>::const_iterator const_pareto_iter;

    // Permanent and temporary labels per node.
	std::vector<std::vector<Label> > labels;

	graph_slot graph;
	SequentialParetoQueue<Data> pq;
	ParetoSearchStatistics stats;

	struct GroupLabelsByWeightComp {
		bool operator() (const Label& i, const Label& j) const {
			if (i.first_weight == j.first_weight) {
				return i.second_weight < j.second_weight;
			}
			return i.first_weight < j.first_weight;
		}
	} groupLabelsByWeight;

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

	void updateLabelSet(NodeID node, std::vector<Label>& labelset, const const_label_iter start, const const_label_iter end, std::vector<Operation<Data> >& updates) {
		typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

		label_iter labelset_iter = labelset.begin();
		for (const_label_iter candidate = start; candidate != end; ++candidate) {
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
				// delete range is empty, so just insert
				labelset_iter = labelset.insert(first_nondominated, new_label);
			} else {
				// schedule deletion of dominated labels
				for (label_iter i = iter; i != first_nondominated; ++i) {
					updates.push_back(Operation<Data>(Operation<Data>::DELETE, Data(node, *i)));
				}
				// replace first dominated label and remove the rest
				*iter = new_label;
				labelset_iter = labelset.erase(++iter, first_nondominated);
			}
		}
	}

public:
	SequentialParetoSearch(const graph_slot& graph_):
		labels(graph_.numberOfNodes()),
		graph(graph_),
		pq(graph_.numberOfNodes())
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
		std::vector<std::vector<Label> > candidates(graph.numberOfNodes());
		std::vector<NodeID> activeNodes;
		std::vector<Operation<Data> > updates;
		pq.init(Data(node, Label(0,0)));

		while (!pq.empty()) {
			stats.report(ITERATION, pq.size());

			pq.findParetoMinima(globalMinima);
			stats.report(MINIMA_COUNT, globalMinima.size());

			for (pareto_iter i = globalMinima.begin(); i != globalMinima.end(); ++i) {
				FORALL_EDGES(graph, i->node, eid) {
					const Edge& edge = graph.getEdge(eid);
					//  Mark target node bucket as used && write the candidate to this bucket
					if (candidates[edge.target].empty()) {
						activeNodes.push_back(edge.target);
					}
					candidates[edge.target].push_back(createNewLabel(*i, edge));
				}
			}

			// process all non-emtpy buckets
			for (typename std::vector<NodeID>::iterator iter = activeNodes.begin(); iter != activeNodes.end(); ++iter) {
				NodeID node = *iter;
				stats.report(IDENTICAL_TARGET_NODE, candidates[node].size());
				std::sort(candidates[node].begin(), candidates[node].end(), groupLabelsByWeight);
				// batch process labels belonging to the same target node
				updateLabelSet(node, labels[node], candidates[node].begin(), candidates[node].end(), updates);
				candidates[node].clear();
			}

			// Schedule optima for deletion
			for (const_pareto_iter i = globalMinima.begin(); i != globalMinima.end(); ++i) {
				updates.push_back(Operation<Data>(Operation<Data>::DELETE, *i));
			}
			// Sort sequence for batch update
			std::sort(updates.begin(), updates.end(), groupByWeight);
			pq.applyUpdates(updates);

			globalMinima.clear();
			activeNodes.clear();
			updates.clear();
		}		
	}
	
	void printStatistics() {
		std::cout << stats.toString() << std::endl;
	}

	// Subtraction / addition used to hide the sentinals
	size_t size(NodeID node) {return labels[node].size()-2; }

	label_iter begin(NodeID node) { return ++labels[node].begin(); }
	const_label_iter begin(NodeID node) const { return ++labels[node].begin(); }
	label_iter end(NodeID node) { return --labels[node].end(); }
	const_label_iter end(NodeID node) const { return --labels[node].end(); }
};


#endif 
