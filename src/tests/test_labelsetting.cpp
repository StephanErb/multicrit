#include <iostream>
#include "../BiCritShortestPathAlgorithm.hpp"
#include "../GraphGenerator.hpp"

void assertTrue(bool cond, std::string msg) {
	if (!cond) {
		std::cout << "FAILED: " << msg << std::endl;
		exit(-1);
	}
}

bool contains(LabelSettingAlgorithm& algo, const NodeID node, const Label label) {
	return std::find(algo.begin(node), algo.end(node), label) != algo.end(node);
}

void createGridSimple(Graph& graph) {
	graph.addNode();
	const NodeID START = NodeID(0);

	graph.addNode();
	const NodeID END = NodeID(1);

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(2), Edge::edge_data(1,2)));
	graph.addEdge(NodeID(2), Edge(END, Edge::edge_data(1,1)));

	graph.addNode(); // non-dominated path from start to end
	graph.addEdge(START, Edge(NodeID(3), Edge::edge_data(2,1)));
	graph.addEdge(NodeID(3), Edge(END, Edge::edge_data(1,1)));

	graph.addNode(); // dominated path from start to end
	graph.addEdge(START, Edge(NodeID(4), Edge::edge_data(1,1)));
	graph.addEdge(NodeID(4), Edge(END, Edge::edge_data(4,4)));

	graph.finalize();
}

void testGridSimple() {
	Graph graph;
	createGridSimple(graph);

	LabelSettingAlgorithm algo(graph);
	algo.run(NodeID(0));

	assertTrue(algo.size(NodeID(1)) == 2, "Should not contain dominated labels");
	assertTrue(contains(algo, NodeID(1), Label(2,3)), "");
	assertTrue(contains(algo, NodeID(1), Label(3,2)), "");
}

void testGridExponential() {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialGraph(graph, 20);

	LabelSettingAlgorithm algo(graph);
	algo.run(NodeID(0));

	assertTrue(algo.size(NodeID(0)) == 0, "Start node should have no labels");
	assertTrue(algo.size(NodeID(1)) == 1, "Second node should have one labels");

	unsigned int label_count = 2;
	for (unsigned int i=2; i<graph.numberOfNodes(); i=i+2) {
		assertTrue(algo.size(NodeID(i)) == label_count, "Expect exponential num of labels");
		label_count = 2 * label_count;
	}
}
 
int main() {
	testGridSimple();
	testGridExponential();

	std::cout << "Tests passed successfully." << std::endl;
	return 0;
}

