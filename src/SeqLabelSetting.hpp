/*
 * Implementations of a sequential bi-objective shortest path label setting algorithm.
 *  
 * Author: Stephan Erb
 */
#ifndef SEQ_LABELSETTING_H_
#define SEQ_LABELSETTING_H_

#include "utility/datastructure/graph/GraphMacros.h"
#include "utility/datastructure/container/BinaryHeap.hpp"
#include "UnboundBinaryHeap.hpp"

#include <iostream>
#include <vector>
#include "SeqLabelSet.hpp"
#include "SeqLabelSettingStatistics.hpp"

template<typename graph_slot>
class NodeHeapLabelSettingAlgorithm {
private:

	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Label;
	typedef typename LabelSet<Label>::Priority Priority;
	typedef typename utility::datastructure::BinaryHeap<NodeID, Priority, std::numeric_limits<Priority>, Label> BinaryHeap;

	BinaryHeap heap;
	std::vector<LabelSet<Label> > labels;
	graph_slot graph;
	LabelSettingStatistics stats;

	static Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

public:

	typedef typename LabelSet<Label>::iterator iterator;
	typedef typename LabelSet<Label>::iterator const_iterator;

	NodeHeapLabelSettingAlgorithm(const graph_slot& graph_):
		heap((NodeID)graph_.numberOfNodes()),
		labels(graph_.numberOfNodes()),
		graph(graph_),
		stats(graph.numberOfNodes())
	 {}

	void run(NodeID node) {
		heap.push(node, (Priority) 0, Label(0,0));

		while (!heap.empty()) {
			const NodeID current_node = heap.getMin();
			const Label current_label =  heap.getUserData(current_node);
			stats.report(BEST_NODE, current_node);

			//std::cout << "Selecting node " << current_node  << " via label (" << 
			//	current_label.first_weight << "," << current_label.second_weight << "):"<< std::endl;

			labels[current_node].markBestLabelAsPermantent();
			if (labels[current_node].hasTemporaryLabels()) {
				heap.getUserData(current_node) = labels[current_node].getBestTemporaryLabel();
				heap.increaseKey(current_node, labels[current_node].getPriorityOfBestTemporaryLabel());
			} else {
				heap.deleteMin();
			}

			FORALL_EDGES(graph, current_node, eid) {
				const Edge& edge = graph.getEdge(eid);
				Label new_label = createNewLabel(current_label, edge);

				//std::cout << "  relax edge to " << edge.target  << ". New label (" <<  new_label.first_weight << "," << new_label.second_weight << "). ";

				if (labels[edge.target].add(new_label)) {
					stats.report(NEW_LABEL_NONDOMINATED, edge.target, labels[edge.target].size());

					//std::cout << "Label added." << std::endl;
					// label is non-dominated and has been added to the label set
					Priority priority = LabelSet<Label>::computePriority(new_label);

					if (!heap.contains(edge.target)) {
						heap.reinsertingPush(edge.target, priority, new_label);

					} else if (priority < heap.getKey(edge.target)) {
						// new label represents the new best label / best known path to the target
						heap.decreaseKey(edge.target, priority); 
						heap.getUserData(edge.target) = new_label;
						stats.report(NEW_BEST_LABEL, edge.target);
					}
				} else {
					stats.report(NEW_LABEL_DOMINATED, edge.target, labels[edge.target].size());
					//std::cout << "Label DOMINATED." << std::endl;
				}
			}
		}		
	}
	
	void printStatistics() {
		std::cout << stats.toString() << std::endl;
	}

	size_t size(NodeID node) {return labels[node].size(); }

	iterator begin(NodeID node) { return labels[node].begin(); }
	const_iterator begin(NodeID node) const { return labels[node].begin(); }
	iterator end(NodeID node) { return labels[node].end(); }
	const_iterator end(NodeID node) const { return labels[node].end(); }
};



template<typename graph_slot>
class SharedHeapLabelSettingAlgorithm {
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Label;
	typedef typename LabelSet<Label>::Priority Priority;

	struct Data {
		NodeID node;
  		Label label;
 		Data(const NodeID& x, const Label& y) : node(x), label(y) {}
    	Data(const Data &p) : node(p.node), label(p.label) { }
	};
	typedef UnboundBinaryHeap<Priority, std::numeric_limits<Priority>, Data> BinaryHeap;
	typedef typename BinaryHeap::handle handle;
	typedef std::pair<NodeID, Label> NodeLabelPair;

	BinaryHeap heap;
	std::vector<SharedHeapLabelSet<Label, BinaryHeap> > labels;
	graph_slot graph;
	LabelSettingStatistics stats;

	static Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

public:

	typedef typename SharedHeapLabelSet<Label, BinaryHeap>::iterator iterator;
	typedef typename SharedHeapLabelSet<Label, BinaryHeap>::iterator const_iterator;

	SharedHeapLabelSettingAlgorithm(const graph_slot& graph_):
		labels(graph_.numberOfNodes()),
		graph(graph_),
		stats(graph.numberOfNodes())
	 {}

	void run(NodeID node) {
		heap.push((Priority) 0, Data(node, Label(0,0)));

		while (!heap.empty()) {
			const Data current = heap.getUserData(heap.getMin());
			heap.deleteMin();
			stats.report(BEST_NODE, current.node);

			//std::cout << "Selecting node " << current.node  << " via label (" << 
			//	current.label.first_weight << "," << current.label.second_weight << "):"<< std::endl;

			FORALL_EDGES(graph, current.node, eid) {
				const Edge& edge = graph.getEdge(eid);
				Label new_label = createNewLabel(current.label, edge);

				//std::cout << "  relax edge to " << edge.target  << ". New label (" <<  new_label.first_weight << "," << new_label.second_weight << "). ";

				if (labels[edge.target].add(edge.target, new_label, heap)) {
					stats.report(NEW_LABEL_NONDOMINATED, edge.target, labels[edge.target].size());
					//std::cout << "Label added." << std::endl;
				} else {
					stats.report(NEW_LABEL_DOMINATED, edge.target, labels[edge.target].size());
					//std::cout << "Label DOMINATED." << std::endl;
				}
			}
		}		
	}
	
	void printStatistics() {
		std::cout << stats.toString() << std::endl;
	}

	size_t size(NodeID node) {return labels[node].size(); }

	iterator begin(NodeID node) { return labels[node].begin(); }
	const_iterator begin(NodeID node) const { return labels[node].begin(); }
	iterator end(NodeID node) { return labels[node].end(); }
	const_iterator end(NodeID node) const { return labels[node].end(); }
};

#endif 
