
#include "tbb/parallel_sort.h"
#include "tbb/tick_count.h"
#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/partitioner.h"

#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>


#define ITER_COUNT 10
#define N 10000000

int main(int, char**) {
#ifdef PARALLEL_BUILD
	for (int p=1; p <= tbb::task_scheduler_init::default_num_threads(); ++p) {
		tbb::task_scheduler_init init(p);
		std::cout << "Num Threads: " << p << std::endl;

		double timings[3] = {0, 0, 0};
		std::vector<double> elements;
		elements.resize(N);

		for (int i=0; i<ITER_COUNT; ++i) {
			// Dummy loop. Just make sure everything (i.e. worker threads) is initialized
			tbb::parallel_for(tbb::blocked_range<size_t>(0, elements.size(), 100000),
				[&](const tbb::blocked_range<size_t>& r) {
					for (size_t i=r.begin(); i!=r.end(); ++i) {
						elements[i] =  r.end() - i;
					}
				},
				tbb::simple_partitioner()
			);

			tbb::tick_count loop_start = tbb::tick_count::now();
			tbb::parallel_for(tbb::blocked_range<size_t>(1, elements.size()-1),
				[&](const tbb::blocked_range<size_t>& r) {
					for (size_t i=r.begin(); i!=r.end(); ++i) {
						elements[i] += elements[i-1] + sin(elements[i+1]);
					}
				},
				tbb::auto_partitioner()
			);
			tbb::tick_count loop_stop = tbb::tick_count::now();
			timings[0] += (loop_stop-loop_start).seconds();

			tbb::tick_count stat_loop_start = tbb::tick_count::now();
			tbb::parallel_for(tbb::blocked_range<size_t>(1, elements.size()-1, ceil(elements.size()/p)),
				[&](const tbb::blocked_range<size_t>& r) {
					for (size_t i=r.begin(); i!=r.end(); ++i) {
						elements[i] += elements[i-1] + sin(elements[i+1]);
					}
				},
				tbb::simple_partitioner()
			);
			tbb::tick_count stat_loop_stop = tbb::tick_count::now();
			timings[1] += (stat_loop_stop-stat_loop_start).seconds();

			tbb::tick_count sort_start = tbb::tick_count::now();
			//tbb::parallel_sort(elements.begin(), elements.end());
			tbb::tick_count sort_stop = tbb::tick_count::now();
			timings[2] += (sort_stop-sort_start).seconds();
		}

		std::cout << "  For (dynamic): " << timings[0] << std::endl;
		std::cout << "  For (static ): " << timings[1] << std::endl;
		//std::cout << "  Parallel Sort: " << timings[2] << std::endl;
	}
#endif
}
