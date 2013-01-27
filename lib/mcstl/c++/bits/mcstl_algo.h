/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_algo.h
 *  @brief Parallel STL function calls corresponding to the stl_algo.h header.
 * 
 *  The functions defined here mainly do case switches and
 *  call the actual parallelized versions in other files.
 *  Inlining policy: Functions that basically only contain one function call,
 *  are declared inline. */

#ifndef _MCSTL_ALGORITHM_H
#define _MCSTL_ALGORITHM_H 1

#include <mod_stl/stl_algobase.h>
#include <mod_stl/stl_algo.h>

#include <bits/mcstl_iterator.h>

#include <bits/mcstl_base.h>
#include <bits/mcstl_sort.h>

#include <bits/mcstl_workstealing.h>
#include <bits/mcstl_par_loop.h>
#include <bits/mcstl_omp_loop.h>
#include <bits/mcstl_omp_loop_static.h>
#include <bits/mcstl_for_each_selectors.h>
#include <bits/mcstl_for_each.h>

#include <bits/mcstl_find.h>
#include <bits/mcstl_find_selectors.h>

#include <bits/mcstl_search.h>

#include <bits/mcstl_random_shuffle.h>

#include <bits/mcstl_partition.h>

#include <bits/mcstl_merge.h>

#include <bits/mcstl_unique_copy.h>

#include <bits/mcstl_set_operations.h>

namespace std
{

////  mismatch  ////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2>
inline
pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_mismatch<InputIterator1, InputIterator2>(begin1, end1, begin2);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2, typename Predicate>
inline
pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
	Predicate pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_mismatch(begin1, end1, begin2, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator1, typename InputIterator2, typename Predicate, typename IteratorTag1, typename IteratorTag2>
inline pair<InputIterator1, InputIterator2>
mismatch_switch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
	Predicate pred, IteratorTag1, IteratorTag2)
{
	return __mcstl_sequential_mismatch(begin1, end1, begin2, pred);
}

// parallel mismatch for random access iterators
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Predicate>
pair<RandomAccessIterator1, RandomAccessIterator2>
mismatch_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1, RandomAccessIterator2 begin2,	Predicate pred, random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(true))
	{
		RandomAccessIterator1 res_first =
		mcstl::find_template(begin1, end1, begin2, pred, mcstl::mismatch_selector()).first;
		return make_pair(res_first, begin2 + (res_first - begin1));
	}
	else
		return __mcstl_sequential_mismatch(begin1, end1, begin2, pred);
}

// public interface
template<typename InputIterator1, typename InputIterator2>
inline pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2)
{
	return mismatch_switch(begin1, end1, begin2, mcstl::equal_to<typename iterator_traits<InputIterator1>::value_type, typename iterator_traits<InputIterator2>::value_type>(),
			typename std::iterator_traits<InputIterator1>::iterator_category(), typename std::iterator_traits<InputIterator2>::iterator_category());
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename Predicate>
inline pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
	Predicate pred)
{
	return mismatch_switch(begin1, end1, begin2, pred,
		typename std::iterator_traits<InputIterator1>::iterator_category(), 
		typename std::iterator_traits<InputIterator2>::iterator_category());
}


////  for_each  ////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename Function>
inline
Function
for_each(InputIterator begin, InputIterator end, Function f, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_for_each<InputIterator, Function>(begin, end, f);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename Function, typename IteratorTag>
Function
for_each_switch(InputIterator begin, InputIterator end, Function f, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	return std::for_each<InputIterator, Function>(begin, end, f, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Function>
Function
for_each_switch(RandomAccessIterator begin, RandomAccessIterator end, Function f, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::for_each_minimal_n && parallelism_tag.is_parallel()))
	{
		bool dummy;
		mcstl::for_each_selector<RandomAccessIterator> functionality;
		return mcstl::for_each_template_random_access(begin, end, f, functionality, mcstl::dummy_reduct(), true, dummy, -1, parallelism_tag);
	}
	else
		return std::for_each<RandomAccessIterator, Function>(begin, end, f, mcstl::sequential_tag());
}

// public interface
template<typename Iterator, typename Function>
inline
Function
for_each(Iterator begin, Iterator end, Function f, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return for_each_switch(begin, end, f, typename std::iterator_traits<Iterator>::iterator_category(), parallelism_tag);
}


////  find  ////////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename T>
inline
InputIterator
find(InputIterator begin, InputIterator end,
	const T& val, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_find<InputIterator, T>(begin, end, val);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename T, typename IteratorTag>
inline InputIterator
find_switch(InputIterator begin, InputIterator end, const T& val, IteratorTag)
{
	return __mcstl_sequential_find(begin, end, val);
}

// parallel find for random access iterators
template<typename RandomAccessIterator, typename T>
RandomAccessIterator
find_switch(RandomAccessIterator begin, RandomAccessIterator end,
	const T& val, random_access_iterator_tag)
{
	typedef typename iterator_traits<RandomAccessIterator>::value_type ValueType;
	
	if(MCSTL_PARALLEL_CONDITION(true))
	{
		binder2nd<mcstl::equal_to<ValueType, T> > comp(mcstl::equal_to<ValueType, T>(), val);
		return mcstl::find_template(begin, end, begin, comp, mcstl::find_if_selector()).first;
	}
	else
		return __mcstl_sequential_find(begin, end, val);
}

// public interface
template<typename InputIterator, typename T>
inline InputIterator
find(InputIterator begin, InputIterator end, const T& val)
{
	return std::find_switch(begin, end, val,
		typename std::iterator_traits<InputIterator>::iterator_category());
}


////  find_if  /////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename Predicate>
inline
InputIterator
find_if(InputIterator begin, InputIterator end,
		Predicate pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_find_if<InputIterator, Predicate>(begin, end, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename Predicate, typename IteratorTag>
inline InputIterator
find_if_switch(InputIterator begin, InputIterator end,
	Predicate pred, IteratorTag)
{
	return __mcstl_sequential_find_if(begin, end, pred);
}

// parallel find_if for random access iterators
template<typename RandomAccessIterator, typename Predicate>
RandomAccessIterator
find_if_switch(RandomAccessIterator begin, RandomAccessIterator end,
	Predicate pred, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(true))
		return mcstl::find_template(begin, end, begin, pred, mcstl::find_if_selector()).first;
	else
		return __mcstl_sequential_find_if(begin, end, pred);
}

// public interface
template<typename InputIterator, typename Predicate>
inline InputIterator
find_if(InputIterator begin, InputIterator end,
	Predicate pred)
{
	return find_if_switch(begin, end, pred,
			typename std::iterator_traits<InputIterator>::iterator_category());
}


////  find_first_of  ///////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename ForwardIterator>
inline
InputIterator
find_first_of(InputIterator begin1, InputIterator end1,
		ForwardIterator begin2, ForwardIterator end2, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_find_first_of(begin1, end1, begin2, end2);
}

// sequential fallback
template<typename InputIterator, typename ForwardIterator,
	typename BinaryPredicate>
inline
InputIterator
find_first_of(InputIterator begin1, InputIterator end1,
		ForwardIterator begin2, ForwardIterator end2,
		BinaryPredicate comp, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_find_first_of(begin1, end1, begin2, end2, comp);
}

// sequential fallback for input iterator type
template<typename InputIterator, typename ForwardIterator, typename IteratorTag1, typename IteratorTag2>
inline
InputIterator
find_first_of_switch(InputIterator begin1, InputIterator end1,
		ForwardIterator begin2, ForwardIterator end2, IteratorTag1, IteratorTag2)
{
	return find_first_of(begin1, end1, begin2, end2, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename ForwardIterator, typename BinaryPredicate, typename IteratorTag>
inline
RandomAccessIterator
find_first_of_switch(RandomAccessIterator begin1, RandomAccessIterator end1,
		ForwardIterator begin2, ForwardIterator end2, BinaryPredicate comp, random_access_iterator_tag, IteratorTag)
{
	return mcstl::find_template(begin1, end1, begin1, comp, mcstl::find_first_of_selector<ForwardIterator>(begin2, end2)).first;
}

// sequential fallback for input iterator type
template<typename InputIterator, typename ForwardIterator, typename BinaryPredicate, typename IteratorTag1, typename IteratorTag2>
inline
InputIterator
find_first_of_switch(InputIterator begin1, InputIterator end1,
		ForwardIterator begin2, ForwardIterator end2, BinaryPredicate comp, IteratorTag1, IteratorTag2)
{
	return find_first_of(begin1, end1, begin2, end2, comp, mcstl::sequential_tag());
}

// public interface
template<typename InputIterator, typename ForwardIterator, typename BinaryPredicate>
inline
InputIterator
find_first_of(InputIterator begin1, InputIterator end1,
		ForwardIterator begin2, ForwardIterator end2, BinaryPredicate comp)
{
	return find_first_of_switch(begin1, end1, begin2, end2, comp, typename std::iterator_traits<InputIterator>::iterator_category(), typename std::iterator_traits<ForwardIterator>::iterator_category());
}

// public interface, insert default comparator
template<typename InputIterator, typename ForwardIterator>
InputIterator
find_first_of(InputIterator begin1, InputIterator end1,
		ForwardIterator begin2, ForwardIterator end2)
{
	return find_first_of(begin1, end1, begin2, end2, mcstl::equal_to<typename iterator_traits<InputIterator>::value_type, typename iterator_traits<ForwardIterator>::value_type>());
}


////  unique_copy  /////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename OutputIterator>
inline OutputIterator
unique_copy(InputIterator begin1, InputIterator end1, OutputIterator out,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_unique_copy<InputIterator, OutputIterator>(begin1, end1, out);
}

// sequential fallback
template<typename InputIterator, typename OutputIterator, typename Predicate>
inline OutputIterator
unique_copy(InputIterator begin1, InputIterator end1, OutputIterator out,
		Predicate pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_unique_copy<InputIterator, OutputIterator, Predicate>(begin1, end1, out, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator, typename OutputIterator, typename Predicate, typename IteratorTag1, typename IteratorTag2>
inline
OutputIterator
unique_copy_switch(InputIterator begin, InputIterator last, OutputIterator out,
    Predicate pred, IteratorTag1, IteratorTag2)
{
	return __mcstl_sequential_unique_copy(begin, last, out, pred);
}

// parallel unique_copy for random access iterators
template<typename RandomAccessIterator, typename RandomAccessOutputIterator, typename Predicate>
RandomAccessOutputIterator
unique_copy_switch(RandomAccessIterator begin, RandomAccessIterator last, RandomAccessOutputIterator out,
    Predicate pred, random_access_iterator_tag, random_access_iterator_tag)
{
	if (MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(last - begin) > mcstl::SETTINGS::unique_copy_minimal_n))
		return mcstl::parallel_unique_copy(begin, last, out, pred);
	else
		return __mcstl_sequential_unique_copy(begin, last, out, pred);
}

// public interface
template<typename InputIterator, typename OutputIterator>
inline OutputIterator
unique_copy(InputIterator begin1, InputIterator end1, OutputIterator out)
{
	return unique_copy_switch(begin1, end1, out, equal_to<typename iterator_traits<InputIterator>::value_type>(),
            typename std::iterator_traits<InputIterator>::iterator_category(),
            typename std::iterator_traits<OutputIterator>::iterator_category());
}

// public interface
template<typename InputIterator, typename OutputIterator, typename Predicate>
inline OutputIterator
unique_copy(InputIterator begin1, InputIterator end1, OutputIterator out,
    Predicate pred)
{
	return unique_copy_switch(begin1, end1, out, pred,
            typename std::iterator_traits<InputIterator>::iterator_category(),
            typename std::iterator_traits<OutputIterator>::iterator_category());
}


////  set_union  ///////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator>
inline OutputIterator
set_union(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, mcstl::sequential_tag)
{
        return std::__mcstl_sequential_set_union(begin1, end1, begin2, end2, out);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename Predicate>
inline OutputIterator
set_union(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, Predicate pred, mcstl::sequential_tag)
{
        return std::__mcstl_sequential_set_union(begin1, end1, begin2, end2, out, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator1, typename InputIterator2, typename Predicate,
    typename OutputIterator, typename IteratorTag1, typename IteratorTag2, typename IteratorTag3>
inline OutputIterator set_union_switch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
            InputIterator2 end2, OutputIterator result, Predicate pred, IteratorTag1,
            IteratorTag2, IteratorTag3)
{
	return __mcstl_sequential_set_union(begin1, end1, begin2, end2, result);
}

// parallel set_union for random access iterators
template<typename RandomAccessIterator1, typename RandomAccessIterator2,
    typename OutputRandomAccessIterator, typename Predicate>
OutputRandomAccessIterator set_union_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1, RandomAccessIterator2 begin2,
            RandomAccessIterator2 end2, OutputRandomAccessIterator result, Predicate pred,
            random_access_iterator_tag, random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end1 - begin1) >= mcstl::SETTINGS::set_union_minimal_n || static_cast<mcstl::sequence_index_t>(end2 - begin2) >= mcstl::SETTINGS::set_union_minimal_n))
		return mcstl::parallel_set_union(begin1, end1, begin2, end2, result, pred);
	else
		return __mcstl_sequential_set_union(begin1, end1, begin2, end2, result, pred);
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename OutputIterator>
inline OutputIterator set_union(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, OutputIterator out)
{
        return set_union_switch(begin1, end1, begin2, end2, out, mcstl::less<typename iterator_traits<InputIterator1>::value_type, typename iterator_traits<InputIterator2>::value_type>(),
			typename std::iterator_traits<InputIterator1>::iterator_category(),
			typename std::iterator_traits<InputIterator2>::iterator_category(),
			typename std::iterator_traits<OutputIterator>::iterator_category());
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
inline OutputIterator set_union(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
            InputIterator2 end2, OutputIterator out, Predicate pred)
{
        return set_union_switch(begin1, end1, begin2, end2, out, pred,
                    typename std::iterator_traits<InputIterator1>::iterator_category(),
                    typename std::iterator_traits<InputIterator2>::iterator_category(),
                    typename std::iterator_traits<OutputIterator>::iterator_category());
}


////  set_intersection  ////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator>
inline OutputIterator
set_intersection(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_set_intersection(begin1, end1, begin2, end2, out);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename Predicate>
inline OutputIterator
set_intersection(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, Predicate pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_set_intersection(begin1, end1, begin2, end2, out, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator1, typename InputIterator2, typename Predicate,
    typename OutputIterator, typename IteratorTag1, typename IteratorTag2, typename IteratorTag3>
inline OutputIterator set_intersection_switch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
            InputIterator2 end2, OutputIterator result, Predicate pred, IteratorTag1,
            IteratorTag2, IteratorTag3)
{
	return __mcstl_sequential_set_intersection(begin1, end1, begin2, end2, result);
}

// parallel set_intersection for random access iterators
template<typename RandomAccessIterator1, typename RandomAccessIterator2,
    typename OutputRandomAccessIterator, typename Predicate>
OutputRandomAccessIterator set_intersection_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1, RandomAccessIterator2 begin2,
            RandomAccessIterator2 end2, OutputRandomAccessIterator result, Predicate pred,
            random_access_iterator_tag, random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end1 - begin1) >= mcstl::SETTINGS::set_union_minimal_n || static_cast<mcstl::sequence_index_t>(end2 - begin2) >= mcstl::SETTINGS::set_union_minimal_n))
		return mcstl::parallel_set_intersection(begin1, end1, begin2, end2, result, pred);
	else
		return __mcstl_sequential_set_intersection(begin1, end1, begin2, end2, result, pred);
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename OutputIterator>
inline OutputIterator set_intersection(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, OutputIterator out)
{
	return set_intersection_switch(begin1, end1, begin2, end2, out, mcstl::less<typename iterator_traits<InputIterator1>::value_type, typename iterator_traits<InputIterator2>::value_type>(),
		typename std::iterator_traits<InputIterator1>::iterator_category(),
		typename std::iterator_traits<InputIterator2>::iterator_category(),
		typename std::iterator_traits<OutputIterator>::iterator_category());
}

template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
inline OutputIterator set_intersection(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
            InputIterator2 end2, OutputIterator out, Predicate pred)
{
	return set_intersection_switch(begin1, end1, begin2, end2, out, pred,
		typename std::iterator_traits<InputIterator1>::iterator_category(),
		typename std::iterator_traits<InputIterator2>::iterator_category(),
		typename std::iterator_traits<OutputIterator>::iterator_category());
}


////  set_symmetric_difference  ////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator>
inline OutputIterator
set_symmetric_difference(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, mcstl::sequential_tag)
{
        return std::__mcstl_sequential_set_symmetric_difference(begin1,end1, begin2, end2, out);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename Predicate>
inline OutputIterator
set_symmetric_difference(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, Predicate pred, mcstl::sequential_tag)
{
        return std::__mcstl_sequential_set_symmetric_difference(begin1, end1, begin2, end2, out, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator1, typename InputIterator2, typename Predicate, typename OutputIterator, typename IteratorTag1, typename IteratorTag2, typename IteratorTag3>
inline OutputIterator set_symmetric_difference_switch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, OutputIterator result, Predicate pred, IteratorTag1, IteratorTag2, IteratorTag3)
{
	return __mcstl_sequential_set_symmetric_difference(begin1, end1, begin2, end2, result);
}

// parallel set_symmetric_difference for random access iterators
template<typename RandomAccessIterator1, typename RandomAccessIterator2,
    typename OutputRandomAccessIterator, typename Predicate>
OutputRandomAccessIterator set_symmetric_difference_switch(RandomAccessIterator1 begin1,
			RandomAccessIterator1 end1, RandomAccessIterator2 begin2,
			RandomAccessIterator2 end2, OutputRandomAccessIterator result, Predicate pred,
            random_access_iterator_tag, random_access_iterator_tag, random_access_iterator_tag)
{
        if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end1 - begin1) >= mcstl::SETTINGS::set_symmetric_difference_minimal_n || static_cast<mcstl::sequence_index_t>(end2 - begin2) >= mcstl::SETTINGS::set_symmetric_difference_minimal_n))
		return mcstl::parallel_set_symmetric_difference(begin1, end1, begin2, end2, result, pred);
        else
		return __mcstl_sequential_set_symmetric_difference(begin1, end1, begin2, end2, result, pred);
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename OutputIterator>
inline OutputIterator set_symmetric_difference(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, OutputIterator out)
{
        return set_symmetric_difference_switch(begin1, end1, begin2, end2, out, mcstl::less<typename iterator_traits<InputIterator1>::value_type, typename iterator_traits<InputIterator2>::value_type>(),
                            typename std::iterator_traits<InputIterator1>::iterator_category(),
                            typename std::iterator_traits<InputIterator2>::iterator_category(),
                            typename std::iterator_traits<OutputIterator>::iterator_category());
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
inline OutputIterator set_symmetric_difference(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
            InputIterator2 end2, OutputIterator out, Predicate pred)
{
        return set_symmetric_difference_switch(begin1, end1, begin2, end2, out, pred,
                    typename std::iterator_traits<InputIterator1>::iterator_category(),
                    typename std::iterator_traits<InputIterator2>::iterator_category(),
                    typename std::iterator_traits<OutputIterator>::iterator_category());
}


////  set_difference  //////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator>
inline OutputIterator
set_difference(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, mcstl::sequential_tag)
{
        return std::__mcstl_sequential_set_difference(begin1,end1, begin2, end2, out);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename Predicate>
inline OutputIterator
set_difference(InputIterator1 begin1, InputIterator1 end1,
		InputIterator2 begin2, InputIterator2 end2,
		OutputIterator out, Predicate pred, mcstl::sequential_tag)
{
        return std::__mcstl_sequential_set_difference(begin1, end1, begin2, end2, out, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator1, typename InputIterator2, typename Predicate,
    typename OutputIterator, typename IteratorTag1, typename IteratorTag2, typename IteratorTag3>
inline OutputIterator set_difference_switch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
            InputIterator2 end2, OutputIterator result, Predicate pred, IteratorTag1, IteratorTag2, IteratorTag3)
{
	return __mcstl_sequential_set_difference(begin1, end1, begin2, end2, result);
}

// parallel set_difference for random access iterators
template<typename RandomAccessIterator1, typename RandomAccessIterator2,
    typename OutputRandomAccessIterator, typename Predicate>
OutputRandomAccessIterator set_difference_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1, RandomAccessIterator2 begin2,
            RandomAccessIterator2 end2, OutputRandomAccessIterator result, Predicate pred,
            random_access_iterator_tag, random_access_iterator_tag, random_access_iterator_tag)
{
        if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end1 - begin1) >= mcstl::SETTINGS::set_difference_minimal_n || static_cast<mcstl::sequence_index_t>(end2 - begin2) >= mcstl::SETTINGS::set_difference_minimal_n))
            return mcstl::parallel_set_difference(begin1, end1, begin2, end2, result, pred);
        else
            return __mcstl_sequential_set_difference(begin1, end1, begin2, end2, result, pred);
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename OutputIterator>
inline OutputIterator set_difference(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, OutputIterator out)
{
        return set_difference_switch(begin1, end1, begin2, end2, out, mcstl::less<typename iterator_traits<InputIterator1>::value_type, typename iterator_traits<InputIterator2>::value_type>(),
                            typename std::iterator_traits<InputIterator1>::iterator_category(),
                            typename std::iterator_traits<InputIterator2>::iterator_category(),
                            typename std::iterator_traits<OutputIterator>::iterator_category());
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate>
inline OutputIterator set_difference(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
            InputIterator2 end2, OutputIterator out, Predicate pred)
{
        return set_difference_switch(begin1, end1, begin2, end2, out, pred,
                    typename std::iterator_traits<InputIterator1>::iterator_category(),
                    typename std::iterator_traits<InputIterator2>::iterator_category(),
                    typename std::iterator_traits<OutputIterator>::iterator_category());
}


////  adjacent_find  ///////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator>
inline
ForwardIterator
adjacent_find(ForwardIterator begin, ForwardIterator end,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_adjacent_find<ForwardIterator>(begin, end);
}

// sequential fallback
template<typename ForwardIterator, typename BinaryPredicate>
inline
ForwardIterator
adjacent_find(ForwardIterator begin, ForwardIterator end,
		BinaryPredicate binary_pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_adjacent_find<ForwardIterator, BinaryPredicate>(begin, end, binary_pred);
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator>
RandomAccessIterator
adjacent_find_switch(RandomAccessIterator begin, RandomAccessIterator end, random_access_iterator_tag)
{
	typedef typename iterator_traits<RandomAccessIterator>::value_type ValueType;

	if(MCSTL_PARALLEL_CONDITION(true))
	{
		RandomAccessIterator spot = mcstl::find_template(begin, end - 1, begin, equal_to<ValueType>(), mcstl::adjacent_find_selector()).first;
		if(spot == (end - 1))
			return end;
		else
			return spot;
	}
	else
		return std::adjacent_find<RandomAccessIterator>(begin, end, mcstl::sequential_tag());
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename IteratorTag>
ForwardIterator
inline
adjacent_find_switch(ForwardIterator begin, ForwardIterator end, IteratorTag)
{
	return adjacent_find<ForwardIterator>(begin, end, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator>
inline
ForwardIterator
adjacent_find(ForwardIterator begin, ForwardIterator end)
{
	return adjacent_find_switch(begin, end, typename std::iterator_traits<ForwardIterator>::iterator_category());
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename BinaryPredicate, typename IteratorTag>
inline
ForwardIterator
adjacent_find_switch(ForwardIterator begin, ForwardIterator end, BinaryPredicate binary_pred, IteratorTag)
{
	return adjacent_find<ForwardIterator, BinaryPredicate>(begin, end, binary_pred, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename BinaryPredicate>
RandomAccessIterator
adjacent_find_switch(RandomAccessIterator begin, RandomAccessIterator end, BinaryPredicate binary_pred, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(true))
		return mcstl::find_template(begin, end, begin, binary_pred, mcstl::adjacent_find_selector()).first;
	else
		return std::adjacent_find(begin, end, binary_pred, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename BinaryPredicate>
inline
ForwardIterator
adjacent_find(ForwardIterator begin, ForwardIterator end, BinaryPredicate binary_pred)
{
	return adjacent_find_switch<ForwardIterator, BinaryPredicate>(begin, end, binary_pred, typename std::iterator_traits<ForwardIterator>::iterator_category());
}


////  count  ///////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename T>
inline
typename iterator_traits<InputIterator>::difference_type
count(InputIterator begin, InputIterator end, const T& value, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_count<InputIterator, T>(begin, end, value);
}

// parallel code for random access iterators
template<typename RandomAccessIterator, typename T>
typename iterator_traits<RandomAccessIterator>::difference_type
count_switch(RandomAccessIterator begin, RandomAccessIterator end, const T& value, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	typedef typename iterator_traits<RandomAccessIterator>::value_type ValueType;
	typedef typename iterator_traits<RandomAccessIterator>::difference_type DiffType;
	
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::count_minimal_n && parallelism_tag.is_parallel()))
	{
		DiffType res = 0;
		mcstl::count_selector<RandomAccessIterator, DiffType> functionality;
		mcstl::for_each_template_random_access(begin, end, value, functionality, std::plus<mcstl::sequence_index_t>(), res, res, -1, parallelism_tag);
		return res;
	}
	else
		return std::count<RandomAccessIterator, T>(begin, end, value, mcstl::sequential_tag());
}

// sequential fallback for input iterator case
template<typename InputIterator, typename T, typename IteratorTag>
typename iterator_traits<InputIterator>::difference_type
count_switch(InputIterator begin, InputIterator end, const T& value, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	return std::count<InputIterator, T>(begin, end, value, mcstl::sequential_tag());
}

// public interface
template<typename InputIterator, typename T>
inline
typename iterator_traits<InputIterator>::difference_type
count(InputIterator begin, InputIterator end, const T& value, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_unbalanced)
{
	return count_switch(begin, end, value, typename std::iterator_traits<InputIterator>::iterator_category(), parallelism_tag);
}


////  count_if  ////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename Predicate>
inline
typename iterator_traits<InputIterator>::difference_type
count_if(InputIterator begin, InputIterator end, Predicate pred, mcstl::sequential_tag)
{
	return __mcstl_sequential_count_if(begin, end, pred);
}

// parallel count_if for random access iterators
template<typename RandomAccessIterator, typename Predicate>
typename iterator_traits<RandomAccessIterator>::difference_type
count_if_switch(RandomAccessIterator begin, RandomAccessIterator end, Predicate pred, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	typedef typename iterator_traits<RandomAccessIterator>::value_type ValueType;
	typedef typename iterator_traits<RandomAccessIterator>::difference_type DiffType;
	
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::count_minimal_n && parallelism_tag.is_parallel()))
	{
		DiffType res = 0;
		mcstl::count_if_selector<RandomAccessIterator, DiffType> functionality;
		mcstl::for_each_template_random_access(begin, end, pred, functionality, std::plus<mcstl::sequence_index_t>(), res, res, -1, parallelism_tag);
		return res;
	}
	else
		return std::count_if<RandomAccessIterator, Predicate>(begin, end, pred, mcstl::sequential_tag());
}

// sequential fallback for input iterator case
template<typename InputIterator, typename Predicate, typename IteratorTag>
typename iterator_traits<InputIterator>::difference_type
count_if_switch(InputIterator begin, InputIterator end, Predicate pred, IteratorTag, mcstl::PARALLELISM)
{
	return std::count_if<InputIterator, Predicate>(begin, end, pred, mcstl::sequential_tag());
}

// public interface
template<typename InputIterator, typename Predicate>
inline
typename iterator_traits<InputIterator>::difference_type
count_if(InputIterator begin, InputIterator end, Predicate pred, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_unbalanced)
{
	return count_if_switch(begin, end, pred, typename std::iterator_traits<InputIterator>::iterator_category(), parallelism_tag);
}


////  search  //////////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator1, typename ForwardIterator2>
inline
ForwardIterator1
search(ForwardIterator1 begin1, ForwardIterator1 end1,
	ForwardIterator2 begin2, ForwardIterator2 end2, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_search(begin1, end1, begin2, end2);
}

// parallel algorithm for random access iterator
template<typename RandomAccessIterator1, typename RandomAccessIterator2>
RandomAccessIterator1
search_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1,
	RandomAccessIterator2 begin2, RandomAccessIterator2 end2, random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(true))
		return mcstl::search_template(begin1, end1, begin2, end2, mcstl::equal_to<typename iterator_traits<RandomAccessIterator1>::value_type, typename iterator_traits<RandomAccessIterator2>::value_type>());
	else
		return search(begin1, end1, begin2, end2, mcstl::sequential_tag());
}

// sequential fallback for input iterator case
template<typename ForwardIterator1, typename ForwardIterator2, typename IteratorTag1, typename IteratorTag2>
inline
ForwardIterator1
search_switch(ForwardIterator1 begin1, ForwardIterator1 end1,
	ForwardIterator2 begin2, ForwardIterator2 end2, IteratorTag1, IteratorTag2)
{
	return search(begin1, end1, begin2, end2, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator1, typename ForwardIterator2>
inline
ForwardIterator1
search(ForwardIterator1 begin1, ForwardIterator1 end1,
	ForwardIterator2 begin2, ForwardIterator2 end2)
{
	return search_switch(begin1, end1, begin2, end2, typename std::iterator_traits<ForwardIterator1>::iterator_category(), typename std::iterator_traits<ForwardIterator2>::iterator_category());
}

// public interface
template<typename ForwardIterator1, typename ForwardIterator2,
	typename BinaryPredicate>
inline
ForwardIterator1
search(ForwardIterator1 begin1, ForwardIterator1 end1,
	ForwardIterator2 begin2, ForwardIterator2 end2,
	BinaryPredicate pred, mcstl::sequential_tag)
{
	return __mcstl_sequential_search(begin1, end1, begin2, end2, pred);
}

// parallel algorithm for random access iterator
template<typename RandomAccessIterator1, typename RandomAccessIterator2,
	typename BinaryPredicate>
RandomAccessIterator1
search_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1,
	RandomAccessIterator2 begin2, RandomAccessIterator2 end2, BinaryPredicate  pred, random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(true))
		return mcstl::search_template(begin1, end1, begin2, end2, pred);
	else
		return std::search(begin1, end1, begin2, end2, pred, mcstl::sequential_tag());
}

// sequential fallback for input iterator case
template<typename ForwardIterator1, typename ForwardIterator2,
	typename BinaryPredicate, typename IteratorTag1, typename IteratorTag2>
inline
ForwardIterator1
search_switch(ForwardIterator1 begin1, ForwardIterator1 end1,
	ForwardIterator2 begin2, ForwardIterator2 end2, BinaryPredicate pred, IteratorTag1, IteratorTag2)
{
	return search(begin1, end1, begin2, end2, pred, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator1, typename ForwardIterator2,
	typename BinaryPredicate>
inline
ForwardIterator1
search(ForwardIterator1 begin1, ForwardIterator1 end1,
	ForwardIterator2 begin2, ForwardIterator2 end2,
	BinaryPredicate  pred)
{
	return search_switch(begin1, end1, begin2, end2, pred, typename std::iterator_traits<ForwardIterator1>::iterator_category(), typename std::iterator_traits<ForwardIterator2>::iterator_category());
}


////  search_n  ////////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator, typename Integer, typename T>
inline
ForwardIterator
search_n(ForwardIterator begin, ForwardIterator end,
	Integer count, const T& val, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_search_n(begin, end, count, val);
}

// sequential fallback
template<typename ForwardIterator, typename Integer, typename T,
	typename BinaryPredicate>
inline
ForwardIterator
search_n(ForwardIterator begin, 
	ForwardIterator end,
	Integer count,
	const T& val,
	BinaryPredicate binary_pred, 
	mcstl::sequential_tag)
{
	return std::__mcstl_sequential_search_n(begin, end, count, val, binary_pred);
}

// public interface
template<typename ForwardIterator, typename Integer, typename T>
inline
ForwardIterator
search_n(ForwardIterator begin, ForwardIterator end,
	Integer count, const T& val)
{
	typedef typename iterator_traits<ForwardIterator>::value_type ValueType;

	return search_n(begin, end, count, val, mcstl::equal_to<ValueType, T>());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Integer, typename T, typename BinaryPredicate>
RandomAccessIterator
search_n_switch(RandomAccessIterator begin, RandomAccessIterator end,
	Integer count, const T& val, BinaryPredicate binary_pred, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(true))
	{
		mcstl::pseudo_sequence<T, Integer> ps(val, count);
		return mcstl::search_template(begin, end, ps.begin(), ps.end(), binary_pred);
	}
	else
		return std::__search_n(begin, end, count, val, binary_pred, random_access_iterator_tag());
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename Integer, typename T, typename BinaryPredicate, typename IteratorTag>
inline
ForwardIterator
search_n_switch(ForwardIterator begin, ForwardIterator end,
	Integer count, const T& val, BinaryPredicate binary_pred, IteratorTag)
{
	return __search_n(begin, end, count, val, binary_pred, IteratorTag());
}

// public interface
template<typename ForwardIterator, typename Integer, typename T, typename BinaryPredicate>
inline
ForwardIterator
search_n(ForwardIterator begin, ForwardIterator end,
	Integer count, const T& val, BinaryPredicate binary_pred)
{
	return search_n_switch(begin, end, count, val, binary_pred, typename std::iterator_traits<ForwardIterator>::iterator_category());
}


////  transform  ///////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator, typename OutputIterator,
	typename UnaryOperation>
inline
OutputIterator
transform(InputIterator begin, InputIterator end,
	OutputIterator result, UnaryOperation unary_op, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_transform(begin, end, result, unary_op);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2, typename OutputIterator,
	typename BinaryOperation>
inline
OutputIterator
transform(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
	OutputIterator result, BinaryOperation binary_op, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_transform(begin1, end1, begin2, result, binary_op);
}

// parallel unary transform for random access iterators
template<typename RandomAccessIterator1, typename RandomAccessIterator3, typename UnaryOperation>
RandomAccessIterator3
transform1_switch(RandomAccessIterator1 begin, RandomAccessIterator1 end, RandomAccessIterator3 result, UnaryOperation unary_op, random_access_iterator_tag, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::transform_minimal_n && parallelism_tag.is_parallel()))
	{
		bool dummy = true;
		typedef mcstl::iterator_pair<RandomAccessIterator1, RandomAccessIterator3, random_access_iterator_tag> ip;
		ip begin_pair(begin, result), end_pair(end, result + (end - begin));
		mcstl::transform1_selector<ip> functionality;
		mcstl::for_each_template_random_access(begin_pair, end_pair, unary_op, functionality, mcstl::dummy_reduct(), dummy, dummy, -1, parallelism_tag);
		return functionality.finish_iterator;
	}
	else
		return std::transform(begin, end, result, unary_op, mcstl::sequential_tag());
}

// sequential fallback for input iterator case
template<typename RandomAccessIterator1, typename RandomAccessIterator3, typename UnaryOperation, typename IteratorTag1, typename IteratorTag2>
inline
RandomAccessIterator3
transform1_switch(RandomAccessIterator1 begin, RandomAccessIterator1 end, RandomAccessIterator3 result, UnaryOperation unary_op, IteratorTag1, IteratorTag2, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return __mcstl_sequential_transform(begin, end, result, unary_op);
}


// parallel binary transform for random access iterators
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename RandomAccessIterator3, typename BinaryOperation>
RandomAccessIterator3
transform2_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1, RandomAccessIterator2 begin2, RandomAccessIterator3 result, BinaryOperation binary_op, random_access_iterator_tag, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	if(MCSTL_PARALLEL_CONDITION((end1 - begin1) >= mcstl::SETTINGS::transform_minimal_n && parallelism_tag.is_parallel()))
	{
		bool dummy = true;
		typedef mcstl::iterator_triple<RandomAccessIterator1, RandomAccessIterator2, RandomAccessIterator3, random_access_iterator_tag> ip;
		ip begin_triple(begin1, begin2, result), end_triple(end1, begin2 + (end1 - begin1), result + (end1 - begin1));
		mcstl::transform2_selector<ip> functionality;
		mcstl::for_each_template_random_access(begin_triple, end_triple, binary_op, functionality, mcstl::dummy_reduct(), dummy, dummy, -1, parallelism_tag);
		return functionality.finish_iterator;
	}
	else
		return std::transform(begin1, end1, begin2, result, binary_op, mcstl::sequential_tag());
}

// sequential fallback for input iterator case
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename RandomAccessIterator3, typename BinaryOperation, typename tag1, typename tag2, typename tag3>
inline
RandomAccessIterator3
transform2_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1, RandomAccessIterator2 begin2, RandomAccessIterator3 result, BinaryOperation binary_op, tag1, tag2, tag3, mcstl::PARALLELISM)
{
	return __mcstl_sequential_transform(begin1, end1, begin2, result, binary_op);
}

//public interface
template<typename InputIterator, typename OutputIterator,
	typename UnaryOperation>
inline
OutputIterator
transform(InputIterator begin, InputIterator end, OutputIterator result,
	UnaryOperation unary_op, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return transform1_switch(begin, end, result, unary_op,
	typename std::iterator_traits<InputIterator>::iterator_category(),
	typename std::iterator_traits<OutputIterator>::iterator_category(), parallelism_tag);
}

//public interface
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename BinaryOperation>
inline
OutputIterator
transform(InputIterator1 begin1, InputIterator1 end1,
	InputIterator2 begin2, OutputIterator result,
	BinaryOperation binary_op, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return transform2_switch(begin1, end1, begin2, result, binary_op,
	typename std::iterator_traits<InputIterator1>::iterator_category(), typename std::iterator_traits<InputIterator2>::iterator_category(), typename std::iterator_traits<OutputIterator>::iterator_category(), parallelism_tag);
}


////  replace  /////////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator, typename T>
inline
void
replace(ForwardIterator begin, ForwardIterator end,
	const T& old_value, const T& new_value, mcstl::sequential_tag)
{
	std::__mcstl_sequential_replace(begin, end, old_value, new_value);
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename T, typename IteratorTag>
void
replace_switch(ForwardIterator begin, ForwardIterator end, const T& old_value, const T& new_value, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	std::replace(begin, end, old_value, new_value, mcstl::sequential_tag());
}

// parallel replace for random access iterators
template<typename RandomAccessIterator, typename T>
void
replace_switch(RandomAccessIterator begin, RandomAccessIterator end, const T& old_value, const T& new_value, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	std::replace(begin, end, old_value, new_value, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename T>
inline
void
replace(ForwardIterator begin, ForwardIterator end, const T& old_value, const T& new_value, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	replace_switch(begin, end, old_value, new_value, typename std::iterator_traits<ForwardIterator>::iterator_category(), parallelism_tag);
}


////  replace_if  //////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator, typename Predicate, typename T>
inline
void
replace_if(ForwardIterator begin, ForwardIterator end,
	Predicate pred, const T& new_value, mcstl::sequential_tag)
{
	std::__mcstl_sequential_replace_if(begin, end, pred, new_value);
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename Predicate, typename T, typename IteratorTag>
void
replace_if_switch(ForwardIterator begin, ForwardIterator end, Predicate pred, const T& new_value, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	std::replace_if(begin, end, pred, new_value, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Predicate, typename T>
void
replace_if_switch(RandomAccessIterator begin, RandomAccessIterator end, Predicate pred, const T& new_value, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::replace_minimal_n && parallelism_tag.is_parallel()))
	{
		bool dummy;
		mcstl::replace_if_selector<RandomAccessIterator, Predicate, T> functionality(new_value);
		mcstl::for_each_template_random_access(begin, end, pred, functionality, mcstl::dummy_reduct(), true, dummy, -1, parallelism_tag);
	}
	else
		std::replace_if(begin, end, pred, new_value, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename Predicate, typename T>
inline
void
replace_if(ForwardIterator begin, ForwardIterator end,
Predicate pred, const T& new_value, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	replace_if_switch(begin, end, pred, new_value, typename std::iterator_traits<ForwardIterator>::iterator_category(), parallelism_tag);
}


////  generate  ////////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator, typename Generator>
inline
void
generate(ForwardIterator begin, ForwardIterator end, Generator gen, mcstl::sequential_tag)
{
	std::__mcstl_sequential_generate<ForwardIterator, Generator>(begin, end, gen);
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename Generator, typename IteratorTag>
void
generate_switch(ForwardIterator begin, ForwardIterator end,
	Generator gen, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	std::generate(begin, end, gen, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Generator>
void
generate_switch(RandomAccessIterator begin, RandomAccessIterator end,
	Generator gen, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::generate_minimal_n && parallelism_tag.is_parallel()))
	{
		bool dummy;
		mcstl::generate_selector<RandomAccessIterator> functionality;
		mcstl::for_each_template_random_access(begin, end, gen, functionality, mcstl::dummy_reduct(), true, dummy, -1, parallelism_tag);
	}
	else
		std::generate(begin, end, gen, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename Generator>
inline
void
generate(ForwardIterator begin, ForwardIterator end,
	Generator gen, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	generate_switch(begin, end, gen, typename std::iterator_traits<ForwardIterator>::iterator_category(), parallelism_tag);
}


////  generate_n  //////////////////////////////////////////////////////////

// sequential fallback
template<typename OutputIterator, typename Size, typename Generator>
inline
OutputIterator
generate_n(OutputIterator begin, Size n, Generator gen, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_generate_n(begin, n, gen);
}

// sequential fallback for input iterator case
template<typename OutputIterator, typename Size, typename Generator, typename IteratorTag>
OutputIterator
generate_n_switch(OutputIterator begin, Size n, Generator gen, IteratorTag, mcstl::PARALLELISM)
{
	return std::generate_n(begin, n, gen, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Size, typename Generator>
RandomAccessIterator
generate_n_switch(RandomAccessIterator begin, Size n, Generator gen, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	return std::generate_n(begin, n, gen, mcstl::sequential_tag());
}

// public interface
template<typename OutputIterator, typename Size, typename Generator>
OutputIterator
inline
generate_n(OutputIterator begin, Size n, Generator gen, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return generate_n_switch(begin, n, gen, typename std::iterator_traits<OutputIterator>::iterator_category(), parallelism_tag);
}


////  random_shuffle  //////////////////////////////////////////////////////

// sequential fallback
template<typename RandomAccessIterator>
inline
void
random_shuffle(RandomAccessIterator begin, RandomAccessIterator end, mcstl::sequential_tag)
{
	std::__mcstl_sequential_random_shuffle(begin, end);
}

// sequential fallback
template<typename RandomAccessIterator, typename RandomNumberGenerator>
inline
void
random_shuffle(RandomAccessIterator begin, RandomAccessIterator end,
		RandomNumberGenerator& rand, mcstl::sequential_tag)
{
	std::__mcstl_sequential_random_shuffle(begin, end, rand);
}


/** @brief Functor wrapper for std::rand(). */
template<typename must_be_int = int>
struct c_rand_number
{
	inline int operator()(int limit)
	{
		return rand() % limit;
	}
};

// fill in random number generator
template<typename RandomAccessIterator>
inline void
random_shuffle(RandomAccessIterator begin, RandomAccessIterator end)
{
	c_rand_number<> r;
	random_shuffle(begin, end, r);	//parallelization still possible
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename RandomNumberGenerator>
void
random_shuffle(RandomAccessIterator begin, RandomAccessIterator end,
		RandomNumberGenerator& rand)
{
	if(begin == end)
		return;
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::random_shuffle_minimal_n))
		mcstl::parallel_random_shuffle(begin, end, rand);
	else
		mcstl::sequential_random_shuffle(begin, end, rand);
}


////  partition  ///////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator, typename Predicate>
inline
ForwardIterator
partition(ForwardIterator begin, ForwardIterator end,
	Predicate pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_partition(begin, end, pred);
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename Predicate, typename IteratorTag>
inline
ForwardIterator
partition_switch(ForwardIterator begin, ForwardIterator end,
		Predicate pred,
		IteratorTag)
{
	return std::partition(begin, end, pred, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Predicate>
RandomAccessIterator
partition_switch(RandomAccessIterator begin, RandomAccessIterator end,
		Predicate pred,
		random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::partition_minimal_n))
	{
		typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
		DiffType middle = mcstl::parallel_partition(begin, end, pred, mcstl::SETTINGS::num_threads);
		return begin + middle;
	}
	else
		return partition(begin, end, pred, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename Predicate>
inline ForwardIterator
partition(ForwardIterator begin, ForwardIterator end,
	Predicate pred)
{
	return std::partition_switch(begin, end, pred, typename std::iterator_traits<ForwardIterator>::iterator_category());
}


////  sort  ////////////////////////////////////////////////////////////////

// sequential fallback
template<typename RandomAccessIterator>
inline
void
sort(RandomAccessIterator begin, RandomAccessIterator end,
	mcstl::sequential_tag)
{
	std::__mcstl_sequential_sort<RandomAccessIterator>(begin, end);
}

// sequential fallback
template<typename RandomAccessIterator, typename Comparator>
inline
void
sort(RandomAccessIterator begin, RandomAccessIterator end,
	Comparator comp,
	mcstl::sequential_tag)
{
	std::__mcstl_sequential_sort<RandomAccessIterator, Comparator>(begin, end, comp);
}

// public interface, insert default comparator
template<typename RandomAccessIterator>
inline void
sort(RandomAccessIterator begin, RandomAccessIterator end)
{
	sort(begin, end, std::less<typename iterator_traits<RandomAccessIterator>::value_type>());
}

template<typename RandomAccessIterator, typename Comparator>
void
sort(RandomAccessIterator begin, RandomAccessIterator end,
	Comparator comp)
{
	typedef typename iterator_traits<RandomAccessIterator>::value_type ValueType;

	if (begin != end)
	{
		if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::sort_minimal_n))
			mcstl::parallel_sort(begin, end, comp, false);
		else
			std::sort<RandomAccessIterator, Comparator>(begin, end, comp, mcstl::sequential_tag());
	}
}


////  stable_sort  /////////////////////////////////////////////////////////

// sequential fallback
template<typename RandomAccessIterator>
inline
void
stable_sort(RandomAccessIterator begin, RandomAccessIterator end,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_stable_sort<RandomAccessIterator>(begin, end);
}

// sequential fallback
template<typename RandomAccessIterator, typename Comparator>
inline
void
stable_sort(RandomAccessIterator begin, RandomAccessIterator end,
		Comparator comp,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_stable_sort<RandomAccessIterator, Comparator>(begin, end, comp);
}

template<typename RandomAccessIterator>
void
stable_sort(RandomAccessIterator begin, RandomAccessIterator end)
{
	stable_sort(begin, end, std::less<typename iterator_traits<RandomAccessIterator>::value_type>());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Comparator>
void
stable_sort(RandomAccessIterator begin, RandomAccessIterator end,
		Comparator comp)
{
	typedef typename iterator_traits<RandomAccessIterator>::value_type ValueType;
	typedef typename iterator_traits<RandomAccessIterator>::difference_type DiffType;

	if (begin != end)
	{
		if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::sort_minimal_n))
			mcstl::parallel_sort(begin, end, comp, true);
		else
			std::stable_sort<RandomAccessIterator, Comparator>(begin, end, comp, mcstl::sequential_tag());
	}
}


////  merge  ///////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator>
inline
OutputIterator
merge(InputIterator1 begin1, InputIterator1 end1,
	InputIterator2 begin2, InputIterator2 end2,
	OutputIterator result,
	mcstl::sequential_tag)
{
	return std::__mcstl_sequential_merge(begin1, end1, begin2, end2, result);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename Comparator>
inline
OutputIterator
merge(InputIterator1 begin1, InputIterator1 end1,
	InputIterator2 begin2, InputIterator2 end2,
	OutputIterator result, Comparator comp,
	mcstl::sequential_tag)
{
	return std::__mcstl_sequential_merge(begin1, end1, begin2, end2, result, comp);
}

// sequential fallback for input iterator case
template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Comparator, typename IteratorTag1, typename IteratorTag2, typename IteratorTag3>
inline OutputIterator
merge_switch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, OutputIterator result, Comparator comp, IteratorTag1, IteratorTag2, IteratorTag3)
{
	return __mcstl_sequential_merge(begin1, end1, begin2, end2, result, comp);
}

// parallel algorithm for random access iterators
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename Comparator>
OutputIterator
merge_switch(InputIterator1 begin1, InputIterator1 end1,
	InputIterator2 begin2, InputIterator2 end2,
	OutputIterator result, Comparator comp, random_access_iterator_tag, random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION((static_cast<mcstl::sequence_index_t>(end1 - begin1) >= mcstl::SETTINGS::merge_minimal_n || static_cast<mcstl::sequence_index_t>(end2 - begin2) >= mcstl::SETTINGS::merge_minimal_n)))
		return mcstl::parallel_merge_advance(begin1, end1, begin2, end2, result, (end1 - begin1) + (end2 - begin2), comp);
	else
		return mcstl::merge_advance(begin1, end1, begin2, end2, result, (end1 - begin1) + (end2 - begin2), comp);
}

// public interface
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator, typename Comparator>
inline
OutputIterator
merge(InputIterator1 begin1, InputIterator1 end1,
	InputIterator2 begin2, InputIterator2 end2,
	OutputIterator result, Comparator comp)
{
	typedef typename iterator_traits<InputIterator1>::value_type ValueType;

	return merge_switch(begin1, end1, begin2, end2, result, comp, typename std::iterator_traits<InputIterator1>::iterator_category(), typename std::iterator_traits<InputIterator2>::iterator_category(), typename std::iterator_traits<OutputIterator>::iterator_category());
}


// public interface, insert default comparator
template<typename InputIterator1, typename InputIterator2,
	typename OutputIterator>
inline
OutputIterator
merge(InputIterator1 begin1, InputIterator1 end1,
	InputIterator2 begin2, InputIterator2 end2,
	OutputIterator result)
{
	return merge(begin1, end1, begin2, end2, result, mcstl::less<typename iterator_traits<InputIterator1>::value_type, typename iterator_traits<InputIterator2>::value_type>());
}

////  nth_element  /////////////////////////////////////////////////////////

// sequential fallback
template<typename RandomAccessIterator>
inline
void
nth_element(	RandomAccessIterator begin,
		RandomAccessIterator nth,
		RandomAccessIterator end,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_nth_element(begin, nth, end);
}

// sequential fallback
template<typename RandomAccessIterator, typename Comparator>
void
nth_element(RandomAccessIterator begin,
		RandomAccessIterator nth,
		RandomAccessIterator end,
		Comparator comp,
		mcstl::sequential_tag)
{
	return std::__mcstl_sequential_nth_element(begin, nth, end, comp);
}

// public interface
template<typename RandomAccessIterator, typename Comparator>
inline
void
nth_element(	RandomAccessIterator begin,
		RandomAccessIterator nth,
		RandomAccessIterator end,
		Comparator comp)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::nth_element_minimal_n))
		mcstl::parallel_nth_element(begin, nth, end, comp);
	else
		std::nth_element(begin, nth, end, comp, mcstl::sequential_tag());
}

// public interface, insert default comparator
template<typename RandomAccessIterator>
void
nth_element(RandomAccessIterator begin,
		RandomAccessIterator nth,
		RandomAccessIterator end)
{
	nth_element(begin, nth, end, std::less<typename iterator_traits<RandomAccessIterator>::value_type>());
}


////  partial_sort  ////////////////////////////////////////////////////////

// sequential fallback
template<typename _RandomAccessIterator, typename _Compare>
void
partial_sort(_RandomAccessIterator begin,
		_RandomAccessIterator middle,
		_RandomAccessIterator end,
		_Compare comp,
		mcstl::sequential_tag)
{
	std::__mcstl_sequential_partial_sort(begin, middle, end, comp);
}

// sequential fallback
template<typename _RandomAccessIterator>
void
partial_sort(_RandomAccessIterator begin,
		_RandomAccessIterator middle,
		_RandomAccessIterator end,
		mcstl::sequential_tag)
{
	std::__mcstl_sequential_partial_sort(begin, middle, end);
}

// public interface, parallel algorithm for random access iterators
template<typename _RandomAccessIterator, typename _Compare>
void
partial_sort(_RandomAccessIterator begin,
		_RandomAccessIterator middle,
		_RandomAccessIterator end,
		_Compare comp)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::partial_sort_minimal_n))
		mcstl::parallel_partial_sort(begin, middle, end, comp);
	else
		std::partial_sort(begin, middle, end, comp, mcstl::sequential_tag());
}

// public interface, insert default comparator
template<typename _RandomAccessIterator>
void
partial_sort(_RandomAccessIterator begin,
		_RandomAccessIterator middle,
		_RandomAccessIterator end)
{
	partial_sort(begin, middle, end, std::less<typename iterator_traits<_RandomAccessIterator>::value_type>());
}


////  max_element  /////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator>
inline
ForwardIterator
max_element(ForwardIterator begin, ForwardIterator end, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_max_element(begin, end);
}

// sequential fallback
template<typename ForwardIterator, typename Comparator>
inline
ForwardIterator
max_element(ForwardIterator begin, ForwardIterator end,
		Comparator comp, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_max_element(begin, end, comp);
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename Comparator, typename IteratorTag>
ForwardIterator
max_element_switch(ForwardIterator begin, ForwardIterator end, Comparator comp, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	return max_element(begin, end, comp, mcstl::sequential_tag());
}

// public interface, insert default comparator
template<typename ForwardIterator>
inline
ForwardIterator
max_element(ForwardIterator begin, ForwardIterator end, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return max_element(begin, end, std::less<typename std::iterator_traits<ForwardIterator>::value_type>(), parallelism_tag);
}

template<typename RandomAccessIterator, typename Comparator>
RandomAccessIterator
max_element_switch(RandomAccessIterator begin, RandomAccessIterator end, Comparator comp, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::max_element_minimal_n && parallelism_tag.is_parallel()))
	{
		RandomAccessIterator res(begin);
		mcstl::identity_selector<RandomAccessIterator> functionality;
		mcstl::for_each_template_random_access(begin, end, mcstl::nothing(), functionality, mcstl::max_element_reduct<Comparator, RandomAccessIterator>(comp), res, res, -1, parallelism_tag);
		return res;
	}
	else
		return std::max_element(begin, end, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename Comparator>
inline
ForwardIterator
max_element(ForwardIterator begin, ForwardIterator end, Comparator comp, mcstl::PARALLELISM
parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return max_element_switch(begin, end, comp, typename std::iterator_traits<ForwardIterator>::iterator_category(), parallelism_tag);
}


////  min_element  /////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator>
inline
ForwardIterator
min_element(ForwardIterator begin, ForwardIterator end, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_min_element(begin, end);
}

// sequential fallback
template<typename ForwardIterator, typename Comparator>
inline
ForwardIterator
min_element(ForwardIterator begin, ForwardIterator end,
		Comparator comp, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_min_element(begin, end, comp);
}

// public interface
template<typename ForwardIterator>
inline
ForwardIterator
min_element(ForwardIterator begin, ForwardIterator end, mcstl::PARALLELISM parallelism_tag  = mcstl::SETTINGS::parallel_balanced)
{
	return min_element(begin, end, std::less<typename std::iterator_traits<ForwardIterator>::value_type>(), parallelism_tag);
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename Comparator, typename IteratorTag>
ForwardIterator
min_element_switch(ForwardIterator begin, ForwardIterator end, Comparator comp, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	return std::min_element(begin, end, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Comparator>
RandomAccessIterator
min_element_switch(RandomAccessIterator begin, RandomAccessIterator end, Comparator comp, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::min_element_minimal_n && parallelism_tag.is_parallel()))
	{
		RandomAccessIterator res(begin);
		mcstl::identity_selector<RandomAccessIterator> functionality;
		mcstl::for_each_template_random_access(begin, end, mcstl::nothing(), functionality, mcstl::min_element_reduct<Comparator, RandomAccessIterator>(comp), res, res, -1, parallelism_tag);
		return res;
	}
	else
		return std::min_element(begin, end, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename Comparator>
inline
ForwardIterator
min_element(ForwardIterator begin, ForwardIterator end, Comparator comp, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return min_element_switch(begin, end, comp, typename std::iterator_traits<ForwardIterator>::iterator_category(), parallelism_tag);
}

} /* namespace std */

#endif /* _MCSTL_ALGORITHM_H */
