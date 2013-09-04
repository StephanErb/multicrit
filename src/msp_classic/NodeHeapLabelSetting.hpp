/*
 * Implementations of a sequential bi-objective shortest path label setting algorithm.
 *  
 * Author: Stephan Erb
 */
#ifndef NODEHEAP_LABELSETTING_H_
#define NODEHEAP_LABELSETTING_H_


#include "../options.hpp"

#include "../Label.hpp"
#include "../Graph.hpp"

#include "NodeHeapLabelSet.hpp"
#include "LabelSettingStatistics.hpp"
#include "../datastructures/container/BinaryHeap.hpp"

#include <iostream>
#include <vector>


/**
 * Label setting algorithm where we store just the best label of each node within a heap used to find the next label to explore.
 * The heap is small, but unfornately it is difficult to find the labels used to populate this heap.
 */
class NodeHeapLabelSettingAlgorithm {	
private:
	template<typename label_type_slot>
	class LabelSet : public NODEHEAP_LABEL_SET<label_type_slot> {};

	typedef typename LabelSet<Label>::Priority Priority;
	typedef typename utility::datastructure::BinaryHeap<NodeID, Priority, std::numeric_limits<Priority>, Label> BinaryHeap;

	BinaryHeap heap;
	std::vector<LabelSet<Label> > labels;
	const Graph& graph;
	LabelSettingStatistics stats;

	static inline Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

public:

	typedef typename LabelSet<Label>::iterator iterator;
	typedef typename LabelSet<Label>::const_iterator const_iterator;

	NodeHeapLabelSettingAlgorithm(const Graph& graph_):
		heap((NodeID)graph_.numberOfNodes()),
		labels(graph_.numberOfNodes()),
		graph(graph_),
		stats(graph.numberOfNodes())
	 {}

	void run(NodeID node) {
		heap.push(node, (Priority) 0, Label(0,0));
		labels[node].init(Label(0,0));

		while (!heap.empty()) {
			const NodeID current_node = heap.getMin();
			const Label current_label =  heap.getUserData(current_node);
			stats.report(NEXT_ITERATION, current_node);

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
				const Label& new_label = createNewLabel(current_label, edge);

				//std::cout << "  relax edge to " << edge.target  << ". New label (" <<  new_label.first_weight << "," << new_label.second_weight << "). ";

				if (labels[edge.target].add(new_label)) {
					stats.report(NEW_LABEL_NONDOMINATED, edge.target, labels[edge.target].size());

					//std::cout << "Label added." << std::endl;
					// label is non-dominated and has been added to the label set
					const Priority priority = LabelSet<Label>::computePriority(new_label);

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
	
	void printStatistics() { std::cout << stats.toString() << std::endl; }
	void printComponentTimings() const { }

	size_t size(NodeID node) {return labels[node].size(); }

	iterator begin(NodeID node) { return labels[node].begin(); }
	const_iterator begin(NodeID node) const { return labels[node].begin(); }
	iterator end(NodeID node) { return labels[node].end(); }
	const_iterator end(NodeID node) const { return labels[node].end(); }
};


#endif 
