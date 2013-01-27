/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_undefine.h 
 *  @brief Undefines renamings from mcstl_define.h.
 *
 *  Must be used pairwise with mcstl_define.h. */

#ifdef MCSTL_DEFINED

// renames for GCC bits/stl_algobase.h
#undef mismatch
#undef equal
#undef lexicographical_compare

// renames for GCC bits/stl_algo.h
#undef find
#undef find_if
#undef adjacent_find
#undef count
#undef count_if
#undef search
#undef search_n
#undef replace
#undef replace_if
#undef replace_copy
#undef replace_copy_if
#undef transform
#undef remove_copy
#undef random_shuffle
#undef partition
#undef sort
#undef stable_sort
#undef merge
#undef nth_element
#undef partial_sort
#undef min_element
#undef max_element
#undef find_first_of
#undef for_each
#undef generate
#undef generate_n
#undef fill
#undef fill_n
#undef unique_copy
#undef set_union
#undef set_intersection
#undef set_difference
#undef set_symmetric_difference

// renames for GCC bits/stl_numeric.h
#undef partial_sum
#undef accumulate
#undef inner_product
#undef adjacent_difference

#undef MCSTL_DEFINED

#endif
