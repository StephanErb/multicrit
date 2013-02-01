#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/tool/timer.h"
#include <valgrind/callgrind.h>


void benchmark(Graph& graph, NodeID start, NodeID target, bool verbose) {
	LabelSettingAlgorithm algo(graph);

	utility::tool::TimeOfDayTimer timer;
	timer.start();
	CALLGRIND_START_INSTRUMENTATION;
	algo.run(start);
	CALLGRIND_STOP_INSTRUMENTATION;
	timer.stop();

	if (verbose) {
		algo.printStatistics();
		NodeID node = NodeID(target);
		std::cout << "Target node label count: " << algo.size(node) << std::endl;
	}
	std::cout << timer.getTimeInSeconds()  << " # time in [s]" << std::endl << std::endl;
}

void timeGrid(int width, int height, bool verbose) {
	std::cout << "# Raith & Ehrgott Grid (" << width << "x" << height << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraph(graph, width, height);
	benchmark(graph, NodeID(0), NodeID(1), verbose);
}

void timeCorrelatedGrid1(int width, int height, bool verbose) {
	double p = 0.8;
	std::cout << "# Machuca Grid Correlated (p=" << p << ", "<< width << "x" << height << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraphWithCostCorrleation(graph, width, height, p);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1), verbose);
}

void timeCorrelatedGrid2(int width, int height, bool verbose) {
	double p = -0.8;
	std::cout << "# Machuca Grid Correlated (p=" << p << ", "<< width << "x" << height << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraphWithCostCorrleation(graph, width, height, p);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1), verbose);
}

void timeExponentialGraph(bool verbose, int n) {
	std::cout << "# Exponential Graph (root degree&depth=" << n << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialStarGraph(graph, n);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1), verbose);
}

int main(int argc, char ** args) {
	int width = 50;
	int height = 50;
	int nodes = 12;
	bool verbose = false;

	int c;
	while( (c = getopt( argc, args, "w:h:v") ) != -1  ){
		switch(c){
		case 'w':
			width = atoi(optarg);
			nodes = width * 0.16;
			break;
		case 'h':
			height = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}
	std::cout << "# " << currentConfig() << std::endl;
	timeExponentialGraph(verbose, nodes);
	timeGrid(width, height, verbose);
	timeCorrelatedGrid1(width, height, verbose);
	timeCorrelatedGrid2(width, height, verbose);
	return 0;
}

