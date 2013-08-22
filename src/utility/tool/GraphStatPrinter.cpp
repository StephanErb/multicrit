/*
 * GraphStatPrinter.cpp
 *
 *  Created on: 12.04.2012
 *      Author: kobitzsch
 */




#include "../datastructure/graph/KGraph.hpp"
#include "../datastructure/graph/GraphTypes.hpp"
#include "../datastructure/graph/GraphMacros.h"
#include "../datastructure/graph/Edge.hpp"


typedef utility::datastructure::DirectedIntegerWeightedEdge Edge;
typedef utility::datastructure::DirectedIntegerWeightedEdgeWithMiddleNode EdgeWithNode;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::KGraph<EdgeWithNode> CHGraph;

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <fstream>

template< typename graph_slot >
void printStats( const graph_slot & graph ){
	size_t max_degree = 0;
	size_t max_weight = 0;
	FORALL_NODES( graph, node ){
		max_degree = std::max( max_degree, (size_t) graph.numberOfEdges( node ) );
		FORALL_EDGES( graph, node, eid ){
			max_weight = std::max( max_weight, (size_t) graph.getEdge( eid ).weight );
		}
	}
	std::vector<size_t> degree_count( max_degree + 1 );
	std::vector<size_t> weight_count( max_weight + 1 );
	FORALL_NODES( graph, node ){
		degree_count[ graph.numberOfEdges( node ) ]++;
		FORALL_EDGES( graph, node, eid ){
			weight_count[(size_t) graph.getEdge( eid ).weight]++;
		}
	}
	std::cout << "Degree Statistics: " << std::endl;
	for( size_t i = 0; i < degree_count.size(); ++i ){
		if( degree_count[i] ){
			std::cout << i << ": " << (double) degree_count[i] / (double) graph.numberOfNodes() * 100.0 << "%" << std::endl;
		}
	}
	std::cout << "Weights: " << std::endl;
	for( size_t i = 0; i < weight_count.size(); ++i ){
		if( weight_count[i] ){
			std::cout << i << ": " << (double) weight_count[i] / (double) graph.numberOfEdges() * 100.0 << "%" << std::endl;
		}
	}
}

int main( int argc, char ** args){
	if( argc == 1 ){
		std::cout << "Usage: " << args[0] << std::endl
				  << "\t-s static weight<int>, BasicNode Graph [.ksg]" << std::endl
				  << "\t-c static weight<int>, BasicNode Graph [.ch.ksg]" << std::endl;
		return 0;
	}

	int c;
	std::ifstream input_graph_file;
	while( ( c = getopt( argc, args, "s:c:" ) ) != -1 ){
		switch( c ){
		case 's':{
				input_graph_file.open( optarg, std::ios::in | std::ios::binary );
				if( !input_graph_file.is_open() )
					std::cout << "Could not open " << optarg << std::endl;
				else {
					Graph graph;
					graph.deserialize( input_graph_file );
					input_graph_file.close();

					printStats( graph );
				}
				break;
			}
		case 'c':{
				input_graph_file.open( optarg, std::ios::in | std::ios::binary );
				if( !input_graph_file.is_open() )
					std::cout << "Could not open " << optarg << std::endl;
				else {
					CHGraph graph;
					graph.deserialize( input_graph_file );
					input_graph_file.close();

					printStats( graph );
				}
				break;
			}
		}
	}



	return 0;
}
