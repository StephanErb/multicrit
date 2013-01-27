 /***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                 *
 *   singler@ira.uka.de                                                     *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_random_shuffle.h
 *  @brief Parallel implementation of std::random_shuffle(). */

#ifndef _MCSTL_RANDOM_SHUFFLE_H
#define _MCSTL_RANDOM_SHUFFLE_H 1

#include <limits>

#include <bits/mcstl_basic_iterator.h>
#include <mod_stl/stl_algo.h>

#include <mcstl.h>
#include <bits/mcstl_base.h>
#include <bits/mcstl_random_number.h>
#include <meta/mcstl_timing.h>

namespace mcstl
{

/** @brief Type to hold the index of a bin.
 *
 *  Since many variables of this type are allocated, it should be chosen as small as possible. */
typedef unsigned short bin_index;

/** @brief Data known to every thread participating in mcstl::parallel_random_shuffle(). */
template<typename RandomAccessIterator>
struct DRandomShufflingGlobalData
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;

	/** @brief Begin iterator of the source. */
	RandomAccessIterator& source;
	/** @brief Temporary arrays for each thread. */
	ValueType** temporaries;
	/** @brief Two-dimensional array to hold the thread-bin distribution. 
	 * 
	 *  Dimensions (num_threads + 1) x (num_bins + 1). */
	DiffType** dist;
	/** @brief Start indexes of the threads' chunks. */
	DiffType* starts;
	/** @brief Number of the thread that will further process the corresponding bin. */
	thread_index_t* bin_proc;
	/** @brief Number of bins to distribute to. */
	int num_bins;
	/** @brief Number of bits needed to address the bins. */
	int num_bits;

	/** @brief Constructor. */
	DRandomShufflingGlobalData(RandomAccessIterator& _source) : source(_source) { }
};

/** @brief Local data for a thread participating in mcstl::parallel_random_shuffle(). */
template<typename RandomAccessIterator, typename RandomNumberGenerator>
struct DRSSorterPU
{
	/** @brief Number of threads participating in total. */
	int num_threads;
	/** @brief Number of owning thread. */
	int iam;
	/** @brief Begin index for bins taken care of by this thread. */
	bin_index bins_begin;
	/** @brief End index for bins taken care of by this thread. */
	bin_index bins_end;
	/** @brief Random seed for this thread. */
	uint32 seed;
	/** @brief Pointer to global data. */
	DRandomShufflingGlobalData<RandomAccessIterator>* sd;
};

/** @brief Generate a random number in @c [0,2^logp).
 *  @param logp Logarithm (basis 2) of the upper range bound.
 *  @param rng Random number generator to use. */
template<typename RandomNumberGenerator>
inline int random_number_pow2(int logp, RandomNumberGenerator& rng)
{
	return rng.genrand_bits(logp);
}

/** @brief Random shuffle code executed by each thread. 
 *  @param pus Arary of thread-local data records. */
template<typename RandomAccessIterator, typename RandomNumberGenerator>
inline void parallel_random_shuffle_drs_pu(DRSSorterPU<RandomAccessIterator, RandomNumberGenerator>* pus)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;

	Timing<inactive_tag> t;
	t.tic();
	
	DRSSorterPU<RandomAccessIterator, RandomNumberGenerator>* d = &pus[omp_get_thread_num()];
	DRandomShufflingGlobalData<RandomAccessIterator>* sd = d->sd;
	thread_index_t iam = d->iam;

	//indexing: dist[bin][processor]
	
	DiffType length = sd->starts[iam + 1] - sd->starts[iam];
	bin_index* oracles = new bin_index[length];
	DiffType* dist = new DiffType[sd->num_bins + 1];
	bin_index* bin_proc = new bin_index[sd->num_bins];
	ValueType** temporaries = new ValueType*[d->num_threads];
	
	//compute oracles and count appearances
	for(bin_index b = 0; b < sd->num_bins + 1; b++)
		dist[b] = 0;
	int num_bits = sd->num_bits;
	
	random_number rng(d->seed);

	//first main loop
	for(DiffType i = 0; i < length; i++)
	{
		bin_index oracle = random_number_pow2(num_bits, rng);
		oracles[i] = oracle;
		dist[oracle + 1]++;	//to allow prefix (partial) sum
	} 
	
	for(bin_index b = 0; b < sd->num_bins + 1; b++)
		sd->dist[b][iam + 1] = dist[b];
	
	t.tic();

	#pragma omp barrier
	
	t.tic();

	#pragma omp single
	{
		for(bin_index s = 0; s < sd->num_bins; s++)
			partial_sum(sd->dist[s + 1], sd->dist[s + 1] + d->num_threads + 1, sd->dist[s + 1]);	//sum up bins, sd->dist[s + 1][d->num_threads] now contains the total number of items in bin s
	}
	
	#pragma omp barrier
	
	t.tic();
	
	sequence_index_t offset = 0, global_offset = 0;
	for(bin_index s = 0; s < d->bins_begin; s++)
		global_offset += sd->dist[s + 1][d->num_threads];
	
	#pragma omp barrier

	for(bin_index s = d->bins_begin; s < d->bins_end; s++)
	{
		for(int t = 0; t < d->num_threads + 1; t++)
			sd->dist[s + 1][t] += offset;
		offset = sd->dist[s + 1][d->num_threads];
	}

	sd->temporaries[iam] = new ValueType[offset];

	t.tic();

	#pragma omp barrier

	t.tic();
		
	//draw local copies to avoid false sharing
	for(bin_index b = 0; b < sd->num_bins + 1; b++)
		dist[b] = sd->dist[b][iam];
	for(bin_index b = 0; b < sd->num_bins; b++)
		bin_proc[b] = sd->bin_proc[b];
	for(thread_index_t t = 0; t < d->num_threads; t++)
		temporaries[t] = sd->temporaries[t];
	
	RandomAccessIterator source = sd->source;
	DiffType start = sd->starts[iam];
	
	//distribute according to oracles, second main loop
	for(DiffType i = 0; i < length; i++)
	{
		bin_index target_bin = oracles[i];
		thread_index_t target_p = bin_proc[target_bin];
		temporaries[target_p][dist[target_bin + 1]++] = *(source + i + start);
		//last column [d->num_threads] stays unchanged
	}
	
	delete[] oracles;
	delete[] dist;
	delete[] bin_proc;
	delete[] temporaries;

	t.tic();

	#pragma omp barrier

	t.tic();
	
	//shuffle bins internally
	for(bin_index b = d->bins_begin; b < d->bins_end; b++)
	{
		ValueType* begin = sd->temporaries[iam] + ((b == d->bins_begin) ? 0 : sd->dist[b][d->num_threads]), 
				* end = sd->temporaries[iam] + sd->dist[b + 1][d->num_threads];
		sequential_random_shuffle(begin, end, rng);
		std::copy(begin, end, sd->source + global_offset + ((b == d->bins_begin) ? 0 : sd->dist[b][d->num_threads]));
	}
	
	delete[] sd->temporaries[iam];
		
	t.tic();
	
	t.print();
}

/** @brief Round up to the next greater power of 2.
 *  @param x Integer to round up */
template<typename T>
T round_up_to_pow2(T x)
{
	if(x <= 1)
		return 1;
	else
		return (T)1 << (log2(x - 1) + 1);
}

/** @brief Main parallel random shuffle step.
 *  @param begin Begin iterator of sequence.
 *  @param end End iterator of sequence.
 *  @param n Length of sequence.
 *  @param num_threads Number of threads to use.
 *  @param rng Random number generator to use.
 */
template<typename RandomAccessIterator, typename RandomNumberGenerator>
inline void
parallel_random_shuffle_drs(RandomAccessIterator begin, RandomAccessIterator end,
			typename std::iterator_traits<RandomAccessIterator>::difference_type n,
			int num_threads,
			RandomNumberGenerator& rng)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	MCSTL_CALL(n)
	
	if(num_threads > n)
		num_threads = static_cast<thread_index_t>(n);

	bin_index num_bins, num_bins_cache;
	
#if MCSTL_RANDOM_SHUFFLE_CONSIDER_L1
	//try the L1 cache first

	//must fit into L1
	num_bins_cache = std::max((DiffType)1, (DiffType)(n / (SETTINGS::L1_cache_size_lb / sizeof(ValueType))));
	num_bins_cache = round_up_to_pow2(num_bins_cache);
	//no more buckets than TLB entries, power of 2
	num_bins = std::min(n, (DiffType)num_bins_cache);	//power of 2 and at least one element per bin, at most the TLB size
#if MCSTL_RANDOM_SHUFFLE_CONSIDER_TLB
	num_bins = std::min((DiffType)SETTINGS::TLB_size / 2, num_bins);	//2 TLB entries needed per bin
#endif
	num_bins = round_up_to_pow2(num_bins);

	if(num_bins < num_bins_cache)
	{	
#endif
		//now try the L2 cache
		//must fit into L2
		num_bins_cache = static_cast<bin_index>(std::max((DiffType)1, (DiffType)(n / (SETTINGS::L2_cache_size / sizeof(ValueType)))));
		num_bins_cache = round_up_to_pow2(num_bins_cache);
		//no more buckets than TLB entries, power of 2
		num_bins = static_cast<bin_index>(std::min(n, (DiffType)num_bins_cache));	//power of 2 and at least one element per bin, at most the TLB size
#if MCSTL_RANDOM_SHUFFLE_CONSIDER_TLB
		num_bins = std::min((DiffType)SETTINGS::TLB_size / 2, num_bins);	//2 TLB entries needed per bin
#endif
		num_bins = round_up_to_pow2(num_bins);
#if MCSTL_RANDOM_SHUFFLE_CONSIDER_L1
	}
#endif

	num_threads = std::min((bin_index)num_threads, (bin_index)num_bins);

	if(num_threads <= 1)
		return sequential_random_shuffle(begin, end, rng);

	DRandomShufflingGlobalData<RandomAccessIterator> sd(begin);

	DRSSorterPU<RandomAccessIterator, random_number >* pus = new DRSSorterPU<RandomAccessIterator, random_number >[num_threads];
	
	sd.temporaries = new ValueType*[num_threads];
	//sd.oracles = new bin_index[n];
	sd.dist = new DiffType*[num_bins + 1];
	sd.bin_proc = new thread_index_t[num_bins];
	for(bin_index b = 0; b < num_bins + 1; b++)
		sd.dist[b] = new DiffType[num_threads + 1];
	for(bin_index b = 0; b < (num_bins + 1); b++)
	{
		sd.dist[0][0] = 0;
		sd.dist[b][0] = 0;
	}
	DiffType* starts = sd.starts = new DiffType[num_threads + 1];
	int bin_cursor = 0;
	sd.num_bins = num_bins;
	sd.num_bits = log2(num_bins);

	DiffType chunk_length = n / num_threads, split = n % num_threads, start = 0;
	int bin_chunk_length = num_bins / num_threads, bin_split = num_bins % num_threads;
	for(int i = 0; i < num_threads; i++)
	{
		starts[i] = start;
		start += (i < split) ? (chunk_length + 1) : chunk_length;
		int j = pus[i].bins_begin = bin_cursor;
		
		bin_cursor += (i < bin_split) ? (bin_chunk_length + 1) : bin_chunk_length;	//range of bins for this processor
		pus[i].bins_end = bin_cursor;
		for(; j < bin_cursor; j++)
			sd.bin_proc[j] = i;
		pus[i].num_threads = num_threads;
		pus[i].iam = i;
		pus[i].seed = rng(std::numeric_limits<uint32>::max());
		pus[i].sd = &sd;
	}
	starts[num_threads] = start;

	//now shuffle in parallel
	
	#pragma omp parallel num_threads(num_threads)
	parallel_random_shuffle_drs_pu(pus);

	delete[] starts;
	delete[] sd.bin_proc;
	for(int s = 0; s < (num_bins + 1); s++)
		delete[] sd.dist[s];
	delete[] sd.dist;
	delete[] sd.temporaries;

	delete[] pus;
}

/** @brief Sequential cache-efficient random shuffle.
 *  @param begin Begin iterator of sequence.
 *  @param end End iterator of sequence.
 *  @param rng Random number generator to use.
 */
template<typename RandomAccessIterator, typename RandomNumberGenerator>
inline void
sequential_random_shuffle(	RandomAccessIterator begin,
				RandomAccessIterator end,
				RandomNumberGenerator& rng)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	DiffType n = end - begin;

	bin_index num_bins, num_bins_cache;

#if MCSTL_RANDOM_SHUFFLE_CONSIDER_L1
	//try the L1 cache first

	//must fit into L1
	num_bins_cache = std::max((DiffType)1, (DiffType)(n / (SETTINGS::L1_cache_size_lb / sizeof(ValueType))));
	num_bins_cache = round_up_to_pow2(num_bins_cache);
	//no more buckets than TLB entries, power of 2
	num_bins = std::min(n, (DiffType)num_bins_cache);	//power of 2 and at least one element per bin, at most the TLB size
#if MCSTL_RANDOM_SHUFFLE_CONSIDER_TLB
	num_bins = std::min((DiffType)SETTINGS::TLB_size / 2, num_bins);	//2 TLB entries needed per bin
#endif
	num_bins = round_up_to_pow2(num_bins);

	if(num_bins < num_bins_cache)
	{	
#endif
		//now try the L2 cache
		//must fit into L2
		num_bins_cache = static_cast<bin_index>(std::max((DiffType)1, (DiffType)(n / (SETTINGS::L2_cache_size / sizeof(ValueType)))));
		num_bins_cache = round_up_to_pow2(num_bins_cache);
		//no more buckets than TLB entries, power of 2
		num_bins = static_cast<bin_index>(std::min(n, (DiffType)num_bins_cache));	//power of 2 and at least one element per bin, at most the TLB size
#if MCSTL_RANDOM_SHUFFLE_CONSIDER_TLB
		num_bins = std::min((DiffType)SETTINGS::TLB_size / 2, num_bins);	//2 TLB entries needed per bin
#endif
		num_bins = round_up_to_pow2(num_bins);
#if MCSTL_RANDOM_SHUFFLE_CONSIDER_L1
	}
#endif

	int num_bits = log2(num_bins);

	if(num_bins > 1)
	{
		ValueType* target = new ValueType[n];
		bin_index* oracles = new bin_index[n];
		DiffType* dist0 = new DiffType[num_bins + 1], * dist1 = new DiffType[num_bins + 1];
	
		for(int b = 0; b < num_bins + 1; b++)
			dist0[b] = 0;
	
		Timing<inactive_tag> t;
		t.tic();
		
		random_number bitrng(rng(0xFFFFFFFF));
		
		for(DiffType i = 0; i < n; i++)
		{
			bin_index oracle = random_number_pow2(num_bits, bitrng);
			oracles[i] = oracle;
			dist0[oracle + 1]++;	//to allow prefix (partial) sum
		}
	
		t.tic();
		
		partial_sum(dist0, dist0 + num_bins + 1, dist0);	//sum up bins
	
		for(int b = 0; b < num_bins + 1; b++)
			dist1[b] = dist0[b];
			
		t.tic();
		
		//distribute according to oracles
		for(DiffType i = 0; i < n; i++)
			target[(dist0[oracles[i]])++] = *(begin + i);
		
		for(int b = 0; b < num_bins; b++)
		{
			sequential_random_shuffle(
				target + dist1[b], 
				target + dist1[b + 1],
				rng);
			t.tic();
		}
		t.print();
		
		delete[] dist0;
		delete[] dist1;
		delete[] oracles;
		delete[] target;
	}
	else
		std::__mcstl_sequential_random_shuffle(
				begin,
				end,
				rng);
}

/** @brief Parallel random public call.
 *  @param begin Begin iterator of sequence.
 *  @param end End iterator of sequence.
 *  @param rng Random number generator to use.
 */
template<typename RandomAccessIterator, typename RandomNumberGenerator>
inline void
parallel_random_shuffle(RandomAccessIterator begin, RandomAccessIterator end, RandomNumberGenerator rng = random_number())
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;

	DiffType n = end - begin;

	parallel_random_shuffle_drs(begin, end, n, SETTINGS::num_threads, rng) ; 
}

}

#endif
