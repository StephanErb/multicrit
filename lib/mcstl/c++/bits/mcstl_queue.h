/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_queue.h
 *  @brief Lock-free double-ended queue. */

#ifndef _MCSTL_QUEUE_H
#define _MCSTL_QUEUE_H 1

#include <bits/mcstl_types.h>
#include <bits/mcstl_base.h>
#include <bits/mcstl_compatibility.h>

/** @brief Decide whether to declare certain variable volatile in this file. */
#define MCSTL_VOLATILE volatile

namespace mcstl
{

/**@brief Double-ended queue of bounded size, allowing lock-free atomic access. 
 *  push_front() and pop_front() must not be called concurrently to each other, while
 *  pop_back() can be called concurrently at all times.
 *  @c empty(), @c size(), and @c top() are intentionally not provided.
 *  Calling them would not make sense in a concurrent setting.
 *  @param T Contained element type. */
template<class T>
class RestrictedBoundedConcurrentQueue
{
private:
	/** @brief Array of elements, seen as cyclic buffer. */
	T* base;
	
	/** @brief Maximal number of elements contained at the same time. */
	sequence_index_t max_size;
	
	/** @brief Cyclic begin and end pointers contained in one atomically changeable value. */
	MCSTL_VOLATILE lcas_t borders;
	
public:
	/** @brief Constructor. Not to be called concurrent, of course.
	 *  @param max_size Maximal number of elements to be contained. */
	RestrictedBoundedConcurrentQueue(sequence_index_t max_size)
	{
		this->max_size = max_size;
		base = new T[max_size];
		borders = encode2(0, 0);
		#pragma omp flush
	}
	
	/** @brief Destructor. Not to be called concurrent, of course. */
	~RestrictedBoundedConcurrentQueue()
	{
		delete[] base;
	}

	/** @brief Pushes one element into the queue at the front end.
	 *  Must not be called concurrently with pop_front(). */
	void push_front(const T& t)
	{
		lcas_t former_borders = borders;
		int former_front, former_back;
		decode2(former_borders, former_front, former_back);
		*(base + former_front % max_size) = t;
#if MCSTL_ASSERTIONS
		assert(((former_front + 1) - former_back) <= max_size);	//otherwise: front - back > max_size eventually
#endif
		fetch_and_add(&borders, encode2(1, 0));
	}
	
	/** @brief Pops one element from the queue at the front end.
	 *  Must not be called concurrently with pop_front(). */
	bool pop_front(T& t)
	{
		int former_front, former_back;
		#pragma omp flush
		decode2(borders, former_front, former_back);
		while(former_front > former_back)
		{	//chance
			lcas_t former_borders = encode2(former_front, former_back);
			lcas_t new_borders = encode2(former_front - 1, former_back);
			if(compare_and_swap(&borders, former_borders, new_borders))
			{
				t = *(base + (former_front - 1) % max_size);
				return true;
			}
			#pragma omp flush
			decode2(borders, former_front, former_back);
		}
		return false;
	}
	
	/** @brief Pops one element from the queue at the front end.
	 *  Must not be called concurrently with pop_front(). */
	bool pop_back(T& t)	//queue behavior
	{
		int former_front, former_back;
		#pragma omp flush
		decode2(borders, former_front, former_back);
		while(former_front > former_back)
		{	//chance
			lcas_t former_borders = encode2(former_front, former_back);
			lcas_t new_borders = encode2(former_front, former_back + 1);
			if(compare_and_swap(&borders, former_borders, new_borders))
			{
				t = *(base + former_back % max_size);
				return true;
			}
			#pragma omp flush
			decode2(borders, former_front, former_back);
		}
		return false;
	}
	
};

}	//namespace mcstl

#undef MCSTL_VOLATILE

#endif
