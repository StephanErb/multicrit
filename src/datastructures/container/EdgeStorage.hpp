/*
 * GraphStorage.hpp
 *
 *  Created on: 28.03.2012
 *      Author: kobitzsch
 *
 * Defines storages to all
 */

#ifndef EDGESTORAGE_HPP_
#define EDGESTORAGE_HPP_

#include "../../utility/exception.h"
#include "../../utility/TemplateTricks.hpp"
#include "../graph/GraphTypes.hpp"
#include "../graph/DummyOperators.hpp"

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

namespace utility{
	namespace datastructure{
		namespace container{
			namespace implementations{

				template< typename edge_slot >
				class StaticStorage{
					typedef utility::datastructure::NodeID NodeID;
					typedef utility::datastructure::EdgeID EdgeID;
					typedef edge_slot Edge;
				public:
					/*************************************************************
					 * Construction / Destruction
					 *************************************************************/
					StaticStorage( size_t largest_node_id, size_t number_of_edges, size_t reserve_size = 0 )
					{
						if( largest_node_id )
							first_edges.reserve( largest_node_id + 1);
						if( reserve_size )
							edges.reserve( std::max( number_of_edges, reserve_size ) );
					}

					void reallocate( size_t number_of_nodes, size_t number_of_edges ){
						if( number_of_nodes )
							first_edges.reserve( number_of_nodes + 1);
						if( number_of_edges )
							edges.reserve( number_of_edges );
					}

					/*************************************************************
					 * Property Queries
					 *************************************************************/
					size_t numberOfNodes() const { return first_edges.size() - 1; }
					size_t numberOfEdges() const { return edges.size(); }

					/*************************************************************
					 * Access to the stored data
					 *************************************************************/
					inline EdgeID edgeBegin( const NodeID & nid ) const {
						GUARANTEE( (size_t)nid+1 < first_edges.size(), std::runtime_error, "[error] Node ID out of bounds" )
						return (EdgeID) first_edges[nid];
					}

					inline EdgeID edgeEnd( const NodeID & nid ) const {
						GUARANTEE( (size_t)nid+1 < first_edges.size(), std::runtime_error, "[error] Node ID out of bounds" )
						return (EdgeID) first_edges[nid+1];
					}

					inline const Edge & getEdge( const EdgeID & edge_id ) const {
						GUARANTEE( (size_t)edge_id < edges.size(), std::runtime_error, "[error] Edge ID out of bounds" )
						return edges[edge_id];
					}

					inline Edge & getEdge( const EdgeID & edge_id ){
						GUARANTEE( (size_t)edge_id < edges.size(), std::runtime_error, "[error] Edge ID out of bounds" )
						return edges[edge_id];
					}

					/*************************************************************
					 * Insertion of new data / deletion of data
					 * Functions invalidate possible external ids
					 *************************************************************/
					void addNode( void ){
						first_edges.push_back( edges.size() );
					}

					void addEdge( const NodeID & nid, const Edge & edge ){
						GUARANTEE( nid < first_edges.size(), std::runtime_error, "[error] node id does not exist (yet)" )
						if( ((size_t)nid + 1) == first_edges.size() ){
							edges.push_back( edge );
						} else {
							//insert edge at appropriate position, increase adjacency array position for all nodes with id larger than nid
							edges.insert( edges.begin() + first_edges[(size_t) nid + 1] - ( first_edges[(size_t) nid] != first_edges[(size_t) nid + 1]), edge );
							for( size_t i = (size_t)nid + 1, end = first_edges.size(); i != end; ++i ) ++first_edges[i];
						}
					}

					//very expansive operation
					void removeEdge( const NodeID & nid, const EdgeID & edge_id ){
						GUARANTEE( (size_t) nid + 1 < first_edges.size(), std::runtime_error, "[error] removing edge from nonexisting node or during construction of last node (not supported)");
						GUARANTEE( edge_id >= first_edges[nid] && edge_id < first_edges[nid+1], std::runtime_error, "[error] edge_id does not belong to specified node during removeEdge" )
						edges.erase( edges.begin() + (size_t)edge_id );
						for( size_t i = (size_t)nid + 1, end = first_edges.size(); i != end; ++i ) --first_edges[i];
					}

					void finalize( void ){
						first_edges.push_back( edges.size() );
					}

					/*************************************************************
					 * Serialization
					 *************************************************************/
					template< typename comparator_slot >
					void sortEdges( comparator_slot & comparator ){
						for( size_t i = 0; i+1 < first_edges.size(); ++i ){
							std::sort( &edges[first_edges[i]], &edges[first_edges[i]+1], comparator );
						}
					}

					template< typename output_edge_slot >
					bool serialize( std::ostream & os ) const {
						size_t tmp = first_edges.size() - 1;	//the number of nodes
						os.write( reinterpret_cast<const char*>(&tmp), sizeof(tmp) );
						tmp = edges.size();
						os.write( reinterpret_cast<const char*>(&tmp), sizeof(tmp) );
						os.write( reinterpret_cast<const char*>(&first_edges[0]), (first_edges.size()-1) * sizeof( first_edges[0] ) );
						for( size_t i = 0, end = edges.size(); i != end; ++i ){
							output_edge_slot out_edge = edges[i];	//implicit conversion by constructor. sets default values or omits values not used
							out_edge.serialize( os );
						}
						return true;
					}

					template< typename input_edge_slot >
					bool deserialize( std::istream & is ){
						size_t num_nodes, num_edges;
						is.read( reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes) );
						is.read( reinterpret_cast<char*>(&num_edges), sizeof(num_edges) );

						first_edges.resize( num_nodes+1 );
						is.read( reinterpret_cast<char*>(&first_edges[0]), num_nodes * sizeof( first_edges[0] ) );
						first_edges[num_nodes] = num_edges;

						edges.resize( num_edges );
						for( size_t i = 0, end = edges.size(); i != end; ++i ){
							input_edge_slot in_edge;
							in_edge.deserialize( is );
							edges[i] = in_edge;	//implicit conversion by constructor. sets default values or omits values not used
						}
//						std::cout << "[info] loaded " << num_nodes << " nodes, " << num_edges << " edges." << std::endl;
						return true;
					}
				protected:
					std::vector<size_t> first_edges;
					std::vector<Edge> edges;
				};

				template< typename edge_slot, typename dummy_operator, bool check_after = true, bool check_before = true >
				class DynamicStorage{
					typedef utility::datastructure::NodeID NodeID;
					typedef utility::datastructure::EdgeID EdgeID;
					typedef edge_slot Edge;
				public:
					/*************************************************************
					 * Construction / Destruction
					 *************************************************************/
					DynamicStorage( size_t largest_node_id, size_t number_of_edges, size_t reserve_size = 0 ) :
						dummy_count( std::max( reserve_size, number_of_edges ) / (std::max(largest_node_id,(size_t)1)) )
					{
						nodes.reserve( largest_node_id );
						if( number_of_edges or reserve_size )
							edges.reserve( std::max( number_of_edges, reserve_size) );
					}

					void reallocate( size_t number_of_nodes, size_t number_of_edges ){
						if( number_of_nodes )
							nodes.reserve( number_of_nodes);
						if( number_of_edges )
							edges.reserve( number_of_edges );
					}

					/*************************************************************
					 * Property Queries
					 *************************************************************/
					size_t numberOfNodes() const { return nodes.size(); }
					size_t numberOfEdges() const { return edges.size(); }

					/*************************************************************
					 * Access to the stored data
					 *************************************************************/
					inline EdgeID edgeBegin( const NodeID & nid ) const {
						GUARANTEE( (size_t)nid < nodes.size(), std::runtime_error, "[error] Node ID out of bounds" )
						return (EdgeID) nodes[nid].first;
					}

					inline EdgeID edgeEnd( const NodeID & nid ) const {
						GUARANTEE( (size_t)nid < nodes.size(), std::runtime_error, "[error] Node ID out of bounds" )
						return (EdgeID) nodes[nid].second;
					}

					inline const Edge & getEdge( const EdgeID & edge_id ) const {
						GUARANTEE( (size_t)edge_id < edges.size(), std::runtime_error, "[error] Edge ID out of bounds" )
						return edges[edge_id];
					}

					inline Edge & getEdge( const EdgeID & edge_id ){
						GUARANTEE( (size_t)edge_id < edges.size(), std::runtime_error, "[error] Edge ID out of bounds" )
						return edges[edge_id];
					}

					/*************************************************************
					 * Insertion of new data / Deletion of data
					 * Functions invalidate possible external ids
					 *************************************************************/
					void addNode(){
						nodes.push_back( std::make_pair(edges.size(), edges.size() ) );
					}

					void addEdge( const NodeID & nid, const Edge & edge ){
						if( check_after and (nodes[nid].second < edges.size()) and dummy_operator::isDummy( edges[nodes[nid].second] ) ){
							edges[nodes[nid].second++] = edge;
							//dummy_operator::unsetDummy( (EdgeID) nodes[nid].second-1, edges[nodes[nid].second-1] );
						} else if ( check_before and (nodes[nid].first -1 < edges.size()) and  dummy_operator::isDummy( edges[nodes[nid].first -1] )) {
							edges[--nodes[nid].first] = edge;
							//dummy_operator::unsetDummy( (EdgeID) nodes[nid].first, edges[nodes[nid].first] );
						} else {
							reallocate( nid, edge );
						}
					}

					void removeEdge( const NodeID & nid, const EdgeID & eid ){
						if( (size_t) eid != (--nodes[nid].second) ){
							std::swap( edges[nodes[nid].second], edges[eid] );
						}
						dummy_operator::setDummy( edges[nodes[nid].second] );
					}

					void finalize(){
						/* do nothing */
					}

					/*************************************************************
					 * Serialization
					 * Reads from / Stores into a static version of the graph
					 * The temporarily used dummy edges are not written to disk
					 *************************************************************/
					template< typename comparator_slot >
					void sortEdges( comparator_slot & comparator ){
						for( size_t i = 0; i < nodes.size(); ++i ){
							std::sort( &edges[nodes[i].first], &edges[nodes[i].second], comparator );
						}
					}

					template< typename output_edge_slot >
					bool serialize( std::ostream & os ) const {
						size_t num_nodes = nodes.size();
						size_t num_edges = 0;
						for( size_t i = 0; i < nodes.size(); ++i ){
							num_edges += (nodes[i].second - nodes[i].first);
						}

						os.write( reinterpret_cast<const char*>(&num_nodes), sizeof(num_nodes) );
						os.write( reinterpret_cast<const char*>(&num_edges), sizeof(num_edges) );

						//Write the adjacency array information
						num_edges = 0;
						for( size_t i = 0; i < nodes.size(); ++i ){
							os.write( reinterpret_cast<const char*>(&num_edges), sizeof(num_edges) );
							num_edges += (nodes[i].second - nodes[i].first);
						}

						for( size_t i = 0; i < nodes.size(); ++i ){
							for( size_t eid = nodes[i].first; eid != nodes[i].second; ++eid ){
								output_edge_slot out_edge = edges[eid];
								out_edge.serialize( os );
							}
						}
						return true;
					}

					template< typename input_edge_slot >
					bool deserialize( std::istream & is ){
						size_t num_nodes, num_edges;
						is.read( reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes) );
						is.read( reinterpret_cast<char*>(&num_edges), sizeof(num_edges) );

						std::vector<size_t> adjacency_array( num_nodes + 1 );
						is.read( reinterpret_cast<char*>(&adjacency_array[0]), num_nodes * sizeof( adjacency_array[0] ) );
						adjacency_array[num_nodes] = num_edges;

						nodes.clear();
						edges.clear();

						for( size_t i = 0; i < num_nodes; ++i ){
							addNode();
							for( size_t eid = adjacency_array[i]; eid < adjacency_array[i+1]; ++eid ){
								input_edge_slot in_edge;
								in_edge.deserialize( is );
								addEdge( (NodeID) i, (Edge) in_edge );
							}
							Edge dummy_edge;
							dummy_operator::setDummy( dummy_edge );
							for( size_t i = 0; i < dummy_count; ++i ){
								edges.push_back( dummy_edge );
							}
						}
						return true;
					}
				protected:
					//begin and end indices for the edge subsequence of a given node
					size_t dummy_count;
					std::vector< std::pair<size_t,size_t> > nodes;
					std::vector<Edge> edges;

					void reallocate( const NodeID & nid, const Edge & edge ){
						size_t new_node_begin = edges.size();
						for( size_t eid = nodes[nid].first; eid != nodes[nid].second; ++eid ){
							edges.push_back( edges[eid] );
							dummy_operator::setDummy( edges[eid] );
						}
						edges.push_back( edge );
						nodes[nid].first = new_node_begin;
						nodes[nid].second = edges.size();

						Edge dummy_edge;
						dummy_operator::setDummy( dummy_edge );
						for( size_t i = 0; i < dummy_count; ++i ){
							edges.push_back( dummy_edge );
						}
					}
				};

			}

			template< bool dynamic, typename edge_slot, typename dummy_operator_slot = utility::datastructure::EdgeFlagsDummyOperator<edge_slot> >
			class EdgeStorage : public tool::TypeSwitch<dynamic,implementations::DynamicStorage<edge_slot,dummy_operator_slot>,implementations::StaticStorage<edge_slot> >::type {
				typedef utility::datastructure::NodeID NodeID;
				typedef utility::datastructure::EdgeID EdgeID;
				typedef typename tool::TypeSwitch<dynamic,implementations::DynamicStorage<edge_slot,dummy_operator_slot>,implementations::StaticStorage<edge_slot> >::type BaseType;
				typedef edge_slot Edge;
			public:
				/*************************************************************
				 * Construction / Destruction
				 *************************************************************/
				EdgeStorage( size_t largest_node_id, size_t initial_size, size_t reserve_size = 0 )
					: BaseType( largest_node_id, initial_size, reserve_size )
				{}

				/*************************************************************
				 * Property Queries
				 *************************************************************/
				size_t numberOfNodes() const { return BaseType::numberOfNodes(); }
				size_t numberOfEdges() const { return BaseType::numberOfEdges(); }

				/*************************************************************
				 * Access to the stored data
				 *************************************************************/
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

				/*************************************************************
				 * Insertion/deletion of new data
				 * Functions invalidate possible external ids
				 *************************************************************/
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

				/*************************************************************
				 * Serialization
				 *************************************************************/
				template< typename comparator_slot >
				void sortEdges( comparator_slot & comparator ){
					BaseType::template sortEdges<comparator_slot>( comparator );
				}

				bool serialize( std::ostream & os ) const {
					return BaseType::template serialize<Edge>( os );
				}
				bool deserialize( std::istream & is ){
					return BaseType::template deserialize<Edge>( is );
				}

				template< typename output_edge_slot >
				bool serializeTo( std::ostream & os ) const {
					return BaseType::template serialize<output_edge_slot>( os );
				}

				template< typename input_edge_slot >
				bool deserializeFrom( std::istream & is ){
					return BaseType::template deserialize<input_edge_slot>( is );
				}

			};
		}
	}
}


#endif /* EDGESTORAGE_HPP_ */
