
#include <iostream>
#include <sstream>
#include "utility/datastructure/graph/GraphTypes.hpp"


#ifndef LABEL_HPP_
#define LABEL_HPP_

struct Label {
	typedef unsigned int weight_type;
	weight_type first_weight;
	weight_type second_weight;

	inline Label(const weight_type& first_weight_=0, const weight_type& second_weight_=0)
		: first_weight( first_weight_ ), second_weight( second_weight_ ) 
	{}

	inline uint64_t combined() const {
		const uint32_t& mostSignificantWord = first_weight;
		const uint32_t& leastSignificantWord = second_weight;
		return (uint64_t) mostSignificantWord << 32 | leastSignificantWord;
	}

	bool operator==(const Label& other) const {
		return other.first_weight == first_weight && other.second_weight == second_weight;
	}

	bool operator!=(const Label& other) const {
		return other.first_weight != first_weight || other.second_weight != second_weight;
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << (first_weight ) << ":" << (second_weight) ;
		return oss.str();
	}

	friend std::ostream& operator<<(std::ostream& os, const Label& data) {
		os << " (" << data.first_weight << "," << data.second_weight << ")";
		return os;
	}
};

struct NodeLabel : public Label {
	typedef utility::datastructure::NodeID NodeID;
	NodeID node;

	inline NodeLabel(const NodeID& x, const Label::weight_type& y1, const Label::weight_type& y2)
 		: Label(y1, y2), node(x)
 	{}
 	inline NodeLabel(const NodeID& x, const Label& y)
 		: Label(y), node(x)
 	{}
	inline NodeLabel() : Label(0,0), node(0) {}

	friend std::ostream& operator<<(std::ostream& os, const NodeLabel& data) {
		os << " (" << data.node << ": " << data.first_weight << "," << data.second_weight << ")";
		return os;
	}
};


#endif