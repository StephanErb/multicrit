/*
 * GraphBuilder.hpp
 *
 *  Created on: 30.03.2012
 *      Author: kobitzsch
 */

#ifndef GRAPHBUILDER_HPP_
#define GRAPHBUILDER_HPP_

#include <algorithm>
#include <utility>
#include <vector>

namespace utility{
	namespace datastructure{
		class GraphBuilder{
		public:
			template< typename graph_type, typename id_type, typename edge_type >
			static void buildGraph( graph_type & graph, std::vector< std::pair<id_type,edge_type> > & edges ){
				std::sort( edges.begin(), edges.end() );
				id_type max = (id_type) 0;
				for( size_t i = 0; i < edges.size(); ++i ){
					max = std::max( edges[i].first, max );
					max = std::max( edges[i].second.target, max );
				}
				unsigned int node_id = 0;
				graph.addNode();
				for( size_t i = 0; i < edges.size(); ++i ){
					while( edges[i].first > (id_type) node_id ){
						++node_id;
						graph.addNode();
					}
					graph.addEdge( (id_type) edges[i].first, edges[i].second );
				}
				while( max > node_id )
					graph.addNode();
				graph.finalize();
			}
		};
	}
}


#endif /* GRAPHBUILDER_HPP_ */
