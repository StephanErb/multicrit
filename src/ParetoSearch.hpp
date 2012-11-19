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
#include <algorithm>
#include <iterator>

#include "LabelSet.hpp"
//#include "ParetoSearchStatistics.hpp"


template<typename data_type>
struct Operation {
	enum OpType {INSERT, DELETE};
	OpType type;
	data_type data;
 	Operation(const OpType& x, const data_type& y) : type(x), data(y) {}
   	Operation(const Operation& p) : type(p.type), data(p.data) { }
};

/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename data_type>
class ParetoQueue {
private:
	std::vector<data_type> labels;
	typedef typename std::vector<data_type>::iterator iterator;
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

	void findParetoMinima(std::vector<data_type>& minima) {
		iterator iter = ++labels.begin(); // ignore the sentinal
		iterator end = labels.end() - 1;  // ignore the sentinal

		weight_type min = iter->second_weight;
		minima.push_back(*iter);
		while (iter != end) {
			if (iter->second_weight < min) {
				min = iter->second_weight;
				minima.push_back(*iter);
			}
			++iter;
		}
		//printLabels(labels.begin(), labels.end());
	}

	void init(const data_type& data) {
		labels.insert(labels.begin()+1, data);
	}

	void applyUpdates(const std::vector<Operation<data_type> >& updates) {
		
		//std::cout << "applyUpdates";

		typedef typename std::vector<Operation<data_type> >::const_iterator u_iterator;

		iterator label_iter = labels.begin();
		u_iterator update_iter = updates.begin();

  		std::vector<data_type> merged;
  		merged.reserve(labels.size()+ updates.size());

  		int removed = 0;
  		int inserted = 0;

		while (update_iter != updates.end()) {
			switch (update_iter->type) {
			case Operation<data_type>::DELETE:
				// We know the element is in here
				while (label_iter->first_weight != update_iter->data.first_weight) {
					merged.push_back(*label_iter++);
				}
				while (label_iter->second_weight != update_iter->data.second_weight) {
					merged.push_back(*label_iter++);
				}
				++label_iter; // delete the element by jumping over it
				++update_iter;
				++removed;
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
				merged.push_back(update_iter->data);
				++update_iter;
				++inserted;
				continue;
			}
		}
		/*while (label_iter != labels.end()) {
			merged.push_back(*label_iter++);
		}*/
		std::copy(label_iter, labels.end(), std::back_inserter(merged));
		labels = merged;
		//std::cout << ": " << labels.size()-2 << " +" << inserted << " -" << removed << std::endl;
	}

	bool empty() {
		return labels.size() == 2;
	}

};


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
	};

    // Permanent and temporary labels per node.
	std::vector<NaiveLabelSet<Label> > labels;

	graph_slot graph;
	ParetoQueue<Data> pq;
	//ParetoSearchStatistics stats;

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
  				return i.data.second_weight < j.data.second_weight;
  			}
  			return i.data.first_weight < j.data.first_weight;
  		}
	} groupByWeight;

	static Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

public:

	typedef typename NaiveLabelSet<Label>::iterator label_iter;
	typedef typename NaiveLabelSet<Label>::iterator const_label_iter;
	typedef typename std::vector<Data>::iterator pareto_iter;

	SequentialParetoSearch(const graph_slot& graph_):
		labels(graph_.numberOfNodes()),
		graph(graph_)
	 {}

	void run(const NodeID node) {
		std::vector<Data> globalMinima;
		std::vector<Data> candidates;
		std::vector<Operation<Data> > updates;
		pq.init(Data(node, Label(0,0)));

		while (!pq.empty()) {
			pq.findParetoMinima(globalMinima);

			for (pareto_iter i = globalMinima.begin(); i != globalMinima.end(); ++i) {
				//std::cout << " min (" << i->node << ": " << i->first_weight << "," << i->second_weight << ")" << std::endl;

				FORALL_EDGES(graph, i->node, eid) {
					const Edge& edge = graph.getEdge(eid);
					candidates.push_back(Data(edge.target, createNewLabel(*i, edge)));
				}
			}
			std::sort(candidates.begin(), candidates.end(), groupByNode);

			merge(candidates, globalMinima, updates);
			pq.applyUpdates(updates);

			globalMinima.clear();
			candidates.clear();
			updates.clear();
		}		
	}

	void merge(const std::vector<Data>& candidates, const std::vector<Data>& minima, std::vector<Operation<Data> >& updates) {
		for (typename std::vector<Data>::const_iterator i = minima.begin(); i != minima.end(); ++i) {
			updates.push_back(Operation<Data>(Operation<Data>::DELETE, *i));
		}
		for (typename std::vector<Data>::const_iterator i = candidates.begin(); i != candidates.end(); ++i) {
			//std::cout << " new candidate (" << i->node << ": " << i->first_weight << "," << i->second_weight << ")" << std::endl;
			if (labels[i->node].add(*i)) {
				updates.push_back(Operation<Data>(Operation<Data>::INSERT, *i));
			}
		}
		std::sort(updates.begin(), updates.end(), groupByWeight);

	}
	
	void printStatistics() {
		
	}

	size_t size(NodeID node) {return labels[node].size(); }

	label_iter begin(NodeID node) { return labels[node].begin(); }
	const_label_iter begin(NodeID node) const { return labels[node].begin(); }
	label_iter end(NodeID node) { return labels[node].end(); }
	const_label_iter end(NodeID node) const { return labels[node].end(); }
};


#endif 
