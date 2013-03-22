#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "timing.h"
#include "memory.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"


void timeGrid(int num, int height, int width, bool verbose, int iterations, double q, int p) {
	GraphGenerator<Graph> generator;
	double timings[iterations];
	double label_count[iterations];
	double memory[iterations];
	std::fill_n(timings, iterations, 0);
	std::fill_n(label_count, iterations, 0);
	std::fill_n(memory, iterations, 0);

	for (int i = 0; i < iterations; i++) {
		Graph graph;
		generator.generateRandomGridGraphWithCostCorrleation(graph, height, width, q);

		LabelSettingAlgorithm algo(graph, p);

		tbb::tick_count start = tbb::tick_count::now();
		algo.run(NodeID(0));
		tbb::tick_count stop = tbb::tick_count::now();

		timings[i] = (stop-start).seconds();
		memory[i] = getCurrentMemorySize();

		label_count[i] = algo.size(NodeID(graph.numberOfNodes()-1));
		if (verbose) {
			algo.printStatistics();
		}
	}
	std::cout << num << " " << height << "x" << width << " " << pruned_average(timings, iterations, 0) << " "
		<< pruned_average(label_count, iterations, 0) << " " << q << " " << pruned_average(memory, iterations, 0)/1024 << " " 
		<< getPeakMemorySize()/1024 << " " << p << " # time in [s], target node label count, q, memory [mb], peak memory [mb], p " << std::endl;
	
}

int main(int argc, char ** args) {
	bool verbose = false;
	int iterations = 1;
	double q = 0;
	int p = tbb::task_scheduler_init::default_num_threads();

	int c;
	while( (c = getopt( argc, args, "c:q:p:v") ) != -1  ){
		switch(c){
		case 'c':
			iterations = atoi(optarg);
			break;
		case 'q':
			q = atof(optarg);
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


	std::cout << "# Class 1 Grid Instances of [Machuca 2012]" << std::endl;
	// He consideres instances from 10 to 100 in increments of 10
	// Results from his thesis:
	//	- Nodes & edges of (100 × 100) is 10,000 and 39,600 respectively. 
	//	- Average number of Pareto-optimal solution paths for the ten larger problems (100 × 100) 
	//		for each correlation is 8.6, 96.2, 274.7, 435.6 and 772.3 in order of decreasing value of ρ.

	std::cout << "# " << currentConfig() << std::endl;
	timeGrid(1,   20, 20, verbose, iterations, q, p);
	timeGrid(2,   40, 40, verbose, iterations, q, p);
	timeGrid(3,   60, 60, verbose, iterations, q, p);
	timeGrid(4,   80, 80, verbose, iterations, q, p);
	timeGrid(5,  100, 100, verbose, iterations, q, p);
	timeGrid(6,  120, 120, verbose, iterations, q, p);
	timeGrid(7,  140, 140, verbose, iterations, q, p);
	timeGrid(8,  160, 160, verbose, iterations, q, p);
	timeGrid(9,  180, 180, verbose, iterations, q, p);
	timeGrid(10, 200, 200, verbose, iterations, q, p);
	timeGrid(11, 220, 220, verbose, iterations, q, p);
	timeGrid(12, 240, 240, verbose, iterations, q, p);
	timeGrid(13, 260, 260, verbose, iterations, q, p);
	timeGrid(14, 280, 280, verbose, iterations, q, p);
	timeGrid(15, 300, 300, verbose, iterations, q, p);

	return 0;
}

