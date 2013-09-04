#undef NDEBUG // uncomment to enable assertions
//#define BTREE_DEBUG // uncomment to enable debug print
#define TBB_USE_DEBUG 1
#define TBB_USE_ASSERT 1
#define TBB_USE_THREADING_TOOLS 1

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE btree_findparetomin_tests
#include <boost/test/auto_unit_test.hpp>

#include "../Graph.hpp"
#include "../Label.hpp"
#include <iostream>
#include <algorithm>

#define BRANCHING_PARAMETER_B 8
#define LEAF_PARAMETER_K 8 

#ifdef PARALLEL_BUILD
#include "../msp_pareto/ParetoQueue_parallel.hpp"
#else 
#include "../msp_pareto/ParetoQueue_sequential.hpp"
#endif

#include "tbb/task_scheduler_init.h"                                                                                                                                                              
#ifdef PARALLEL_BUILD
	unsigned short p = tbb::task_scheduler_init::default_num_threads();     
#else
	unsigned short p = 0;
#endif

void assertTrue(bool cond, std::string msg) {
	BOOST_REQUIRE_MESSAGE(cond, msg);
}

template<typename ParetoQueue>
void testParetoMinInLeaf(ParetoQueue&& pq) {
	assertTrue(pq.size() == 0, "Empty");

	Graph empty_graph;
	std::vector<NodeLabel> empty_candidates;
	empty_graph.addNode();
	empty_graph.finalize();

	std::vector<Operation<NodeLabel> > updates;
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 10, 30});
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 20, 20});
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 30, 10});

	pq.applyUpdates(updates);
	assertTrue(pq.size() == 3, "Insert into empty tree");

	std::vector<Operation<NodeLabel>> minima;
	pq.findParetoMinima(minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 3, "Three, as there are no dominated");

	updates.clear();
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 10, 22}); // non-dominated, domiantes (10, 30)
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 15, 25}); // dominated 
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 40, 10}); // dominated
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 50, 5}); // non-dominated

	pq.applyUpdates(updates);
	assertTrue(pq.size() == 7, "");

	minima.clear();
	pq.findParetoMinima(minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 4, "4 non-dominated");

	assertTrue(empty_candidates.empty(), "Candidates should be empty");
}

template<typename ParetoQueue>
void testParetoMinInInternalNode(ParetoQueue&& pq) {
	assertTrue(pq.size() == 0, "Empty");

	Graph empty_graph;
	std::vector<NodeLabel> empty_candidates;
	empty_graph.addNode();
	empty_graph.finalize();

	std::vector<Operation<NodeLabel> > updates;
	for (int i=1; i<141; ++i) {
		for (int j=0; j<5; j++) {
			updates.push_back({Operation<NodeLabel>::INSERT, 0, i*10, i*10});
		}
	}
	pq.applyUpdates(updates);
	assertTrue(pq.size() == 700, "Tree size");

	std::vector<Operation<NodeLabel>> minima;
	pq.findParetoMinima(minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 5, "Report all elements of the first (smallest leaf)");

	// overflow leaf with 10000 pareto opt labels
	updates.clear();
	for (int i=0; i<10000; ++i) {
		updates.push_back({Operation<NodeLabel>::INSERT, 0, 1, 0});
	}
	pq.applyUpdates(updates);
	assertTrue(pq.size() == 10700, "Tree size");

	minima.clear();
	pq.findParetoMinima(minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 10000, "Find all minima");

	assertTrue(empty_candidates.empty(), "Candidates should be empty");
}


#ifdef PARALLEL_BUILD

	/**
	 * Interface tends to change to often. Currently only tested via 
     * test_labelsetting.cpp and the cross validation checks
	 */
	 BOOST_AUTO_TEST_CASE(nop) {

	 }

#else 
	
	BOOST_AUTO_TEST_CASE(testParetoMinInLeaf_Vector) {
		testParetoMinInLeaf(VectorParetoQueue());
	}
	BOOST_AUTO_TEST_CASE(testParetoMinInInternalNode_Vector) {
		testParetoMinInInternalNode(VectorParetoQueue());
	}


	BOOST_AUTO_TEST_CASE(testParetoMinInLeaf_Btree) {
		testParetoMinInLeaf(BTreeParetoQueue());
	}
	BOOST_AUTO_TEST_CASE(testParetoMinInInternalNode_Btree) {
		testParetoMinInInternalNode(BTreeParetoQueue());
	}

#endif