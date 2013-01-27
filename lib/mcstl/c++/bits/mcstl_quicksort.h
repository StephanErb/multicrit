/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_quicksort.h
 *  @brief Implementation of a unbalanced parallel quicksort.
 * 
 *  It works in-place. */

#ifndef _MCSTL_QUICKSORT_H
#define _MCSTL_QUICKSORT_H 1

#include <mcstl.h>
#include <bits/mcstl_partition.h>

namespace mcstl
{

/** @brief Unbalanced quicksort divide step.
 *  @param begin Begin iterator of subsequence.
 *  @param end End iterator of subsequence.
 *  @param comp Comparator.
 *  @param pivot_rank Desired rank of the pivot.
 *  @param num_samples Chosse pivot from that many samples.
 *  @param num_threads Number of threads that are allowed to work on this part. */
template<typename RandomAccessIterator, typename Comparator>
inline typename std::iterator_traits<RandomAccessIterator>::difference_type
parallel_sort_qs_divide(
	RandomAccessIterator begin,
	RandomAccessIterator end,
	Comparator comp,
	typename std::iterator_traits<RandomAccessIterator>::difference_type pivot_rank,
	typename std::iterator_traits<RandomAccessIterator>::difference_type num_samples,
	thread_index_t num_threads)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	DiffType n = end - begin;
	num_samples = std::min(num_samples, n);
	ValueType samples[num_samples];

	for(DiffType s = 0; s < num_samples; s++)
		samples[s] = begin[(unsigned long long)s * n / num_samples];

	std::__mcstl_sequential_sort(samples, samples + num_samples, comp);

	ValueType& pivot = samples[pivot_rank * num_samples / n];

	std::binder2nd<Comparator> pred(comp, pivot);
	DiffType split = parallel_partition(begin, end, pred, num_threads);

	return split;
}

/** @brief Unbalanced quicksort conquer step.
 *  @param begin Begin iterator of subsequence.
 *  @param end End iterator of subsequence.
 *  @param comp Comparator.
 *  @param num_threads Number of threads that are allowed to work on this part. */
template<typename RandomAccessIterator, typename Comparator>
inline void
parallel_sort_qs_conquer(
	RandomAccessIterator begin, 
	RandomAccessIterator end, 
	Comparator comp,
	int num_threads)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	if(num_threads <= 1)
	{
		std::__mcstl_sequential_sort(begin, end, comp);
		return;
	}

	DiffType n = end - begin, pivot_rank;
	
	if(n <= 1)
		return;
	
	thread_index_t num_processors_left;

	if((num_threads % 2) == 1)
		num_processors_left = num_threads / 2 + 1;
	else
		num_processors_left = num_threads / 2;

	pivot_rank = n * num_processors_left / num_threads;

	DiffType split = parallel_sort_qs_divide(begin, end, comp, pivot_rank, SETTINGS::sort_qs_num_samples_preset, num_threads);

#	pragma omp parallel sections
	{
#		pragma omp section
		parallel_sort_qs_conquer(begin, begin + split, comp, num_processors_left);
#		pragma omp section
		parallel_sort_qs_conquer(begin + split, end, comp, num_threads - num_processors_left);
	}
}



/** @brief Unbalanced quicksort main call.
 *  @param begin Begin iterator of input sequence.
 *  @param end End iterator input sequence, ignored.
 *  @param comp Comparator.
 *  @param n Length of input sequence.
 *  @param num_threads Number of threads that are allowed to work on this part. */
template<typename RandomAccessIterator, typename Comparator>
inline void
parallel_sort_qs(
		RandomAccessIterator begin,
		RandomAccessIterator end,
		Comparator comp,
		typename std::iterator_traits<RandomAccessIterator>::difference_type n,
		int num_threads)
{
	MCSTL_CALL(n)
	
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;

	if(n == 0)
		return;

	if(num_threads > n)
		num_threads = static_cast<thread_index_t>(n);	//at least one element per processor

	SETTINGS::sort_qs_num_samples_preset = 100;

	omp_set_num_threads(num_threads);	//hard to avoid
	bool old_nested = (omp_get_nested() != 0);
	omp_set_nested(true);

	parallel_sort_qs_conquer(begin, begin + n, comp, num_threads);

	omp_set_nested(old_nested);
}

}	//namespace mcstl

#endif
