/*
 * A implementation of a bi-objective shortest path label setting algorithm
 * that always relaxes _all_ globaly pareto optimal labels instead of just one.
 *  
 * Author: Stephan Erb
 */
#ifndef PARETO_SEARCH_H_
#define PARETO_SEARCH_H_

#include "utility/datastructure/graph/GraphMacros.h"
#include "options.hpp"
#include "ParetoQueue.hpp"
#include "ParetoSearchStatistics.hpp"

#include <deque>

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

	typedef typename LABELSET_SET_SEQUENCE_TYPE<Label>::iterator label_iter;
	typedef typename LABELSET_SET_SEQUENCE_TYPE<Label>::iterator const_label_iter;
	typedef typename std::vector<Data>::iterator pareto_iter;
	typedef typename std::vector<Data>::const_iterator const_pareto_iter;

    // Permanent and temporary labels per node.
	std::vector<LABELSET_SET_SEQUENCE_TYPE<Label> > labels;

	graph_slot graph;
	SequentialParetoQueue<Data> pq;
	ParetoSearchStatistics<Label> stats;

	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> set_changes;
	#endif

	struct GroupByNodeComp {
		bool operator() (const Data& i, const Data& j) const {
			if (i.node == j.node) {
				if (i.first_weight == j.first_weight) {
					return i.second_weight < j.second_weight;
				}
				return i.first_weight < j.first_weight;
			} 
			return i.node < j.node;
		}
	} groupByNode;

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

	void updateLabelSet(LABELSET_SET_SEQUENCE_TYPE<Label>& labelset, const const_pareto_iter start, const const_pareto_iter end, std::vector<Operation<Data> >& updates) {
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
			label_iter iter;
			if (isDominated(labelset_iter, labelset.end(), new_label, iter)) {
				stats.report(LABEL_DOMINATED);
				min = iter->second_weight; 
				continue;
			}
			min = new_label.second_weight;
			stats.report(LABEL_NONDOMINATED);
			updates.push_back(Operation<Data>(Operation<Data>::INSERT, new_label));

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

					updates.push_back(Operation<Data>(Operation<Data>::DELETE, Data(new_label.node, *i)));
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
