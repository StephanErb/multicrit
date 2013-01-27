/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_algobase.h
 *  @brief Parallel STL function calls corresponding to the stl_algobase.h header.
 *  The functions defined here mainly do case switches and 
 *  call the actual parallelized versions in other files.
 *  Inlining policy: Functions that basically only contain one function call,
 *  are declared inline. */

#ifndef _MCSTL_ALGOBASE_H
#define _MCSTL_ALGOBASE_H 1

#include <mod_stl/stl_algobase.h>

#include <bits/mcstl_base.h>
#include <bits/mcstl_tags.h>
#include <bits/mcstl_settings.h>
#include <bits/mcstl_find.h>
#include <bits/mcstl_find_selectors.h>
#include <bits/mcstl_for_each.h>
#include <bits/mcstl_for_each_selectors.h>

namespace std
{

////  equal  ///////////////////////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2>
inline 
bool
equal(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
	mcstl::sequential_tag)
{
	return std::__mcstl_sequential_equal<InputIterator1, InputIterator2>(begin1, end1, begin2);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2, typename Predicate>
inline 
bool
equal(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2,
	Predicate pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_equal(begin1, end1, begin2, pred);
}

// public interface
template<typename InputIterator1, typename InputIterator2>
inline 
bool
equal(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2)
{
	return mismatch(begin1, end1, begin2).first == end1;
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename Predicate>
inline bool
equal(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, 
	Predicate pred)
{
	return mismatch(begin1, end1, begin2, pred).first == end1;
}


////  lexicographical_compare  /////////////////////////////////////////////

// sequential fallback
template<typename InputIterator1, typename InputIterator2>
inline 
bool
lexicographical_compare(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2,
	mcstl::sequential_tag)
{
	return std::__mcstl_sequential_lexicographical_compare<InputIterator1, InputIterator2>(begin1, end1, begin2, end2);
}

// sequential fallback
template<typename InputIterator1, typename InputIterator2, typename Predicate>
inline 
bool
lexicographical_compare(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2,
	Predicate pred, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_lexicographical_compare(begin1, end1, begin2, end2, pred);
}

// sequential fallback for input iterator case
template<typename InputIterator1, typename InputIterator2, typename Predicate, typename IteratorTag1, typename IteratorTag2>
inline
bool
lexicographical_compare_switch(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, 
	Predicate pred, IteratorTag1, IteratorTag2)
{
	return __mcstl_sequential_lexicographical_compare(begin1, end1, begin2, end2, pred);
}

// parallel lexicographical_compare for random access iterators
// limitation: Both valuetypes must be the same
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename Predicate>
bool
lexicographical_compare_switch(RandomAccessIterator1 begin1, RandomAccessIterator1 end1, RandomAccessIterator2 begin2, RandomAccessIterator2 end2,
	Predicate pred, random_access_iterator_tag, random_access_iterator_tag)
{
	if(MCSTL_PARALLEL_CONDITION(true))
	{
		//longer sequence in first place
		if((end1 - begin1) < (end2 - begin2))
		{
			pair<RandomAccessIterator1, RandomAccessIterator2> mm = 
			mismatch_switch(begin1, end1, begin2, 
			mcstl::equal_from_less<Predicate, typename iterator_traits<RandomAccessIterator1>::value_type, typename iterator_traits<RandomAccessIterator2>::value_type>(pred),
			random_access_iterator_tag(), random_access_iterator_tag());
			return (mm.first == end1) /*less because shorter*/ || pred(*mm.first, *mm.second) /*less because differing element less*/;
		}
		else
		{
			pair<RandomAccessIterator2, RandomAccessIterator1> mm = 
			mismatch_switch(begin2, end2, begin1, 
			mcstl::equal_from_less<Predicate, typename iterator_traits<RandomAccessIterator1>::value_type, typename iterator_traits<RandomAccessIterator2>::value_type>(pred),
			random_access_iterator_tag(), random_access_iterator_tag());
			return (mm.first != end2) /*less because shorter*/ && pred(*mm.second, *mm.first) /*less because differing element less*/;
		}
	}
	else
		return __mcstl_sequential_lexicographical_compare(begin1, end1, begin2, end2, pred);
}

// public interface
template<typename InputIterator1, typename InputIterator2>
inline
bool
lexicographical_compare(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2)
{
	return lexicographical_compare_switch(begin1, end1, begin2, end2, mcstl::less<typename iterator_traits<InputIterator1>::value_type, typename iterator_traits<InputIterator2>::value_type>(),
			typename std::iterator_traits<InputIterator1>::iterator_category(), typename std::iterator_traits<InputIterator2>::iterator_category());
}

// public interface
template<typename InputIterator1, typename InputIterator2, typename Predicate>
inline
bool
lexicographical_compare(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2, Predicate pred)
{
	return lexicographical_compare_switch(begin1, end1, begin2, end2, pred,
			typename std::iterator_traits<InputIterator1>::iterator_category(), typename std::iterator_traits<InputIterator2>::iterator_category());
}

////  fill  ////////////////////////////////////////////////////////////

// sequential fallback
template<typename ForwardIterator, typename Generator>
inline
void
fill(ForwardIterator begin, ForwardIterator end, Generator gen, mcstl::sequential_tag)
{
	std::__mcstl_sequential_fill<ForwardIterator, Generator>(begin, end, gen);
}

// sequential fallback for input iterator case
template<typename ForwardIterator, typename Generator, typename IteratorTag>
void
fill_switch(ForwardIterator begin, ForwardIterator end,
	Generator gen, IteratorTag, mcstl::PARALLELISM parallelism_tag)
{
	std::fill(begin, end, gen, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Generator>
void
fill_switch(RandomAccessIterator begin, RandomAccessIterator end,
	Generator gen, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	if(MCSTL_PARALLEL_CONDITION(static_cast<mcstl::sequence_index_t>(end - begin) >= mcstl::SETTINGS::fill_minimal_n && parallelism_tag.is_parallel()))
	{
		bool dummy;
		mcstl::fill_selector<RandomAccessIterator> functionality;
		mcstl::for_each_template_random_access(begin, end, gen, functionality, mcstl::dummy_reduct(), true, dummy, -1, parallelism_tag);
	}
	else
		std::fill(begin, end, gen, mcstl::sequential_tag());
}

// public interface
template<typename ForwardIterator, typename Generator>
inline
void
fill(ForwardIterator begin, ForwardIterator end,
	Generator gen, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	fill_switch(begin, end, gen, typename std::iterator_traits<ForwardIterator>::iterator_category(), parallelism_tag);
}


////  fill_n  //////////////////////////////////////////////////////////

// sequential fallback
template<typename OutputIterator, typename Size, typename Generator>
inline
OutputIterator
fill_n(OutputIterator begin, Size n, Generator gen, mcstl::sequential_tag)
{
	return std::__mcstl_sequential_fill_n(begin, n, gen);
}

// sequential fallback for input iterator case
template<typename OutputIterator, typename Size, typename Generator, typename IteratorTag>
OutputIterator
fill_n_switch(OutputIterator begin, Size n, Generator gen, IteratorTag, mcstl::PARALLELISM)
{
	return std::fill_n(begin, n, gen, mcstl::sequential_tag());
}

// parallel algorithm for random access iterators
template<typename RandomAccessIterator, typename Size, typename Generator>
RandomAccessIterator
fill_n_switch(RandomAccessIterator begin, Size n, Generator gen, random_access_iterator_tag, mcstl::PARALLELISM parallelism_tag)
{
	return std::fill_n(begin, n, gen, mcstl::sequential_tag());
}

// public interface
template<typename OutputIterator, typename Size, typename Generator>
OutputIterator
inline
fill_n(OutputIterator begin, Size n, Generator gen, mcstl::PARALLELISM parallelism_tag = mcstl::SETTINGS::parallel_balanced)
{
	return fill_n_switch(begin, n, gen, typename std::iterator_traits<OutputIterator>::iterator_category(), parallelism_tag);
}


} /* namespace std */

#endif /* _MCSTL_ALGOBASE_H */
