/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_balanced_quicksort.h
 *  @brief Implementation of a dynamically load-balanced parallel quicksort.
 *
 *  It works in-place and needs only logarithmic extra memory. */

#ifndef _MCSTL_BAL_QUICKSORT_H
#define _MCSTL_BAL_QUICKSORT_H 1

#include <bits/mcstl_basic_iterator.h>
#include <mod_stl/stl_algo.h>

#include <bits/mcstl_settings.h>
#include <bits/mcstl_partition.h>
#include <bits/mcstl_random_number.h>
#include <bits/mcstl_queue.h>
#include <functional>

#if MCSTL_ASSERTIONS
#include <meta/mcstl_checkers.h>
#endif

namespace mcstl
{

/** @brief Information local to one thread in the parallel quicksort run. */
template<typename RandomAccessIterator>
struct QSBThreadLocal
{
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
	/** @brief Continuous part of the sequence, described by an iterator pair. */
	typedef std::pair<RandomAccessIterator, RandomAccessIterator> Piece;
	/** @brief Initial piece to work on. */
	Piece initial;
	/** @brief Work-stealing queue. */
	RestrictedBoundedConcurrentQueue<Piece> leftover_parts;
	/** @brief Number of threads involved in this algorithm. */
	thread_index_t num_threads;
	/** @brief Pointer to a counter of elements left over to sort. */
	volatile DiffType* elements_leftover;
	/** @brief The complete sequence to sort. */
	Piece global;

	/** @brief Constructor. 
	 *  @param queue_size Size of the work-stealing queue. */
	QSBThreadLocal(int queue_size) : leftover_parts(queue_size) { }
};

/** @brief Initialize the thread local storage. 
 *  @param tls Array of thread-local storages.
 *  @param queue_size Size of the work-stealing queue. */
template<typename RandomAccessIterator>
inline void qsb_initialize(
	QSBThreadLocal<RandomAccessIterator>** tls,
	int queue_size)
{
	int iam = omp_get_thread_num();

	tls[iam] = new QSBThreadLocal<RandomAccessIterator>(queue_size);
}


/** @brief Balanced quicksort divide step.
 *  @param begin Begin iterator of subsequence.
 *  @param end End iterator of subsequence.
 *  @param comp Comparator.
 *  @param num_threads Number of threads that are allowed to work on this part.
 *  @pre @c (end-begin)>=1 */
template<typename RandomAccessIterator, typename Comparator>
inline typename std::iterator_traits<RandomAccessIterator>::difference_type
qsb_divide(
	RandomAccessIterator begin,
	RandomAccessIterator end,
	Comparator comp,
	int num_threads)
{
	assert(num_threads > 0);

	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	RandomAccessIterator pivot_pos = median_of_three_iterators(begin, begin + (end - begin) / 2, end  - 1, comp);

#if defined(MCSTL_ASSERTIONS)
	DiffType n = end - begin;
	//must be in between somewhere
	assert(	(!comp(*pivot_pos, *begin          ) && !comp(*(begin + n / 2), *pivot_pos)) ||
		(!comp(*pivot_pos, *begin          ) && !comp(*end            , *pivot_pos)) ||
		(!comp(*pivot_pos, *(begin + n / 2)) && !comp(*begin          , *pivot_pos)) ||
		(!comp(*pivot_pos, *(begin + n / 2)) && !comp(*end            , *pivot_pos)) ||
		(!comp(*pivot_pos, *end            ) && !comp(*begin          , *pivot_pos)) ||
		(!comp(*pivot_pos, *end            ) && !comp(*(begin + n / 2), *pivot_pos)));
#endif

	if(pivot_pos != (end - 1))
		std::swap(*pivot_pos, *(end - 1));	//swap pivot value to end
	pivot_pos = end - 1;

	std::binder2nd<Comparator> pred(comp, *pivot_pos);

	//divide
	DiffType split_pos = parallel_partition(begin, end - 1, pred, num_threads);	//returning end - begin - 1 in the worst case

	std::swap(*(begin + split_pos), *pivot_pos);	//swap back pivot to middle
	pivot_pos = begin + split_pos;

#if MCSTL_ASSERTIONS
	RandomAccessIterator r;
	for(r = begin; r != pivot_pos; r++)
		assert(comp(*r, *pivot_pos));
	for(; r != end; r++)
		assert(!comp(*r, *pivot_pos));
#endif

	return split_pos;
}

/** @brief Quicksort conquer step.
 *  @param tls Array of thread-local storages.
 *  @param begin Begin iterator of subsequence.
 *  @param end End iterator of subsequence.
 *  @param comp Comparator.
 *  @param iam Number of the thread processing this function.
 *  @param num_threads Number of threads that are allowed to work on this part. */
template<typename RandomAccessIterator, typename Comparator>
inline void
qsb_conquer(
	QSBThreadLocal<RandomAccessIterator>** tls,
	RandomAccessIterator begin, 
	RandomAccessIterator end, 
	Comparator comp,
	thread_index_t iam,
	thread_index_t num_threads)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	DiffType n = end - begin;

	if(num_threads <= 1 || n < 2)
	{
		tls[iam]->initial.first  = begin;
		tls[iam]->initial.second = end;
		
		qsb_local_sort_with_helping(tls, comp, iam);
		
		return;
	}

	//divide step
	DiffType split_pos = qsb_divide(begin, end, comp, num_threads);	

#if MCSTL_ASSERTIONS
	assert(0 <= split_pos && split_pos < (end - begin));
#endif

	thread_index_t num_threads_leftside = std::max<thread_index_t>(1, std::min<thread_index_t>(num_threads - 1, split_pos * num_threads / n));

	#pragma omp atomic
	*tls[iam]->elements_leftover -= (DiffType)1;
		
	//conquer step
	#pragma omp parallel sections num_threads(2)
	{
		#pragma omp section
		qsb_conquer(tls, begin, begin + split_pos, comp, iam, num_threads_leftside);
		//the pivot_pos is left in place, to ensure termination
		#pragma omp section
		qsb_conquer(tls, begin + split_pos + 1, end, comp, iam + num_threads_leftside, num_threads - num_threads_leftside);
	}
}

/** @brief Quicksort step doing load-balanced local sort.
 *  @param tls Array of thread-local storages.
 *  @param comp Comparator.
 *  @param iam Number of the thread processing this function. */
template<typename RandomAccessIterator, typename Comparator>
inline void qsb_local_sort_with_helping(
	QSBThreadLocal<RandomAccessIterator>** tls, 
	Comparator& comp, 
	int iam)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
	typedef std::pair<RandomAccessIterator, RandomAccessIterator> Piece;

	QSBThreadLocal<RandomAccessIterator>& tl = *tls[iam];

	DiffType base_case_n = SETTINGS::sort_qsb_base_case_maximal_n;
	if(base_case_n < 2)
		base_case_n = 2;	//at least size 2
	thread_index_t num_threads = tl.num_threads;	//total number of threads
	random_number rng(iam + 1);	//every thread has its own random number generator

	Piece current = tl.initial;

	DiffType elements_done = 0;
#if MCSTL_ASSERTIONS
	DiffType total_elements_done = 0;
#endif

	for(;;)
	{	//invariant: current must be a valid (maybe empty) range
		RandomAccessIterator begin = current.first, end = current.second;
		
		DiffType n = end - begin;
		
		if(n > base_case_n)
		{
			//divide
			RandomAccessIterator pivot_pos = begin +  rng(n);
			
			if(pivot_pos != (end - 1))
				std::swap(*pivot_pos, *(end - 1));	//swap pivot_pos value to end
			pivot_pos = end - 1;

			std::binder2nd<Comparator> pred(comp, *pivot_pos);

			//divide, leave pivot unchanged in last place
			RandomAccessIterator split_pos1, split_pos2;
			split_pos1 = std::__mcstl_sequential_partition(begin, end - 1, pred); 
			//left side: < pivot_pos; right side: >= pivot_pos
#if MCSTL_ASSERTIONS
			assert(begin <= split_pos1 && split_pos1 < end);
#endif
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


			elements_done += (split_pos2 - split_pos1);	//elements equal to pivot are done
#if MCSTL_ASSERTIONS
			total_elements_done += (split_pos2 - split_pos1);
#endif
			//always push larger part onto stack
			if(((split_pos1 + 1) - begin) < (end - (split_pos2)))
			{	//right side larger
				if((split_pos2) != end)
					tl.leftover_parts.push_front(std::make_pair(split_pos2, end));
				
				//current.first = begin;	//already set anyway
				current.second = split_pos1;
				continue;
			}
			else
			{	//left side larger
				if(begin != split_pos1)
					tl.leftover_parts.push_front(std::make_pair(begin, split_pos1));
				
				current.first = split_pos2;
				//current.second = end;	//already set anyway
				continue;
			}
		}
		else
		{
			std::__mcstl_sequential_sort(begin, end, comp);
			elements_done += n;
#if MCSTL_ASSERTIONS
			total_elements_done += n;
#endif

			if(tl.leftover_parts.pop_front(current))	//prefer own stack, small pieces
				continue;

			#pragma omp atomic
			*tl.elements_leftover -= elements_done;
			elements_done = 0;
			
#if MCSTL_ASSERTIONS
			double search_start = omp_get_wtime();
#endif

			//look for new work
			bool success = false;
			while(*tl.elements_leftover > 0 && !success
#if MCSTL_ASSERTIONS
				&& (omp_get_wtime() < (search_start + 1.0))	//possible dead-lock
#endif
			     )
			{
				thread_index_t victim;
 				victim = rng(num_threads);
				success = (victim != iam) && tls[victim]->leftover_parts.pop_back(current);	//large pieces
				if(!success)
					yield();
#if !defined(__ICC) && !defined(__ECC)
				#pragma omp flush
#endif
			}

#if MCSTL_ASSERTIONS			
		    	if(omp_get_wtime() >= (search_start + 1.0))
			{
				sleep(1);
				assert(omp_get_wtime() < (search_start + 1.0));
			}
#endif
			if(!success)
			{
#if MCSTL_ASSERTIONS
				assert(*tl.elements_leftover == 0);
#endif
				return;
			}
			
			//success == true
		}
	}
}

/** @brief Top-level quicksort routine.
 *  @param begin Begin iterator of sequence.
 *  @param end End iterator of sequence.
 *  @param comp Comparator.
 *  @param n Length of the sequence to sort.
 *  @param num_threads Number of threads that are allowed to work on this part. */
template<typename RandomAccessIterator, typename Comparator>
inline void
parallel_sort_qsb(	//quicksort balanced
		RandomAccessIterator begin, 
		RandomAccessIterator end,
		Comparator comp,
		typename std::iterator_traits<RandomAccessIterator>::difference_type n,
		int num_threads)
{
	MCSTL_CALL(end - begin)
	
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
	typedef std::pair<RandomAccessIterator, RandomAccessIterator> Piece;

	if(n <= 1)
		return;

	if(num_threads > n)
		num_threads = static_cast<thread_index_t>(n);	//at least one element per processor

	QSBThreadLocal<RandomAccessIterator>** tls = new QSBThreadLocal<RandomAccessIterator>*[num_threads];

	//initialize thread local variables etc.
	#pragma omp parallel num_threads(num_threads)
	qsb_initialize(tls, num_threads * (thread_index_t)(log2(n) + 1));	//initialize variables per processor

	//There can never be more than ceil(log2(n)) ranges on the stack, because
	//1. Only one processor pushes onto the stack
	//2. The largest range has at most length n
	//3. Each range is larger than half of the range remaining

	volatile DiffType elements_leftover = n;
	
	for(int i = 0; i < num_threads; i++)
	{
		tls[i]->elements_leftover = &elements_leftover;
		tls[i]->num_threads = num_threads;
		tls[i]->global = std::make_pair(begin, end);
		tls[i]->initial = std::make_pair(end, end);	//just in case nothing is left to assign
	}

	//initial splitting, recursively
	int old_nested = omp_get_nested();
	omp_set_nested(true);
	
	//main recursion call
	qsb_conquer(tls, begin, begin + n, comp, 0, num_threads);
	
	omp_set_nested(old_nested);
	
#if MCSTL_ASSERTIONS
	Piece dummy;
	for(int i = 1; i < num_threads; i++)
		assert(!tls[i]->leftover_parts.pop_back(dummy));	//all stack must be empty
#endif

	for(int i = 0; i < num_threads; i++)
		delete tls[i];
	delete[] tls;
}

}	//namespace mcstl

#endif
