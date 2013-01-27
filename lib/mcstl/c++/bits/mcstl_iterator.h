/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_iterator.h 
 * @brief Helper iterator classes for the std::transform() functions. */
 
#ifndef _MCSTL_ITERATOR_H
#define _MCSTL_ITERATOR_H 1

#include <bits/mcstl_basic_iterator.h>
#include <bits/stl_pair.h>

namespace mcstl
{

/** @brief A pair of iterators. The usual iterator operations are applied to both child iterators. */
template<typename Iterator1, typename Iterator2, typename IteratorCategory>
class iterator_pair : public std::pair<Iterator1, Iterator2>
{
private:
	typedef iterator_pair<Iterator1, Iterator2, IteratorCategory> type;
	typedef std::pair<Iterator1, Iterator2> base;

public:
	typedef IteratorCategory iterator_category;
	typedef void value_type;
	typedef typename std::iterator_traits<Iterator1>::difference_type difference_type;
	typedef type* pointer;
	typedef type& reference;

	iterator_pair() {}
	
	iterator_pair(const Iterator1& first, const Iterator2& second) :
		base(first, second)
	{
	}
	
	//pre-increment operator
	type&
	operator++()
	{
		++base::first;
		++base::second;
		return *this;
	}
	
	//post-increment operator
	const type
	operator++(int)
	{
		return type(base::first++, base::second++);
	}
	
	//pre-decrement operator
	type&
	operator--()
	{
		--base::first;
		--base::second;
		return *this;
	}
	
	//post-decrement operator
	const type
	operator--(int)
	{
		return type(base::first--, base::second--);
	}
	
	//type conversion
	operator Iterator2() const
	{
		return base::second;
	}
	
	type&
	operator=(const type& other)
	{
		base::first = other.first;
		base::second = other.second;
		return *this;
	}
	
	type
	operator+(difference_type delta) const
	{
		return type(base::first + delta, base::second + delta);
	}	

	difference_type
	operator-(const type& other) const
	{
		return base::first - other.first;
	}	
};


/** @brief A triple of iterators. The usual iterator operations are applied to all three child iterators. */
template<typename Iterator1, typename Iterator2, typename Iterator3, typename IteratorCategory>
class iterator_triple
{
private:
	typedef iterator_triple<Iterator1, Iterator2, Iterator3, IteratorCategory> type;
	typedef type base;

public:
	typedef IteratorCategory iterator_category;
	typedef void value_type;
	typedef typename Iterator1::difference_type difference_type;
	typedef type* pointer;
	typedef type& reference;
	
	Iterator1 first;
	Iterator2 second;
	Iterator3 third;

	iterator_triple() {}
	
	iterator_triple(const Iterator1& _first, const Iterator2& _second, const Iterator3& _third)
	{
		first = _first;
		second = _second;
		third = _third;
	}
	
	//pre-increment operator
	type&
	operator++()
	{
		++first;
		++second;
		++third;
		return *this;
	}
	
	//post-increment operator
	const type
	operator++(int)
	{
		return type(first++, second++, third++);
	}
	
	//pre-decrement operator
	type&
	operator--()
	{
		--first;
		--second;
		--third;
		return *this;
	}
	
	//post-decrement operator
	const type
	operator--(int)
	{
		return type(first--, second--, third--);
	}
	
	//type conversion
	operator Iterator3() const
	{
		return third;
	}
	
	type&
	operator=(const type& other)
	{
		first = other.first;
		second = other.second;
		third = other.third;
		return *this;
	}
	
	type
	operator+(difference_type delta) const
	{
		return type(first + delta, second + delta, third + delta);
	}	

	difference_type
	operator-(const type& other) const
	{
		return first - other.first;
	}
};

}

#endif
