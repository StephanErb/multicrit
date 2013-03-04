
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

	std::string toString() const {
		std::ostringstream oss;
		oss << (first_weight ) << ":" << (second_weight) ;
		return oss.str();
	}
};

struct NodeLabel : public Label {
	typedef utility::datastructure::NodeID NodeID;
	NodeID node;

 	NodeLabel(const NodeID& x, const Label& y)
 		: Label(y), node(x)
 	{}
	NodeLabel() : Label(0,0), node(0) {}

	friend std::ostream& operator<<(std::ostream& os, const NodeLabel& data) {
		os << " (" << data.node << ": " << data.first_weight << "," << data.second_weight << ")";
		return os;
	}
};


#endif