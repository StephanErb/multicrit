/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/**
 * @file mcstl_numeric.h
 * @brief Parallel STL fucntion calls corresponding to the stl_numeric.h header.
 * The functions defined here mainly do case switches and 
 * call the actual parallelized versions in other files.
 * Inlining policy: Functions that basically only contain one function call,
 * are declared inline.
 */

#ifndef _MCSTL_NUMERIC_H
#define _MCSTL_NUMERIC_H 1

#include <bits/mcstl_basic_iterator.h>
#include <mod_stl/stl_numeric.h>

#include <bits/mcstl_for_each.h>
#include <bits/mcstl_for_each_selectors.h>
#include <bits/mcstl_partial_sum.h>

#include <functional>

namespace std
{

////  accumulate  //////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename T>
inline
T
accumulate(InputIterator begin, InputIterator end, T init, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_accumulate(begin, end, init);
}

// sequential fallback
template<typename InputIterator, typename T, typename BinaryOperation>
inline
T
accumulate(InputIterator begin, InputIterator end, T init,
	BinaryOperation binary_op, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_accumulate(begin, end, init, binary_op);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename T, typename IteratorTag>
inline
T
accumulate_switch(InputIterator begin, InputIterator end, T init, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	return std::accumulate(begin, end, init, mcstl::sequential_tag());
}

// public interface
template<typename InputIterator, typename T>
inline
T
accumulate(InputIterator begin, InputIterator end, T init, mcstl::PARALLELISM parallelism_tag = mcstl::PARALLELISM::PARALLEL_UNBALANCED)
{
	return accumulate_switch(begin, end, init, std::plus<typename std::iterator_traits<InputIterator>::value_type>(), typename std::iterator_traits<InputIterator>::iterator_category(), parallelism_tag);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename T, typename BinaryOperation, typename IteratorTag>
T
accumulate_switch(InputIterator begin, InputIterator end, T init, BinaryOperation binary_op, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	return std::accumulate(begin, end, init, binary_op, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename _RandomAccessIterator, typename T, typename BinaryOperation>
T
accumulate_switch(_RandomAccessIterator begin, _RandomAccessIterator end, T init, BinaryOperation binary_op, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::accumulate_minimal_n && parallelism_tag.is_parallel()))
	{
		T res = init;
		mcstl::accumulate_selector<_RandomAccessIterator> my_selector;
		mcstl::for_each_template_random_access(begin, end, mcstl::nothing(), my_selector, mcstl::accumulate_binop_reduct<BinaryOperation>(binary_op), res, res, -1, parallelism_tag);
		return res;
	}
	else
		return std::accumulate(begin, end, init, binary_op, mcstl::sequential_tag());
}  

// public interface
template<typename InputIterator, typename T, typename BinaryOperation>
inline
T
accumulate(InputIterator begin, InputIterator end, T init, BinaryOperation binary_op, mcstl::PARALLELISM parallelism_tag = mcstl::PARALLELISM::PARALLEL_UNBALANCED)
{
	return accumulate_switch(begin, end, init, binary_op, typename std::iterator_traits<InputIterator>::iterator_category(), parallelism_tag);
}


////  inner_product  ///////////////////////////////////////////////////////

// sequential fallback
template <class InputIterator1, class InputIterator2, class T, class BinaryFunction1, class BinaryFunction2>
inline
T
inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init, BinaryFunction1 binary_op1, BinaryFunction2 binary_op2, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_inner_product(first1, last1, first2, init, binary_op1, binary_op2);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2, typename T>
inline
T
inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_inner_product(first1, last1, first2, init);
}

// parallel algorithm for random access iterators

template <class RandomAccessIterator1, class RandomAccessIterator2, class T, class BinaryFunction1, class BinaryFunction2>
T
inner_product_switch(RandomAccessIterator1 first1, RandomAccessIterator1 last1, RandomAccessIterator2 first2, T init, BinaryFunction1 binary_op1, BinaryFunction2 binary_op2, random_access_iterator_tag, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION((last1 - first1) >= mcstl::SETTINGS::accumulate_minimal_n && parallelism_tag.is_parallel()))
	{
		T res = init;
		mcstl::inner_product_selector<RandomAccessIterator1, RandomAccessIterator2, T> my_selector(first1, first2);
		mcstl::for_each_template_random_access(first1, last1, binary_op2, my_selector, binary_op1, res, res, -1, parallelism_tag);
		return res;
	}
	else
		return std::inner_product(first1, last1, first2, init, mcstl::sequential_tag());	
}

// no parallelism for input iterators

template <class InputIterator1, class InputIterator2, class T, class BinaryFunction1, class BinaryFunction2, typename IteratorTag1, typename IteratorTag2>
inline
T
inner_product_switch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init, BinaryFunction1 binary_op1, BinaryFunction2 binary_op2, IteratorTag1, IteratorTag2, mcstl::PARALLELISM parallelism_tag)
{
	return __mcstl_sequential_inner_product(first1, last1, first2, init, binary_op1, binary_op2);
}

template <class InputIterator1, class InputIterator2, class T, class BinaryFunction1, class BinaryFunction2>
inline
T
inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init, BinaryFunction1 binary_op1, BinaryFunction2 binary_op2, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_unbalanced)
{
	return inner_product_switch(first1, last1, first2, init, binary_op1, binary_op2, typename std::iterator_traits<InputIterator1>::iterator_category(), typename std::iterator_traits<InputIterator2>::iterator_category(), parallelism_tag);	
}

template<typename InputIterator1, typename InputIterator2, typename T>
inline
T
inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_unbalanced)
{
	return inner_product(first1, last1, first2, init, std::plus<typename std::iterator_traits<InputIterator1>::value_type>(), std::multiplies<typename std::iterator_traits<InputIterator1>::value_type>(), parallelism_tag);
}


////  partial_sum  /////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename OutputIterator>
inline
OutputIterator
partial_sum(InputIterator begin, InputIterator end,
		OutputIterator result, 
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_partial_sum(begin, end, result);
}

// sequential fallback
template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
inline
OutputIterator
partial_sum(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation bin_op,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_partial_sum(begin, end, result, bin_op);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename OutputIterator, typename BinaryOperation, typename IteratorTag1, typename IteratorTag2>
inline
OutputIterator
partial_sum_switch(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation bin_op,
		IteratorTag1, IteratorTag2)
{
	return __mcstl_sequential_partial_sum(begin, end, result, bin_op);
}

// parallel algorithm for random access iterators
template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
OutputIterator
partial_sum_switch(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation bin_op,
		random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::partial_sum_minimal_n))
		return mcstl::parallel_partial_sum(begin, end, result, bin_op);
	else
		return partial_sum(begin, end, result, bin_op, mcstl::sequential_tag());
}

// public interface
template<typename InputIterator, typename OutputIterator>
inline
OutputIterator
partial_sum(InputIterator begin, InputIterator end,
		OutputIterator result)
{
	return partial_sum(begin, end, result, std::plus<typename iterator_traits<InputIterator>::value_type>());
}

// public interface
template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
inline
OutputIterator
partial_sum(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation binary_op)
{
	return std::partial_sum_switch(begin, end, result, binary_op,
		typename std::iterator_traits<InputIterator>::iterator_category(), typename std::iterator_traits<OutputIterator>::iterator_category());
}


////  adjacent_difference  /////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename OutputIterator>
inline
OutputIterator
adjacent_difference(InputIterator begin, InputIterator end,
		OutputIterator result, 
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_adjacent_difference(begin, end, result);
}

// sequential fallback
template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
inline
OutputIterator
adjacent_difference(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation bin_op,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_adjacent_difference(begin, end, result, bin_op);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename OutputIterator, typename BinaryOperation, typename IteratorTag1, typename IteratorTag2>
inline
OutputIterator
adjacent_difference_switch(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation bin_op,
		IteratorTag1, IteratorTag2, mcstl::PARALLELISM)
{
	return std::adjacent_difference(begin, end, result, bin_op);
}

// parallel algorithm for random access iterators
template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
OutputIterator
adjacent_difference_switch(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation bin_op,
		random_access_iterator_tag, random_access_iterator_tag, 
		mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::adjacent_difference_minimal_n && parallelism_tag.is_parallel()))
	{
		bool dummy = true;
		typedef mcstl::iterator_pair<InputIterator, OutputIterator, random_access_iterator_tag> ip;
		*result = *begin;
		ip begin_pair(begin + 1, result + 1), end_pair(end, result + (end - begin));
		mcstl::adjacent_difference_selector<ip> functionality;
		mcstl::for_each_template_random_access(begin_pair, end_pair, bin_op, functionality, mcstl::dummy_reduct(), dummy, dummy, -1, parallelism_tag);
		return functionality.finish_iterator;
	}
	else
		return adjacent_difference(begin, end, result, bin_op, mcstl::sequential_tag());
}

// public interface
template<typename InputIterator, typename OutputIterator>
inline
OutputIterator
adjacent_difference(InputIterator begin, InputIterator end,
		OutputIterator result, 
		mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return adjacent_difference(begin, end, result, std::minus<typename iterator_traits<InputIterator>::value_type>());
}

// public interface
template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
inline
OutputIterator
adjacent_difference(InputIterator begin, InputIterator end,
		OutputIterator result, 
		BinaryOperation binary_op,
		mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return std::adjacent_difference_switch(begin, end, result, binary_op,
		typename std::iterator_traits<InputIterator>::iterator_category(), typename std::iterator_traits<OutputIterator>::iterator_category(), parallelism_tag);
}



} /* namespace std */

#endif /* _MCSTL_NUMERIC_H */
