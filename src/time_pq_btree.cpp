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
#include "tbb/parallel_sort.h"

#include "datastructures/BTree.hpp"

#include "timing.h"
#include "memory.h"
#include "algorithm"
#include "limits"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

#ifdef USE_GRAPH_LABEL
	struct Label {
		unsigned int x;
		unsigned int y;
		unsigned int node;

		Label(unsigned int _x=0, unsigned int _y=0, unsigned int _node=0)
			: x(_x), y(_y), node(_node)
		{}

		friend std::ostream& operator<<(std::ostream& os, const Label& label) {
			os << "{" << label.x << ", " << label.y  << ", " << label.node << "}";
			return os;
		}
	};

	struct Comparator {
		inline bool operator() (const Label& i, const Label& j) const {
			if (i.x == j.x) {
				if (i.y == j.y) {
					return i.node < j.node;
				}
				return i.y < j.y;
			}
			return i.x < j.x;
		}
	};
#else 
	typedef unsigned int Label;
	typedef std::less<Label> Comparator;
#endif

Comparator cmp;
typedef btree<Label, Comparator> Tree;

struct OpComparator {
	bool operator() (const Operation<Label>& i, const Operation<Label>& j) const {
		return cmp(i.data, j.data);
	}
} opCmp;

boost::mt19937 gen;

void bulkConstruct(Tree& tree, size_t n) {
		boost::uniform_int<unsigned int> dist(1, std::numeric_limits<unsigned int>::max());
		std::vector<Operation<Label>> updates;
		updates.reserve(n);
		for (size_t j=0; j < n; ++j) {
			#ifdef USE_GRAPH_LABEL
				updates.emplace_back(Operation<Label>::INSERT, dist(gen), dist(gen), dist(gen));
			#else
				updates.emplace_back(Operation<Label>::INSERT, dist(gen));
			#endif
		}
		#ifdef PARALLEL_BUILD
			tbb::parallel_sort(updates.begin(), updates.end(), opCmp);
		#else
			std::sort(updates.begin(), updates.end(), opCmp);
		#endif
		tree.apply_updates(updates, INSERTS_ONLY);
		#ifndef NDEBUG
			tree.verify();
		#endif
		assert(tree.size() == n);
}

void timeBulkInsertion(size_t k, double ratio, double skew, size_t iterations, int p, bool heatmap) {
	std::vector<double> timings(iterations);
	std::vector<double> memory(iterations);

	size_t n = ratio * k; // See [Parallelization of bulk operations for STL dictionaries, 2008]

	boost::uniform_int<unsigned int> skewed_dist(1, std::numeric_limits<unsigned int>::max() * skew);

	Tree tree(p);
	for (size_t i = 0; i < iterations; ++i) {
		tree.clear();
		bulkConstruct(tree, n);
		flushDataCache();

		// Prepare the updates
		std::vector<Operation<Label>> updates;
		updates.reserve(k);
		for (size_t j=0; j < k; ++j) {
			#ifdef USE_GRAPH_LABEL
				updates.emplace_back(Operation<Label>::INSERT, skewed_dist(gen), skewed_dist(gen), skewed_dist(gen));
			#else
				updates.emplace_back(Operation<Label>::INSERT, skewed_dist(gen));
			#endif
		}
		#ifdef PARALLEL_BUILD
			tbb::parallel_sort(updates.begin(), updates.end(), opCmp);
		#else
			std::sort(updates.begin(), updates.end(), opCmp);
		#endif
		
		memory[i] = getCurrentMemorySize();
		tbb::tick_count start = tbb::tick_count::now();

		tree.apply_updates(updates, INSERTS_ONLY);
		assert(tree.size() == n+k);

		tbb::tick_count stop = tbb::tick_count::now();
		timings[i] = (stop-start).seconds() * 1000.0 * 1000.0;
		memory[i] = getCurrentMemorySize() - memory[i];

		#ifndef NDEBUG
			tree.verify();
		#endif
	}
	if (heatmap) {
		std::cout << pruned_average(timings.data(), iterations, 0.25);
	} else {
		std::cout << pruned_average(timings.data(), iterations, 0.25) << " " << pruned_average(memory.data(), iterations, 0.25)/1024 << " "  << getPeakMemorySize()/1024
			<< " " << k << " " << btree<Label, Comparator>::traits::leafparameter_k << " " << btree<Label, Comparator>::traits::branchingparameter_b << " " << ratio << " " << skew << " " << p
			<< " # time in [µs], memory [mb], peak memory [mb], k, tree_k, tree_b, ratio, skew, p" << std::endl;
	}
}


int main(int argc, char ** args) {
	size_t iterations = 1;
	double ratio = 0;
	double skew = 1;
	int p = tbb::task_scheduler_init::default_num_threads();
	size_t k = 0;
	bool heatmap = false;

	int c;
	while( (c = getopt( argc, args, "c:r:s:p:k:h") ) != -1  ){
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
		case 'h':
			heatmap = true;
			break;
		case '?':
            std::cout << "Unrecognized option: " <<  optopt << std::endl;
		}
	}
#ifdef PARALLEL_BUILD
		tbb::task_scheduler_init init(p);
#else
		p = 0;
#endif

	if (k != 0) {
		timeBulkInsertion(k, ratio, skew, iterations, p, heatmap);
	} else {
		if (ratio > 0.0) {
			std::cout << "# Bulk Insertion" << std::endl;
		} else {
			std::cout << "# Bulk Construction" << std::endl;
		}
		timeBulkInsertion(100, ratio, skew, iterations *     10000, p, heatmap);
		timeBulkInsertion(316, ratio, skew, iterations *     10000, p, heatmap);
		timeBulkInsertion(1000, ratio, skew, iterations *    10000, p, heatmap);
		timeBulkInsertion(3162, ratio, skew, iterations *    10000, p, heatmap);
		timeBulkInsertion(10000, ratio, skew, iterations *    1000, p, heatmap);
		timeBulkInsertion(31622, ratio, skew, iterations *    1000, p, heatmap);
		timeBulkInsertion(100000, ratio, skew, iterations *    100, p, heatmap);
		timeBulkInsertion(316227, ratio, skew, iterations *    100, p, heatmap);
		timeBulkInsertion(1000000, ratio, skew, iterations *   50, p, heatmap);
		timeBulkInsertion(3162277, ratio, skew, iterations *   50, p, heatmap);
		timeBulkInsertion(10000000, ratio, skew, iterations *  50, p, heatmap);
	}
	return 0;
} 
