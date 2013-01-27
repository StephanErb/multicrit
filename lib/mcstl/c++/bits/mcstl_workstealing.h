/***************************************************************************
 *   Copyright (C) 2007 by Felix Putze                                     *
 *   kontakt@felix-putze.de                                                *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_workstealing.h
 *  @brief Parallelization of embarrassingly parallel execution by means of work-stealing. */

#ifndef _MCSTL_WORKSTEALING_H
#define _MCSTL_WORKSTEALING_H 1

#include <mod_stl/stl_algobase.h>

#include <mcstl.h>
#include <bits/mcstl_random_number.h>
#include <bits/mcstl_compatibility.h>

namespace mcstl
{

#define MCSTL_JOB_VOLATILE volatile
	
/** @brief One job for a certain thread. */
template<typename DiffType>
struct Job
{
	/** @brief First element.
	 *
	 *  Changed by owning and stealing thread. By stealing thread, always incremented. */
	MCSTL_JOB_VOLATILE DiffType first;
	/** @brief Last element.
	 *
	 *  Changed by owning thread only. */
	MCSTL_JOB_VOLATILE DiffType last;
	/** @brief Number of elements, i. e. @c last-first+1. 
	 *
	 *  Changed by owning thread only. */
	MCSTL_JOB_VOLATILE DiffType load;
};

/** @brief Work stealing algorithm for random access iterators.
 * 
 *  Uses O(1) additional memory. Synchronization at job lists is done with atomic operations.
 *  @param begin Begin iterator of element sequence.
 *  @param end End iterator of element sequence.
 *  @param op User-supplied functor (comparator, predicate, adding functor, ...).
 *  @param f Functor to "process" an element with op (depends on desired functionality, e. g. for std::for_each(), ...).
 *  @param r Functor to "add" a single result to the already processed elements (depends on functionality).
 *  @param base Base value for reduction.
 *  @param output Pointer to position where final result is written to
 *  @param bound Maximum number of elements processed (e. g. for std::count_n()). 
 *  @return User-supplied functor (that may contain a part of the result). */
template<typename RandomAccessIterator, typename Op, typename Fu, typename Red, typename Result>
Op
for_each_template_random_access_workstealing(	RandomAccessIterator begin,
						RandomAccessIterator end,
						Op op,
						Fu& f,
						Red r,
						Result base,
						Result& output,
						typename std::iterator_traits<RandomAccessIterator>::difference_type bound
						)
{
	MCSTL_CALL(end - begin)
	
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type DiffType;
	
	DiffType chunk_size = static_cast<DiffType>(SETTINGS::workstealing_chunk_size);
	
	DiffType length = (bound < 0) ? (end - begin) : bound;	// how many jobs?
	const int stride = SETTINGS::cache_line_size * 10 / sizeof(Job<DiffType>) + 1;	// to avoid false sharing in a cache line
	thread_index_t busy = 0;	// total number of threads currently working
	thread_index_t num_threads = SETTINGS::num_threads;
	num_threads = static_cast<thread_index_t>(std::max<DiffType>(std::min<DiffType>(num_threads, end - begin), 1));	//no more threads than jobs, at least one thread
	Job<DiffType> *job = new Job<DiffType>[num_threads * stride];		// create job description array
	
	output = base;	// write base value to output
	
	#pragma omp parallel shared(busy) num_threads(num_threads)
	{
		// initialization phase
	
		bool iam_working = false;	// flags for every thread if it is doing productive work
		thread_index_t iam = omp_get_thread_num();		// thread id
		Job<DiffType>& my_job = job[iam * stride];	// my job
		thread_index_t victim;		// random number (for work stealing)
		Result result = Result();	// local value for reduction
		DiffType steal;			// no. of elements to steal in one attempt
		
		random_number rand_gen(iam, num_threads);	// every thread has its own random number generator (modulo num_threads)

		#pragma omp atomic
		busy++;				// this thread is currently working
		
		iam_working = true;

		my_job.first = static_cast<DiffType>(iam * (length / num_threads));	// how many jobs per thread? last thread gets the rest
		my_job.last = (iam == (num_threads - 1)) ? (length - 1) : ((iam + 1) * (length / num_threads) - 1);
		my_job.load = my_job.last - my_job.first + 1;
		
		if(my_job.first <= my_job.last)	// init result with first value (to have a base value for reduction)
		{
			DiffType my_first = my_job.first;	// cannot use volatile variable directly
			result = f(op, begin + my_first);
			my_job.first++;
			my_job.load--;
		}
			
		RandomAccessIterator current;
		
		#pragma omp barrier
		
		// actual work phase
		
		// work on own or stolen start
		while (busy > 0)		// work until no productive thread left
		{
			#pragma omp flush(busy)
			
			while (my_job.first <= my_job.last)	// thread has own work to do
			{
				//fetch-and-add call
				DiffType current_job = fetch_and_add<DiffType>(&(my_job.first), chunk_size);	// reserve current job block (size chunk_size) in my queue
				my_job.load = my_job.last - my_job.first + 1;	//update load, to make the three values consistent, first might have been changed in the meantime
				for(DiffType job_counter = 0; job_counter < chunk_size && current_job <= my_job.last; job_counter++)
				{
					current = begin + current_job;		// yes: process it!
					current_job++;
					result = r(result, f(op, current));	// do actual work
				}
				
				#pragma omp flush(busy)

			}
			
			// reaching this point, a thread's job list is empty
			
			if(iam_working)
			{
				#pragma omp atomic
				busy--;		// this thread no longer has work
				
				iam_working = false;
			}
			
			DiffType supposed_first, supposed_last, supposed_load;
			do	// find random nonempty deque (not own) and do consistency check
			{
				yield();
				#pragma omp flush(busy)
				victim = rand_gen();
				supposed_first = job[victim * stride].first;
				supposed_last = job[victim * stride].last;
				supposed_load = job[victim * stride].load;
			} while (busy > 0 && ((supposed_load <= 0) || ((supposed_first + supposed_load - 1) != supposed_last)));

			if(busy == 0)
				break;

			if(supposed_load > 0)		// has work and work to do
			{
				steal = (supposed_load < 2) ? 1 : supposed_load / 2;		// no. of elements to steal (at least one)
				
				//omp_set_lock(&(job[victim * stride].lock));	//protects against stealing threads
				
				DiffType stolen_first = fetch_and_add<DiffType>(&(job[victim * stride].first), steal); // push victim's start forward
				
				//protects against working thread
				//omp_unset_lock(&(job[victim * stride].lock));
				
				my_job.first = stolen_first;	
				my_job.last = std::min<DiffType>(stolen_first + steal - (DiffType)1, supposed_last);	// no overlapping
				my_job.load = my_job.last - my_job.first + 1;
				
				//omp_unset_lock(&(my_job.lock));

				#pragma omp atomic
				busy++;	// has potential work again
				iam_working = true;
					
				#pragma omp flush(busy)
			}
			#pragma omp flush(busy)
		} // end while busy > 0
		#pragma omp critical(writeOutput)
		output = r(output, result);			// add accumulated result to output
		
		//omp_destroy_lock(&(my_job.lock));
	} // end parallel
	
	delete[] job;		// cleanup
	
	f.finish_iterator = begin + length;		// points to last element processed (needed as return value for some algorithms like transform)
		
	return op;
} // end function

} // end namespace

#endif
