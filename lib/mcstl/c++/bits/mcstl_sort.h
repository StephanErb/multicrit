/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_sort.h
 *  @brief Parallel sorting algorithm switch. */

#ifndef _MCSTL_SORT_H
#define _MCSTL_SORT_H 1

#include <bits/mcstl_basic_iterator.h>

#include <bits/mcstl_features.h>
#include <mcstl.h>

#if MCSTL_ASSERTIONS
#include <meta/mcstl_checkers.h>
#endif

#if MCSTL_MERGESORT
#include <bits/mcstl_multiway_mergesort.h>
#endif

#if MCSTL_QUICKSORT
#include <bits/mcstl_quicksort.h>
#endif

#if MCSTL_BAL_QUICKSORT
#include <bits/mcstl_balanced_quicksort.h>
#endif

namespace mcstl
{

/** @brief Choose a parallel sorting algorithm. 
 *  @param begin Begin iterator of input sequence.
 *  @param end End iterator of input sequence.
 *  @param comp Comparator.
 *  @param stable Sort stable. 
 *  @callgraph */
template<typename RandomAccessIterator, typename Comparator>
inline void
parallel_sort(RandomAccessIterator begin, RandomAccessIterator end,
		Comparator comp,
		bool stable)
{
	MCSTL_CALL(end - begin)
	
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;

	if (begin != end)
	{
		DiffType n = end - begin;

		if(false) ;
#if MCSTL_MERGESORT
		else if(SETTINGS::sort_algorithm == SETTINGS::MWMS || stable)
			parallel_sort_mwms(begin, end, comp, n, SETTINGS::num_threads, stable);
#endif
#if MCSTL_QUICKSORT
		else if(SETTINGS::sort_algorithm == SETTINGS::QS && !stable)
			parallel_sort_qs(begin, end, comp, n, SETTINGS::num_threads);
#endif
#if MCSTL_BAL_QUICKSORT
		else if(SETTINGS::sort_algorithm == SETTINGS::QS_BALANCED && !stable)
			parallel_sort_qsb(begin, end, comp, n, SETTINGS::num_threads);
#endif
		else
			std::__mcstl_sequential_sort(begin, end, comp);
	}
}

}	//namespace mcstl

#endif
