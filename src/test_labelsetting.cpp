
//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"

//NodeID and EdgdeID
#include "utility/datastructure/graph/GraphTypes.hpp"

#include "utility/datastructure/graph/GraphMacros.h"
#include "utility/datastructure/container/BinaryHeap.hpp"

#include <iostream>
#include <vector>

#include "LabelSettingAlgorithm.hpp"
#include "GraphGenerator.hpp"


typedef utility::datastructure::DirectedIntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Weight;

void createGrid(Graph& graph) {
	graph.addNode();
	const NodeID START = NodeID(0);

	graph.addNode();
	const NodeID END = NodeID(1);

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(2), Weight(1,2)));
	graph.addEdge(NodeID(2), Edge(END, Weight(1,1)));

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(3), Weight(2,1)));
	graph.addEdge(NodeID(3), Edge(END, Weight(1,1)));

	graph.addNode(); // dominated path from start to end
	graph.addEdge(START, Edge(NodeID(4), Weight(1,1)));
	graph.addEdge(NodeID(4), Edge(END, Weight(4,4)));

	graph.finalize();
}
 
int main() {
	GraphGenerator<Graph> generator;
	Graph graph = generator.generateRandomGridGraph(3,3);
	createGrid(graph);

	LabelSettingAlgorithm<Graph> algo(graph);
	algo.run();

	FORALL_NODES(graph, node) {
		std::cout << "Node " << node << ":"; 
		for (LabelSettingAlgorithm<Graph>::const_iterator i = algo.begin(node); i!=algo.end(node); ++i) {
			std::cout << " (" << i->first_weight << ", " << i->second_weight << ")";
		}
		std::cout << std::endl;
	}
	std::cout << "Tests passed successfully." << std::endl;


	return 0;
}

