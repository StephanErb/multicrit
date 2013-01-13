#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/tool/timer.h"
#include "timing.h"
#include "memory.h"


void timeGrid(int num, int height, int width, bool verbose, int iterations, double p) {
	GraphGenerator<Graph> generator;
	double timings[iterations];
	double label_count[iterations];
	double memory[iterations];
	std::fill_n(timings, iterations, 0);
	std::fill_n(label_count, iterations, 0);
	std::fill_n(memory, iterations, 0);

	for (int i = 0; i < iterations; i++) {
		Graph graph;
		generator.generateRandomGridGraphWithCostCorrleation(graph, height, width, p);

		LabelSettingAlgorithm algo(graph);

		utility::tool::TimeOfDayTimer timer;
		timer.start();
		algo.run(NodeID(0));
		timer.stop();
		timings[i] = timer.getTimeInSeconds();
		memory[i] = getCurrentMemorySize();

		label_count[i] = algo.size(NodeID(graph.numberOfNodes()-1));
		if (verbose) {
			algo.printStatistics();
		}
	}
	std::cout << num << " " << height << "x" << width << " " << pruned_average(timings, iterations, 0) << " "
		<< pruned_average(label_count, iterations, 0) << " " << p << " " << pruned_average(memory, iterations, 0)/1024 << " " 
		<< getPeakMemorySize()/1024 << " # time in [s], target node label count, p, memory [mb], peak memory [mb] " << std::endl;
	
}

int main(int argc, char ** args) {
	bool verbose = false;
	int iterations = 1;
	double p = 0;

	int c;
	while( (c = getopt( argc, args, "c:p:v") ) != -1  ){
		switch(c){
		case 'c':
			iterations = atoi(optarg);
			break;
		case 'p':
			p = atof(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}


	std::cout << "# Class 1 Grid Instances of [Machuca 2012]" << std::endl;
	// He consideres instances from 10 to 100 in increments of 10
	// Results from his thesis:
	//	- Nodes & edges of (100 × 100) is 10,000 and 39,600 respectively. 
	//	- Average number of Pareto-optimal solution paths for the ten larger problems (100 × 100) 
	//		for each correlation is 8.6, 96.2, 274.7, 435.6 and 772.3 in order of decreasing value of ρ.

	std::cout << "# " << currentConfig() << std::endl;
	timeGrid(1, 20, 20, verbose, iterations, p);
	timeGrid(2, 40, 40, verbose, iterations, p);
	timeGrid(3, 60, 60, verbose, iterations, p);
	timeGrid(4, 80, 80, verbose, iterations, p);
	timeGrid(5, 100, 100, verbose, iterations, p);
	timeGrid(6, 120, 120, verbose, iterations, p);
	timeGrid(7, 140, 140, verbose, iterations, p);
	timeGrid(8, 160, 160, verbose, iterations, p);
	timeGrid(9, 180, 180, verbose, iterations, p);
	timeGrid(10, 200, 200, verbose, iterations, p);

	return 0;
}

