/***************************************************************************
 *   Copyright (C) 2007 by Felix Putze                                     *
 *   kontakt@felix-putze.de                                                *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_par_loop.h
 *  @brief Parallelization of embarrassingly parallel execution by means of equal splitting. */

#ifndef _MCSTL_PAR_LOOP_H
#define _MCSTL_PAR_LOOP_H 1

#include <omp.h>

#include <mod_stl/stl_algobase.h>

#include <bits/mcstl_settings.h>

namespace mcstl
{

/** @brief Embarrassingly parallel algorithm for random access iterators, using hand-crafted parallelization by equal splitting the work.
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
for_each_template_random_access_ed(	RandomAccessIterator begin,
					RandomAccessIterator end,
					Op o,
					Fu& f,
					Red r,
					Result base,
					Result& output,
					typename std::iterator_traits<RandomAccessIterator>::difference_type bound)
{	
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
		
	thread_index_t num_threads = static_cast<thread_index_t>(std::max<DiffType>(std::min<DiffType>(static_cast<DiffType>(SETTINGS::num_threads), end - begin), 1));
	DiffType length = end - begin;
	
	Result *thread_results = new Result[num_threads];
	
	#pragma omp parallel num_threads(num_threads)
	{
		Result reduct = Result();	//neutral element
		
		thread_index_t p = num_threads;
		thread_index_t iam = omp_get_thread_num();
		DiffType start = iam * length / p;
		DiffType limit = (iam == p - 1) ? length : (iam + 1) * length / p;
				
		if(start < limit)
		{
			reduct = f(o, begin + start);
			start++;
		}
		
		for(; start < limit; start++)
		{
			reduct = r(reduct, f(o, begin + start));
		}
		
		thread_results[iam] = reduct;
	}	
		
	for(thread_index_t i = 0; i < num_threads; i++)
	{		
		output = r(output, thread_results[i]);
	}
	
	f.finish_iterator = begin + length;	// points to last element processed (needed as return value for some algorithms like transform)
	
	return o;
} // end function

} // end namespace

#endif
