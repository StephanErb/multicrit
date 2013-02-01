//#undef NDEBUG // uncomment to enable assertions
//#define BTREE_DEBUG

#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <ostream>
#include <set>

#include "tbb/tick_count.h"
#include "tbb/scalable_allocator.h"

#ifdef ENABLE_MCSTL
#include "mcstl.h"
#endif

#include "utility/tool/timer.h"
#include "timing.h"
#include "memory.h"
#include "algorithm"
#include "limits"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

#include <valgrind/callgrind.h>


#ifdef USE_GRAPH_LABEL
	struct Label {
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
#else 
	typedef unsigned int Label;
	typedef std::less<Label> Comparator;
#endif

Comparator cmp;

struct RedBlackTree : public std::set<Label, Comparator> {
	RedBlackTree(size_t n) {if (0) n++;};

	void apply_updates(const std::vector<Label>& updates) {
		// For benchmarik purposes, we only support inserts
		insert(updates.begin(), updates.end());
	}
	void verify() {

	}
};
typedef RedBlackTree Tree;


boost::mt19937 gen;

void bulkConstruct(Tree& tree, size_t n) {
		boost::uniform_int<unsigned int> dist(1, std::numeric_limits<unsigned int>::max());
		std::vector<Label> updates;
		updates.reserve(n);
		for (size_t j=0; j < n; ++j) {
			updates.push_back(dist(gen));
		}
		std::sort(updates.begin(), updates.end(), cmp);
		tree.apply_updates(updates);
		#ifndef NDEBUG
			tree.verify();
		#endif
		#ifndef KEEP_CONSTRUCTED_TREE_IN_CACHE
			flushDataCache();
		#endif
}

void timeBulkInsertion(size_t k, double ratio, double skew, size_t iterations, int p) {
	std::vector<double> timings(iterations);
	std::vector<double> memory(iterations);

	size_t n = ratio * k; // See [Parallelization of bulk operations for STL dictionaries, 2008]

	boost::uniform_int<unsigned int> skewed_dist(1, std::numeric_limits<unsigned int>::max() * skew);

	for (size_t i = 0; i < iterations; ++i) {
		// Prepare the updates
		std::vector<Label> updates;
		updates.reserve(k);
		for (size_t j=0; j < k; ++j) {
			updates.push_back(skewed_dist(gen));
		}
		std::sort(updates.begin(), updates.end(), cmp);
		
		Tree tree(std::max(n, k));
		if (n > 0) bulkConstruct(tree, n);

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
	std::cout << pruned_average(timings.data(), iterations, 0.25) << " " << pruned_average(memory.data(), iterations, 0.25)/1024 << " "  << getPeakMemorySize()/1024
		<< " " << k << " " << ratio << " " << skew << " " << p
		<< " # time in [µs], memory [mb], peak memory [mb], k, ratio, skew, p" << std::endl;
}


int main(int argc, char ** args) {
	size_t iterations = 1;
	double ratio = 0;
	double skew = 1;
	int p = 0;
	size_t k = 0;

	int c;
	while( (c = getopt( argc, args, "c:r:s:p:k:") ) != -1  ){
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
		case 'k':
			k = atoi(optarg);
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}
#ifdef ENABLE_MCSTL
		mcstl::SETTINGS::num_threads = p;
#else
		p = 0;
#endif

	if (k != 0) {
		timeBulkInsertion(k, ratio, skew, iterations, p);
	} else {
		if (ratio > 0.0) {
			std::cout << "# Bulk Insertion" << std::endl;
		} else {
			std::cout << "# Bulk Construction" << std::endl;
		}
		timeBulkInsertion(100, ratio, skew, iterations * 100000, p);
		timeBulkInsertion(1000, ratio, skew, iterations * 10000, p);
		timeBulkInsertion(10000, ratio, skew, iterations * 1000, p);
		timeBulkInsertion(100000, ratio, skew, iterations * 100, p);
		timeBulkInsertion(1000000, ratio, skew, iterations * 10, p);
		if (ratio < 40) timeBulkInsertion(10000000, ratio, skew, iterations, p);
	}
	return 0;
} 