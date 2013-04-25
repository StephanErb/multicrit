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


#ifndef BATCH_SIZE /* only used by the parallel impl*/
#define BATCH_SIZE 0 
#endif

void timeGrid(int num, int height, int width, bool verbose, int iterations, double q, int p, int max_costs) {
	GraphGenerator<Graph> generator;
	double timings[iterations];
	double label_count[iterations];
	double memory[iterations];
	std::fill_n(timings, iterations, 0);
	std::fill_n(label_count, iterations, 0);
	std::fill_n(memory, iterations, 0);

	for (int i = 0; i < iterations; i++) {
		Graph graph;
		generator.generateRandomGridGraphWithCostCorrleation(graph, height, width, q, max_costs);

		LabelSettingAlgorithm algo(graph, p);

		tbb::tick_count start = tbb::tick_count::now();
		algo.run(NodeID(0));
		tbb::tick_count stop = tbb::tick_count::now();

		timings[i] = (stop-start).seconds();
		memory[i] = getCurrentMemorySize();

		label_count[i] = algo.size(NodeID(graph.numberOfNodes()-1));
		if (verbose && i == 0) {
			algo.printStatistics();
		}
	}
	std::cout << num << " " << height << " " << width << " " << pruned_average(timings, iterations, 0) << " "
		<< pruned_average(label_count, iterations, 0) << " " << q << " " << pruned_average(memory, iterations, 0)/1024 << " " 
		<< getPeakMemorySize()/1024 << " " << p << " " << max_costs << " " <<  BATCH_SIZE << " # time in [s], target node label count, q, memory [mb], peak memory [mb], p, max_costs, buffer_batch_size" << std::endl;
}

int main(int argc, char ** args) {
	bool verbose = false;
	int iterations = 1;
	double q = 0;
	int p = tbb::task_scheduler_init::default_num_threads();
	int max_costs = 10;
	int n = 0;

	int c;
	while( (c = getopt( argc, args, "c:q:p:m:n:v") ) != -1  ){
		switch(c){
		case 'c':
			iterations = atoi(optarg);
			break;
		case 'q':
			q = atof(optarg);
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'm':
			max_costs = atoi(optarg);
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


	if (n == 0 || BATCH_SIZE == 1) {
		std::cout << "# Class 1 Grid Instances of [Machuca 2012]" << std::endl;	
		std::cout << "# " << currentConfig() << std::endl;
	}
	// He consideres instances from 10 to 100 in increments of 10
	// Results from his thesis:
	//	- Nodes & edges of (100 × 100) is 10,000 and 39,600 respectively. 
	//	- Average number of Pareto-optimal solution paths for the ten larger problems (100 × 100) 
	//		for each correlation is 8.6, 96.2, 274.7, 435.6 and 772.3 in order of decreasing value of ρ.


	if (n != 0) {
		timeGrid(0, n, n, verbose, iterations, q, p, max_costs);
	} else {
		timeGrid(1,   20, 20, verbose, iterations, q, p, max_costs);
		timeGrid(2,   40, 40, verbose, iterations, q, p, max_costs);
		timeGrid(3,   60, 60, verbose, iterations, q, p, max_costs);
		timeGrid(4,   80, 80, verbose, iterations, q, p, max_costs);
		timeGrid(5,  100, 100, verbose, iterations, q, p, max_costs);
		timeGrid(6,  120, 120, verbose, iterations, q, p, max_costs);
		timeGrid(7,  140, 140, verbose, iterations, q, p, max_costs);
		timeGrid(8,  160, 160, verbose, iterations, q, p, max_costs);
		timeGrid(9,  180, 180, verbose, iterations, q, p, max_costs);
		timeGrid(10, 200, 200, verbose, iterations, q, p, max_costs);
		timeGrid(11, 220, 220, verbose, iterations, q, p, max_costs);
		timeGrid(12, 240, 240, verbose, iterations, q, p, max_costs);
		timeGrid(13, 260, 260, verbose, iterations, q, p, max_costs);
		timeGrid(14, 280, 280, verbose, iterations, q, p, max_costs);
		timeGrid(15, 300, 300, verbose, iterations, q, p, max_costs);
		timeGrid(16, 320, 320, verbose, iterations, q, p, max_costs);
		timeGrid(17, 340, 340, verbose, iterations, q, p, max_costs);
		timeGrid(18, 360, 360, verbose, iterations, q, p, max_costs);
		timeGrid(19, 380, 380, verbose, iterations, q, p, max_costs);
		timeGrid(20, 400, 400, verbose, iterations, q, p, max_costs);
		timeGrid(21, 420, 420, verbose, iterations, q, p, max_costs);
		timeGrid(22, 440, 440, verbose, iterations, q, p, max_costs);
		timeGrid(23, 460, 460, verbose, iterations, q, p, max_costs);
		timeGrid(24, 480, 480, verbose, iterations, q, p, max_costs);
		timeGrid(25, 500, 500, verbose, iterations, q, p, max_costs);		
	}

	return 0;
}

