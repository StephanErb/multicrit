/*
 * Edge.hpp
 *
 * Definition of an arbitrary Edge
 *  Created on: Jul 5, 2011
 *      Author: kobitzsch
 */

#ifndef EDGE_HPP_
#define EDGE_HPP_

#include "../../exception.h"
#include "../NullData.hpp"

#include "GraphTypes.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <limits>

namespace utility{
namespace datastructure{

struct DirectionData{
	/*******************************************/
	DirectionData( bool forward_ = false, bool backward_ = false )
	: forward( forward_ ),
	  backward( backward_ )
	{}

	template< typename edge_with_additional_data_slot >
	DirectionData( const edge_with_additional_data_slot & edge )
	: forward( edge.forward ),
	  backward( edge.backward )
	{}

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		os.write( reinterpret_cast<const char*> (& forward ), sizeof( forward ));
		os.write( reinterpret_cast<const char*> (& backward ), sizeof( backward ));
	}

	inline void deserialize( std::istream & is ) {
		is.read( reinterpret_cast<char*> (& forward ), sizeof( forward ));
		is.read( reinterpret_cast<char*> (& backward ), sizeof( backward ));
	}

	bool operator==( const DirectionData & other ) const {
		return other.forward == forward && other.backward == backward;
	}

	bool operator!=( const DirectionData & other ) const {
		return other.forward != forward || other.backward != backward;
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << (forward ? "<" : "-" ) << ":" << (backward ? ">" : "-" );
		return oss.str();
	}

	/*******************************************/
	bool isForward() const { return forward; }
	bool isBackward() const { return backward; }
	void setForward( bool forward_ ){ forward = forward_; }
	void setBackward( bool backward_ ){ forward = backward_; }

	bool isClosed() const { return !(forward && backward); }
	/*******************************************/
	bool forward;
	bool backward;
};

//Generates a Static Weighted edge
template< typename weight_type_slot, typename meta_weight_type_slot = std::numeric_limits<weight_type_slot> >
struct UndirectedWeightData{
	/*******************************************/
	typedef weight_type_slot weight_type;
	typedef meta_weight_type_slot meta_weight;

	/*******************************************/
	UndirectedWeightData( const weight_type & weight_ = 0 )
	: weight( weight_ )
	{}

	template< typename edge_with_additional_data_slot >
	UndirectedWeightData( const edge_with_additional_data_slot & edge )
	: weight( edge.weight )
	{}

	//Cast the element to ints weight type
	inline operator const weight_type_slot &() const { return weight; }

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		os.write( reinterpret_cast<const char*> (& weight ), sizeof( weight ));
	}

	inline void deserialize( std::istream & is ) {
		is.read( reinterpret_cast<char*> (& weight ), sizeof( weight ));
	}

	bool operator==( const UndirectedWeightData<weight_type_slot,meta_weight_type_slot> & other ) const {
		return other.weight == weight;
	}

	bool operator!=( const UndirectedWeightData<weight_type_slot,meta_weight_type_slot> & other ) const {
		return other.weight != weight;
	}

	bool covers( const UndirectedWeightData<weight_type_slot,meta_weight_type_slot> & other ) const {
		return weight <= other.weight;
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << weight;
		return oss.str();
	}

	bool isClosed() const { return false; }

	/*******************************************/
	const weight_type & getWeight() const { return weight; }
	weight_type & getWeight(){ return weight; }
	void setWeight( const weight_type & weight_ ){ weight = weight_; }

	/*******************************************/
	weight_type weight;
};

//Generates a Static Weighted edge
template< typename weight_type_slot, typename meta_weight_type_slot = std::numeric_limits<weight_type_slot> >
struct DirectedWeightData : public UndirectedWeightData< weight_type_slot, meta_weight_type_slot >{

	/*******************************************/
	DirectedWeightData( const UndirectedWeightData<weight_type_slot,meta_weight_type_slot> & data_without_direction )
	: UndirectedWeightData< weight_type_slot, meta_weight_type_slot >( data_without_direction ),
	  forward( false ),
	  backward( false )
	{}

	DirectedWeightData( const weight_type_slot & weight = 0, bool forward_ = false, bool backward_ = false )
	: UndirectedWeightData< weight_type_slot, meta_weight_type_slot >( weight ),
	  forward( forward_ ),
	  backward( backward_ )
	{}

	template< typename edge_with_additional_data_slot >
	DirectedWeightData( const edge_with_additional_data_slot & edge )
	: UndirectedWeightData< weight_type_slot, meta_weight_type_slot >( edge.weight ),
	  forward( edge.forward ),
	  backward( edge.backward )
	{}

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::serialize( os );
		os.write( reinterpret_cast<const char*> (& forward ), sizeof( forward ));
		os.write( reinterpret_cast<const char*> (& backward ), sizeof( backward ));
	}

	inline void deserialize( std::istream & is ) {
		UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::deserialize( is );
		is.read( reinterpret_cast<char*> (& forward ), sizeof( forward ));
		is.read( reinterpret_cast<char*> (& backward ), sizeof( backward ));
	}

	bool operator==( const DirectedWeightData<weight_type_slot, meta_weight_type_slot> & other ) const {
		return other.forward == forward && other.backward == backward && UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::operator ==( other );
	}

	bool operator!=( const DirectedWeightData<weight_type_slot, meta_weight_type_slot> & other ) const {
		return other.forward != forward && other.backward == backward && UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::operator !=( other );
	}

	bool covers( const DirectedWeightData<weight_type_slot,meta_weight_type_slot> & other ) const {
		return (!other.forward || forward) && (!other.backward || backward) && UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::covers( other );
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << (backward ? "<" : "-") << (forward ? ">" : "-") << " " << UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::toString();
		return oss.str();
	}

	bool isClosed() const { return !(forward && backward); }

	/*******************************************/
	const bool & isForward() const { return forward; }
	const bool & isBackward() const { return backward; }
	void setForward( bool value ) { forward = value; }
	void setBackward( bool value ) { backward = value; }
	void reverse(){ std::swap( forward, backward ); }

	/*******************************************/
	bool forward;
	bool backward;
};

//Generates a Static Weighted edge
template< typename node_id_slot, typename weight_type_slot, typename meta_weight_type_slot = std::numeric_limits<weight_type_slot> >
struct UndirectedWeightDataWithMiddleNode : public UndirectedWeightData< weight_type_slot, meta_weight_type_slot >{

	/*******************************************/
	UndirectedWeightDataWithMiddleNode( const UndirectedWeightData<weight_type_slot,meta_weight_type_slot> & data_without_middle_node )
	: UndirectedWeightData< weight_type_slot, meta_weight_type_slot >( data_without_middle_node ),
	  middle_node( (node_id_slot) - 1 )
	{}

	UndirectedWeightDataWithMiddleNode( const weight_type_slot & weight = 0, const node_id_slot & middle_node_ = (node_id_slot) -1 )
	: UndirectedWeightData< weight_type_slot, meta_weight_type_slot >( weight ),
	  middle_node( middle_node_ )
	{}

	template< typename edge_with_additional_data_slot >
	UndirectedWeightDataWithMiddleNode( const edge_with_additional_data_slot & edge )
	: UndirectedWeightData< weight_type_slot, meta_weight_type_slot >( edge.weight ),
	  middle_node( edge.middle_node )
	{}

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::serialize( os );
		os.write( reinterpret_cast<const char*> (& middle_node ), sizeof( middle_node ));
	}

	inline void deserialize( std::istream & is ) {
		UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::deserialize( is );
		is.read( reinterpret_cast<char*> (& middle_node ), sizeof( middle_node ));
	}

	bool operator==( const DirectedWeightData<weight_type_slot, meta_weight_type_slot> & other ) const {
		return other.middle_node == middle_node && UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::operator ==( other );
	}

	bool operator!=( const DirectedWeightData<weight_type_slot, meta_weight_type_slot> & other ) const {
		return other.middle_node != middle_node && UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::operator !=( other );
	}

	bool covers( const DirectedWeightData<weight_type_slot,meta_weight_type_slot> & other ) const {
		return UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::covers( other );
	}

	bool isClosed() const { return UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::isClosed(); }

	std::string toString() const {
		std::ostringstream oss;
		oss << "(" << middle_node << ") " << UndirectedWeightData< weight_type_slot, meta_weight_type_slot >::toString();
		return oss.str();
	}

	/*******************************************/
	const node_id_slot & getMiddleNode() const { return middle_node; }
	node_id_slot & getMiddleNode(){ return middle_node; }
	void setMiddleNode( node_id_slot value ) { middle_node = value; }

	/*******************************************/
	node_id_slot middle_node;
};

//Generates a Static Weighted edge
template< typename node_id_slot, typename weight_type_slot, typename meta_weight_type_slot = std::numeric_limits<weight_type_slot> >
struct DirectedWeightDataWithMiddleNode : public DirectedWeightData< weight_type_slot, meta_weight_type_slot >{
	/*******************************************/
	typedef node_id_slot NodeID;

	/*******************************************/
	DirectedWeightDataWithMiddleNode( const weight_type_slot & weight = 0,
											  bool forward = false,
											  bool backward = false,
											  NodeID middle_node_ =  (NodeID) -1 )
	: DirectedWeightData< weight_type_slot, meta_weight_type_slot >( weight, forward, backward ),
	  middle_node( middle_node_ )
	{}

	DirectedWeightDataWithMiddleNode( const DirectedWeightData< weight_type_slot, meta_weight_type_slot > & edge )
	: DirectedWeightData< weight_type_slot, meta_weight_type_slot >( edge.weight, edge.forward, edge.backward ),
	  middle_node( (NodeID) -1 )
	{}

	template< typename edge_with_additional_data_slot >
	DirectedWeightDataWithMiddleNode( const edge_with_additional_data_slot & edge )
	: DirectedWeightData< weight_type_slot, meta_weight_type_slot >( edge.weight, edge.forward, edge.backward ),
	  middle_node( edge.middle_node )
	{}

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		DirectedWeightData< weight_type_slot, meta_weight_type_slot >::serialize( os );
		os.write( reinterpret_cast<const char*> (& middle_node ), sizeof( middle_node ));
	}

	inline void deserialize( std::istream & is ) {
		DirectedWeightData< weight_type_slot, meta_weight_type_slot >::deserialize( is );
		is.read( reinterpret_cast<char*> (& middle_node ), sizeof( middle_node ));
	}

	bool operator==( const DirectedWeightDataWithMiddleNode<node_id_slot,weight_type_slot, meta_weight_type_slot> & other ) const {
		return other.middle_node == middle_node && DirectedWeightData< weight_type_slot, meta_weight_type_slot >::operator ==( other );
	}

	bool operator!=( const DirectedWeightDataWithMiddleNode<node_id_slot,weight_type_slot, meta_weight_type_slot> & other ) const {
		return other.middle_node != middle_node && DirectedWeightData< weight_type_slot, meta_weight_type_slot >::operator !=( other );
	}

	/* middle node is not a cover criterion */
	bool covers( const DirectedWeightDataWithMiddleNode<node_id_slot,weight_type_slot, meta_weight_type_slot> & other ) const {
		return DirectedWeightData< weight_type_slot, meta_weight_type_slot >::covers( other );
	}

	bool isClosed() const { return DirectedWeightDataWithMiddleNode<node_id_slot,weight_type_slot, meta_weight_type_slot>::isClosed(); }

	/*******************************************/
	const NodeID & getMiddleNode() const { return middle_node; }
	NodeID & getMiddleNode() { return middle_node; }
	void setMittdleNode( const NodeID & middle_node_ ) { middle_node = middle_node_; }

	std::string toString() const {
		std::ostringstream oss;
		oss << "(" << middle_node << ") " << DirectedWeightData< weight_type_slot, meta_weight_type_slot >::toString();
		return oss.str();
	}

	/*******************************************/
	NodeID middle_node;
};

template< typename weight_type_slot, typename meta_weight_type_slot = std::numeric_limits<weight_type_slot>, unsigned int resolution = 100 >
struct UndirectedBiWeightData{
	/*******************************************/
	typedef weight_type_slot weight_type;
	typedef meta_weight_type_slot meta_weight;

	/*******************************************/
	UndirectedBiWeightData( const weight_type & first_weight_ = 0, const weight_type & second_weight_ = 0 )
	: first_weight( first_weight_ ),
	  second_weight( second_weight_ )
	{}

	unsigned int getResolution() const { return resolution; }

	//Query the Weight
	weight_type operator()( weight_type interpolation ){
		return (interpolation * first_weight + (resolution - interpolation) * second_weight) / resolution;
	}

	//
	weight_type operator[]( size_t index ){
		switch( index ){
		case 0: return first_weight; break;
		case 1: return second_weight; break;
		default: assert( false ); break;
		}
		return 0;
	}

	bool dominates( const UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution> & other ){
		return first_weight <= other.first_weight && second_weight < other.second_weight;
	}

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		os.write( reinterpret_cast<const char*> (& first_weight ), sizeof( first_weight ));
		os.write( reinterpret_cast<const char*> (& second_weight ), sizeof( second_weight ));
	}

	inline void deserialize( std::istream & is ) {
		is.read( reinterpret_cast<char*> (& first_weight ), sizeof( first_weight ));
		is.read( reinterpret_cast<char*> (& second_weight ), sizeof( second_weight ));
	}

	bool operator==( const UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution> & other ) const {
		return other.first_weight == first_weight && other.second_weight == second_weight;
	}

	bool operator!=( const UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution> & other ) const {
		return other.first_weight != first_weight || other.second_weight != second_weight;
	}

	bool isClosed() const { return false; }

	std::string toString() const {
		std::ostringstream oss;
		oss << (first_weight ) << ":" << (second_weight) ;
		return oss.str();
	}

	/*******************************************/
	weight_type first_weight;
	weight_type second_weight;
};

template< typename weight_type_slot, typename meta_weight_type_slot = std::numeric_limits<weight_type_slot>, unsigned int resolution = 100 >
struct DirectedBiWeightData : public UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>{
	/*******************************************/
	DirectedBiWeightData( const weight_type_slot & first_weight = 0, const weight_type_slot & second_weight = 0,
								bool forward_ = false, bool backward_ = false )
	: UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>( first_weight, second_weight ),
	  forward( forward_ ),
	  backward( backward_ )
	{}

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>::serialize( os );
		os.write( reinterpret_cast<const char*> (& forward ), sizeof( forward ));
		os.write( reinterpret_cast<const char*> (& backward ), sizeof( backward ));
	}

	inline void deserialize( std::istream & is ) {
		UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>::deserialize( is );
		is.read( reinterpret_cast<char*> (& forward ), sizeof( forward ));
		is.read( reinterpret_cast<char*> (& backward ), sizeof( backward ));
	}

	bool operator==( const DirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution> & other ) const {
		return other.forward == forward && other.backward == backward && UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>::operator ==( other );
	}

	bool operator!=( const DirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution> & other ) const {
		return (other.forward != forward) || (other.backward == backward) || UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>::operator !=( other );
	}

	bool isClosed() const { return UndirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>::isClosed(); }

	/*******************************************/
	const bool & isForward() const { return forward; }
	const bool & isBackward() const { return backward; }
	void setForward( bool value ) { forward = value; }
	void setBackward( bool value ) { backward = value; }
	void reverse(){ std::swap( forward, backward ); }

	/*******************************************/
	bool forward;
	bool backward;
};

template< typename weight_type_slot, typename meta_weight_type_slot = std::numeric_limits<weight_type_slot>, typename node_id_slot = utility::datastructure::NodeID, unsigned int resolution = 100 >
struct DirectedBiWeightDataWithMiddleNode : public DirectedBiWeightData< weight_type_slot,meta_weight_type_slot,resolution >{
	/*******************************************/
	typedef node_id_slot NodeID;

	/*******************************************/
	DirectedBiWeightDataWithMiddleNode( const weight_type_slot & first_weight = 0, const weight_type_slot & second_weight = 0,
								bool forward = false, bool backward = false, const NodeID middle_node_ = (NodeID) -1 )
	: DirectedBiWeightData< weight_type_slot,meta_weight_type_slot,resolution >( first_weight, second_weight, forward, backward),
	  middle_node( middle_node_ )
	{}

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		DirectedBiWeightData< weight_type_slot,meta_weight_type_slot,resolution >::serialize( os );
		os.write( reinterpret_cast<const char*> (& middle_node ), sizeof( middle_node ));
	}

	inline void deserialize( std::istream & is ) {
		DirectedBiWeightData< weight_type_slot,meta_weight_type_slot,resolution >::deserialize( is );
		is.read( reinterpret_cast<char*> (& middle_node ), sizeof( middle_node ));
	}

	bool operator==( const DirectedBiWeightDataWithMiddleNode<weight_type_slot,meta_weight_type_slot,node_id_slot,resolution> & other ) const {
		return other.middle_node == middle_node && DirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>::operator ==( other );
	}

	bool operator!=( const DirectedBiWeightDataWithMiddleNode<weight_type_slot,meta_weight_type_slot,node_id_slot,resolution> & other ) const {
		return other.middle_node != middle_node || DirectedBiWeightData<weight_type_slot,meta_weight_type_slot,resolution>::operator !=( other );
	}

	bool isClosed() const { return DirectedBiWeightDataWithMiddleNode<weight_type_slot,meta_weight_type_slot,node_id_slot,resolution>::isClosed(); }

	/*******************************************/
	const NodeID & getMiddleNode() const { return middle_node; }
	NodeID & getMiddleNode() { return middle_node; }
	void setMittdleNode( const NodeID & middle_node_ ) { middle_node = middle_node_; }

	/*******************************************/
	NodeID middle_node;
};

template< typename node_id_slot, typename edge_data_slot = utility::NullData, typename user_data_slot = utility::NullData >
class Edge_t : public edge_data_slot, public user_data_slot {	//use base class optimization with NullData to use zero space if no data associated
public:
	/*******************************************/
	typedef edge_data_slot edge_data;
	typedef user_data_slot user_data;

	/*******************************************/
	Edge_t()
	: edge_data_slot(),
	  user_data_slot(),
	  target( (node_id_slot) 0 )
	{ /* do nothing */ }

	explicit Edge_t( const node_id_slot & target_, const edge_data_slot & edge_data_ )
	: edge_data_slot(edge_data_),
	  user_data_slot(),
	  target( target_ )
	{ /* do nothing */ }

	template< class edge_with_additional_information_type >
	explicit Edge_t( const edge_with_additional_information_type & edge )
	: edge_data_slot( (typename edge_with_additional_information_type::edge_data) edge ),
	  user_data_slot(),
	  target( edge.target )
	{ /* do nothing */ }

	explicit Edge_t( const user_data_slot & user_data_ )
	: edge_data_slot(),
	  user_data_slot(user_data_),
	  target( (node_id_slot) 0 )
	{ /* do nothing */ }

	explicit Edge_t( const node_id_slot & target_, const edge_data_slot & edge_data_, const user_data_slot & user_data_ )
	: edge_data_slot(edge_data_),
	  user_data_slot(user_data_),
	  target( target_ )
	{ /* do nothing */ }

	template< class edge_with_additional_information_type >
	explicit Edge_t( const edge_with_additional_information_type & edge, const user_data_slot & user_data_ )
	: edge_data_slot( (typename edge_with_additional_information_type::edge_data) edge ),
	  user_data_slot( user_data_ ),
	  target( edge.target )
	{ /* do nothing */ }

	/*******************************************/
	inline void serialize( std::ostream & os ) const {
		os.write( reinterpret_cast<const char*>(&target), sizeof( target ) );
		edge_data_slot::serialize( os );
	}

	inline void deserialize( std::istream & is ) {
		is.read( reinterpret_cast<char*>(&target), sizeof( target ) );
		edge_data_slot::deserialize( is );
	}

	bool operator==( const Edge_t<node_id_slot,edge_data_slot> & other ) const {
		return other.target == target && edge_data_slot::operator ==( other );
	}

	bool operator!=( const Edge_t<node_id_slot,edge_data_slot> & other ) const {
		return other.target != target && edge_data_slot::operator !=( other );
	}

	bool covers( const Edge_t<node_id_slot,edge_data_slot> & other ) const {
		return target == other.target && edge_data_slot::covers( other );
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << target << " " << edge_data_slot::toString();
		return oss.str();
	}

	/*******************************************/
	const node_id_slot & getTarget() const { return target; }
	node_id_slot & getTarget() { return target; }
	void setTarget( const node_id_slot & target_ ){ target = target_; }

	/*******************************************/
	node_id_slot target;
};

//Definition of basic edge types
typedef Edge_t< utility::datastructure::NodeID > UndirectedEdge;
typedef Edge_t< utility::datastructure::NodeID, UndirectedWeightData<unsigned int> > UndirectedIntegerWeightedEdge;
typedef Edge_t< utility::datastructure::NodeID, DirectedWeightData<unsigned int> > DirectedIntegerWeightedEdge;
typedef Edge_t< utility::datastructure::NodeID, DirectedWeightDataWithMiddleNode<utility::datastructure::NodeID,unsigned int> > DirectedIntegerWeightedEdgeWithMiddleNode;
typedef Edge_t< utility::datastructure::NodeID, UndirectedWeightDataWithMiddleNode<utility::datastructure::NodeID,unsigned int> > UndirectedIntegerWeightedEdgeWithMiddleNode;

typedef Edge_t< utility::datastructure::NodeID, UndirectedBiWeightData<unsigned int> > IntegerBiWeightedEdge;
typedef Edge_t< utility::datastructure::NodeID, DirectedBiWeightData<unsigned int> > DirectedIntegerBiWeightedEdge;
typedef Edge_t< utility::datastructure::NodeID, DirectedBiWeightDataWithMiddleNode<utility::datastructure::NodeID,unsigned int> > DirectedIntegerBiWeightedEdgeWithMiddleNode;


}
}


#endif /* EDGE_HPP_ */
