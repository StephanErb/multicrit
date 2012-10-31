
//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include <iostream>
#include <vector>

#include "LabelSettingAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/tool/timer.h"

typedef utility::datastructure::DirectedIntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Label;

 
int main() {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraph(graph, 50, 50);
	LabelSettingAlgorithm<Graph> algo(graph);

	utility::tool::TimeOfDayTimer timer;
	timer.start();

	algo.run(NodeID(0));

	timer.stop();
	std::cout << timer.getTimeInSeconds()  << " [s]" << std::endl;

	NodeID node = NodeID(1);
	std::cout << "Label count (target): " << algo.size(node) << std::endl;
	
	return 0;
}

