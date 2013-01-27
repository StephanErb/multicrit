/***************************************************************************
 *   Copyright (C) 2007 by Leonor Frias Moya, Johannes Singler             *
 *   lfrias@lsi.upc.edu, singler@ira.uka.de                                 *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_list_partition.h
 *  @brief Functionality to split sequence referenced by only input iterators. */

#ifndef _MCSTL_LIST_PARTITION_H
#define _MCSTL_LIST_PARTITION_H 1

#include <mcstl.h>
#include <vector>

namespace mcstl
{

/** @brief Shrinks and doubles the ranges.
 *  @param os_starts Start positions worked on (oversampled).
 *  @param count_to_two Counts up to 2.
 *  @param range_length Current length of a chunk.
 *  @param make_twice Whether the @c os_starts is allowed to be grown or not */
template <typename InputIterator>
void shrink_and_double(std::vector<InputIterator>& os_starts, size_t& count_to_two, size_t& range_length, const bool make_twice)
{	
	++count_to_two;
	if (not make_twice or count_to_two < 2)
	{
		shrink(os_starts, count_to_two, range_length);
	}
	else
	{
		os_starts.resize((os_starts.size() - 1) * 2 + 1);
		count_to_two = 0;
	}
}

/** @brief Combines two ranges into one and thus halves the number of ranges.
 *  @param os_starts Start positions worked on (oversampled).
 *  @param count_to_two Counts up to 2.
 *  @param range_length Current length of a chunk. */
template <typename InputIterator>
void shrink(std::vector<InputIterator>& os_starts, size_t& count_to_two, size_t& range_length)
{
	for (typename std::vector<InputIterator>::size_type i = 0; i <= (os_starts.size() / 2); ++i)
	{
		os_starts[i] = os_starts[i * 2];
	}
	range_length *= 2;
}

/** @brief Splits a sequence given by input iterators into parts of almost equal size
 *
 *  The function needs only one pass over the sequence.
 *  @param begin Begin iterator of input sequence.
 *  @param end End iterator of input sequence.
 *  @param starts Start iterators for the resulting parts, dimension @c num_parts+1. For convenience, @c starts @c [num_parts] contains the end iterator of the sequence.
 *  @param lengths Length of the resulting parts.
 *  @param num_parts Number of parts to split the sequence into.
 *  @param f Functor to be applied to each element by traversing it
 *  @param oversampling Oversampling factor. If 0, then the partitions will differ in at most @f$ \sqrt{\mathrm{end} - \mathrm{begin}} @f$ elements. Otherwise, the ratio between the longest and the shortest part is bounded by @f$ 1/(\mathrm{oversampling} \cdot \mathrm{num\_parts}) @f$.
 *  @return Length of the whole sequence. */
template<typename InputIterator, typename FunctorType>
size_t 
list_partition(
	const InputIterator begin,
	const InputIterator end,
	InputIterator* starts,
	size_t* lengths,
	const int num_parts,
	FunctorType& f, 
	int oversampling = 0)
{
	bool make_twice = false;
	//according to the oversampling factor, the resizing algorithm is chosen.
	if (oversampling == 0)
	{
		make_twice = true;
		oversampling = 1;
	}

	std::vector<InputIterator> os_starts(2 * oversampling * num_parts + 1);

	os_starts[0]= begin;
	InputIterator prev = begin, it = begin;
	size_t dist_limit = 0, dist = 0;
	size_t cur = 1, next = 1;
	size_t range_length = 1;
	size_t count_to_two = 0;
	while (it != end){
		cur = next;
		for (; cur < os_starts.size() and it != end; ++cur){
			for(dist_limit += range_length; dist < dist_limit and it != end; ++dist){
				f(it);
				++it;
			}
			os_starts[cur] = it;
		}
		// we MUST compare for end and not cur < os_starts.size() , because cur could be == os_starts.size() as well
		if(it == end)
			break;

		shrink_and_double(os_starts, count_to_two, range_length, make_twice);
		next = os_starts.size()/2 + 1;
	}

	//calculation of the parts (one must be extracted from current because the partition beginning at end, consists only of itself)
	size_t size_part = (cur - 1) / num_parts;
	int size_greater = static_cast<int>((cur - 1) % num_parts);
	starts[0] = os_starts[0];

	size_t index = 0;
	//smallest partitions
	for (int i = 1; i < (num_parts + 1 - size_greater); ++i) {
		lengths[i-1] =  size_part * range_length;
		index += size_part;
		starts[i] = os_starts[index];
	}
	//biggest partitions
	for (int i = num_parts + 1 - size_greater; i <= num_parts; ++i) {
		lengths[i-1] =  (size_part+1) * range_length;
		index += (size_part+1);
		starts[i] = os_starts[index];
	}
	//correction of the end size (the end iteration has not finished)
	lengths[num_parts - 1] -= (dist_limit - dist);

	return dist;
}

}

#endif
