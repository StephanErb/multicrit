
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

 
void timeGrid() {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraph(graph, 200, 200);
	LabelSettingAlgorithm<Graph> algo(graph);

	utility::tool::TimeOfDayTimer timer;
	timer.start();

	algo.run(NodeID(0));

	timer.stop();
	std::cout << timer.getTimeInSeconds()  << " [s]" << std::endl;

	NodeID node = NodeID(1);
	std::cout << "Label count (target): " << algo.size(node) << std::endl;
}

void timeCorrelatedGrid() {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraphWithCostCorrleation(graph, 200, 200, -0.4);
	LabelSettingAlgorithm<Graph> algo(graph);

	utility::tool::TimeOfDayTimer timer;
	timer.start();

	algo.run(NodeID(0));

	timer.stop();
	std::cout << timer.getTimeInSeconds()  << " [s]" << std::endl;

	NodeID node = NodeID(graph.numberOfNodes()-1);
	std::cout << "Label count (target): " << algo.size(node) << std::endl;
}

void timeExponentialGraph() {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialGraph(graph, 22);
	LabelSettingAlgorithm<Graph> algo(graph);

	utility::tool::TimeOfDayTimer timer;
	timer.start();

	algo.run(NodeID(0));

	timer.stop();
	std::cout << timer.getTimeInSeconds()  << " [s]" << std::endl;

	NodeID node = NodeID(graph.numberOfNodes()-1);
	std::cout << "Label count (target): " << algo.size(node) << std::endl;
}

int main(int argc, char ** args) {
	timeGrid();
	timeCorrelatedGrid();

/*	std::ifstream ksg_input_stream;

	int c;
	while( (c = getopt( argc, args, "g:") ) != -1  ){
		switch(c){
		case 'g':
			ksg_input_stream.open( optarg, std::ios::in | std::ios::binary );
			std::cout << "Opening: " << optarg << std::endl;
			break;
		}
	}

	Graph graph;

	std::cout << "Loading ksg..." << std::flush;
	graph.deserialize( ksg_input_stream );
	std::cout << "done." << std::endl;
	ksg_input_stream.close();

	graph.checkIntegrity();
	
*/
	/*GraphGenerator<Graph> generator;
	//generator.printGraph(graph);

	std::cout << "Algo started" << std::endl;
	LabelSettingAlgorithm<Graph> algo(graph);

	utility::tool::TimeOfDayTimer timer;
	timer.start();

	algo.run(NodeID( 100));

	timer.stop();
	std::cout << timer.getTimeInSeconds()  << " [s]" << std::endl;

	NodeID node = NodeID(graph.numberOfNodes()-1);
	std::cout << "Label count (target): " << algo.size(node) << std::endl;
	*/
	
	return 0;
}

