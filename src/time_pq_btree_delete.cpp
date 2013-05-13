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

void bulkConstruct(std::vector<Operation<Label>>& updates, Tree& tree, size_t n) {
		boost::uniform_int<unsigned int> dist(1, std::numeric_limits<unsigned int>::max());
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

void timeBulkDeletion(size_t k, double ratio, double skew, size_t iterations, int p, bool insert_and_delete_separated, bool insert_and_delete_combined) {
	std::vector<double> timings(iterations);
	std::vector<double> memory(iterations);

	const size_t n = ratio * k; // See [Parallelization of bulk operations for STL dictionaries, 2008]
	const unsigned int MAX = std::numeric_limits<unsigned int>::max() * skew;
	boost::uniform_int<unsigned int> skewed_dist(1, MAX);

	Tree tree(p);
	for (size_t i = 0; i < iterations; ++i) {
		tree.clear();
		std::vector<Operation<Label>> elements;
		bulkConstruct(elements, tree, n);
		flushDataCache();

		// Elements to be considered according to our skew factor
		#ifdef USE_GRAPH_LABEL
			Operation<Label> op = {Operation<Label>::INSERT, MAX, MAX, MAX};
		#else
			Operation<Label> op = {Operation<Label>::INSERT, MAX};
		#endif
		auto skewed_range_end = std::upper_bound(elements.begin(), elements.end(), op, opCmp);
		std::random_shuffle(elements.begin(), skewed_range_end);
		// Remap the remaing elements to be deleted
		elements.resize(k);
		for (auto& e : elements) {
			e.type = Operation<Label>::DELETE;
		}
		// The elements to insert
		std::vector<Operation<Label>> inserts;
		inserts.reserve(k);
		for (size_t j=0; j < k; ++j) {
			#ifdef USE_GRAPH_LABEL
				inserts.emplace_back(Operation<Label>::INSERT, skewed_dist(gen), skewed_dist(gen), skewed_dist(gen));
			#else
				inserts.emplace_back(Operation<Label>::INSERT, skewed_dist(gen));
			#endif
		}

		if (insert_and_delete_separated) {
			#ifdef PARALLEL_BUILD
				tbb::parallel_sort(elements.begin(), elements.end(), opCmp);
				tbb::parallel_sort(inserts.begin(), inserts.end(), opCmp);
			#else
				std::sort(elements.begin(), elements.end(), opCmp);
				std::sort(inserts.begin(), inserts.end(), opCmp);
			#endif

			memory[i] = getCurrentMemorySize();
			tbb::tick_count start = tbb::tick_count::now();

			tree.apply_updates(elements, DELETES_ONLY);
			assert(tree.size() == n-k);
			tree.apply_updates(inserts, INSERTS_ONLY);
			assert(tree.size() == n-k+k);

			tbb::tick_count stop = tbb::tick_count::now();
			timings[i] = (stop-start).seconds() * 1000.0 * 1000.0;
			memory[i] = getCurrentMemorySize() - memory[i];

		} else if (insert_and_delete_combined) {

			elements.insert(elements.end(), inserts.begin(), inserts.end());
			#ifdef PARALLEL_BUILD
				tbb::parallel_sort(elements.begin(), elements.end(), opCmp);
			#else
				std::sort(elements.begin(), elements.end(), opCmp);
			#endif

			memory[i] = getCurrentMemorySize();
			tbb::tick_count start = tbb::tick_count::now();

			tree.apply_updates(elements, INSERTS_AND_DELETES);
			assert(tree.size() == n-k+k);

			tbb::tick_count stop = tbb::tick_count::now();
			timings[i] = (stop-start).seconds() * 1000.0 * 1000.0;
			memory[i] = getCurrentMemorySize() - memory[i];

		} else { // Just delete
			#ifdef PARALLEL_BUILD
				tbb::parallel_sort(elements.begin(), elements.end(), opCmp);
			#else
				std::sort(elements.begin(), elements.end(), opCmp);
			#endif
			
			memory[i] = getCurrentMemorySize();
			tbb::tick_count start = tbb::tick_count::now();

			tree.apply_updates(elements, DELETES_ONLY);
			assert(tree.size() == n-k);

			tbb::tick_count stop = tbb::tick_count::now();
			timings[i] = (stop-start).seconds() * 1000.0 * 1000.0;
			memory[i] = getCurrentMemorySize() - memory[i];
		}
		#ifndef NDEBUG
			tree.verify();
		#endif
	}
	std::cout << pruned_average(timings.data(), iterations, 0.25) << " " << pruned_average(memory.data(), iterations, 0.25)/1024 << " "  << getPeakMemorySize()/1024
		<< " " << k << " " << btree<Label, Comparator>::traits::leafparameter_k << " " << btree<Label, Comparator>::traits::branchingparameter_b << " " << ratio << " " << skew << " " << p
		<< " # time in [µs], memory [mb], peak memory [mb], k, tree_k, tree_b, ratio, skew, p" << std::endl;
}


int main(int argc, char ** args) {
	size_t iterations = 1;
	double ratio = 10;
	double skew = 1;
	int p = tbb::task_scheduler_init::default_num_threads();
	bool insert_and_delete_separated = false;
	bool insert_and_delete_combined = false;

	int c;
	while( (c = getopt( argc, args, "c:r:s:p:k:xy") ) != -1  ){
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
		case 'x':
			insert_and_delete_separated = true;
			break;
		case 'y':
			insert_and_delete_combined = true;
			break;
		case 's':
			skew = atof(optarg);
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
	std::cout << "# Bulk Deletion" << std::endl;
	timeBulkDeletion(100, ratio, skew, iterations *     3000, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(316, ratio, skew, iterations *     3000, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(1000, ratio, skew, iterations *    3000, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(3162, ratio, skew, iterations *    3000, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(10000, ratio, skew, iterations *    300, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(31622, ratio, skew, iterations *    300, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(100000, ratio, skew, iterations *    30, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(316227, ratio, skew, iterations *    30, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(1000000, ratio, skew, iterations *   10, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(3162277, ratio, skew, iterations *   10, p, insert_and_delete_separated, insert_and_delete_combined);
	timeBulkDeletion(10000000, ratio, skew, iterations *   3, p, insert_and_delete_separated, insert_and_delete_combined);
	return 0;
} 
