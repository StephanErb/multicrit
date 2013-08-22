/*
 * GraphMacros.h
 *
 *  Created on: Feb 21, 2011
 *      Author: kobitzsch
 */

#ifndef GRAPHMACROS_H_
#define GRAPHMACROS_H_

#include "GraphTypes.hpp"

using utility::datastructure::NodeID;
using utility::datastructure::EdgeID;

#ifndef FORALL_NODES
#define FORALL_NODES(graph,node) for( NodeID node = NodeID(0), number_of_nodes_for_loop_macro = NodeID(graph.numberOfNodes()); node < number_of_nodes_for_loop_macro; ++node )

#define FORALL_NODES_STARTING_AT(graph,node,start) for( NodeID node = start, number_of_nodes_for_loop_macro = NodeID(graph.numberOfNodes()); node < number_of_nodes_for_loop_macro; ++node )
#endif

#ifndef FORALL_EDGES

#define FORALL_EDGES(graph,node,edge) for( EdgeID edge = graph.edgeBegin(node), end_id_edge_for_macro = graph.edgeEnd( node ); edge != end_id_edge_for_macro; ++edge )

#define FORALL_EDGES_STARTING_AT(graph,node,edge,start) for( EdgeID edge = start, end_id_edge_for_macro = graph.edgeEnd( node ); edge != end_id_edge_for_macro; ++edge )

#endif

#endif /* GRAPHMACROS_H_ */
