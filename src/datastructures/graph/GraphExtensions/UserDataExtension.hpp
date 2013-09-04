/*
 * UserDataExtension.hpp
 *
 *  Created on: 21.05.2012
 *      Author: kobitzsch
 */

#ifndef USERDATAEXTENSION_HPP_
#define USERDATAEXTENSION_HPP_

#include <vector>

namespace utility{
namespace datastructure{

template< typename data_slot >
class KGraphUserDataExtension{
public:
	void initializeDataExtension( size_t number_of_data_objects, const data_slot & initial_data = data_slot() ){
		user_data.resize( number_of_data_objects, initial_data );
	}

	const data_slot & getData( const NodeID & nid ) const {
		return user_data[nid];
	}

	data_slot & getData( const NodeID & nid ) {
		return user_data[nid];
	}

protected:
	std::vector<data_slot> user_data;
};

} /* namespace datastructrue */
} /* namespace utility */

#endif /* USERDATAEXTENSION_HPP_ */
