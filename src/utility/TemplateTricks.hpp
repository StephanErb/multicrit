/*
 * TemplateTricks.hpp
 *
 *  Created on: 27.03.2012
 *      Author: kobitzsch
 */

#ifndef TEMPLATETRICKS_HPP_
#define TEMPLATETRICKS_HPP_

namespace utility{
	namespace tool{
		template< bool condition, typename T = void >
		class enable_if_c{
			public:
			typedef T type;
		};

		template<>
		class enable_if_c<false>{
		};

		template< class Cond, class T = void >
		struct enble_if : public enable_if_c<Cond::value, T>{};

		template< bool switch_slot, typename first_type, typename second_type > class TypeSwitch {};

		template< typename first_type, typename second_type >
		class TypeSwitch<true,first_type,second_type>{
		public: typedef first_type type;
		};

		template< typename first_type, typename second_type >
		class TypeSwitch<false,first_type,second_type>{
		public: typedef second_type type;
		};


		template< int, typename first_type, typename second_type, typename third_type > class TypeSwitch3{};

		template< typename first_type, typename second_type, typename third_type >
		class TypeSwitch3<0,first_type,second_type,third_type>{
		public: typedef first_type type;
		};

		template< typename first_type, typename second_type, typename third_type >
		class TypeSwitch3<1,first_type,second_type,third_type>{
		public: typedef second_type type;
		};

		template< typename first_type, typename second_type, typename third_type >
		class TypeSwitch3<2,first_type,second_type,third_type>{
		public: typedef third_type type;
		};


		template< int, typename first_type, typename second_type, typename third_type, typename fourth_type > class TypeSwitch4{};

		template< typename first_type, typename second_type, typename third_type, typename fourth_type >
		class TypeSwitch4<0,first_type,second_type,third_type,fourth_type>{
		public: typedef first_type type;
		};

		template< typename first_type, typename second_type, typename third_type, typename fourth_type >
		class TypeSwitch4<1,first_type,second_type,third_type,fourth_type>{
		public: typedef second_type type;
		};

		template< typename first_type, typename second_type, typename third_type, typename fourth_type >
		class TypeSwitch4<2,first_type,second_type,third_type,fourth_type>{
		public: typedef third_type type;
		};

		template< typename first_type, typename second_type, typename third_type, typename fourth_type >
		class TypeSwitch4<3,first_type,second_type,third_type,fourth_type>{
		public: typedef fourth_type type;
		};
	}
}


#endif /* TEMPLATETRICKS_HPP_ */
