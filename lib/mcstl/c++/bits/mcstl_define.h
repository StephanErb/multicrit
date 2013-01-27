/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_define.h 
 *  @brief Renames parallelized algorithmic functions, namely prefixes them with __mcstl_sequential.
 *
 *  Thus, they can be used as sequential fallback. Must be used pairwise with mcstl_undefine.h. */

#ifndef MCSTL_DEFINED

// renames for GCC bits/stl_algobase.h
#define mismatch __mcstl_sequential_mismatch
#define equal __mcstl_sequential_equal
#define lexicographical_compare __mcstl_sequential_lexicographical_compare

// renames for GCC bits/stl_algo.h
#define find __mcstl_sequential_find
#define find_if __mcstl_sequential_find_if
#define adjacent_find __mcstl_sequential_adjacent_find
#define count __mcstl_sequential_count
#define count_if __mcstl_sequential_count_if
#define search __mcstl_sequential_search
#define search_n __mcstl_sequential_search_n
#define replace __mcstl_sequential_replace
#define replace_if __mcstl_sequential_replace_if
#define replace_copy __mcstl_sequential_replace_copy
#define replace_copy_if __mcstl_sequential_replace_copy_if
#define transform __mcstl_sequential_transform
#define remove_copy __mcstl_sequential_remove_copy
#define random_shuffle __mcstl_sequential_random_shuffle
#define partition __mcstl_sequential_partition
#define sort __mcstl_sequential_sort
#define stable_sort __mcstl_sequential_stable_sort
#define merge __mcstl_sequential_merge
#define nth_element __mcstl_sequential_nth_element
#define partial_sort __mcstl_sequential_partial_sort
#define min_element __mcstl_sequential_min_element
#define max_element __mcstl_sequential_max_element
#define find_first_of __mcstl_sequential_find_first_of
#define for_each __mcstl_sequential_for_each
#define generate __mcstl_sequential_generate
#define generate_n __mcstl_sequential_generate_n
#define fill __mcstl_sequential_fill
#define fill_n __mcstl_sequential_fill_n
#define unique_copy __mcstl_sequential_unique_copy
#define set_union __mcstl_sequential_set_union
#define set_intersection __mcstl_sequential_set_intersection
#define set_difference __mcstl_sequential_set_difference
#define set_symmetric_difference __mcstl_sequential_set_symmetric_difference

// renames for GCC bits/stl_numeric.h
#define partial_sum __mcstl_sequential_partial_sum
#define accumulate __mcstl_sequential_accumulate
#define inner_product __mcstl_sequential_inner_product
#define adjacent_difference __mcstl_sequential_adjacent_difference

#define MCSTL_DEFINED

#endif
