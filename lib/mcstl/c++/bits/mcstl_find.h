/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_find.h
 *  @brief Parallel implementation base for std::find(), std::equal() and related functions. */

#ifndef _MCSTL_FIND_H
#define _MCSTL_FIND_H 1

#include <mod_stl/stl_algobase.h>

#include <bits/mcstl_features.h>
#include <mcstl.h>
#include <bits/mcstl_compatibility.h>
#include <bits/mcstl_equally_split.h>

namespace mcstl
{

/** @brief Parallel std::find, switch for different algorithms.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence. Must have same length as first sequence.
 *  @param pred Find predicate.
 *  @param selector Functionality (e. g. std::find_if(), std::equal(),...)
 *  @return Place of finding in both sequences. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred, typename Selector> 
std::pair<RandomAccessIterator1, RandomAccessIterator2> find_template(
		RandomAccessIterator1 begin1,
		RandomAccessIterator1 end1,
		RandomAccessIterator2 begin2,
		Pred pred,
		Selector selector
		)
{
	switch(SETTINGS::find_distribution)
	{
		case Settings<>::GROWING_BLOCKS:
			return find_template(begin1, end1, begin2, pred, selector, growing_blocks_tag());
		case Settings<>::CONSTANT_SIZE_BLOCKS:
			return find_template(begin1, end1, begin2, pred, selector, constant_size_blocks_tag());
		case Settings<>::EQUAL_SPLIT:
			return find_template(begin1, end1, begin2, pred, selector, equal_split_tag());
		default:
			assert(false);
			return std::make_pair(begin1, begin2);
	}
}
	
#if MCSTL_FIND_EQUAL_SPLIT

/** @brief Parallel std::find, equal splitting variant.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence. Second sequence must have same length as first sequence.
 *  @param pred Find predicate.
 *  @param selector Functionality (e. g. std::find_if(), std::equal(),...)
 *  @return Place of finding in both sequences. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred, typename Selector> 
std::pair<RandomAccessIterator1, RandomAccessIterator2> find_template(
		RandomAccessIterator1 begin1,
		RandomAccessIterator1 end1,
		RandomAccessIterator2 begin2,
		Pred pred,
		Selector selector,
		equal_split_tag
		)
{
	MCSTL_CALL(end1 - begin1)

	typedef typename std::iterator_traits<RandomAccessIterator1>::difference_type DiffType;
	typedef typename std::iterator_traits<RandomAccessIterator1>::value_type ValueType;

	DiffType length = end1 - begin1;

	DiffType result = length;

	thread_index_t num_threads = SETTINGS::num_threads;
	
	DiffType borders[num_threads + 1];
	equally_split(length, num_threads, borders);

	#pragma omp parallel shared(result) num_threads(num_threads)
	{
		int iam = omp_get_thread_num();
		DiffType pos = borders[iam], limit = borders[iam + 1];
		
		RandomAccessIterator1 i1 = begin1 + pos;
		RandomAccessIterator2 i2 = begin2 + pos;
		for(; pos < limit; pos++)
		{
			#pragma omp flush(result)
			if(result < pos)	//result has been set to something lower
				break;

			if(selector(i1, i2, pred))
			{
				#pragma omp critical (result)
				if(result > pos)
					result = pos;
				break;
			}
			i1++;
			i2++;
		}
	}	//end parallel
	return std::pair<RandomAccessIterator1, RandomAccessIterator2>(begin1 + result, begin2 + result);
} // end find_template

#endif

#if MCSTL_FIND_GROWING_BLOCKS

/** @brief Parallel std::find, growing block size variant.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence. Second sequence must have same length as first sequence.
 *  @param pred Find predicate.
 *  @param selector Functionality (e. g. std::find_if(), std::equal(),...)
 *  @return Place of finding in both sequences.
 *  @see mcstl::Settings::find_sequential_search_size
 *  @see mcstl::Settings::find_initial_block_size
 *  @see mcstl::Settings::find_maximum_block_size
 *  @see mcstl::Settings::find_increasing_factor
 *
 *  There are two main differences between the growing blocks and the constant-size blocks variants.
 *  1. For GB, the block size grows; for CSB, the block size is fixed.
 *  2. For GB, the blocks are allocated dynamically;
 *     for CSB, the blocks are allocated in a predetermined manner, namely spacial round-robin. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred, typename Selector> 
std::pair<RandomAccessIterator1, RandomAccessIterator2> 
find_template(
		RandomAccessIterator1 begin1,
		RandomAccessIterator1 end1,
		RandomAccessIterator2 begin2,
		Pred pred,
		Selector selector,
		growing_blocks_tag
		)
{
	MCSTL_CALL(end1 - begin1)

	typedef typename std::iterator_traits<RandomAccessIterator1>::difference_type DiffType;
	typedef typename std::iterator_traits<RandomAccessIterator1>::value_type ValueType;

	DiffType length = end1 - begin1;
	
	DiffType sequential_search_size = std::min<DiffType>(length, SETTINGS::find_sequential_search_size);

	//try it sequentially first
	std::pair<RandomAccessIterator1, RandomAccessIterator2> find_seq_result =
		selector.sequential_algorithm(begin1, begin1 + sequential_search_size, begin2, pred);
		
	if(find_seq_result.first != (begin1 + sequential_search_size))
		return find_seq_result;

	DiffType next_block_pos = sequential_search_size;	// index of beginning of next free block (after sequential find)
	DiffType result = length;
	thread_index_t num_threads = SETTINGS::num_threads;
	
	#pragma omp parallel shared(result) num_threads(num_threads)		// not within first k elements -> start parallel
	{			
		thread_index_t iam = omp_get_thread_num();
		
		DiffType block_size = SETTINGS::find_initial_block_size;
		DiffType start = fetch_and_add<DiffType>(&next_block_pos, block_size);	// get new block, update pointer to next block
		DiffType stop = std::min<DiffType>(length, start + block_size);
		
		std::pair<RandomAccessIterator1, RandomAccessIterator2> local_result;
		
		while(start < length)
		{
			#pragma omp flush(result)	// get new value of result
			if(result < start)		// no chance to find first element
				break;

			local_result = selector.sequential_algorithm(begin1 + start, begin1 + stop, begin2 + start, pred);
			if(local_result.first != (begin1 + stop))
			{
				#pragma omp critical(result)
				if((local_result.first - begin1) < result)
				{
					result = local_result.first - begin1;
					fetch_and_add<DiffType>(&next_block_pos, length);	// result cannot be in future blocks, stop algorithm
				}
			}
				
			block_size = std::min<DiffType>(block_size * SETTINGS::find_increasing_factor, SETTINGS::find_maximum_block_size);
			start = fetch_and_add<DiffType>(&next_block_pos, block_size);	// get new block, update pointer to next block
			stop = (length < (start + block_size)) ? length : (start + block_size);
		}
	} // end parallel
	return std::pair<RandomAccessIterator1, RandomAccessIterator2>(begin1 + result, begin2 + result);		// return iterator on found element
} // end find_template

#endif

#if MCSTL_FIND_CONSTANT_SIZE_BLOCKS

/** @brief Parallel std::find, constant block size variant.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence. Second sequence must have same length as first sequence.
 *  @param pred Find predicate.
 *  @param selector Functionality (e. g. std::find_if(), std::equal(),...)
 *  @return Place of finding in both sequences.
 *  @see mcstl::Settings::find_sequential_search_size
 *  @see mcstl::Settings::find_block_size
 *  There are two main differences between the growing blocks and the constant-size blocks variants.
 *  1. For GB, the block size grows; for CSB, the block size is fixed.
 *  2. For GB, the blocks are allocated dynamically;
 *     for CSB, the blocks are allocated in a predetermined manner, namely spacial round-robin. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred, typename Selector> 
std::pair<RandomAccessIterator1, RandomAccessIterator2> find_template(
		RandomAccessIterator1 begin1,
		RandomAccessIterator1 end1,
		RandomAccessIterator2 begin2,
		Pred pred,
		Selector selector,
		constant_size_blocks_tag
		)
{		
	MCSTL_CALL(end1 - begin1)

	typedef typename std::iterator_traits<RandomAccessIterator1>::difference_type DiffType;
	typedef typename std::iterator_traits<RandomAccessIterator1>::value_type ValueType;		// can this be done better? (not iterator dependent)

	DiffType length = end1 - begin1;
	
	DiffType sequential_search_size = std::min<DiffType>(length, SETTINGS::find_sequential_search_size);

	//try it sequentially first
	std::pair<RandomAccessIterator1, RandomAccessIterator2> find_seq_result =
		selector.sequential_algorithm(begin1, begin1 + sequential_search_size, begin2, pred);
		
	if(find_seq_result.first != (begin1 + sequential_search_size))
		return find_seq_result;

	DiffType result = length;
	thread_index_t num_threads = SETTINGS::num_threads;
		
	#pragma omp parallel shared(result) num_threads(num_threads)	// not within first sequential_search_size elements -> start parallel
	{			
		thread_index_t iam = omp_get_thread_num();
		DiffType block_size = SETTINGS::find_initial_block_size;
		
		DiffType start, stop;
		DiffType iteration_start = sequential_search_size;	// first element of thread's current iteration
		
		start = iteration_start + iam * block_size;		// where to work (initialization)
		stop = std::min<DiffType>(length, start + block_size);
		
		std::pair<RandomAccessIterator1, RandomAccessIterator2> local_result;
		
		while(start < length)
		{
			#pragma omp flush(result)	// get new value of result
			if(result < start)		// no chance to find first element
				break;
			
			local_result = selector.sequential_algorithm(begin1 + start, begin1 + stop, begin2 + start, pred);
			if(local_result.first != (begin1 + stop))
			{
				#pragma omp critical(result)
				if((local_result.first - begin1) < result)
					result = local_result.first - begin1;
				break;		// will not find better value in its interval
			}	
			
			iteration_start += num_threads * block_size;
			
			start = iteration_start + iam * block_size;		// where to work
			stop = std::min<DiffType>(length, start + block_size);
		}
	} // end parallel
	return std::pair<RandomAccessIterator1, RandomAccessIterator2>(begin1 + result, begin2 + result);		// return iterator on found element
} // end find_template

#endif

} // end namespace

#endif

