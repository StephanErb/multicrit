/***************************************************************************
 *   Copyright (C) 2007 by Felix Putze                                     *
 *   kontakt@felix-putze.de                                                *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_for_each_selectors.h
 *  @brief Functors representing different tasks to be plugged into the 
 *  generic parallelization methods for embarrassingly parallel functions. */
 
#ifndef _MCSTL_FOR_EACH_SELECTORS_H
#define _MCSTL_FOR_EACH_SELECTORS_H 1

#include <bits/mcstl_basic_iterator.h>
#include <mod_stl/stl_algobase.h>

namespace mcstl
{

/** @brief Generic selector for embarrassingly parallel functions. */
template<typename It>
struct generic_for_each_selector
{
	/** @brief Iterator on last element processed; needed for some algorithms (e. g. std::transform()). */
	It finish_iterator;
};


/** @brief std::for_each() selector. */
template<typename It>
struct for_each_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param o Operator.
	 *  @param i Iterator referencing object. */
	template<typename Op>
	inline bool operator()(Op& o, It i)
	{
		o(*i);
		return true;
	}
};	 

/** @brief std::generate() selector. */
template<typename It>
struct generate_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param o Operator.
	 *  @param i Iterator referencing object. */
	template<typename Op>
	inline bool operator()(Op& o, It i)
	{
		*i = o();
		return true;
	}
};	 

/** @brief std::fill() selector. */
template<typename It>
struct fill_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param v Current value.
	 *  @param i Iterator referencing object. */
	template<typename Val>
	inline bool operator()(Val& v, It i)
	{
		*i = v;
		return true;
	}
};	 

/** @brief std::transform() selector, one input sequence variant. */
template<typename It>
struct transform1_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param o Operator.
	 *  @param i Iterator referencing object. */
	template<typename Op>
	inline bool operator()(Op& o, It i)
	{
		*i.second = o(*i.first);
		return true;
	}
};	

/** @brief std::transform() selector, two input sequences variant. */
template<typename It>
struct transform2_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param o Operator.
	 *  @param i Iterator referencing object. */
	template<typename Op>
	inline bool operator()(Op& o, It i)
	{
		*i.third = o(*i.first, *i.second);
		return true;
	}
};	

/** @brief std::replace() selector. */
template<typename It, typename T>
struct replace_selector : public generic_for_each_selector<It>
{
	/** @brief Value to replace with. */
	const T& new_val;
	
	/** @brief Constructor
	 *  @param new_val Value to replace with. */
	explicit replace_selector(const T &new_val) : new_val(new_val) {}
		
	/** @brief Functor execution.
	 *  @param v Current value.
	 *  @param i Iterator referencing object. */
	inline bool operator()(T& v, It i)
	{
		if (*i == v)
			*i = new_val;
		return true;
	}
};	 

/** @brief std::replace() selector. */
template<typename It, typename Op, typename T>
struct replace_if_selector : public generic_for_each_selector<It>
{
	/** @brief Value to replace with. */
	const T& new_val;
	
	/** @brief Constructor.
	 *  @param new_val Value to replace with. */
	explicit replace_if_selector(const T &new_val) : new_val(new_val) {}	
		
	/** @brief Functor execution.
	 *  @param o Operator.
	 *  @param i Iterator referencing object. */
	inline bool operator()(Op& o, It i)
	{
		if (o(*i))
			*i = new_val;
		return true;
	}
};	 
	
/** @brief std::count() selector. */
template<typename It, typename Diff>	
struct count_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param v Current value.
	 *  @param i Iterator referencing object. 
	 *  @return 1 if count, 0 if does not count. */
	template<typename Val>
	inline Diff operator()(Val& v, It i)
	{
		return (v == *i) ? 1 : 0;
	}
};

/** @brief std::count_if() selector. */
template<typename It, typename Diff>	
struct count_if_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param o Operator.
	 *  @param i Iterator referencing object. 
	 *  @return 1 if count, 0 if does not count. */
	template<typename Op>
	inline Diff operator()(Op& o, It i)
	{
		return (o(*i)) ? 1 : 0;
	}
};

/** @brief std::accumulate() selector. */
template<typename It>
struct accumulate_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param o Operator (unused).
	 *  @param i Iterator referencing object.
	 *  @return The current value. */
	template<typename Op>
	inline typename std::iterator_traits<It>::value_type operator()(Op o, It i)
	{
		return *i;
	}
};	

/** @brief std::inner_product() selector. */
template<typename It, typename It2, typename T>
struct inner_product_selector : public generic_for_each_selector<It>
{
	/** @brief Begin iterator of first sequence. */
	It begin1_iterator;
	/** @brief Begin iterator of second sequence. */
	It2 begin2_iterator;
	
	/** @brief Constructor.
	 *  @param b1 Begin iterator of first sequence.
	 *  @param b2 Begin iterator of second sequence. */
	explicit inner_product_selector(It b1, It2 b2) : begin1_iterator(b1), begin2_iterator(b2) {}
	
	/** @brief Functor execution.
	 *  @param mult Multiplication functor.
	 *  @param current Iterator referencing object.
	 *  @return Inner product elemental result. */
	template<typename Op>
	inline T operator()(Op mult, It current)
	{
		typename std::iterator_traits<It>::difference_type position = current - begin1_iterator;
		return mult(*current, *(begin2_iterator + position));
	}
};
	
/** @brief Selector that just returns the passed iterator. */
template<typename It>		// for some reduction tasks
struct identity_selector : public generic_for_each_selector<It>
{
	/** @brief Functor execution.
	 *  @param o Operator (unused).
	 *  @param i Iterator referencing object.
	 *  @return Passed iterator. */
	template<typename Op>
	inline It operator()(Op o, It i)
	{ 
		return i;
	}
};

/** @brief Selector that returns the difference between two adjacent elements. */
template<typename It>
struct adjacent_difference_selector : public generic_for_each_selector<It>
{		
	template<typename Op>
	inline bool operator()(Op& o, It i)
	{
		typename It::first_type go_back_one = i.first;
		--go_back_one;
		*i.second = o(*i.first, *go_back_one);
		return true;
	}
};	

/** @brief Functor doing nothing
 * 
 *  For some reduction tasks (this is not a function object, but is passed as selector dummy parameter. */
struct nothing 
{
	/** @brief Functor execution.
	 *  @param i Iterator referencing object. */
	template<typename It>
	inline void operator()(It i)
	{
	}
};

// reduction functions

/** @brief Reduction function doing nothing. */
struct dummy_reduct
{
	inline bool operator()(bool /*x*/, bool /*y*/) const 
	{
		return true;
	} 
};

/** @brief Reduction for finding the maximum element, using a comparator. */
template<typename Comp, typename It>
struct min_element_reduct
{
	Comp& comp;
	
	explicit min_element_reduct(Comp &c) : comp(c)
	{
	}
	
	inline It operator()(It x, It y)
	{
		if (comp(*x, *y))
			return x;
		else
			return y;
	}
};

/** @brief Reduction for finding the maximum element, using a comparator. */
template<typename Comp, typename It>
struct max_element_reduct
{
	Comp& comp;
	
	explicit max_element_reduct(Comp& c) : comp(c)
	{
	}
	
	inline It operator()(It x, It y)
	{
		if (comp(*x, *y))
			return y;
		else
			return x;
	}
};

/** @brief General reduction, using a binary operator. */
template<typename BinOp>
struct accumulate_binop_reduct
{
	BinOp& binop;
	
	explicit accumulate_binop_reduct(BinOp& b) : binop(b) {}	
	
	template<typename T>
	inline T operator()(T x, T y) { return binop(x, y); }
};

}

#endif
