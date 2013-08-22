/*
 * SeparatedGraphSet.hpp
 *
 *  Created on: 02.04.2012
 *      Author: kobitzsch
 *
 *  Set of two graphs, forward and backward.
 *  Can only be read from disk, not written.
 */

#ifndef SEPARATEDGRAPHSET_HPP_
#define SEPARATEDGRAPHSET_HPP_

#include <iostream>
#include <vector>

namespace utility{
	namespace datastructure{
		template< typename graph_slot >
		class SeparatedGraphSet{
		public:
			typedef graph_slot Graph;
			typedef typename Graph::NodeID NodeID;

			const Graph & forwardGraph() const { return forward_graph; }
			const Graph & backwardGraph() const { return backward_graph; }

			Graph & forwardGraph() { return forward_graph; }
			Graph & backwardGraph() { return backward_graph; }

			template <typename input_edge_slot>
			bool deserialize( std::istream & is ){
				size_t num_nodes, num_edges;
				is.read( reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes) );
				is.read( reinterpret_cast<char*>(&num_edges), sizeof(num_edges) );

				std::vector<size_t>first_edges( num_nodes+1 );
				is.read( reinterpret_cast<char*>(&first_edges[0]), num_nodes * sizeof( first_edges[0] ) );
				first_edges[num_nodes] = num_edges;

				for( size_t i = 0; i < num_nodes; ++i ){
					forward_graph.addNode();
					backward_graph.addNode();
					for( size_t j = first_edges[i], end = first_edges[i+1]; j != end; ++j ){
						input_edge_slot in_edge;
						in_edge.deserialize( is );
						if( in_edge.forward ){
							forward_graph.addEdge( (NodeID) i, in_edge );
						}
						if( in_edge.backward ){
							backward_graph.addEdge( (NodeID) i, in_edge );
						}
					}
				}
				forward_graph.finalize();
				backward_graph.finalize();
				return true;
			}

			template <typename input_edge_slot, typename unpacker_slot >
			bool deserialize( std::istream & is, unpacker_slot & unpacker ){
				size_t num_nodes, num_edges;
				is.read( reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes) );
				is.read( reinterpret_cast<char*>(&num_edges), sizeof(num_edges) );

				unpacker.reserve( num_nodes, num_edges );

				std::vector<size_t>first_edges( num_nodes+1 );
				is.read( reinterpret_cast<char*>(&first_edges[0]), num_nodes * sizeof( first_edges[0] ) );
				first_edges[num_nodes] = num_edges;

				for( size_t i = 0; i < num_nodes; ++i ){
					forward_graph.addNode();
					backward_graph.addNode();
					for( size_t j = first_edges[i], end = first_edges[i+1]; j != end; ++j ){
						input_edge_slot in_edge;
						in_edge.deserialize( is );
						if( in_edge.forward ){
							forward_graph.addEdge( (NodeID) i, in_edge );
							unpacker.template pushEdge<true>( (NodeID) i, in_edge );
						}
						if( in_edge.backward ){
							backward_graph.addEdge( (NodeID) i, in_edge );
							unpacker.template pushEdge<false>( (NodeID) i, in_edge );
						}
					}
				}
				forward_graph.finalize();
				backward_graph.finalize();

				unpacker.finalize();

				return true;
			}

		protected:
			Graph forward_graph;
			Graph backward_graph;
		};
	}
}


#endif /* SEPARATEDGRAPHSET_HPP_ */
