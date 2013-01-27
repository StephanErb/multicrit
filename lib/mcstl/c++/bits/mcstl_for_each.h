/***************************************************************************
 *   Copyright (C) 2007 by Felix Putze                                     *
 *   kontakt@felix-putze.de                                                *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_for_each.h
 *  @brief Main interface for embarassingly parallel functions.
 *
 *  The explicit implementation are in other header files, like 
 *  mcstl_workstealing.h, mcstl_par_loop.h, mcstl_omp_loop.h, mcstl_omp_loop_static.h */

#ifndef _MCSTL_FOR_EACH_H
#define _MCSTL_FOR_EACH_H 1

#include <bits/mcstl_settings.h>
#include <bits/mcstl_par_loop.h>
#include <bits/mcstl_omp_loop.h>
#include <bits/mcstl_workstealing.h>

namespace mcstl
{

/** @brief Chose the desired algorithm by evaluating @c parallelism_tag.
 *  @param begin Begin iterator of input sequence.
 *  @param end End iterator of input sequence.
 *  @param user_op A user-specified functor (comparator, predicate, associative operator,...)
 *  @param functionality functor to "process" an element with user_op (depends on desired functionality, e. g. accumulate, for_each,...
 *  @param reduction Reduction functor.
 *  @param reduction_start Initial value for reduction.
 *  @param output Output iterator.
 *  @param bound Maximum number of elements processed.
 *  @param parallelism_tag Parallelization method */
template<typename InputIterator, typename UserOp, typename Functionality, typename Red, typename Result>
UserOp for_each_template_random_access(	InputIterator begin,
					InputIterator end,
					UserOp user_op,
					Functionality& functionality,
					Red reduction,
					Result reduction_start,
					Result& output,
					typename std::iterator_traits<InputIterator>::difference_type bound,
					PARALLELISM parallelism_tag)
{
	if(parallelism_tag.par == PARALLELISM::PARALLEL_UNBALANCED)
		return for_each_template_random_access_ed(begin, end, user_op, functionality, reduction, reduction_start, output, bound);
	else if(parallelism_tag.par == PARALLELISM::PARALLEL_OMP_LOOP)
		return for_each_template_random_access_omp_loop(begin, end, user_op, functionality, reduction, reduction_start, output, bound);
	else if(parallelism_tag.par == PARALLELISM::PARALLEL_OMP_LOOP_STATIC)
		return for_each_template_random_access_omp_loop(begin, end, user_op, functionality, reduction, reduction_start, output, bound);
	else	//e. g. PARALLEL_BALANCED
		return for_each_template_random_access_workstealing(begin, end, user_op, functionality, reduction, reduction_start, output, bound);
}

}	//end namespace

#endif
