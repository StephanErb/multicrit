#ifndef _TBB_EXT_SCHEDULER_OBSERVER_
#define _TBB_EXT_SCHEDULER_OBSERVER_

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#include "tbb/atomic.h"

#include <iostream>
#include <fstream>

#include "tbb/task_scheduler_observer.h"
#include <pthread.h>


/** 
 * To enable pinning of threads to cores, just include this header.
 */
	struct tbb_set_affinity : public tbb::task_scheduler_observer {
		tbb::atomic<int> threadID;

		tbb_set_affinity() : threadID(0) {
			observe(true);
		}
		void on_scheduler_entry(bool) {
			int myID = threadID.fetch_and_increment();

			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(myID, &cpuset);

			std::cout << myID << " on pthread " <<  (unsigned int) pthread_self() << std::endl;

			int error = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
			if (error) {
				std::cout << "bad affinity for " << threadID << ". Errno: " << error << std::endl; 
				exit(1);
			}
		}
	} tbb_set_affinity;


#endif