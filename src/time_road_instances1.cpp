#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <utility>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "timing.h"
#include "memory.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"


static void time(const Graph& graph, NodeID start_node, NodeID end, int total_num, int num, std::string label, bool verbose, int iterations, int p) {
	double timings[iterations];
	double label_count[iterations];
	double memory[iterations];

	for (int i = 0; i < iterations; ++i) {
		LabelSettingAlgorithm algo(graph, p);

		tbb::tick_count start = tbb::tick_count::now();
		algo.run(start_node);
		tbb::tick_count stop = tbb::tick_count::now();

		timings[i] = (stop-start).seconds();
		memory[i] = getCurrentMemorySize();

		label_count[i] = algo.size(end);
		if (verbose && i == 0) {
			algo.printStatistics();
		}
	}
	std::cout << total_num << " " << label << num << " " << pruned_average(timings, iterations, 0) << " " 
		<< pruned_average(label_count, iterations, 0) <<  " " << pruned_average(memory, iterations, 0)/1024 << " " 
		<< getPeakMemorySize()/1024 << " " << p << "  # time in [s], target node label count, memory [mb], peak memory [mb], p " << std::endl;
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

		edges.push_back(std::make_pair(NodeID(start), Edge(NodeID(end), Edge::edge_data(first_weight, second_weight))));
	}
	GraphGenerator<Graph> generator;
	generator.buildGraphFromEdges(graph, edges);
	std::cout << "# Nodes " << graph.numberOfNodes() <<  " Edges " << graph.numberOfEdges() << std::endl;
}

int main(int argc, char ** args) {
	std::cout << "# " << currentConfig() << std::endl;
	bool verbose = false;
	int iterations = 1;
	int total_instance = 1;
	int p = tbb::task_scheduler_init::default_num_threads();

	std::string graphname;
	std::string directory;
	std::ifstream graph_in;
	std::ifstream problems_in;

	int c;
	while( (c = getopt( argc, args, "c:g:d:n:p:v") ) != -1  ){
		switch(c){
		case 'd':
			directory = optarg;
			break;
		case 'g':
			graphname = optarg;
			break;
		case 'n':
			total_instance = atoi(optarg);
			break;
		case 'c':
			iterations = atoi(optarg);
			break;
		case 'p':
			p = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case '?':
			std::cout << "Unrecognized option: " <<  optopt << std::endl;
			break;
		}
	}
	#ifdef PARALLEL_BUILD
		tbb::task_scheduler_init init(p);
	#else
		p = 0;
	#endif

	int instance = 1;
	std::string map(directory + graphname + "1");
	graph_in.open(map.c_str());
	std::cout << "# Map: " << map << std::endl;
	problems_in.open((directory + graphname + "_ODpairs.txt").c_str());

	Graph graph;
	readGraphFromFile(graph, graph_in);
	graph_in.close();

	std::string line;
	while (std::getline(problems_in, line)) {
		int start, end;
		std::istringstream start_stream(line.c_str());
		std::getline(problems_in, line);
		std::istringstream end_stream(line.c_str());
		std::getline(problems_in, line); // ignore empty separator line

		start_stream >> start;
		end_stream >> end;

		time(graph, NodeID(start), NodeID(end), total_instance++, instance++, graphname, verbose, iterations, p);
	}
	problems_in.close();
	return 0;
}

