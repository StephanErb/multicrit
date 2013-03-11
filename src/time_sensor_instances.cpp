#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <utility>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/algorithm/string.hpp>

#include "timing.h"
#include "memory.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"

typedef utility::datastructure::DirectedIntegerWeightedEdge TempEdge;
typedef utility::datastructure::KGraph<TempEdge> TempGraph;

// Mersenne Twister random number genrator
boost::mt19937 gen;

static void time(const Graph& graph, std::string nodecount, std::string degree, bool verbose, int iterations, int p) {
	double timings[iterations];
	double label_count[iterations];
	double memory[iterations];
	std::fill_n(timings, iterations, 0);
	std::fill_n(label_count, iterations, 0);
	std::fill_n(memory, iterations, 0);

	boost::uniform_int<> dist(0, graph.numberOfNodes());

	for (int i = 0; i < iterations; ++i) {
		LabelSettingAlgorithm algo(graph, p);

		NodeID start_node = (NodeID) dist(gen);
		NodeID end_node = (NodeID) dist(gen);

		tbb::tick_count start = tbb::tick_count::now();
		algo.run(start_node);
		tbb::tick_count stop = tbb::tick_count::now();

		timings[i] = (stop-start).seconds();
		memory[i] = getCurrentMemorySize();

		label_count[i] = algo.size(end_node);
		if (verbose) {
			algo.printStatistics();
		}
	}
	std::cout << nodecount << " " << degree << " " << pruned_average(timings, iterations, 0) << " " 
		<< pruned_average(label_count, iterations, 0) <<  " " << pruned_average(memory, iterations, 0)/1024 << " " 
		<< getPeakMemorySize()/1024 << " " << p << "  # n, degree, time in [s], target node label count, memory [mb], peak memory [mb], p" << std::endl;
}


static void translate_to_biweight_graph_with_hopcount(TempGraph& temp_graph, Graph& graph) {
	std::vector< std::pair<NodeID, Edge > > edges;
	FORALL_NODES(temp_graph, node) {
		FORALL_EDGES(temp_graph, node, eid) {
			const TempEdge& temp_edge = temp_graph.getEdge(eid);
			edges.push_back({node, Edge(temp_edge.target, Edge::edge_data(temp_edge.weight, /*hopcount*/ 1))});
		}
	}
	GraphGenerator<Graph> generator;
	generator.buildGraphFromEdges(graph, edges);
	std::cout << "# Nodes " << graph.numberOfNodes() <<  " Edges " << graph.numberOfEdges() << std::endl;
}

int main(int argc, char ** args) {
	std::cout << "# " << currentConfig() << std::endl;
	bool verbose = false;
	int iterations = 1;
	int p = tbb::task_scheduler_init::default_num_threads();

	std::string graphname;
	std::string directory;

	int c;
	while( (c = getopt( argc, args, "c:g:d:p:v") ) != -1  ){
		switch(c){
		case 'd':
			directory = optarg;
			break;
		case 'g':
			graphname = optarg;
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

	std::cout << "# Map: " << graphname << " " << std::endl;

	std::string filename(directory + graphname + ".ksg");
	std::ifstream input_graph_file( filename, std::ios::in | std::ios::binary );
 	if( !input_graph_file.is_open() ){
 		std::cout << "Failed to open Input graph: " << filename << std::endl;
		return 0;
	}
	TempGraph temp_graph;
 	temp_graph.deserializeFrom<TempEdge>( input_graph_file );
 	input_graph_file.close();
 	Graph graph;
 	translate_to_biweight_graph_with_hopcount(temp_graph, graph);


	std::vector<std::string> strs;
	boost::split(strs, graphname, boost::is_any_of("n_d"));

	time(graph, strs[1], strs[3], verbose, iterations, p);
	return 0;
}
