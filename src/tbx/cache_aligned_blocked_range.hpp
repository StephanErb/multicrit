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

#ifndef __TBB_ext_cache_aligned_blocked_range_H
#define __TBB_ext_cache_aligned_blocked_range_H

#include "../options.hpp"
#include "tbb/blocked_range.h"

/**
 * Extensio
 **/
template<typename Value>
class cache_aligned_blocked_range {
public:
    //! Type of a value
    /** Called a const_iterator for sake of algorithms that need to treat a blocked_range
        as an STL container. */
    typedef Value const_iterator;

    //! Type for size of a range
    typedef size_t size_type;

    //! Construct range with default-constructed values for begin and end.
    /** Requires that Value have a default constructor. */
    cache_aligned_blocked_range() : my_end(), my_begin() {}

    //! Construct range over half-open interval [begin,end), with the given grainsize.
    cache_aligned_blocked_range( Value begin_, Value end_, size_type grainsize_=2*DCACHE_LINESIZE/sizeof(Value) ) : 
        my_end(end_), my_begin(begin_), my_grainsize(grainsize_) 
    {
        __TBB_ASSERT( my_grainsize>0, "grainsize must be positive" );
    }

    //! Beginning of range.
    const_iterator begin() const {return my_begin;}

    //! One past last value in range.
    const_iterator end() const {return my_end;}

    //! Size of the range
    /** Unspecified if end()<begin(). */
    size_type size() const {
        __TBB_ASSERT( !(end()<begin()), "size() unspecified if end()<begin()" );
        return size_type(my_end-my_begin);
    }

    //! The grain size for this range.
    size_type grainsize() const {return my_grainsize;}

    //------------------------------------------------------------------------
    // Methods that implement Range concept
    //------------------------------------------------------------------------

    //! True if range is empty.
    bool empty() const {return !(my_begin<my_end);}

    //! True if range is divisible.
    /** Unspecified if end()<begin(). */
    bool is_divisible() const {return my_grainsize<size();}

    //! Split range.  
    /** The new Range *this has the second half, the old range r has the first half. 
        Unspecified if end()<begin() or !is_divisible(). */
    cache_aligned_blocked_range( cache_aligned_blocked_range& r, tbb::split ) : 
        my_end(r.my_end),
        my_begin(do_split(r)),
        my_grainsize(r.my_grainsize)
    {}

private:
    /** NOTE: my_end MUST be declared before my_begin, otherwise the forking constructor will break. */
    Value my_end;
    Value my_begin;
    size_type my_grainsize;

    //! Auxiliary function used by forking constructor.
    /** Using this function lets us not require that Value support assignment or default construction. */
    static Value do_split( cache_aligned_blocked_range& r ) {
        __TBB_ASSERT( r.is_divisible(), "cannot split blocked_range that is not divisible" );
        Value middle = r.my_begin + (r.my_end-r.my_begin)/2u;
        if (r.size() > 2 * DCACHE_LINESIZE) {
            middle = align(middle);
        }
        r.my_end = middle;
        return middle;
    }

    static inline size_t align(size_t numToRound) { 
        const unsigned int remainder = numToRound % DCACHE_LINESIZE;
        if (remainder == 0) {
            return numToRound;
        }
        return numToRound + DCACHE_LINESIZE - remainder;
    } 
};

#endif 
