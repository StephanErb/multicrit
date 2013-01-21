//#undef NDEBUG // uncomment to enable assertions
//#define BTREE_DEBUG

#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <ostream>
#include <set>

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"

#include "ParallelBTree.hpp"

#include "utility/tool/timer.h"
#include "timing.h"
#include "memory.h"
#include "algorithm"
#include "limits"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

#include <valgrind/callgrind.h>

/*struct Label {
	unsigned int x;
	unsigned int y;
	unsigned int node;

	friend std::ostream& operator<<(std::ostream& os, const Label& label) {
		os << "{" << label.x << ", " << label.y  << ", " << label.node << "}";
		return os;
	}
};

struct Comparator {
	bool operator() (const Label& i, const Label& j) const {
		if (i.x == j.x) {
			if (i.y == j.y) {
				return i.node < j.node;
			}
			return i.y < j.y;
		}
		return i.x < j.x;
	}
} cmp;
*/
typedef unsigned int Label;
typedef std::less<Label> Comparator;
Comparator cmp;

//#define REDBLACKTREE_WITHOUT_BULKS
#ifdef REDBLACKTREE_WITHOUT_BULKS
	struct RedBlackTree : std::set<Label, Comparator> {
		void apply_updates(const std::vector<Operation<Label> >& updates) {
			auto update_iter = updates.begin();

			while (update_iter != updates.end()) {
				switch (update_iter->type) {
				case Operation<Label>::DELETE:
				  	erase(update_iter->data);
					++update_iter;
					continue;
				case Operation<Label>::INSERT:
					insert(update_iter->data);
					++update_iter;
					continue;
		        }
			}
		}
		void verify() {

		}
	};
	typedef RedBlackTree Tree;
#else
	typedef btree<Label, Comparator> Tree;
#endif

struct OpComparator {
	bool operator() (const Operation<Label>& i, const Operation<Label>& j) const {
		return cmp(i.data, j.data);
	}
} opCmp;


boost::mt19937 gen;

void timeBulkInsertion(unsigned int k, double ratio, double skew, int iterations, int p) {
	double timings[iterations];
	double memory[iterations];
	std::fill_n(timings, iterations, 0);
	std::fill_n(memory, iterations, 0);

	size_t n = ratio * k; // See [Parallelization of bulk operations for STL dictionaries, 2008]

	boost::uniform_int<unsigned int> dist(1, std::numeric_limits<unsigned int>::max());
	boost::uniform_int<unsigned int> skewed_dist(1, std::numeric_limits<unsigned int>::max() * skew);

	for (int i = 0; i < iterations; ++i) {
		Tree tree(n);

		// Bulk Construct the initial tree
		std::vector<Operation<Label> > updates(n);
		for (size_t i=0; i < updates.size(); ++i) {
			updates[i] = {Operation<Label>::INSERT, dist(gen)};
		}
		std::sort(updates.begin(), updates.end(), opCmp);
		tree.apply_updates(updates);
		#ifndef NDEBUG
			tree.verify();
		#endif

		// Generate & insert updates depending on the skew
		updates.resize(k);
		for (size_t i=0; i < updates.size(); ++i) {
			updates[i] = {Operation<Label>::INSERT, dist(gen)};
		}
		std::sort(updates.begin(), updates.end(), opCmp);

		memory[i] = getCurrentMemorySize();
		tbb::tick_count start = tbb::tick_count::now();

		CALLGRIND_START_INSTRUMENTATION;
		tree.apply_updates(updates);
		CALLGRIND_STOP_INSTRUMENTATION;

		tbb::tick_count stop = tbb::tick_count::now();
		timings[i] = (stop-start).seconds() * 1000.0 * 1000.0;
		memory[i] = getCurrentMemorySize() - memory[i];

		#ifndef NDEBUG
			tree.verify();
		#endif
	}
	std::cout << pruned_average(timings, iterations, 0.25) << " " << pruned_average(memory, iterations, 0.25)/1024 << " "  << getPeakMemorySize()/1024
		<< " " << k << " " << btree<Label, Comparator>::traits::leafparameter_k << " " << btree<Label, Comparator>::traits::branchingparameter_b << " " << ratio << " " << skew << " " << p
		<< " # time in [µs], memory [mb], peak memory [mb], k, tree_k, tree_b, ratio, skew, p" << std::endl;
}

int main(int argc, char ** args) {
	int iterations = 1;
	double ratio = 0;
	double skew = 1;
	int p = tbb::task_scheduler_init::default_num_threads();

	int c;
	while( (c = getopt( argc, args, "c:r:s:p:t:") ) != -1  ){
		switch(c){
		case 'c':
			iterations = atoi(optarg);
			break;
		case 'r':
			ratio = atof(optarg);
			break;
		case 'p':
			p = atoi(optarg);
			break;
		case 's':
			skew = atof(optarg);
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}
	std::cout << "# Running on " << p << " threads" << std::endl;
	tbb::task_scheduler_init init(p);

	if (ratio > 0.0) {
		std::cout << "# Bulk Insertion" << std::endl;
	} else {
		std::cout << "# Bulk Construction" << std::endl;
	}
	timeBulkInsertion(100, ratio, skew, iterations, p);
	timeBulkInsertion(1000, ratio, skew, iterations, p);
	timeBulkInsertion(10000, ratio, skew, iterations, p);
	timeBulkInsertion(100000, ratio, skew, iterations, p);
	timeBulkInsertion(1000000, ratio, skew, iterations, p);
	timeBulkInsertion(10000000, ratio, skew, iterations, p);
	
	return 0;
}

