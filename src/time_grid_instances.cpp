
//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include "LabelSettingAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/tool/timer.h"
#include "timing.h"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

typedef utility::datastructure::DirectedIntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Label;


std::string currentConfig() {
	std::ostringstream out_stream;

	out_stream << STR(LABEL_SETTING_ALGORITHM) << "_";
	if (strcmp(STR(LABEL_SETTING_ALGORITHM), "NodeHeapLabelSettingAlgorithm") == 0) {
		out_stream << STR(LABEL_SET) << "_";
	}
	#ifdef TREE_SET
		out_stream << "TreeSet_";
	#else
		out_stream << "VectorSet_";
	#endif

	#ifdef PRIORITY_LEX
		out_stream << "Lex";
	#endif
	#ifdef PRIORITY_SUM
		out_stream << "Sum";
	#endif
	#ifdef PRIORITY_MAX
		out_stream << "Max";
	#endif

	return out_stream.str();
}

void timeGrid(int num, std::string label, int height, int width, bool verbose, int iterations) {
	Graph graph;
	GraphGenerator<Graph> generator;
	generator.generateRandomGridGraph(graph, height, width);

	double timings[iterations];
	std::fill_n(timings, iterations, 0);
	size_t label_count;

	for (int i = 0; i < iterations; i++) {
		LabelSettingAlgorithm<Graph> algo(graph);

		utility::tool::TimeOfDayTimer timer;
		timer.start();
		algo.run(NodeID(0));
		timer.stop();
		timings[i] = timer.getTimeInSeconds();

		NodeID node = NodeID(NodeID(1));
		label_count = algo.size(node);
		if (verbose) {
			algo.printStatistics();
			std::cout << "Target node label count: " << algo.size(node) << std::endl;
		}
	}
	std::cout << num << " " << label << " " << pruned_average(timings, iterations, 0.25) << " " << label_count << " # instance GX: time in [s], target node label count " << std::endl;
}

int main(int argc, char ** args) {
	bool verbose = false;
	int iterations = 1;

	int c;
	while( (c = getopt( argc, args, "i:v") ) != -1  ){
		switch(c){
		case 'i':
			iterations = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}
	std::cout << "# " << currentConfig() << std::endl;
	timeGrid(1, "G5", 50, 200, verbose, iterations);
	timeGrid(2, "G6", 200, 50, verbose, iterations);
	timeGrid(3, "G14", 200, 200, verbose, iterations);
	timeGrid(4, "G32", 4, 1225, verbose, iterations);
	timeGrid(5, "G33", 2, 2450, verbose, iterations);
	return 0;
}

