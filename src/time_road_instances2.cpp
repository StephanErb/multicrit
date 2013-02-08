#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <utility>

#include "BiCritShortestPathAlgorithm.hpp"
#include "GraphGenerator.hpp"

#include "utility/tool/timer.h"
#include "timing.h"
#include "memory.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"

typedef utility::datastructure::DirectedIntegerWeightedEdge TempEdge;
typedef utility::datastructure::KGraph<TempEdge> TempGraph;


static void time(const Graph& graph, NodeID start, NodeID end, int total_num, int num, std::string label, bool verbose, int iterations, int p) {
	double timings[iterations];
	double label_count[iterations];
	double memory[iterations];
	std::fill_n(timings, iterations, 0);
	std::fill_n(label_count, iterations, 0);
	std::fill_n(memory, iterations, 0);

	for (int i = 0; i < iterations; ++i) {
		LabelSettingAlgorithm algo(graph, p);

		utility::tool::TimeOfDayTimer timer;
		timer.start();
		algo.run(start);
		timer.stop();
		timings[i] = timer.getTimeInSeconds();
		memory[i] = getCurrentMemorySize();

		label_count[i] = algo.size(end);
		if (verbose) {
			algo.printStatistics();
		}
	}
	std::cout << total_num << " " << label << num << " " << pruned_average(timings, iterations, 0) << " " 
		<< pruned_average(label_count, iterations, 0) <<  " " << pruned_average(memory, iterations, 0)/1024 << " " 
		<< getPeakMemorySize()/1024 << " " << p << "  # time in [s], target node label count, memory [mb], peak memory [mb], p" << std::endl;
}

static TempEdge::weight_type getWeightOf(TempGraph& graph, unsigned int start, unsigned int end) {
	FORALL_EDGES(graph, NodeID(start), eid) {
		const TempEdge& edge = graph.getEdge(eid);
		if (edge.target == NodeID(end)) {
			return edge.weight;
		}
	}
	std::cout << "Encountered unknown edge" << std::endl;
	return 0;
}

static void readGraphFromFile(Graph& graph, std::ifstream& timings, std::ifstream& economics) {
	std::string ignore;
	char c_line[256];

	// Read all timing instances into a temporary Graph.
	TempGraph temp_graph;
	std::vector< std::pair<NodeID, TempEdge > > temp_edges;
	while (timings.getline(c_line, 256)) {
		std::istringstream in_stream( c_line );
		switch (c_line[0]) {
		case 'a':
			unsigned int start, end, weight;
			in_stream >> ignore >> start >> end >> weight;
			temp_edges.push_back(std::make_pair(NodeID(start), TempEdge(NodeID(end), TempEdge::edge_data(weight))));
			break;
		case 'c':
			continue;
		case 'p':
			int node_count, edge_count;
			in_stream >> ignore >> ignore >> node_count >> edge_count;
			std::cout << "# Nodes " << node_count <<  " Edges " << edge_count << std::endl;
			break;
		}
	}
	GraphGenerator<TempGraph> temp_gen;
	temp_gen.buildGraphFromEdges(temp_graph, temp_edges);

	// Now read the economic cost data and merge with the timing values 
	std::vector< std::pair<NodeID, Edge > > edges;
	while (economics.getline(c_line, 256)) {
		std::istringstream in_stream( c_line );
		switch (c_line[0]) {
		case 'a':
			unsigned int start, end, weight;
			in_stream >> ignore >> start >> end >> weight;
			edges.push_back(std::make_pair(NodeID(start), Edge(NodeID(end), Label(getWeightOf(temp_graph, start, end), weight))));
			break;
		case 'c':
			continue;
		case 'p':
			int node_count, edge_count;
			in_stream >> ignore >> ignore >> node_count >> edge_count;
			std::cout << "# Nodes " << node_count <<  " Edges " << edge_count << std::endl;
			break;
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
	int total_instance = 1;
	int p = tbb::task_scheduler_init::default_num_threads();


	std::string graphname;
	std::string directory;
	std::ifstream timings_in;
	std::ifstream ecomonics_in;
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
	std::string tim(directory + "USA-road-t." + graphname + ".gr");
	std::string eco(directory + "USA-road-m." + graphname + ".gr");
	std::cout << "# Map: " << tim << " " << eco << std::endl;

	timings_in.open(tim.c_str());
	ecomonics_in.open(eco.c_str());

	problems_in.open((directory + graphname + "_ODpairs.txt").c_str());

	Graph graph;
	readGraphFromFile(graph, timings_in, ecomonics_in);
	timings_in.close();
	ecomonics_in.close();

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

