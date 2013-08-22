/*
 * KSGPrinter.cpp
 *
 *  Created on: 09.09.2010
 *      Author: kobitzsch
 */

#include "../datastructure/graph/KGraph.hpp"
#include "../datastructure/graph/GraphTypes.hpp"
#include "../datastructure/graph/Edge.hpp"
//#include "../datastructure/graph/ParameterizedEdge.hpp"

using utility::datastructure::NodeID;
using utility::datastructure::EdgeID;

typedef utility::datastructure::DirectedIntegerWeightedEdge Edge;
//typedef utility::datastructure::defaultParameterizedEdge parameterizedEdge;
typedef utility::datastructure::KGraph<Edge> Graph;
//pedef utility::datastructure::KStaticGraph<Node,parameterizedEdge> dynamicGraph;

#include <iostream>
#include <cstdlib>
#include <fstream>

bool findParallel( const Graph & graph, const NodeID & nid ){
	bool sane = true;
	for( EdgeID eid = graph.edgeBegin( nid ), end_id = graph.edgeEnd( nid ); eid != end_id; ++eid ){
		const Edge & edge = graph.getEdge( eid );
		for( EdgeID seid = eid+1; seid != end_id; ++seid ){
			const Edge & sedge = graph.getEdge( seid );
			if( edge.target == sedge.target ){
				if( edge.forward && sedge.forward ){
					std::cout << "[info] Graph has a parallel edge at node " << nid << " in forward direction to target: " << edge.target << " for weights " << edge.weight << " and " << sedge.weight << std::endl;
					std::cout << "[info] Edge: " << edge.toString() << " and " << sedge.toString() << std::endl;
					sane = false;
				}
				if( edge.backward && sedge.backward ){
					std::cout << "[info] Graph has a parallel edge at node " << nid << " in backward direction to target: " << edge.target << " for weights " << edge.weight << " and " << sedge.weight << std::endl;
					std::cout << "[info] Edge: " << edge.toString() << " and " << sedge.toString() << std::endl;
					sane = false;
				}
			}
		}
	}
	return sane;
}

void checkSanity( const Graph & graph ){
	std::cout << "Checking" << std::flush;
	for( unsigned int n = 0; n < graph.numberOfNodes(); ++n ){
		if( n % 9999 == 0 )
			std::cout << "." << std::flush;
		findParallel( graph, (NodeID) n );
		for( EdgeID eid = graph.edgeBegin( NodeID(n) ); eid != graph.edgeEnd(NodeID(n)); ++eid ){
			if( graph.getEdge( eid ).getTarget() == NodeID(n) )
				std::cout << "Self loop at: " << NodeID(n) << " :: " << graph.getEdge( eid ).toString() << std::endl;
			EdgeID rev_eid = graph.locateReverseEdge( NodeID(n), graph.getEdge( eid ) );
			if( rev_eid == (EdgeID)-1 ){ //graph.edgeEnd( graph.getEdge(eid).getTarget() ) ){
				std::cout << "\nGraph not sane, reverse edge of edge #" << eid << " is missing." << std::endl;
			}
		}
	}
	std::cout << "done." << std::endl;
}

void printStats( const Graph & graph ){
	unsigned int max_degree = 0;
	for( unsigned int n = 0; n < graph.numberOfNodes(); ++n ){
		max_degree = std::max( max_degree, (unsigned int)graph.edgeEnd(NodeID(n)) - graph.edgeBegin( NodeID(n) ) );
	}
	std::cout << "Maximal degree of a node is: " << max_degree << std::endl;
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
					printStats( graph );
				}
				break;
			}
		}
	}



	return 0;
}
