
//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include <iostream>
#include <vector>
#include <fstream>

#include "LabelSettingAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/tool/timer.h"

typedef utility::datastructure::DirectedIntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Label;


void benchmark(Graph& graph, NodeID start, NodeID target) {
	LabelSettingAlgorithm<Graph> algo(graph);

	utility::tool::TimeOfDayTimer timer;
	timer.start();
	algo.run(start);
	timer.stop();

	NodeID node = NodeID(graph.numberOfNodes()-1);
	std::cout << "  Target node label count: " << algo.size(node) << std::endl;

	std::cout << "  " << timer.getTimeInSeconds()  << " [s]" << std::endl << std::endl;
}

void timeGrid() {
	std::cout << "Raith & Ehrgott Grid (hard): " << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraph(graph, 200, 200);
	benchmark(graph, NodeID(0), NodeID(1));
}

void timeCorrelatedGrid1() {
	std::cout << "Machuca Grid Correlated (simple): " << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraphWithCostCorrleation(graph, 200, 200, 0.4);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1));
}

void timeCorrelatedGrid2() {
	std::cout << "Machuca Grid Correlated (difficult): " << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraphWithCostCorrleation(graph, 200, 200, -0.4);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1));
}

void timeExponentialGraph() {
	std::cout << "Exponential Graph: " << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialGraph(graph, 20);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1));
}

int main(int argc, char ** args) {
	timeExponentialGraph();
	timeGrid();
	timeCorrelatedGrid1();
	timeCorrelatedGrid2();
	return 0;
}

