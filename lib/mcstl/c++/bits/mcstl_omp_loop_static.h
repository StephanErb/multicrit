/***************************************************************************
 *   Copyright (C) 2007 by Felix Putze                                     *
 *   kontakt@felix-putze.de                                                *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_omp_loop_static.h
 *  @brief Parallelization of embarrassingly parallel execution by means of an OpenMP for loop with static scheduling. */

#ifndef _MCSTL_OMP_LOOP_STATIC_H
#define _MCSTL_OMP_LOOP_STATIC_H 1

#include <omp.h>

#include <bits/mcstl_settings.h>
#include <bits/mcstl_basic_iterator.h>

namespace mcstl
{
 
/** @brief Embarrassingly parallel algorithm for random access iterators, using an OpenMP for loop with static scheduling.
 * 
 *  @param begin Begin iterator of element sequence.
 *  @param end End iterator of element sequence.
 *  @param o User-supplied functor (comparator, predicate, adding functor, ...).
 *  @param f Functor to "process" an element with op (depends on desired functionality, e. g. for std::for_each(), ...).
 *  @param r Functor to "add" a single result to the already processed elements (depends on functionality).
 *  @param base Base value for reduction.
 *  @param output Pointer to position where final result is written to
 *  @param bound Maximum number of elements processed (e. g. for std::count_n()). 
 *  @return User-supplied functor (that may contain a part of the result). */
template<typename RandomAccessIterator, typename Op, typename Fu, typename Red, typename Result>
Op 
for_each_template_random_access_omp_loop_static(	RandomAccessIterator begin,
							RandomAccessIterator end,
							Op o,
							Fu& f,
							Red r,
							Result base,
							Result& output,
							typename std::iterator_traits<RandomAccessIterator>::difference_type bound)
{	
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
		
	thread_index_t num_threads = (SETTINGS::num_threads < (end - begin)) ? SETTINGS::num_threads : (end - begin);		
	Result *thread_results = new Result[num_threads];
	DiffType length = end - begin;
	
	for(thread_index_t i = 0; i < num_threads; i++)
	{		
		thread_results[i] = r(thread_results[i], f(o, begin+i));
	}	
	
	#pragma omp parallel num_threads(num_threads)
	{
		#pragma omp for schedule(static, SETTINGS::workstealing_chunk_size)
		for(DiffType pos = 0; pos < length; pos++)
		{
			thread_results[omp_get_thread_num()] = r(thread_results[omp_get_thread_num()], f(o, begin+pos));
		}	
	}			
		
	for(thread_index_t i = 0; i < num_threads; i++)
	{		
		output = r(output, thread_results[i]);
	}
	
	delete [] thread_results;
	
	f.finish_iterator = begin + length;	// points to last element processed (needed as return value for some algorithms like transform)
	
	return o;
} // end function

} // end namespace

#endif
