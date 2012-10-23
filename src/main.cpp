
//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"

//NodeID and EdgdeID
#include "utility/datastructure/graph/GraphTypes.hpp"

#include "utility/datastructure/graph/GraphMacros.h"
#include "utility/datastructure/container/BinaryHeap.hpp"

#include <iostream>


typedef utility::datastructure::DirectedIntegerWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Weight;
typedef utility::datastructure::BinaryHeap<NodeID, Edge::weight_type, Edge::meta_weight> BinaryHeap;

void createGrid(Graph& graph) {
	graph.addNode();
	const NodeID START = NodeID(0);

	graph.addNode();
	const NodeID END = NodeID(1);

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(2), Weight(1,false,false)));
	graph.addEdge(NodeID(2), Edge(END, Weight(1,false,false)));

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(3), Weight(2,false,false)));
	graph.addEdge(NodeID(3), Edge(END, Weight(1,false,false)));

	graph.addNode(); // dominated path from start to end
	graph.addEdge(START, Edge(NodeID(4), Weight(1,false,false)));
	graph.addEdge(NodeID(4), Edge(END, Weight(2,false,false)));

	graph.finalize();
}

void runDijkstra(const Graph& graph, BinaryHeap& heap) {
	heap.push(NodeID(0), Weight(0,0));

	while(!heap.empty()){
		const NodeID current_node = heap.getMin();
		const Weight current_weight = heap.getMinKey();
		heap.deleteMin();
		
		FORALL_EDGES(graph, current_node, eid){
			const Edge& edge = graph.getEdge(eid);
			const Weight new_weight = Weight(edge.weight + current_weight);

			if (!heap.isReached(edge.target)){
				heap.push(edge.target, new_weight);
			} else if (heap.getKey(edge.target) > new_weight){
				heap.decreaseKey(edge.target, new_weight);
			}
		}
	}
}

int main() {
	Graph graph;
	createGrid(graph);
	BinaryHeap heap((NodeID)graph.numberOfNodes());
	runDijkstra(graph, heap);

	FORALL_NODES(graph, node) {
		std::cout << "Node " << node << " Cost " << heap.getKey(node) << std::endl;
	}

	return 0;
}

