#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/timing.h"
#include "utility/memory.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"


#ifndef BATCH_SIZE /* only used by the parallel impl*/
#define BATCH_SIZE 0 
#endif

void timeGrid(int num, int height, int width, bool verbose, int iterations, double q, int p, int max_costs, bool subcomponent_timings) {
	GraphGenerator<Graph> generator;
	double timings[iterations];
	double label_count[iterations];
	double memory[iterations];

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
		if (subcomponent_timings) {
			std::cout << num << " " << height << " " << width << " ";
			algo.printComponentTimings();
			std::cout << " " << timings[i] << std::endl;
			std::cout << " # ";
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
	bool subcomponent_timings = false;

	int c;
	while( (c = getopt( argc, args, "c:q:p:m:n:vs") ) != -1  ){
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
		case 's':
			subcomponent_timings = true;
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


	if (n == 0 || BATCH_SIZE == 8) {
		std::cout << "# Class 1 Grid Instances of [Machuca 2012]" << std::endl;	
		std::cout << "# " << currentConfig() << std::endl;
	}
	// He consideres instances from 10 to 100 in increments of 10
	// Results from his thesis:
	//	- Nodes & edges of (100 × 100) is 10,000 and 39,600 respectively. 
	//	- Average number of Pareto-optimal solution paths for the ten larger problems (100 × 100) 
	//		for each correlation is 8.6, 96.2, 274.7, 435.6 and 772.3 in order of decreasing value of ρ.


	if (n != 0) {
		timeGrid(0, n, n, verbose, iterations, q, p, max_costs, subcomponent_timings);
	} else {
		timeGrid(1,  100, 100, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(2,  125, 125, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(3,  150, 150, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(4,  175, 175, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(5,  200, 200, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(6,  225, 225, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(7,  250, 250, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(8,  275, 275, verbose, iterations, q, p, max_costs, subcomponent_timings);
		timeGrid(9,  300, 300, verbose, iterations, q, p, max_costs, subcomponent_timings);
		if (q > -0.4 || max_costs < 100) {
			timeGrid(10,  325, 325, verbose, iterations, q, p, max_costs, subcomponent_timings);
			timeGrid(11, 350, 350, verbose, iterations, q, p, max_costs, subcomponent_timings);
			timeGrid(12, 375, 375, verbose, iterations, q, p, max_costs, subcomponent_timings);
			timeGrid(13, 400, 400, verbose, iterations, q, p, max_costs, subcomponent_timings);
			if (q > 0.4 || max_costs < 100) {
				timeGrid(14, 425, 425, verbose, iterations, q, p, max_costs, subcomponent_timings);
				timeGrid(15, 450, 450, verbose, iterations, q, p, max_costs, subcomponent_timings);
				timeGrid(16, 475, 475, verbose, iterations, q, p, max_costs, subcomponent_timings);
				timeGrid(17, 500, 500, verbose, iterations, q, p, max_costs, subcomponent_timings);
			}
		}
	}

	return 0;
}

