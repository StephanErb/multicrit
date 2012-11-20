
//Graph, Node and Edges
#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <algorithm>
#include <utility>

#include "LabelSettingAlgorithm.hpp"
#include "GraphGenerator.hpp"
#include "utility/datastructure/graph/GraphBuilder.hpp"

#include "utility/tool/timer.h"
#include "timing.h"



typedef utility::datastructure::DirectedIntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Label;


static void time(const Graph& graph, NodeID start, NodeID end, int num, std::string label, bool verbose, int iterations) {
	double timings[iterations];
	std::fill_n(timings, iterations, 0);
	size_t label_count;

	for (int i = 0; i < iterations; i++) {
		LabelSettingAlgorithm<Graph> algo(graph);

		utility::tool::TimeOfDayTimer timer;
		timer.start();
		algo.run(start);
		timer.stop();
		timings[i] = timer.getTimeInSeconds();

		label_count = algo.size(end);
		if (verbose) {
			algo.printStatistics();
			std::cout << "Target node label count: " << label_count << std::endl;
		}
	}
	std::cout << num << " " << label << num << " " << pruned_average(timings, iterations, 0.25) << " " << label_count << " # time in [s], target node label count " << std::endl;
}

static void readGraphFromFile(Graph& graph, std::ifstream& in) {
	std::string line;
	char c_line[256];

	// Status line
	std::getline(in, line);
	std::istringstream in_stream( line.c_str() );
	int node_count, edge_count;
	in_stream >> line >> line >> node_count >> edge_count;
	std::cout << "# Nodes " << node_count <<  " Edges " << edge_count << std::endl;

	// Skip two unused lines
	std::getline(in, line);
	std::getline(in, line);

	std::vector< std::pair<NodeID, Edge > > edges;

	while (in.getline(c_line, 256)) {
		std::istringstream in_stream( c_line );
		unsigned int start, end, first_weight, second_weight;
		in_stream >> start >> end >> first_weight >> second_weight;

		edges.push_back(std::make_pair(NodeID(start), Edge(NodeID(end), Label(first_weight, second_weight))));
	}
	GraphGenerator<Graph> generator;
	generator.buildGraphFromEdges(graph, edges);
}

int main(int argc, char ** args) {
	std::cout << "# " << currentConfig() << std::endl;
	bool verbose = false;
	int iterations = 1;
	std::string label;
	std::ifstream graph_in;
	std::ifstream problems_in;

	int c;
	while( (c = getopt( argc, args, "c:vg:i:l:") ) != -1  ){
		switch(c){
		case 'g':
			graph_in.open(optarg);
			std::cout << "# Map: " << optarg << std::endl;
			break;
		case 'i':
			problems_in.open(optarg);
			break;
		case 'l':
			label = optarg;
			break;
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
	Graph graph;
	readGraphFromFile(graph, graph_in);
	graph_in.close();

	int instance = 0;
	std::string line;
	while (std::getline(problems_in, line)) {
		int start, end;
		std::istringstream start_stream(line.c_str());
		std::getline(problems_in, line);
		std::istringstream end_stream(line.c_str());
		std::getline(problems_in, line); // ignore empty separator line

		start_stream >> start;
		end_stream >> end;

		time(graph, NodeID(start), NodeID(end), ++instance, label, verbose, iterations);
	}
	return 0;
}

