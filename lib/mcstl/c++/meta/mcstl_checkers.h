/***************************************************************************
 *   Copyright (C) 2006 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/**
 * @file mcstl_checkers.h
 * @brief Routines for checking the correctness of algorithm results.
 */

#ifndef _MCSTL_CHECKERS
#define _MCSTL_CHECKERS 1

#include <functional>
#include <cstdio>

#include <mod_stl/stl_algobase.h>

namespace mcstl
{

/**
 * @brief Check whether @c [begin, @c end) is sorted according to @c comp.
 * @param begin Begin iterator of sequence.
 * @param end End iterator of sequence.
 * @param comp Comparator.
 * @return @c true if sorted, @c false otherwise.
 */
template<typename InputIterator, typename Comparator>
bool is_sorted(InputIterator begin, InputIterator end, Comparator comp = std::less<typename std::iterator_traits<InputIterator>::value_type>())
{
	if(begin == end)
		return true;

	InputIterator current(begin), recent(begin);
	
	unsigned long long position = 1;
	for(current++; current != end; current++)
	{
		if(comp(*current, *recent))
		{
			printf("is_sorted: check failed before position %i.\n", position);
			//std::cout << "is_sorted: check failed before position " <<  position << ": " << *recent << ", " << *current << std::endl;
			return false;
		}
		recent = current;
		position++;
	}
	
	return true;
}

/**
 * @brief Check whether @c [begin, @c end) is sorted according to @c comp.
 * Prints the position in case an misordered pair is found.
 * @param begin Begin iterator of sequence.
 * @param end End iterator of sequence.
 * @param first_failure The first failure is returned in this variable.
 * @param comp Comparator.
 * @return @c true if sorted, @c false otherwise.
 */
template<typename InputIterator, typename Comparator>
bool is_sorted_failure(InputIterator begin, InputIterator end, InputIterator& first_failure, Comparator comp = std::less<typename std::iterator_traits<InputIterator>::value_type>())
{
	if(begin == end)
		return true;

	InputIterator current(begin), recent(begin);
	
	unsigned long long position = 1;
	for(current++; current != end; current++)
	{
		if(comp(*current, *recent))
		{
			first_failure = current;
			printf("is_sorted: check failed before position %lld.\n", position);
			return false;
		}
		recent = current;
		position++;
	}
	
	first_failure = end;
	return true;
}

/**
 * @brief Check whether @c [begin, @c end) is sorted according to @c comp.
 * Prints all misordered pair, including the surrounding two elements.
 * @param begin Begin iterator of sequence.
 * @param end End iterator of sequence.
 * @param comp Comparator.
 * @return @c true if sorted, @c false otherwise.
 */
template<typename InputIterator, typename Comparator>
bool is_sorted_print_failures(InputIterator begin, InputIterator end, Comparator comp = std::less<typename std::iterator_traits<InputIterator>::value_type>())
{
	if(begin == end)
		return true;

	InputIterator recent(begin);
	bool ok = true;
	
	for(InputIterator pos(begin + 1); pos != end; pos++)
	{
		if(comp(*pos, *recent))
		{
			printf("%ld: %d %d %d %d\n", pos - begin, *(pos - 2), *(pos- 1), *pos, *(pos + 1));
			ok = false;
		}
		recent = pos;
	}
	
	return ok;
}

}

#endif
