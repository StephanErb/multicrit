 /***************************************************************************
 *   Copyright (C) 2007 by Marius Elvert and Felix Bondarenko               *
 *   {marius.elvert felix.b} @ira.uka.de                                    *
 *   Distributed under the Boost Software License, Version 1.0.             *
 *   (See accompanying file LICENSE_1_0.txt or copy at                      *
 *   http://www.boost.org/LICENSE_1_0.txt)                                  *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/   *
 ***************************************************************************/

/**
 * @file mcstl_set_operations.h
 * @brief Parallel implementations of set operations for random-access iterators.
 */

#ifndef _MCSTL_SET_ALGORITHM_
#define _MCSTL_SET_ALGORITHM_

#include <omp.h>

#include <bits/mcstl_settings.h>
#include <bits/mcstl_multiseq_selection.h>

namespace mcstl {


template< typename InputIterator, typename OutputIterator > inline
OutputIterator copy_tail( std::pair< InputIterator, InputIterator > b,
	std::pair< InputIterator, InputIterator > e, OutputIterator r )
{
	if ( b.first != e.first ) 
	{
		do {
			*r++ = *b.first++;
		}
		while ( b.first != e.first );
	}
	else
	{
		while ( b.second != e.second )
			*r++ = *b.second++;
	}
	
	return r;
}
	
template< typename InputIterator, typename OutputIterator, typename Comparator >
struct symmetric_difference_func
{
	typedef typename std::iterator_traits< InputIterator >::difference_type DiffType;
	typedef typename std::pair< InputIterator, InputIterator > iterator_pair;

	symmetric_difference_func( Comparator c ) : comp( c ) {}
	
	Comparator comp;
	
	inline OutputIterator invoke( InputIterator a, InputIterator b,
						   InputIterator c, InputIterator d,
						   OutputIterator r ) const
	{		
		while ( a != b && c != d )
		{
			if ( comp( *a, *c ) )
			{ *r = *a; ++a; ++r; }
			else if ( comp( *c, *a ) )
			{ *r = *c; ++c; ++r; }
			else
			{ ++a; ++c; }
		}
		
		return std::copy( c, d, std::copy( a, b, r ) );
	}
	
	inline DiffType count( InputIterator a, InputIterator b, 
								  InputIterator c, InputIterator d ) const
	{
		DiffType counter = 0;
					
		while ( a != b && c != d )
		{
			if ( comp( *a, *c ) )
			{
				++a;
				++counter;
			}
			else if ( comp( *c, *a ) )
			{
				++c;
				++counter;
			}
			else
			{
				++a;
				++c;
			}
		}
		
		return counter + (b - a) + (d - c);
	}
	
	inline OutputIterator first_empty( InputIterator c, InputIterator d, OutputIterator out) const
	{
		return std::copy(c, d, out);
	}
	
	inline OutputIterator second_empty( InputIterator a, InputIterator b, OutputIterator out) const
	{
		return std::copy(a, b, out);
	}
	
};
	

template< typename InputIterator, typename OutputIterator, typename Comparator >
struct difference_func
{
	typedef typename std::iterator_traits< InputIterator >::difference_type DiffType;
	typedef typename std::pair< InputIterator, InputIterator > iterator_pair;

	difference_func( Comparator c ) : comp( c ) {}
	
	Comparator comp;
	
	inline OutputIterator invoke( InputIterator a, InputIterator b,
								  InputIterator c, InputIterator d,
								  OutputIterator r ) const
	{		
		while ( a != b && c != d )
		{
			if ( comp( *a, *c ) )
			{ *r = *a; ++a; ++r; }
			else if ( comp( *c, *a ) )
			{ ++c; }
			else
			{ ++a; ++c; }
		}
		
		return std::copy( a, b, r );
	}
	
	inline DiffType count( InputIterator a, InputIterator b,
								  InputIterator c, InputIterator d ) const
	{
		DiffType counter = 0;
					
		while ( a != b && c != d )
		{
			if ( comp( *a, *c ) )
			{ ++a; ++counter; }
			else if ( comp( *c, *a ) )
			{ ++c; }
			else
			{ ++a; ++c; }
		}
		
		return counter + (b - a);
	}

	inline OutputIterator first_empty( InputIterator c, InputIterator d, OutputIterator out) const
	{
		return out;
	}
	
	inline OutputIterator second_empty( InputIterator a, InputIterator b, OutputIterator out) const
	{
		return std::copy(a, b, out);
	}
	
};
	

template< typename InputIterator, typename OutputIterator, typename Comparator >
struct intersection_func
{
	typedef typename std::iterator_traits< InputIterator >::difference_type DiffType;
	typedef typename std::pair< InputIterator, InputIterator > iterator_pair;

	intersection_func( Comparator c ) : comp( c ) {}
	
	Comparator comp;
	
	inline OutputIterator invoke( InputIterator a, InputIterator b,
								  InputIterator c, InputIterator d,
								  OutputIterator r ) const
	{		
		while ( a != b && c != d )
		{
			if ( comp( *a, *c ) )
			{ ++a; }
			else if ( comp( *c, *a ) )
			{ ++c; }
			else
			{ *r = *a; ++a; ++c; ++r; }
		}
		
		return r;
	}
	
	inline DiffType count( InputIterator a, InputIterator b, 
								  InputIterator c, InputIterator d ) const
	{
		DiffType counter = 0;
					
		while ( a != b && c != d )
		{
			if ( comp( *a, *c ) )
			{ ++a; }
			else if ( comp( *c, *a ) )
			{ ++c; }
			else
			{ ++a; ++c; ++counter; }
		}
		
		return counter;
	}

	inline OutputIterator first_empty( InputIterator c, InputIterator d, OutputIterator out) const
	{
		return out;
	}
	
	inline OutputIterator second_empty( InputIterator a, InputIterator b, OutputIterator out) const
	{
		return out;
	}
	
};

template< class InputIterator, class OutputIterator, class Comparator >
struct union_func
{
	typedef typename std::iterator_traits< InputIterator >::difference_type DiffType;

	union_func( Comparator c ) : comp( c ) {}
	
	Comparator comp;
	
	inline OutputIterator invoke(	InputIterator a, const InputIterator b,
									InputIterator c, const InputIterator d,
									OutputIterator r ) const
	{
		while ( a != b && c != d )
		{
			if ( comp(*a, *c) )
			{ *r = *a; ++a;	}
			else if ( comp(*c, *a) )
			{ *r = *c; ++c;	}
			else
			{ *r = *a; ++a;	++c; }

			++r;
		}
		return std::copy( c, d, std::copy( a, b, r ) );
	}
	
	inline DiffType count( 	InputIterator a, const InputIterator b,
									InputIterator c, const InputIterator d ) const
	{
		DiffType counter = 0;
					
		while ( a != b && c != d )
		{
			if ( comp( *a, *c ) )
			{ ++a; }
			else if ( comp( *c, *a ) )
			{ ++c; }
			else
			{ ++a; ++c; }
				
			++counter;
		}
			
		counter += ( b - a );
		counter += ( d - c );
		
		return counter;
	}
	
	inline OutputIterator first_empty( InputIterator c, InputIterator d, OutputIterator out) const
	{
		return std::copy(c, d, out);
	}
	
	inline OutputIterator second_empty( InputIterator a, InputIterator b, OutputIterator out) const
	{
		return std::copy(a, b, out);
	}
	
};

template< typename InputIterator, typename OutputIterator, typename Operation >
OutputIterator parallel_set_operation( InputIterator begin1, InputIterator end1,
			InputIterator begin2, InputIterator end2,
			OutputIterator result, Operation op )
{
	MCSTL_CALL(( end1 - begin1 ) + ( end2 - begin2 ))

	typedef typename std::pair< InputIterator, InputIterator > iterator_pair;
	typedef typename std::iterator_traits< InputIterator >::difference_type DiffType;
	
	if(begin1 == end1)
		return op.first_empty(begin2, end2, result);
		
	if(begin2 == end2)
		return op.second_empty(begin1, end1, result);

	const DiffType size = ( end1 - begin1 ) + ( end2 - begin2 );

	thread_index_t num_threads = std::min<DiffType>(std::min( end1 - begin1, end2 - begin2 ), SETTINGS::num_threads);
	
	DiffType borders[num_threads + 2];
	equally_split(size, num_threads + 1, borders);

	const iterator_pair sequence[ 2 ] = { std::make_pair( begin1, end1 ), std::make_pair( begin2, end2 ) } ;

	iterator_pair block_begins[num_threads + 1];
	block_begins[0] = std::make_pair(begin1, begin2);	//very start
	DiffType length[num_threads];

	OutputIterator return_value = result;

	#pragma omp parallel num_threads(num_threads)
	{
		Timing<inactive_tag> t;
		
		t.tic();
	
		InputIterator offset[ 2 ];	//result from multiseq_partition
		const int iam = omp_get_thread_num();
		
		const DiffType rank = borders[iam + 1];
		
		multiseq_partition(sequence, sequence + 2, rank, offset, op.comp );

		if ( offset[ 0 ] != begin1 && offset[ 1 ] != end2		//allowed to read?
			&& !op.comp( *(offset[ 0 ] - 1), *offset[ 1 ] )		//together
			&& !op.comp( *offset[ 1 ], *(offset[ 0 ] - 1) ))	//*(offset[ 0 ] - 1) == *offset[ 1 ]
		{
			// avoid split between globally equal elements
			--offset[ 0 ];	//move one to front in first sequence
		}

		iterator_pair block_end = block_begins[ iam + 1 ] = iterator_pair( offset[ 0 ], offset[ 1 ] );

		t.tic();

		// make sure all threads have their block_begin result written out
		#pragma omp barrier
		
		t.tic();

		iterator_pair block_begin = block_begins[ iam ];

		// begin working for the first block, while the others except the last start to count
		if ( iam == 0)
		{
			// the first thread can copy already
			length[ iam ] = op.invoke( block_begin.first, block_end.first, block_begin.second, block_end.second, result ) - result;
		}
		else
		{
			length[ iam ] = op.count( block_begin.first, block_end.first,
				block_begin.second, block_end.second );
		}
		
		t.tic();
				
		// make sure everyone wrote their lengths
		#pragma omp barrier
		
		t.tic();
				
		OutputIterator r = result;
		
		if ( iam == 0 )
		{

			// do the last block
			for ( int i = 0; i < num_threads; ++i ) 
				r += length[ i ];

			block_begin = block_begins[ num_threads ];

			// return the result iterator of the last block
			return_value = op.invoke( block_begin.first, end1, block_begin.second, end2, r );

		}
		else
		{
			for ( int i = 0; i < iam; ++i ) 
				r += length[ i ];
							
			//reset begins for copy pass
			op.invoke( 	block_begin.first, block_end.first,
						block_begin.second, block_end.second, r );
		}
		
		t.tic();
		
		t.print();
	}
	
	

	return return_value;
}


template< typename InputIterator, typename OutputIterator, typename Comparator >
OutputIterator parallel_set_union( InputIterator begin1, InputIterator end1,
						  InputIterator begin2, InputIterator end2,
						  OutputIterator result, Comparator comp )
{
	return parallel_set_operation( begin1, end1, begin2, end2, result,
			union_func< InputIterator, OutputIterator, Comparator >( comp ) );
}

template< typename InputIterator, typename OutputIterator, typename Comparator >
OutputIterator parallel_set_intersection( InputIterator begin1, InputIterator end1,
								  InputIterator begin2, InputIterator end2,
								  OutputIterator result, Comparator comp )
{
	return parallel_set_operation( begin1, end1, begin2, end2, result,
			intersection_func< InputIterator, OutputIterator, Comparator >( comp ) );
}


template< typename InputIterator, typename OutputIterator >
OutputIterator set_intersection( 
	InputIterator begin1, InputIterator end1, InputIterator begin2, InputIterator end2,
	OutputIterator result )
{
	typedef typename std::iterator_traits< InputIterator >::value_type value_type;

	return set_intersection( begin1, end1, begin2, end2, result, std::less< value_type >() );
}

template< typename InputIterator, typename OutputIterator, typename Comparator >
OutputIterator parallel_set_difference( InputIterator begin1, InputIterator end1,
								InputIterator begin2, InputIterator end2,
								OutputIterator result, Comparator comp )
{
	return parallel_set_operation( begin1, end1, begin2, end2, result,
			difference_func< InputIterator, OutputIterator, Comparator >( comp ) );
}

template< typename InputIterator, typename OutputIterator, typename Comparator >
OutputIterator parallel_set_symmetric_difference( 
	InputIterator begin1, InputIterator end1, InputIterator begin2, InputIterator end2,
	OutputIterator result, Comparator comp )
{
	return parallel_set_operation( begin1, end1, begin2, end2, result,
						  symmetric_difference_func< InputIterator, OutputIterator, Comparator >( comp ) );
}

}

#endif // _MCSTL_SET_ALGORITHM_








