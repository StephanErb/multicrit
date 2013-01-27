/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_partial_sum.h
 *  @brief Parallel implementation of std::partial_sum(), i. e. prefix sums. */

#ifndef _MCSTL_PARTIAL_SUM_H
#define _MCSTL_PARTIAL_SUM_H 1

#include <cassert>
#include <omp.h>

#include <mod_stl/stl_algobase.h>
#include <mcstl.h>
#include <bits/mcstl_numeric_forward.h>


namespace mcstl
{
	//problem: there is no 0-element given

	/** @brief Base case prefix sum routine.
	 *  @param begin Begin iterator of input sequence.
	 *  @param end End iterator of input sequence.
	 *  @param result Begin iterator of output sequence.
	 *  @param bin_op Associative binary function.
	 *  @param value Start value. Must be passed since the neutral element is unknown in general. 
	 *  @return End iterator of output sequence. */
	template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
	OutputIterator
	inline parallel_partial_sum_basecase(InputIterator begin, InputIterator end,
	                                OutputIterator result,
	                                BinaryOperation bin_op,
	                                typename ::std::iterator_traits<InputIterator>::value_type value)
	{
		if (begin == end)
			return result;

		while (begin != end)
		{
			value = bin_op(value, *begin);
			*result = value;
			result++;
			begin++;
		}
		return result;
	}

	/** @brief Parallel partial sum implmenetation, two-phase approach, no recursion. 
	 *  @param begin Begin iterator of input sequence.
	 *  @param end End iterator of input sequence.
	 *  @param result Begin iterator of output sequence.
	 *  @param bin_op Associative binary function.
	 *  @param n Length of sequence.
	 *  @param num_threads Number of threads to use.
	 *  @return End iterator of output sequence. */
	template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
	OutputIterator
	parallel_partial_sum_linear(InputIterator begin, InputIterator end,
	                            OutputIterator result,
	                            BinaryOperation bin_op,
	                            typename ::std::iterator_traits<InputIterator>::difference_type n,
	                            int num_threads)
	{
		typedef typename ::std::iterator_traits<InputIterator>::value_type ValueType;
		typedef typename ::std::iterator_traits<InputIterator>::difference_type DiffType;

		if(num_threads > (n - 1))
			num_threads = static_cast<thread_index_t>(n - 1);
		if(num_threads < 2)
		{
			*result = *begin;
			return parallel_partial_sum_basecase(begin + 1, end, result + 1, bin_op, *begin);
		}

		DiffType borders[num_threads + 2]; 

		if(SETTINGS::partial_sum_dilatation == 1.0f)
			equally_split(n, num_threads + 1, borders);
		else
		{
			DiffType chunk_length = (int)((double)n / ((double)num_threads + SETTINGS::partial_sum_dilatation)), borderstart = n - num_threads * chunk_length;
			borders[0] = 0;
			for(int i = 1; i < (num_threads + 1); i++)
			{
				borders[i] = borderstart;
				borderstart += chunk_length;
			}
			borders[num_threads + 1] = n;
		}

		ValueType* sums = new ValueType[num_threads];
		OutputIterator target_end;

#		pragma omp parallel num_threads(num_threads)
		{
			int id = omp_get_thread_num();
			if(id == 0)
			{
				*result = *begin;
				parallel_partial_sum_basecase(begin + 1, begin + borders[1], result + 1, bin_op, *begin);
				sums[0] = *(result + borders[1] - 1);
			}
			else
			{
				sums[id] = std::accumulate(begin + borders[id] + 1, begin + borders[id + 1], *(begin + borders[id]), bin_op, mcstl::sequential_tag());
			}
			
#			pragma omp barrier

#			pragma omp single
			parallel_partial_sum_basecase(sums + 1, sums + num_threads, sums + 1, bin_op, sums[0]);

#			pragma omp barrier

			//still same team

			parallel_partial_sum_basecase(begin + borders[id + 1], begin + borders[id + 2], result + borders[id + 1], bin_op, sums[id]);
		}

		delete[] sums;

		return result + n;
	}

	/** @brief Parallel partial sum front-end.
	 *  @param begin Begin iterator of input sequence.
	 *  @param end End iterator of input sequence.
	 *  @param result Begin iterator of output sequence.
	 *  @param bin_op Associative binary function.
	 *  @return End iterator of output sequence. */
	template<typename InputIterator, typename OutputIterator, typename BinaryOperation>
	OutputIterator
	parallel_partial_sum(InputIterator begin, InputIterator end,
	                     OutputIterator result,
	                     BinaryOperation bin_op)
	{
		MCSTL_CALL(begin - end);

		typedef typename ::std::iterator_traits<InputIterator>::value_type ValueType;
		typedef typename ::std::iterator_traits<InputIterator>::difference_type DiffType;

		DiffType n = end - begin;

		int num_threads = SETTINGS::num_threads;

		switch(SETTINGS::partial_sum_algorithm)
		{
			case SETTINGS::LINEAR:
				return parallel_partial_sum_linear(begin, end, result, bin_op, n, num_threads);	//need an initial offset
			default:
				assert(0);	//partial_sum algorithm not implemented
				return end;
		}
	}
}

#endif
