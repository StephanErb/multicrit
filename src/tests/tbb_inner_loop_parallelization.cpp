
#include "tbb/parallel_sort.h"
#include "tbb/tick_count.h"
#include "tbb/blocked_range.h"
#include "tbb/task_scheduler_init.h"

#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>


#define ITER_COUNT 10000
#define N 1000000

int main(int, char**) {
#ifdef PARALLEL_BUILD
	for (int p=1; p <= tbb::task_scheduler_init::default_num_threads(); ++p) {
		tbb::task_scheduler_init init(p);

		double timings[2] = {0, 0};
		std::vector<double> elements;
		elements.resize(N);

		// Dummy loop. Just make sure everything (i.e. worker threads) is initialized
		tbb::parallel_for(tbb::blocked_range<size_t>(0, elements.size()),
			[&](const tbb::blocked_range<size_t>& r) {
				for (size_t i=r.begin(); i!=r.end(); ++i) {
					elements[i] = 1;
				}
			}
		);

		for (int i=0; i<ITER_COUNT; ++i) {
			tbb::tick_count loop_start = tbb::tick_count::now();
			tbb::parallel_for(tbb::blocked_range<size_t>(0, elements.size()),
				[&](const tbb::blocked_range<size_t>& r) {
					int x = sin(r.begin());
					for (size_t i=r.begin(); i!=r.end(); ++i) {
						elements[i] = x*elements[i];
					}
				}
			);
			tbb::tick_count loop_stop = tbb::tick_count::now();
			timings[0] += (loop_stop-loop_start).seconds();

			tbb::tick_count sort_start = tbb::tick_count::now();
			tbb::parallel_sort(elements.begin(), elements.end());
			tbb::tick_count sort_stop = tbb::tick_count::now();
			timings[1] += (sort_stop-sort_start).seconds();
		}
		std::cout << "Num Threads: " << p << std::endl
				  << "  par. Loop: " << timings[0] << std::endl
				  << "  par. Sort: " << timings[1] << std::endl;
	}
#endif
}