/*
 * KSGPrinter.cpp
 *
 *  Created on: 09.09.2010
 *      Author: kobitzsch
 */

#include "../datastructure/graph/KGraph.hpp"
#include "../datastructure/graph/GraphTypes.hpp"
#include "../datastructure/graph/GraphMacros.h"
#include "../datastructure/graph/SeparatedGraphSet.hpp"
#include "../datastructure/graph/Edge.hpp"


typedef utility::datastructure::DirectedIntegerWeightedEdge Edge;
typedef utility::datastructure::DirectedIntegerWeightedEdgeWithMiddleNode EdgeWithNode;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::KGraph<EdgeWithNode> CHGraph;

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>

int main( int argc, char ** args){
	if( argc == 1 ){
		std::cout << "Usage: " << args[0] << std::endl
				  << "\t-s static weight<int>, BasicNode Graph [.ksg]" << std::endl
				  << "\t-c static weight<int>, BasicNode Graph [.ch.ksg]" << std::endl
				  << "\t-b enable forward/backward splitting" << std::endl;
		return 0;
	}

	int c;
	std::ifstream input_graph_file;
	bool splitted = false;
	bool middle_node = false;
	std::string graph_filename;
	while( ( c = getopt( argc, args, "s:c:b" ) ) != -1 ){
		switch( c ){
		case 'b': splitted = true; break;
		case 's': graph_filename = optarg; break;
		case 'c': graph_filename = optarg; middle_node = true; break;
		}
	}

	input_graph_file.open( graph_filename.c_str(), std::ios::in | std::ios::binary );

	if( !input_graph_file.is_open() )
		std::cout << "Could not open " << graph_filename << std::endl;

	if( splitted ){
		if( middle_node ){
			typedef utility::datastructure::SeparatedGraphSet<CHGraph> graph_set_t;
			graph_set_t graph_set;
			graph_set.deserialize<EdgeWithNode>( input_graph_file );

			std::ofstream ofsfw( "graph.fw.out", std::ios::out | std::ios::binary );
			FORALL_NODES( graph_set.forwardGraph(), node ){
				ofsfw << "Edges of node " << node << "\n";
				FORALL_EDGES( graph_set.forwardGraph(), node, eid ){
					ofsfw << eid << " :: " <<  graph_set.forwardGraph().getEdge( eid ).toString() << "\n";
				}
			}
			ofsfw.close();
			std::ofstream ofsbw( "graph.bw.out", std::ios::out | std::ios::binary );
			FORALL_NODES( graph_set.backwardGraph(), node ){
				ofsbw << "Edges of node " << node << "\n";
				FORALL_EDGES( graph_set.backwardGraph(), node, eid ){
					ofsbw << eid << " :: " <<  graph_set.backwardGraph().getEdge( eid ).toString() << "\n";
				}
			}
			ofsbw.close();
		} else {
			typedef utility::datastructure::SeparatedGraphSet<Graph> graph_set_t;
			graph_set_t graph_set;
			graph_set.deserialize<Edge>( input_graph_file );

			std::ofstream ofsfw( "graph.fw.out", std::ios::out | std::ios::binary );
			FORALL_NODES( graph_set.forwardGraph(), node ){
				ofsfw << "Edges of node " << node << "\n";
				FORALL_EDGES( graph_set.forwardGraph(), node, eid ){
					ofsfw << eid << " :: " <<  graph_set.forwardGraph().getEdge( eid ).toString() << "\n";
				}
			}
			ofsfw.close();
			std::ofstream ofsbw( "graph.bw.out", std::ios::out | std::ios::binary );
			FORALL_NODES( graph_set.backwardGraph(), node ){
				ofsbw << "Edges of node " << node << "\n";
				FORALL_EDGES( graph_set.backwardGraph(), node, eid ){
					ofsbw << eid << " :: " <<  graph_set.backwardGraph().getEdge( eid ).toString() << "\n";
				}
			}
			ofsbw.close();
		}

	} else {
		if( middle_node ){
			CHGraph graph;
			graph.deserialize( input_graph_file );
			input_graph_file.close();

			FORALL_NODES( graph, node ){
				std::cout << "Edges of node " << node << "\n";
				FORALL_EDGES( graph, node, eid ){
					std::cout << eid << " :: " <<  graph.getEdge( eid ).toString() << "\n";
				}
			}
		} else {
			Graph graph;
			graph.deserialize( input_graph_file );
			input_graph_file.close();

			FORALL_NODES( graph, node ){
				std::cout << "Edges of node " << node << "\n";
				FORALL_EDGES( graph, node, eid ){
					std::cout << eid << "::" << graph.getEdge( eid ).toString() << "\n";
				}
			}
		}
	}

	input_graph_file.close();



	return 0;
}
