#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/tool/timer.h"
#include "timing.h"

void timeGrid(int num, std::string label, int height, int width, bool verbose, int iterations) {
	GraphGenerator<Graph> generator;
	double timings[iterations];
	double label_count[iterations];
	std::fill_n(timings, iterations, 0);
	std::fill_n(label_count, iterations, 0);

	for (int i = 0; i < iterations; i++) {
		Graph graph;
		generator.generateRandomGridGraph(graph, height, width);

		LabelSettingAlgorithm algo(graph);

		utility::tool::TimeOfDayTimer timer;
		timer.start();
		algo.run(NodeID(0));
		timer.stop();
		timings[i] = timer.getTimeInSeconds();

		label_count[i] = algo.size(NodeID(1));
		if (verbose) {
			algo.printStatistics();
		}
	}
	std::cout << num << " " << label << num << " " << pruned_average(timings, iterations, 0.25) << " " << pruned_average(label_count, iterations, 0) << " # time in [s], target node label count " << std::endl;
}

int main(int argc, char ** args) {
	bool verbose = false;
	int iterations = 1;

	int c;
	while( (c = getopt( argc, args, "c:v") ) != -1  ){
		switch(c){
		case 'c':
			iterations = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}
	std::cout << "# Instances of [Raith, Ehrgott 2009]" << std::endl;
	std::cout << "# " << currentConfig() << std::endl;
	timeGrid(1, "G", 30, 40, verbose, iterations);
	timeGrid(2, "G", 20, 80, verbose, iterations);
	timeGrid(3, "G", 50, 90, verbose, iterations);
	timeGrid(4, "G", 90, 50, verbose, iterations);
	timeGrid(5, "G", 50, 200, verbose, iterations);

	timeGrid(6, "G", 200, 50, verbose, iterations);
	timeGrid(7, "G", 100, 150, verbose, iterations);
	timeGrid(8, "G", 150, 100, verbose, iterations);
	timeGrid(9, "G", 100, 200, verbose, iterations);
	timeGrid(10, "G", 200, 100, verbose, iterations);

	timeGrid(11, "G", 200, 150, verbose, iterations);
	timeGrid(12, "G", 50, 50, verbose, iterations);
	timeGrid(13, "G", 100, 100, verbose, iterations);
	timeGrid(14, "G", 200, 200, verbose, iterations);

	timeGrid(15, "G", 2450, 2, verbose, iterations);
	timeGrid(16, "G", 1225, 4, verbose, iterations);
	timeGrid(17, "G", 612, 8, verbose, iterations);
	timeGrid(18, "G", 288, 17, verbose, iterations);
	timeGrid(19, "G", 196, 25, verbose, iterations);
	timeGrid(20, "G", 140, 35, verbose, iterations);
	timeGrid(21, "G", 111, 44, verbose, iterations);
	timeGrid(22, "G", 92, 53, verbose, iterations);
	timeGrid(23, "G", 79, 62, verbose, iterations);
	timeGrid(24, "G", 70, 70, verbose, iterations);
	timeGrid(25, "G", 62, 79, verbose, iterations);
	timeGrid(26, "G", 53, 92, verbose, iterations);
	timeGrid(27, "G", 44, 111, verbose, iterations);
	timeGrid(28, "G", 35, 140, verbose, iterations);
	timeGrid(29, "G", 25, 196, verbose, iterations);
	timeGrid(30, "G", 17, 288, verbose, iterations);
	timeGrid(31, "G", 8, 612, verbose, iterations);
	timeGrid(32, "G", 4, 1225, verbose, iterations);
	timeGrid(33, "G", 2, 2450, verbose, iterations);
	return 0;
}

