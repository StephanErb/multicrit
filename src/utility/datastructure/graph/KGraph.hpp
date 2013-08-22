/*
 * KGraph.hpp
 *
 *  Created on: 28.03.2012
 *      Author: kobitzsch
 */

#ifndef KGRAPH_HPP_
#define KGRAPH_HPP_

#include "GraphTypes.hpp"
#include "../NullData.hpp"
#include "../../tool/TemplateTricks.hpp"

#include "../container/EdgeStorage.hpp"

namespace utility{
	namespace datastructure{
		template< typename edge_slot, bool dynamic = false, typename extension_slot = NullData >
		class KGraph : public utility::datastructure::container::EdgeStorage<dynamic,edge_slot>, public extension_slot {
		public:
			//extractable typedefs
			typedef edge_slot Edge;
			typedef utility::datastructure::NodeID NodeID;
			typedef utility::datastructure::EdgeID EdgeID;
			typedef utility::datastructure::container::EdgeStorage<dynamic,edge_slot> Storage;

			/**************************************
			 * Construction
			 **************************************/
			KGraph( size_t number_of_nodes = 0, size_t number_of_edges = 0, size_t reserve_size = 0 )
			: Storage( number_of_nodes, number_of_edges, reserve_size )
			{}


			/**************************************
			 * DERIVED Functionality
			 * ************************************/

			/***************************************
			 * Property Queries
			 ***************************************
				size_t numberOfNodes() const { return BaseType::numberOfNodes(); }
				size_t numberOfEdges() const { return BaseType::numberOfEdges(); }

			 ***************************************
			 * Access to the stored data
			 ***************************************
				inline EdgeID edgeBegin( const NodeID & nid ) const {
					return BaseType::edgeBegin( nid );
				}

				inline EdgeID edgeEnd( const NodeID & nid ) const {
					return BaseType::edgeEnd( nid );
				}

				inline const Edge & getEdge( const EdgeID & edge_id ) const {
					return BaseType::getEdge( edge_id );
				}

				inline Edge & getEdge( const EdgeID & edge_id ){
					return BaseType::getEdge( edge_id );
				}

			***************************************
			* Insertion/deletion of new data
			* Functions invalidate possible
			* external ids
			***************************************
				void addNode(){
					BaseType::addNode();
				}

				void addEdge( const NodeID & nid, const Edge & edge ){
					BaseType::addEdge( nid, edge );
				}

				void removeEdge( const NodeID & nid, const EdgeID & eid ){
					BaseType::removeEdge( nid, eid );
				}

				void finalize(){
					BaseType::finalize();
				}

			 ***************************************
			 * Serialization
			 ***************************************
			 	template< typename comparator_slot >
				void sortEdges( const comparator_slot & comparator ){
					BaseType::template sortEdges<comparator_slot>( comparator );
				}
				bool serialize( std::ostream & os ) const {
					return BaseType::serialize<Edge>( os );
				}
				bool deserialize( std::istream & is ){
					return BaseType::deserialize<Edge>( is );
				}

				template< typename output_edge_slot >
				bool serializeTo( std::ostream & os ) const {
					return BaseType::serialize<output_edge_slot>( os );
				}

				template< typename input_edge_slot >
				bool deserializeFrom( std::istream & is ) const {
					return BaseType::deserialize<input_edge_slot>( is );
				}
			***************************************
			* End of the derived functionality of
			* EdgeStorage
			**************************************/

			size_t numberOfEdges() const { return Storage::numberOfEdges(); }

			size_t numberOfEdges( const NodeID & nid ) const {
				return (size_t)Storage::edgeEnd( nid ) - (size_t)Storage::edgeBegin( nid );
			}

			EdgeID locate( const NodeID & nid, const Edge & edge ) const {
				for( EdgeID eid = Storage::edgeBegin( nid ), end = Storage::edgeEnd( nid ); eid != end; ++eid ){
					if( edge == Storage::getEdge( eid ) )
						return eid;
				}
				return (EdgeID) - 1;
			}

			EdgeID locateReverseEdge( NodeID nid, Edge edge ) const {
				std::swap( nid, edge.target );
				edge.reverse();
				return locate( nid, edge );
			}


			bool checkIntegrity( NodeID src, bool check_reverse_edges = true ){
				bool result = true;
				if( numberOfEdges( (NodeID) src ) == 0 ){
					result = false;
					std::cout << "Graph has isolated node: " << src << "\n";
				}
				if( check_reverse_edges ){
					for( EdgeID eid = Storage::edgeBegin( src ), end_id = Storage::edgeEnd( src ); eid != end_id; ++eid ){
						if( locateReverseEdge( src, Storage::getEdge(eid) ) == (EdgeID) - 1){
							std::cout << "Did not find reverse edge for " << src << " :: " << Storage::getEdge(eid).toString() << "\n";
							result = false;
						}
					}
				}
				if( !result )
					std::cout << std::flush;
				return result;
			}

			bool checkIntegrity( bool check_reverse_edges = true ){
				bool result = true;
				for( size_t i = 0; i < Storage::numberOfNodes(); ++i ){
					if( numberOfEdges( (NodeID) i ) == 0 ){
						result = false;
						std::cout << "Graph has isolated node: " << i << "\n";
					}
					if( check_reverse_edges ){
						for( EdgeID eid = Storage::edgeBegin( (NodeID) i ), end_id = Storage::edgeEnd( (NodeID) i ); eid != end_id; ++eid ){
							if( locateReverseEdge( (NodeID) i, Storage::getEdge(eid) ) == (EdgeID) - 1){
								std::cout << "Did not find reverse edge for " << i << " :: " << Storage::getEdge(eid).toString() << "\n";
								result = false;
							}
							const Edge & edge = Storage::getEdge( eid );
							if( edge.target >= Storage::numberOfNodes() ){
								std::cout << "Node target out of bounds (" << edge.target << " of " << Storage::numberOfNodes() << ")" << std::endl;
								result = false;
							}
						}
					}
				}
				if( !result )
					std::cout << std::flush;
				return result;
			}

		};
	}
}

#endif /* KGRAPH_HPP_ */
