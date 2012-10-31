
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
typedef Edge::edge_data Label;


void assertTrue(bool cond, std::string msg) {
	if (!cond) {
		std::cout << "FAILED: " << msg << std::endl;
		exit(-1);
	}
}

bool contains(LabelSettingAlgorithm<Graph>& algo, const NodeID node, const Label label) {
	return std::find(algo.begin(node), algo.end(node), label) != algo.end(node);
}

void createGridSimple(Graph& graph) {
	graph.addNode();
	const NodeID START = NodeID(0);

	graph.addNode();
	const NodeID END = NodeID(1);

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(2), Label(1,2)));
	graph.addEdge(NodeID(2), Edge(END, Label(1,1)));

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(3), Label(2,1)));
	graph.addEdge(NodeID(3), Edge(END, Label(1,1)));

	graph.addNode(); // dominated path from start to end
	graph.addEdge(START, Edge(NodeID(4), Label(1,1)));
	graph.addEdge(NodeID(4), Edge(END, Label(4,4)));

	graph.finalize();
}

void testGridSimple() {
	Graph graph;
	createGridSimple(graph);

	LabelSettingAlgorithm<Graph> algo(graph);
	algo.run();

	assertTrue(contains(algo, NodeID(1), Label(2,3)), "");
	assertTrue(contains(algo, NodeID(1), Label(3,2)), "");
}
 
int main() {
	testGridSimple();

	GraphGenerator<Graph> generator;
	Graph graph = generator.generateRandomGridGraph(3,3);

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

