/*
 * Implementations of a sequential bi-objective shortest path label setting algorithm.
 *  
 * Author: Stephan Erb
 */
#ifndef SHAREDHEAP_LABELSETTING_H_
#define SHAREDHEAP_LABELSETTING_H_

#include "../Label.hpp"
#include "../Graph.hpp"

#include "SharedHeapLabelSet.hpp"
#include "LabelSettingStatistics.hpp"
#include "../datastructures/container/UnboundBinaryHeap.hpp"

#include <iostream>
#include <vector>

/**
 * Label setting algorithm where all tenative labels are stored in a single, large heap.
 */
class SharedHeapLabelSettingAlgorithm {
private:
	typedef typename LabelSetBase<Label, Label>::Priority Priority;

	typedef UnboundBinaryHeap<Priority, std::numeric_limits<Priority>, NodeLabel> BinaryHeap;
	typedef typename BinaryHeap::handle handle;

	BinaryHeap heap;
	std::vector<SharedHeapLabelSet<Label, BinaryHeap> > labels;
	const Graph& graph;
	LabelSettingStatistics stats;

	static inline Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

public:

	typedef typename SharedHeapLabelSet<Label, BinaryHeap>::iterator iterator;
	typedef typename SharedHeapLabelSet<Label, BinaryHeap>::const_iterator const_iterator;

	SharedHeapLabelSettingAlgorithm(const Graph& graph_):
		heap(LARGE_ENOUGH_FOR_EVERYTHING),
		labels(graph_.numberOfNodes()),
		graph(graph_),
		stats(graph.numberOfNodes())
	 {}

	void run(NodeID node) {
		heap.push((Priority) 0, NodeLabel(node, Label(0,0)));
		labels[node].init(Label(0,0));

		while (!heap.empty()) {
			const NodeLabel current = heap.getUserData(heap.getMin());
			heap.deleteMin();
			stats.report(NEXT_ITERATION, current.node);

			//std::cout << "Selecting node " << current.node  << " via label (" << 
			//	current.label.first_weight << "," << current.label.second_weight << "):"<< std::endl;

			FORALL_EDGES(graph, current.node, eid) {
				const Edge& edge = graph.getEdge(eid);
				//std::cout << "  relax edge to " << edge.target  << ". New label (" <<  new_label.first_weight << "," << new_label.second_weight << "). ";

				if (labels[edge.target].add(edge.target, createNewLabel(current, edge), heap)) {
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
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			std::cout << "# LabelSet Modifications: insertion position, dominance position" << std::endl;
			std::vector<unsigned long> set_insertions(101);
			std::vector<unsigned long> set_dominations(101);
			for (auto& ls : labels) {
				for (size_t i=0; i < 101; ++i) {
					set_insertions[i] += ls.set_insertions[i];
					set_dominations[i]  += ls.set_dominations[i];
				}
			}
			for (size_t i=0; i < 101; ++i) {
				std::cout << i << " " << set_insertions[i] << " " << set_dominations[i] << std::endl;
			}
		#endif
		std::cout << stats.toString() << std::endl;
	}

	void printComponentTimings() const { }

	size_t size(NodeID node) { return labels[node].size(); }

	iterator begin(NodeID node) { return labels[node].begin(); }
	const_iterator begin(NodeID node) const { return labels[node].begin(); }
	iterator end(NodeID node) { return labels[node].end(); }
	const_iterator end(NodeID node) const { return labels[node].end(); }
};

#endif 
