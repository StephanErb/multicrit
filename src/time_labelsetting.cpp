#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"
#include "memory.h"

#include <valgrind/callgrind.h>

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"


void benchmark(Graph& graph, NodeID start_node, NodeID target, bool verbose, unsigned short thread_count) {
	LabelSettingAlgorithm algo(graph, thread_count);

	double mem_start = getCurrentMemorySize();
	tbb::tick_count start = tbb::tick_count::now();
	CALLGRIND_START_INSTRUMENTATION;
	algo.run(start_node);
	CALLGRIND_STOP_INSTRUMENTATION;
	tbb::tick_count stop = tbb::tick_count::now();
	double mem_stop = getCurrentMemorySize();


	if (verbose) {
		algo.printStatistics();
		NodeID node = NodeID(target);
		std::cout << "Target node label count: " << algo.size(node) << std::endl;
	}
	std::cout << (stop-start).seconds() << " " << (mem_stop - mem_start)/1024 << " # time in [s], mem in [mb]" << std::endl << std::endl;
}

void timeGrid(int width, int height, bool verbose, unsigned short thread_count) {
	std::cout << "# Raith & Ehrgott Grid (" << width << "x" << height << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraph(graph, width, height);
	benchmark(graph, NodeID(0), NodeID(1), verbose, thread_count);
}

void timeCorrelatedGrid1(int width, int height, bool verbose, unsigned short thread_count) {
	double p = 0.8;
	std::cout << "# Machuca Grid Correlated (p=" << p << ", "<< width << "x" << height << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraphWithCostCorrleation(graph, width, height, p);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1), verbose, thread_count);
}

void timeCorrelatedGrid2(int width, int height, bool verbose, unsigned short thread_count) {
	double p = -0.8;
	std::cout << "# Machuca Grid Correlated (p=" << p << ", "<< width << "x" << height << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraphWithCostCorrleation(graph, width, height, p);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1), verbose, thread_count);
}

void timeExponentialGraph(bool verbose, int n, unsigned short thread_count) {
	std::cout << "# Exponential Graph (root degree&depth=" << n << "):" << std::endl;
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateExponentialStarGraph(graph, n);
	benchmark(graph, NodeID(0), NodeID(graph.numberOfNodes()-1), verbose, thread_count);
}

int main(int argc, char ** args) {
	int width = 50;
	int height = 50;
	int nodes = 12;
	bool verbose = false;
	unsigned short p = tbb::task_scheduler_init::default_num_threads();

	int c;
	while( (c = getopt( argc, args, "w:h:p:v") ) != -1  ){
		switch(c){
		case 'w':
			width = atoi(optarg);
			nodes = width * 0.16;
			break;
		case 'h':
			height = atoi(optarg);
			break;
		case 'p':
			p = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}
	#ifdef PARALLEL_BUILD
		tbb::task_scheduler_init init(p);
	#else
		p = 0;
	#endif

	std::cout << "# " << currentConfig() << std::endl;
	timeExponentialGraph(verbose, nodes, p);
	timeGrid(width, height, verbose, p);
	timeCorrelatedGrid1(width, height, verbose, p);
	timeCorrelatedGrid2(width, height, verbose, p);
	return 0;
}

