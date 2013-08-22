/*
 * BasicDynamicNode.h
 *
 *  Created on: 02.09.2010
 *      Author: kobitzsch
 */

#ifndef BASICDYNAMICNODE_H_
#define BASICDYNAMICNODE_H_

#include <string>
#include <iostream>

namespace utility{
namespace datastructure{

class BasicDynamicNode {
public:
	BasicDynamicNode( const unsigned int &, const unsigned int & );
	BasicDynamicNode();
	~BasicDynamicNode();

	static unsigned int byteSize(){
		return 2*sizeof( unsigned int );
	}

	unsigned int firstEdgeID( void ) const;
	void setFirstEdgeID( const unsigned int & );

	unsigned int endEdgeID( void ) const;
	void setEndEdgeID( const unsigned int & );

	std::string toString() const;

	void serialize( std::ostream & ) const;
	void deserialize( std::istream & );

private:
	unsigned int first_node_edge;
	unsigned int end_node_edge;
};

}
}
#endif /* BASICDYNAMICNODE_H_ */
