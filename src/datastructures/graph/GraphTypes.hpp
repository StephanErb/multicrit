/*
 * GraphTypes.hpp
 *
 *  Created on: 03.09.2010
 *      Author: kobitzsch
 */

#ifndef GRAPHTYPES_HPP_
#define GRAPHTYPES_HPP_

#include <iostream>

namespace utility{
namespace datastructure{

#ifdef PARANOID
class NodeID{
public:
	explicit NodeID( unsigned int value_ ) : value( value_ ){}
	NodeID() : value(0){}
	unsigned int get() const { return value; }

	bool operator==( const NodeID & rhs ) const { return value == rhs.value; }
	bool operator!=( const NodeID & rhs ) const { return value != rhs.value; }
	bool operator<( const NodeID & rhs ) const { return value < rhs.value; }
	bool operator>( const NodeID & rhs ) const { return value > rhs.value; }
	bool operator<=( const NodeID & rhs ) const { return value <= rhs.value; }
	bool operator>=( const NodeID & rhs ) const { return value >= rhs.value; }

	/*
	friend bool operator==( const NodeID & lhs, unsigned int rhs ) { return lhs.value == rhs; }
	friend bool operator!=( const NodeID & lhs, unsigned int rhs ) { return lhs.value != rhs; }
	friend bool operator<(  const NodeID & lhs, unsigned int rhs ) { return lhs.value < rhs; }
	friend bool operator>(  const NodeID & lhs, unsigned int rhs ) { return lhs.value > rhs; }
	friend bool operator<=( const NodeID & lhs, unsigned int rhs ) { return lhs.value <= rhs; }
	friend bool operator>=( const NodeID & lhs, unsigned int rhs ) { return lhs.value >= rhs; }

	friend bool operator==( unsigned int lhs, const NodeID & rhs ) { return lhs == rhs.value; }
	friend bool operator!=( unsigned int lhs, const NodeID & rhs ) { return lhs != rhs.value; }
	friend bool operator<(  unsigned int lhs, const NodeID & rhs ) { return lhs < rhs.value; }
	friend bool operator>(  unsigned int lhs, const NodeID & rhs ) { return lhs > rhs.value; }
	friend bool operator<=( unsigned int lhs, const NodeID & rhs ) { return lhs <= rhs.value; }
	friend bool operator>=( unsigned int lhs, const NodeID & rhs ) { return lhs >= rhs.value; }
	*/

	friend std::ostream & operator<<( std::ostream & os, const NodeID & nid ){
		os << "N: " << nid.value;
		return os;
	}

	const NodeID operator+( const NodeID & rhs ) const {
		NodeID retVal = *this;
		retVal += rhs;
		return retVal;
	}

	const NodeID operator+( const int & rhs ){
		NodeID retVal = *this;
		retVal += rhs;
		return retVal;
	}

	const NodeID operator+=( const NodeID & rhs ){
		value += rhs.value;
		return *this;
	}

	const NodeID operator+=( const int & rhs ){
		value += rhs;
		return *this;
	}

	const NodeID operator-( const NodeID & rhs ) const {
		NodeID retVal = *this;
		retVal -= rhs;
		return retVal;
	}

	const NodeID operator-( const int & rhs ){
		NodeID retVal = *this;
		retVal -= rhs;
		return retVal;
	}

	const NodeID operator-=( const NodeID & rhs ){
		value -= rhs.value;
		return *this;
	}

	const NodeID operator-=( const int & rhs ){
		value -= rhs;
		return *this;
	}

	const NodeID operator++(int){		//postfix
		NodeID retVal = *this;
		++value;
		return retVal;
	}

	const NodeID operator--(int){		//postfix
		NodeID retVal = *this;
		--value;
		return retVal;
	}

	const NodeID operator++(){		//prefix
		++value;
		return *this;
	}

	const NodeID operator--(){		//prefix
		--value;
		return *this;
	}

	inline operator const unsigned int &() const { return value; }
	//operator int() const { return (int) value; }
private:
	unsigned int value;
};


class EdgeID{
public:
	explicit EdgeID( unsigned int value_ ) : value( value_ ){}
	EdgeID() : value( 0 ){}
	unsigned int get() const { return value; }

	bool operator==( const EdgeID & other ) const { return value == other.value; }
	bool operator!=( const EdgeID & other ) const { return value != other.value; }
	bool operator<( const EdgeID & other ) const { return value < other.value; }
	bool operator>( const EdgeID & other ) const { return value > other.value; }
	bool operator<=( const EdgeID & other ) const { return value <= other.value; }
	bool operator>=( const EdgeID & other ) const { return value >= other.value; }

	/*
	friend bool operator==( const EdgeID & lhs, unsigned int rhs ) { return lhs.value == rhs; }
	friend bool operator!=( const EdgeID & lhs, unsigned int rhs ) { return lhs.value != rhs; }
	friend bool operator<(  const EdgeID & lhs, unsigned int rhs ) { return lhs.value < rhs; }
	friend bool operator>(  const EdgeID & lhs, unsigned int rhs ) { return lhs.value > rhs; }
	friend bool operator<=( const EdgeID & lhs, unsigned int rhs ) { return lhs.value <= rhs; }
	friend bool operator>=( const EdgeID & lhs, unsigned int rhs ) { return lhs.value >= rhs; }

	friend bool operator==( unsigned int lhs, const EdgeID & rhs ) { return lhs == rhs.value; }
	friend bool operator!=( unsigned int lhs, const EdgeID & rhs ) { return lhs != rhs.value; }
	friend bool operator<(  unsigned int lhs, const EdgeID & rhs ) { return lhs < rhs.value; }
	friend bool operator>(  unsigned int lhs, const EdgeID & rhs ) { return lhs > rhs.value; }
	friend bool operator<=( unsigned int lhs, const EdgeID & rhs ) { return lhs <= rhs.value; }
	friend bool operator>=( unsigned int lhs, const EdgeID & rhs ) { return lhs >= rhs.value; }
	 */

	friend std::ostream & operator<<( std::ostream & os, const EdgeID & nid ){
		os << "E: " << nid.value;
		return os;
	}

	const EdgeID operator+( const EdgeID & rhs ) const {
		EdgeID retVal = *this;
		retVal += rhs;
		return retVal;
	}

	const EdgeID operator+( const int & rhs ){
		EdgeID retVal = *this;
		retVal += rhs;
		return retVal;
	}

	const EdgeID operator+=( const EdgeID & rhs ){
		value += rhs.value;
		return *this;
	}

	const EdgeID operator+=( const int & rhs ){
		value += rhs;
		return *this;
	}

	const EdgeID operator-( const EdgeID & rhs ) const {
		EdgeID retVal = *this;
		retVal -= rhs;
		return retVal;
	}

	const EdgeID operator-( const int & rhs ){
		EdgeID retVal = *this;
		retVal -= rhs;
		return retVal;
	}

	const EdgeID operator-=( const EdgeID & rhs ){
		value -= rhs.value;
		return *this;
	}

	const EdgeID operator-=( const int & rhs ){
		value -= rhs;
		return *this;
	}

	const EdgeID operator++(int){		//postfix
		EdgeID retVal = *this;
		++value;
		return retVal;
	}

	const EdgeID operator--(int){		//postfix
		EdgeID retVal = *this;
		--value;
		return retVal;
	}

	const EdgeID operator++(){		//prefix
		++value;
		return *this;
	}

	const EdgeID operator--(){		//prefix
		--value;
		return *this;
	}

	inline operator const unsigned int &() const { return value; }
private:
	unsigned int value;
};

#else

typedef unsigned int NodeID;
typedef unsigned int EdgeID;

#endif


}
}


#endif /* GRAPHTYPES_HPP_ */
