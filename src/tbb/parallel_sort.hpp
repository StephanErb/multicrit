/*
    Copyright 2005-2013 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef __TBB_ext_parallel_sort_H
#define __TBB_ext_parallel_sort_H

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include <algorithm>
#include <iterator>
#include <functional>
#include "../radix_sort.hpp"


//! Range used in quicksort to split elements into subranges based on a value.
/** The split operation selects a splitter and places all elements less than or equal 
    to the value in the first range and the remaining elements in the second range.
    @ingroup algorithms */
template<typename RandomAccessIterator, typename Compare>
class quick_sort_range {

    inline size_t median_of_three(const RandomAccessIterator &array, size_t l, size_t m, size_t r) const {
        return comp(array[l], array[m]) ? ( comp(array[m], array[r]) ? m : ( comp( array[l], array[r]) ? r : l ) ) 
                                        : ( comp(array[r], array[m]) ? m : ( comp( array[r], array[l] ) ? r : l ) );
    }

    inline size_t pseudo_median_of_nine( const RandomAccessIterator &array, const quick_sort_range &range ) const {
        size_t offset = range.size/8u;
        return median_of_three(array, 
                               median_of_three(array, 0, offset, offset*2),
                               median_of_three(array, offset*3, offset*4, offset*5),
                               median_of_three(array, offset*6, offset*7, range.size - 1) );

    }

public:

    static const size_t grainsize = 500;
    const Compare &comp;
    RandomAccessIterator begin;
    size_t size;

    quick_sort_range( RandomAccessIterator begin_, size_t size_, const Compare &comp_ ) :
        comp(comp_), begin(begin_), size(size_) {}

    bool empty() const {return size==0;}
    bool is_divisible() const {return size>=grainsize;}

    quick_sort_range( quick_sort_range& range, tbb::split ) : comp(range.comp) {
        RandomAccessIterator array = range.begin;
        RandomAccessIterator key0 = range.begin; 
        size_t m = pseudo_median_of_nine(array, range);
        if (m) std::swap ( array[0], array[m] );

        size_t i=0;
        size_t j=range.size;
        // Partition interval [i+1,j-1] with key *key0.
        for(;;) {
            __TBB_ASSERT( i<j, NULL );
            // Loop must terminate since array[l]==*key0.
            do {
                --j;
                __TBB_ASSERT( i<=j, "bad ordering relation?" );
            } while( comp( *key0, array[j] ));
            do {
                __TBB_ASSERT( i<=j, NULL );
                if( i==j ) goto partition;
                ++i;
            } while( comp( array[i],*key0 ));
            if( i==j ) goto partition;
            std::swap( array[i], array[j] );
        }
partition:
        // Put the partition key were it belongs
        std::swap( array[j], *key0 );
        // array[l..j) is less or equal to key.
        // array(j..r) is greater or equal to key.
        // array[j] is equal to key
        i=j+1;
        begin = array+i;
        size = range.size-i;
        range.size = j;
    }
};

template<typename RandomAccessIterator, typename Compare>
struct quick_sort_body {
    void operator()( const quick_sort_range<RandomAccessIterator,Compare>& range ) const {
        std::sort( range.begin, range.begin + range.size, range.comp );
    }
};

template<typename RandomAccessIterator, typename Compare, typename KeyExtractor>
struct radix_sort_body {
    const KeyExtractor& key;
    radix_sort_body( const KeyExtractor& key_) : key(key_ ) {};
    void operator()( const quick_sort_range<RandomAccessIterator,Compare>& range) const {
        radix_sort( range.begin, range.size, key );
    }
};

//! Sorts the data in [begin,end) using the given comparator 
/** The compare function object is used for all comparisons between elements during sorting.
    The compare object must define a bool operator() function.
    @ingroup algorithms **/
template<typename RandomAccessIterator, typename Compare, typename Partitioner>
void parallel_sort(RandomAccessIterator begin, RandomAccessIterator end, const Compare& comp, Partitioner& partitioner) { 
    const int min_parallel_size = 500; 
    if( end > begin ) {
        if (end - begin < min_parallel_size) { 
            std::sort(begin, end, comp);
        } else {
            tbb::parallel_for( quick_sort_range<RandomAccessIterator,Compare>(begin, end-begin, comp), 
                      quick_sort_body<RandomAccessIterator,Compare>(), partitioner );
        }
    }
}


template<typename T, typename KeyExtractor, typename Partitioner>
void parallel_radix_sort(T* data , size_t size, const KeyExtractor& key, Partitioner& partitioner) { 
    const unsigned int min_parallel_size = 500; 
    if( size > 0 ) {
        if (size < min_parallel_size) { 
            radix_sort(data, size, key);
        } else {
            auto key_compare = [key](const T& a, const T& b){ return key(a) < key(b); };
            tbb::parallel_for( quick_sort_range<T*, decltype(key_compare)>(data, size, key_compare), 
                radix_sort_body<T*,decltype(key_compare),KeyExtractor>(key), partitioner);
        }
    }
}


#endif

