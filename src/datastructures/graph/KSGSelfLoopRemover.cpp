/*
 * KSGPrinter.cpp
 *
 *  Created on: 09.09.2010
 *      Author: kobitzsch
 */

#include "KGraph.hpp"
#include "GraphTypes.hpp"
#include "Edge.hpp"
//#include "../datastructure/graph/ParameterizedEdge.hpp"

using utility::datastructure::NodeID;
using utility::datastructure::EdgeID;

typedef utility::datastructure::DirectedIntegerWeightedEdge Edge;
//typedef utility::datastructure::defaultParameterizedEdge parameterizedEdge;
typedef utility::datastructure::KGraph<Edge> Graph;

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>

void checkSanity( Graph & graph ){
	std::cout << "Checking" << std::flush;
	for( unsigned int n = 0; n < graph.numberOfNodes(); ++n ){
		if( n % 9999 == 0 )
			std::cout << "." << std::flush;
		for( EdgeID eid = graph.edgeBegin( NodeID(n) ); eid != graph.edgeEnd(NodeID(n)); ++eid ){
			if( graph.getEdge( eid ).getTarget() == NodeID(n) ){
				graph.removeEdge( NodeID(n), eid );
				--eid;
				std::cout << "Removed self loop at: " << NodeID(n) << " :: " << graph.getEdge( eid ) << std::endl;
				continue;
			}
		}
	}
	std::cout << "done." << std::endl;
}

int main( int argc, char ** args){
	if( argc == 1 ){
		std::cout << "Usage: " << args[0] << std::endl
				  << "\t-s static weight<int>, BasicNode Graph" << std::endl;
		return 0;
	}

	int c;
	std::ifstream input_graph_file;
	while( ( c = getopt( argc, args, "s:" ) ) != -1 ){
		switch( c ){
		case 's':{
				input_graph_file.open( optarg, std::ios::in | std::ios::binary );
				if( !input_graph_file.is_open() )
					std::cout << "Could not open " << optarg << std::endl;
				else {
					Graph graph;
					std::cout << "Reading graph..." << std::flush;
					graph.deserialize( input_graph_file );
					input_graph_file.close();
					std::cout << " done." << std::endl;

					checkSanity( graph );

					std::ostringstream oss;
					oss << optarg << ".slc";
					std::ofstream ofs( oss.str().c_str(), std::ios::out | std::ios::binary );
					graph.serialize( ofs );
					ofs.close();
				}
				break;
			}
		}
	}



	return 0;
}
