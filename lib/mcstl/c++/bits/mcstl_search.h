/***************************************************************************
 *   Copyright (C) 2007 by Felix Putze                                     *
 *   kontakt@felix-putze.de                                                *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_search.h
 *  @brief Parallel implementation base for std::search() and std::search_n(). */
#ifndef _MCSTL_SEARCH_H
#define _MCSTL_SEARCH_H 1

#include <mod_stl/stl_algobase.h>

#include <mcstl.h>
#include <bits/mcstl_equally_split.h>

/** @brief Precalculate advances for Knuth-Morris-Pratt algorithm. 
 *  @param elements Begin iterator of sequence to search for.
 *  @param length Length of sequence to search for.
 *  @param advances Returned offsets. */
template <typename RandomAccessIterator, typename DiffType>
void calc_borders(RandomAccessIterator elements, DiffType length, DiffType *advances)
{
	advances[0] = -1;
	if (length > 1)
		advances[1] = 0;
	DiffType k = 0;
	for(DiffType j = 2; j <= length; j++)
	{
		while((k >= 0) && (elements[k] != elements[j-1]))
			k = advances[k];
		advances[j] = ++k;
	}
}

namespace mcstl
{
	// generic parallel find algorithm (requires random access iterator)
	
/** @brief Parallel std::search.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence.
 *  @param end2 End iterator of second sequence.
 *  @param pred Find predicate.
 *  @return Place of finding in first sequences. */
template<typename _RandomAccessIterator1, typename _RandomAccessIterator2, typename Pred> 
_RandomAccessIterator1 search_template(
	_RandomAccessIterator1 begin1,
	_RandomAccessIterator1 end1,
	_RandomAccessIterator2 begin2,
	_RandomAccessIterator2 end2,
	Pred pred)
{
	typedef typename std::iterator_traits<_RandomAccessIterator1>::difference_type DiffType;

	MCSTL_CALL((end1 - begin1) + (end2 - begin2));

	DiffType pattern_length = end2 - begin2;
	DiffType input_length = (end1 - begin1) - pattern_length;	// last point to start search
	DiffType res = (end1 - begin1);	// where is first occurence of pattern? defaults to end.

	if(input_length < 0)	// pattern too long
		return end1;
		
	thread_index_t num_threads = std::max<DiffType>(1, std::min<DiffType>(input_length, mcstl::SETTINGS::num_threads));
	
	DiffType borders[num_threads + 1];
	mcstl::equally_split(input_length, num_threads, borders);

	DiffType advances[pattern_length];
	calc_borders(begin2, pattern_length, advances);
	
	#pragma omp parallel num_threads(num_threads)
	{			
		thread_index_t iam = omp_get_thread_num();
		
		DiffType start = borders[iam], stop = borders[iam + 1];
		
		DiffType pos_in_pattern = 0;
		bool found_pattern = false;
		
		while(start <= stop && !found_pattern)
		{
			#pragma omp flush(res)	// get new value of res
			if(res < start)		// no chance for this thread to find first occurence
				break;
			while(pred(begin1[start + pos_in_pattern], begin2[pos_in_pattern]))
			{
				++pos_in_pattern;
				if(pos_in_pattern == pattern_length)	// found new candidate for res
				{
					#pragma omp critical (res)
					res = std::min(res, start);
					
					found_pattern = true;
					break;
				}
			}
			start += (pos_in_pattern - advances[pos_in_pattern]);	// make safe jump
			pos_in_pattern = (advances[pos_in_pattern] < 0) ? 0 : advances[pos_in_pattern];
		} // end while !found_pattern
	} // end parallel

	return (begin1 + res);		// return iterator on found element
} // end find_template

} // end namespace

#endif
