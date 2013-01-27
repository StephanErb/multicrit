/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_partition.h
 *  @brief Parallel implementation of std::partition(), std::nth_element(), and std::partial_sort(). */

#ifndef _MCSTL_PARTITION_H
#define _MCSTL_PARTITION_H 1

#include <bits/mcstl_basic_iterator.h>
#include <bits/mcstl_sort.h>
#include <mod_stl/stl_algo.h>
#include <mcstl.h>

/** @brief Decide whether to declare certain variable volatile in this file. */
#define MCSTL_VOLATILE volatile

namespace std
{
//forward declaration
template<typename RandomAccessIterator, typename Comparator>
void
sort(RandomAccessIterator begin, RandomAccessIterator end, Comparator comp);
}

namespace mcstl
{

/** @brief Parallel implementation of std::partition.
 *  @param begin Begin iterator of input sequence to split.
 *  @param end End iterator of input sequence to split.
 *  @param pred Partition predicate, possibly including some kind of pivot.
 *  @param max_num_threads Maximum number of threads to use for this task.
 *  @return Number of elements not fulfilling the predicate. */
template<typename RandomAccessIterator, typename Predicate>
inline typename std::iterator_traits<RandomAccessIterator>::difference_type
parallel_partition(
	RandomAccessIterator begin,
	RandomAccessIterator end,
	Predicate pred,
	thread_index_t max_num_threads)
{	
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	DiffType n = end - begin;

	MCSTL_CALL(n)

	MCSTL_VOLATILE DiffType left = 0, right = n - 1;	//shared
	MCSTL_VOLATILE DiffType leftover_left, leftover_right, leftnew, rightnew;	//shared
	bool* reserved_left, * reserved_right;	//shared
	reserved_left = new bool[max_num_threads];
	reserved_right = new bool[max_num_threads];

	DiffType chunk_size;
	if(SETTINGS::partition_chunk_share > 0.0)
		chunk_size = std::max((DiffType)SETTINGS::partition_chunk_size, (DiffType)((double)n * SETTINGS::partition_chunk_share / (double)max_num_threads));
	else
		chunk_size = SETTINGS::partition_chunk_size;
	
	while(right - left + 1 >= 2 * max_num_threads * chunk_size)	//at least good for two processors
	{
		DiffType num_chunks = (right - left + 1) / chunk_size;
		thread_index_t num_threads = (int)std::min((DiffType)max_num_threads, num_chunks / 2);

		for(int r = 0; r < num_threads; r++)
		{
			reserved_left[r] = false;
			reserved_right[r] = false;
		}
		leftover_left = 0;
		leftover_right = 0;

		#pragma omp parallel num_threads(num_threads)
		{
			DiffType thread_left, thread_left_border, thread_right, thread_right_border;	//private
			thread_left = left + 1;
			thread_left_border = thread_left - 1;	//just to satify the condition below
			thread_right = n - 1;
			thread_right_border = thread_right + 1;	//just to satify the condition below
			
			bool iam_finished = false;
			while(!iam_finished)
			{
				if(thread_left > thread_left_border)
				#pragma omp critical
				{
					if(left + (chunk_size - 1) > right)
						iam_finished = true;
					else
					{
						thread_left = left;
						thread_left_border = left + (chunk_size - 1);
						left += chunk_size;
					}
				}
			
				if(thread_right < thread_right_border)
				#pragma omp critical
				{
					if(left > right - (chunk_size - 1))
						iam_finished = true;
					else
					{
						thread_right = right;
						thread_right_border = right - (chunk_size - 1);
						right -= chunk_size;
					}
				}
				
				if(iam_finished)
					break;
				
				while(thread_left < thread_right)	//swap as usual
				{
					while(pred(begin[thread_left]) && thread_left <= thread_left_border)
						thread_left++;
					while(!pred(begin[thread_right]) && thread_right >= thread_right_border)
						thread_right--;
					
					if(thread_left > thread_left_border || thread_right < thread_right_border)
						break;	//fetch new chunk(s)
					
					std::swap(begin[thread_left], begin[thread_right]);
					thread_left++;
					thread_right--;
				}
			}
			
			//now swap the leftover chunks to the right places
			
			if(thread_left <= thread_left_border)
			#pragma omp atomic
				leftover_left++;
			if(thread_right >= thread_right_border)
			#pragma omp atomic
				leftover_right++;
			
			#pragma omp barrier

			#pragma omp single
			{
				leftnew = left - leftover_left * chunk_size;
				rightnew = right + leftover_right * chunk_size;
			}

			#pragma omp barrier
			
			if(thread_left <= thread_left_border && thread_left_border >= leftnew)//<=> thread_left_border + (chunk_size - 1) >= leftnew
			{
				//chunk already in place, reserve spot
				reserved_left[(left - (thread_left_border + 1)) / chunk_size] = true;
			}
			if(thread_right >= thread_right_border && thread_right_border <= rightnew)//<=> thread_right_border - (chunk_size - 1) <= rightnew
			{
				//chunk already in place, reserve spot
				reserved_right[((thread_right_border - 1) - right) / chunk_size] = true;
			}

			#pragma omp barrier

			if(thread_left <= thread_left_border && thread_left_border < leftnew)
			{	//find spot and swap
				DiffType swapstart = -1;
				#pragma omp critical
				{
					for(int r = 0; r < leftover_left; r++)
						if(!reserved_left[r])
						{
							reserved_left[r] = true;
							swapstart = left - (r + 1) * chunk_size;
							break;
						}
				}

#if MCSTL_ASSERTIONS
				assert(swapstart != -1);
#endif
				
				std::swap_ranges(begin + thread_left_border - (chunk_size - 1), begin + thread_left_border + 1, begin + swapstart);
			}
		
			if(thread_right >= thread_right_border && thread_right_border > rightnew)
			{	//find spot and swap
				DiffType swapstart = -1;
				#pragma omp critical
				{
					for(int r = 0; r < leftover_right; r++)
						if(!reserved_right[r])
						{
							reserved_right[r] = true;
							swapstart = right + r * chunk_size + 1;
							break;
						}
				}
				
#if MCSTL_ASSERTIONS
				assert(swapstart != -1);
#endif

				std::swap_ranges(begin + thread_right_border, begin + thread_right_border + chunk_size, begin + swapstart);
			}
#if MCSTL_ASSERTIONS
			#pragma omp barrier

			#pragma omp single
			{
				for(int r = 0; r < leftover_left; r++)
					assert(reserved_left[r]);
				for(int r = 0; r < leftover_right; r++)
					assert(reserved_right[r]);
			}

			#pragma omp barrier
#endif
			
			#pragma omp barrier
			left = leftnew;
			right = rightnew;
		}	//parallel
	}	//"recursion"
	
	DiffType final_left = left, final_right = right;

	while(final_left < final_right)
	{
		while(pred(begin[final_left]) && final_left < final_right)
			final_left++;	//go right until key is geq than pivot
		while(!pred(begin[final_right]) && final_left < final_right)
			final_right--;//go left until key is less than pivot
		if(final_left == final_right)
			break;
		std::swap(begin[final_left], begin[final_right]);
		final_left++;
		final_right--;
	}
	
	//all elements on the left side are < piv, all elements on the right are >= piv
	
	delete[] reserved_left;
	delete[] reserved_right;
	
	//element "between" final_left and final_right might not have been regarded yet
	if(final_left < n && !pred(begin[final_left]))
		//really swapped
		return final_left;
	else
		return final_left + 1;
}

/** @brief Parallel implementation of std::nth_element().
 *  @param begin Begin iterator of input sequence.
 *  @param nth Iterator of element that must be in position afterwards.
 *  @param end End iterator of input sequence.
 *  @param comp Comparator. */
template<typename RandomAccessIterator, typename Comparator>
void parallel_nth_element(RandomAccessIterator begin, RandomAccessIterator nth, RandomAccessIterator end, Comparator comp)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
	
	MCSTL_CALL(end - begin)

	RandomAccessIterator split;
	ValueType pivot;
	random_number rng;
	
	DiffType minimum_length = std::max<DiffType>(2, SETTINGS::partition_minimal_n);

	while(static_cast<sequence_index_t>(end - begin) >= minimum_length) 	// break if input range to small
	{
		DiffType n = end - begin;
		
		RandomAccessIterator pivot_pos = begin +  rng(n);
		
		if(pivot_pos != (end - 1))
			std::swap(*pivot_pos, *(end - 1));	//swap pivot_pos value to end
		pivot_pos = end - 1;

		std::binder2nd<Comparator> pred(comp, *pivot_pos);

		//divide, leave pivot unchanged in last place
		RandomAccessIterator split_pos1, split_pos2;
		split_pos1 = begin + parallel_partition(begin, end - 1, pred, SETTINGS::num_threads);
		//left side: < pivot_pos; right side: >= pivot_pos
		
		if(split_pos1 != pivot_pos)
			std::swap(*split_pos1, *pivot_pos);	//swap pivot back to middle
		pivot_pos = split_pos1;

		//in case all elements are equal, split_pos1 == 0
		if(	(split_pos1 + 1 - begin) < (n >> 7) ||
			(end - split_pos1) < (n >> 7))
		{	//very unequal split, one part smaller than one 128th
			//elements not stricly larger than the pivot
			std::unary_negate<std::binder1st<Comparator> > pred(std::binder1st<Comparator>(comp, *pivot_pos));
			//find other end of pivot-equal range
			split_pos2 = __mcstl_sequential_partition(split_pos1 + 1, end, pred);
		}
		else	//only skip the pivot
			split_pos2 = split_pos1 + 1;

		if (split_pos2 <= nth)	//compare iterators
			begin = split_pos2;
		else if(nth < split_pos1)
			end = split_pos1;
		else
			break;
	}
	std::__mcstl_sequential_sort(begin, end, comp);	//only at most SETTINGS::partition_minimal_n elements left
}

/** @brief Parallel implementation of std::partial_sort().
 *  @param begin Begin iterator of input sequence.
 *  @param middle Sort until this position.
 *  @param end End iterator of input sequence.
 *  @param comp Comparator. */
template<typename RandomAccessIterator, typename Comparator>
void parallel_partial_sort(RandomAccessIterator begin, RandomAccessIterator middle, RandomAccessIterator end, Comparator comp)
{
	parallel_nth_element(begin, middle, end, comp);
	std::sort(begin, middle, comp);
}
	
}	//namespace mcstl

#undef MCSTL_VOLATILE

#endif
