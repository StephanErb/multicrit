/*
 * DummyOperators.hpp
 *
 *  Created on: 29.03.2012
 *      Author: kobitzsch
 */

#ifndef DUMMYOPERATORS_HPP_
#define DUMMYOPERATORS_HPP_

#include <vector>

namespace utility{
	namespace datastructure{

		template< typename edge_slot >
		class EdgeFlagsDummyOperator{
		public:
			static bool isDummy( const edge_slot & edge );

			static void setDummy( edge_slot & edge );
		};

		template< typename edge_slot >
		bool EdgeFlagsDummyOperator<edge_slot>::isDummy( const edge_slot & edge ){
			return not( edge.forward or edge.backward );
		}

		template< typename edge_slot >
		void EdgeFlagsDummyOperator<edge_slot>::setDummy( edge_slot & edge ){
			edge.forward = edge.backward = false;
		}
	}
}


#endif /* DUMMYOPERATORS_HPP_ */
