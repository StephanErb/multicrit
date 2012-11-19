/*
 * A implementation of a bi-objective shortest path label setting algorithm.
 *  
 * Author: Stephan Erb
 */
#ifndef PARETO_SEARCH_H_
#define PARETO_SEARCH_H_


//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"

//NodeID and EdgdeID
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <iterator>

#include "LabelSet.hpp"
#include "ParetoSearchStatistics.hpp"


template<typename data_type>
struct Operation {
	enum OpType {INSERT, DELETE};
	OpType type;
	data_type data;
 	explicit Operation(const OpType& x, const data_type& y) : type(x), data(y) {}
};

/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename data_type>
class ParetoQueue {
private:
	std::vector<data_type> labels;
	typedef typename std::vector<data_type>::iterator iterator;
	typedef typename std::vector<data_type>::const_iterator const_iterator;
	typedef typename data_type::weight_type weight_type;

	static void printLabels(iterator begin, iterator end) {
		std::cout << "All labels:";
		for (iterator i = begin; i != end; ++i) {
			std::cout << " (" << i->node << ": " << i->first_weight << "," << i->second_weight << ")";
		}
		std::cout << std::endl;
	}

public:

	ParetoQueue() {
		const weight_type min = std::numeric_limits<weight_type>::min();
		const weight_type max = std::numeric_limits<weight_type>::max();

		// add sentinals
		labels.insert(labels.begin(), data_type(NodeID(0), typename data_type::label_type(min, max)));
		labels.insert(labels.end(), data_type(NodeID(0), typename data_type::label_type(max, min)));
	}

	void findParetoMinima(std::vector<data_type>& minima) const {
		const_iterator iter = labels.begin();
		const_iterator end = --labels.end();  // ignore the sentinal

		weight_type min = iter->second_weight;
		while (iter != end) {
			if (iter->second_weight < min) {
				min = iter->second_weight;
				minima.push_back(*iter);
			}
			++iter;
		}
	}

	void init(const data_type& data) {
		labels.insert(++labels.begin(), data);	
	}

	void applyUpdates(const std::vector<Operation<data_type> >& updates) {
		typedef typename std::vector<Operation<data_type> >::const_iterator u_iterator;

		iterator label_iter = labels.begin();
		u_iterator update_iter = updates.begin();

		std::vector<data_type> merged;
		merged.reserve(labels.size()+ updates.size());

		while (update_iter != updates.end()) {
			switch (update_iter->type) {
			case Operation<data_type>::DELETE:
			  	// We know the element is in here
				while (label_iter->first_weight != update_iter->data.first_weight ||
						label_iter->second_weight != update_iter->data.second_weight ||
						label_iter->node != update_iter->data.node) {
					merged.push_back(*label_iter++);
				}
				++label_iter; // delete the element by jumping over it
                ++update_iter;
                continue;
	        case Operation<data_type>::INSERT:
				// insertion bounded by sentinal
				while (label_iter->first_weight < update_iter->data.first_weight) {
					merged.push_back(*label_iter++);
				}
				while (label_iter->first_weight == update_iter->data.first_weight
						&& label_iter->second_weight < update_iter->data.second_weight) {
					merged.push_back(*label_iter++);
				}
				while (label_iter->first_weight == update_iter->data.first_weight
						&& label_iter->second_weight == update_iter->data.second_weight
						&& label_iter->node < update_iter->data.node) {
					merged.push_back(*label_iter++);
				}
				merged.push_back(update_iter->data);
				++update_iter;
				continue;
	        }
		}
		std::copy(label_iter, labels.end(), std::back_inserter(merged));
	    labels = merged;
	}

	bool empty() {
		return size() == 0;
	}

	size_t size() {
		return labels.size() -2;
	}
};


template<typename graph_slot>
class SequentialParetoSearch {
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Label;

	typedef typename std::vector<Label>::iterator label_iter;
	typedef typename std::vector<Label>::iterator const_label_iter;

	struct Data : public Label {
		NodeID node;
  		typedef Label label_type;
 		explicit Data(const NodeID& x, const Label& y) : Label(y), node(x) {}
	};

    // Permanent and temporary labels per node.
	std::vector<std::vector<Label> > labels;

	graph_slot graph;
	ParetoQueue<Data> pq;
	ParetoSearchStatistics stats;

	struct CandidateComparator1 {
  		bool operator() (const Data& i, const Data& j) {
  			if (i.node == j.node) {
  				if (i.first_weight == j.first_weight) {
  					return i.second_weight < j.second_weight;
  				}
  				return i.first_weight < j.first_weight;
  			} 
  			return i.node < j.node;
  		}
	} groupByNode;
	struct CandidateComparator2 {
  		bool operator() (const Operation<Data>& i, const Operation<Data>& j) {
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

		static bool firstWeightLess(const Label& i, const Label& j)  {
		return i.first_weight < j.first_weight;
	}

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

public:
	typedef typename std::vector<Data>::iterator pareto_iter;
	typedef typename std::vector<Data>::const_iterator const_pareto_iter;

	SequentialParetoSearch(const graph_slot& graph_):
		labels(graph_.numberOfNodes()),
		graph(graph_)
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
		std::vector<Data> candidates;
		std::vector<Operation<Data> > updates;
		pq.init(Data(node, Label(0,0)));

		while (!pq.empty()) {
			stats.report(ITERATION, pq.size());

			pq.findParetoMinima(globalMinima);
			stats.report(MINIMA_COUNT, globalMinima.size());

			for (pareto_iter i = globalMinima.begin(); i != globalMinima.end(); ++i) {
				FORALL_EDGES(graph, i->node, eid) {
					const Edge& edge = graph.getEdge(eid);
					candidates.push_back(Data(edge.target, createNewLabel(*i, edge)));
				}
			}
			// Sort sequence to group candidates by their target node
			std::sort(candidates.begin(), candidates.end(), groupByNode);
			updateLabelSets(candidates, updates);

			// Schedule optima for deletion
			for (const_pareto_iter i = globalMinima.begin(); i != globalMinima.end(); ++i) {
				updates.push_back(Operation<Data>(Operation<Data>::DELETE, *i));
			}
			// Sort sequence for batch update
			std::sort(updates.begin(), updates.end(), groupByWeight);
			pq.applyUpdates(updates);

			globalMinima.clear();
			candidates.clear();
			updates.clear();
		}		
	}

	void updateLabelSets(const std::vector<Data>& candidates, std::vector<Operation<Data> >& updates) {
		const_pareto_iter cand_iter = candidates.begin();

		while (cand_iter != candidates.end()) {
			// find all labels belonging to the same target node
			const_pareto_iter range_start = cand_iter;
			while (cand_iter != candidates.end() && range_start->node == cand_iter->node) {
				++cand_iter;
			}
			stats.report(IDENTICAL_TARGET_NODE, cand_iter - range_start);

			// batch process labels belonging to the same target node
			updateLabelSet(labels[range_start->node], range_start, cand_iter, updates);
		}
	}

	void updateLabelSet(std::vector<Label>& labelset, const const_pareto_iter start, const const_pareto_iter end, std::vector<Operation<Data> >& updates) {
		typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

		label_iter labelset_iter = labelset.begin();
		for (const_pareto_iter candidate = start; candidate != end; ++candidate) {
			Data new_label = *candidate;
			// short cut dominated check among candidates
			if (new_label.second_weight >= min) { 
				stats.report(LABEL_DOMINATED);
				stats.report(DOMINATION_SHORTCUT);
				continue; 
			}
			min = new_label.second_weight;
			label_iter iter;
			if (isDominated(labelset_iter, labelset.end(), new_label, iter)) {
				stats.report(LABEL_DOMINATED);
				continue;
			}
			stats.report(LABEL_NONDOMINATED);
			updates.push_back(Operation<Data>(Operation<Data>::INSERT, new_label));

			label_iter first_nondominated = y_predecessor(iter, new_label);
			if (iter == first_nondominated) {
				// delete range is empty, so just insert
				labelset_iter = labelset.insert(first_nondominated, new_label);
			} else {
				// schedule deletion of dominated labels
				for (label_iter i = iter; i != first_nondominated; ++i) {
					updates.push_back(Operation<Data>(Operation<Data>::DELETE, Data(new_label.node, *i)));
				}
				// replace first dominated label and remove the rest
				*iter = new_label;
				labelset_iter = labelset.erase(++iter, first_nondominated);
			}
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
