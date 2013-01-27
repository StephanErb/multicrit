/***************************************************************************
 *   Copyright (C) 2007 by Felix Putze                                     *
 *   kontakt@felix-putze.de                                                *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_find_selectors.h
 *  @brief Function objects representing different tasks to be plugged into the parallel find  algorithm. */
 
#ifndef _MCSTL_FIND_FUNCTIONS_H
#define _MCSTL_FIND_FUNCTIONS_H 1

#include <bits/mcstl_tags.h>
#include <bits/mcstl_basic_iterator.h>
#include <bits/stl_pair.h>

namespace mcstl
{
	/** @brief Base class of all mcstl::find_template selectors. */
	struct generic_find_selector
	{
	};

	/** @brief Test predicate on a single element, used for std::find() and std::find_if(). */
	struct find_if_selector : public generic_find_selector
	{
		/** @brief Test on one position.
		  * @param i1 Iterator on first sequence.
		  * @param i2 Iterator on second sequence (unused).
		  * @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline bool operator()(
				RandomAccessIterator1 i1,
				RandomAccessIterator2 i2,
				Pred pred
				)
		{
			return pred(*i1);
		}
		
		/** @brief Corresponding sequential algorithm on a sequence.
		 *  @param begin1 Begin iterator of first sequence.
		 *  @param end1 End iterator of first sequence.
		 *  @param begin2 Begin iterator of second sequence.
		 *  @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline std::pair<RandomAccessIterator1, RandomAccessIterator2> sequential_algorithm(
				RandomAccessIterator1 begin1,
				RandomAccessIterator1 end1,
				RandomAccessIterator2 begin2,
				Pred pred
				)
		{
			return std::make_pair(
					find_if(begin1, end1, pred, sequential_tag()),
					begin2 /* dummy value */);
		}
	};	
	
	/** @brief Test predicate on two adjacent elements. */
	struct adjacent_find_selector  : public generic_find_selector
	{
		/** @brief Test on one position.
		 *  @param i1 Iterator on first sequence.
		 *  @param i2 Iterator on second sequence (unused).
		 *  @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline bool operator()(
				RandomAccessIterator1 i1,
				RandomAccessIterator2 i2,
				Pred pred)
		{			
			return pred(*i1, *(i1 + 1));	//passed end iterator is one short
		}
		
		/** @brief Corresponding sequential algorithm on a sequence.
		 *  @param begin1 Begin iterator of first sequence.
		 *  @param end1 End iterator of first sequence.
		 *  @param begin2 Begin iterator of second sequence.
		 *  @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline std::pair<RandomAccessIterator1, RandomAccessIterator2> 
		sequential_algorithm(
			RandomAccessIterator1 begin1,
			RandomAccessIterator1 end1,
			RandomAccessIterator2 begin2,
			Pred pred)
		{
			RandomAccessIterator1 spot = 
				adjacent_find(begin1, end1 + 1, pred, sequential_tag());	//passed end iterator is one short
			if(spot == (end1 + 1))
				spot = end1;
			return std::make_pair(spot, begin2 /* dummy value */);
		}
	};	
	
	/** @brief Test inverted predicate on a single element. */
	struct mismatch_selector : public generic_find_selector
	{
		/** @brief Test on one position.
		 *  @param i1 Iterator on first sequence.
		 *  @param i2 Iterator on second sequence (unused).
		 *  @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline bool operator()(
				RandomAccessIterator1 i1,
				RandomAccessIterator2 i2,
				Pred pred
				)
		{			
			return !pred(*i1, *i2);
		}
		
		/** @brief Corresponding sequential algorithm on a sequence.
		 *  @param begin1 Begin iterator of first sequence.
		 *  @param end1 End iterator of first sequence.
		 *  @param begin2 Begin iterator of second sequence.
		 *  @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline std::pair<RandomAccessIterator1, RandomAccessIterator2> 
			sequential_algorithm(
				RandomAccessIterator1 begin1,
				RandomAccessIterator1 end1,
				RandomAccessIterator2 begin2,
				Pred pred
				)
		{
			return mismatch(begin1, end1, begin2, pred, sequential_tag());
		}		
	};
	
	
	/** @brief Test predicate on several elements. */
	template<typename ForwardIterator>
	struct find_first_of_selector : public generic_find_selector
	{
		ForwardIterator begin;
		ForwardIterator end;
		
		explicit find_first_of_selector(ForwardIterator begin, ForwardIterator end) : begin(begin), end(end)
		{
		}
		
		/** @brief Test on one position.
		 *  @param i1 Iterator on first sequence.
		 *  @param i2 Iterator on second sequence (unused).
		 *  @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline bool operator()(
				RandomAccessIterator1 i1,
				RandomAccessIterator2 i2,
				Pred pred
				)
		{			
			for(ForwardIterator pos_in_candidates = begin; pos_in_candidates != end; pos_in_candidates++)
				if(pred(*i1, *pos_in_candidates))
					return true;
			return false;
		}
		
		/** @brief Corresponding sequential algorithm on a sequence.
		 *  @param begin1 Begin iterator of first sequence.
		 *  @param end1 End iterator of first sequence.
		 *  @param begin2 Begin iterator of second sequence.
		 *  @param pred Find predicate. */
		template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Pred>
		inline std::pair<RandomAccessIterator1, RandomAccessIterator2> sequential_algorithm(
				RandomAccessIterator1 begin1,
				RandomAccessIterator1 end1,
				RandomAccessIterator2 begin2,
				Pred pred
				)
		{
			return std::make_pair(
				find_first_of(begin1, end1, begin, end, pred, sequential_tag()), 
				begin2 /* dummy value */ );
		}		
	};
	
}	//namespace mcstl

#endif
