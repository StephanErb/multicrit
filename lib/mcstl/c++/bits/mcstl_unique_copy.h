/***************************************************************************
 *   Copyright (C) 2007 by Robert Geisberger and Robin Dapp                *
 *   {robert.geisberger,robin.dapp}@stud.uni-karlsruhe.de                  *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_unique_copy.h
 *  @brief Parallel implementations of std::unique_copy(). */

#ifndef _MCSTL_UNIQUE_H
#define _MCSTL_UNIQUE_H 1

#include <mcstl.h>
#include <bits/mcstl_multiseq_selection.h>

namespace mcstl 
{

/** @brief Parallel std::unique_copy(), without explicit equality predicate.
 *  @param first Begin iterator of input sequence.
 *  @param last End iterator of input sequence.
 *  @param result Begin iterator of result sequence.
 *  @param binary_pred Equality predicate.
 *  @return End iterator of result sequence. */
template <class InputIterator, class OutputIterator, class BinaryPredicate>
inline OutputIterator parallel_unique_copy(InputIterator first, InputIterator last,
	OutputIterator result, BinaryPredicate binary_pred) 
{
	MCSTL_CALL(last - first)

	typedef typename std::iterator_traits<InputIterator>::value_type ValueType;
	typedef typename std::iterator_traits<InputIterator>::difference_type 
		DiffType;

	DiffType size = last - first;
	int num_threads = mcstl::SETTINGS::num_threads; 
	DiffType counter[num_threads + 1];
	
	if(size == 0)
		return result;

	// let the first thread process two parts
	DiffType borders[num_threads + 2];
	mcstl::equally_split(size, num_threads + 1, borders);
	
	//first part contains at least one element

	#pragma omp parallel num_threads(num_threads)
	{
		int iam = omp_get_thread_num();

		DiffType begin, end;

		// check for length without duplicates
		// needed for position in output
		DiffType i = 0;
		OutputIterator out = result;
		if (iam == 0)
		{
			begin = borders[0] + 1;	// == 1
			end = borders[iam + 1];
			
			i++;
			new (static_cast<void *>(&*out)) ValueType(*first);
			out++;
			
			for (InputIterator iter = first + begin; iter < first + end; ++iter)
			{
				if (!binary_pred(*iter, *(iter-1))) 
				{
					i++;
					new (static_cast<void *>(&*out)) ValueType(*iter);
					out++;
				}
			}
		} 
		else 
		{
			begin = borders[iam]; //one part
			end = borders[iam + 1];
		
			for (InputIterator iter = first + begin; iter < first + end; ++iter) 
			{
				if (!binary_pred(*iter, *(iter-1))) 
				{
					i++;
				}
			}
		}
		counter[iam] = i;
		
		//last part still untouched

		DiffType begin_output;
		 
		#pragma omp barrier

		// store result in output on calculated positions
		begin_output = 0;

		if (iam == 0) 
		{
			for(int t = 0; t < num_threads; t++ ) 
			{
				begin_output += counter[t];
			}

			i = 0;

			OutputIterator iter_out = result + begin_output;
			
			begin = borders[num_threads];
			end = size;
			
			for (InputIterator iter = first + begin; iter < first + end; ++iter) 
			{
				if (iter == first || !binary_pred(*iter, *(iter-1))) 
				{
					i++;
					new (static_cast<void *>(&*iter_out)) ValueType(*iter);
					iter_out++;
				}
			}
			
			counter[num_threads] = i;
		} 
		else 
		{
			for (int t = 0; t < iam; t++) 
				begin_output += counter[t];
			
			OutputIterator iter_out = result + begin_output;
			
			for (InputIterator iter = first + begin; iter < first + end; ++iter) 
			{
				if (!binary_pred(*iter, *(iter-1))) 
				{
					new (static_cast<void *> (&*iter_out)) ValueType(*iter);
					iter_out++;
				}
			}
		}
	}
	
	DiffType end_output = 0;
	for (int t = 0; t < num_threads + 1; t++)
		end_output += counter[t];
	
	return result + end_output; }

/** @brief Parallel std::unique_copy(), without explicit equality predicate
 *  @param first Begin iterator of input sequence.
 *  @param last End iterator of input sequence.
 *  @param result Begin iterator of result sequence.
 *  @return End iterator of result sequence. */
template <class InputIterator, class OutputIterator>
inline OutputIterator parallel_unique_copy(InputIterator first, InputIterator last,
	OutputIterator result) 
{
	
	typedef typename std::iterator_traits<InputIterator>::value_type ValueType;
	
	return parallel_unique_copy(first, last, result, std::equal_to<ValueType>());
}

}//namespace mcstl

#endif
