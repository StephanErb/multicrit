#undef NDEBUG // uncomment to enable assertions
//#define BTREE_DEBUG // uncomment to enable debug print
#define TBB_USE_DEBUG 1
#define TBB_USE_ASSERT 1
#define TBB_USE_THREADING_TOOLS 1

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE btree_findparetomin_tests
#include <boost/test/auto_unit_test.hpp>

#include "../utility/datastructure/graph/KGraph.hpp"
#include "../utility/datastructure/graph/Edge.hpp"
#include "../utility/datastructure/graph/GraphTypes.hpp"
#include "../utility/datastructure/graph/GraphMacros.h"

#define COMPUTE_PARETO_MIN
#include "../datastructures/BTree.hpp"
#undef COMPUTE_PARETO_MIN

#include "../Label.hpp"
#include <iostream>
#include <algorithm>

#include "tbb/task_scheduler_init.h"                                                                                                                                                              
#ifdef PARALLEL_BUILD
	unsigned short p = tbb::task_scheduler_init::default_num_threads();     
#else
	unsigned short p = 0;
#endif

typedef utility::datastructure::IntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;


struct btree_test_set_traits {
    static const bool   selfverify = true;
    static const int    leafparameter_k = 8;
    static const int    branchingparameter_b = 8;
};

void assertTrue(bool cond, std::string msg) {
	BOOST_REQUIRE_MESSAGE(cond, msg);
}

struct LabelComparator {
	bool operator() (const Label& i, const Label& j) const {
		if (i.first_weight == j.first_weight) {
			return i.second_weight < j.second_weight;
		}
		return i.first_weight < j.first_weight;
	}
};

BOOST_AUTO_TEST_CASE(testParetoMinInLeaf) {
	btree<NodeLabel, Label, LabelComparator, btree_test_set_traits> btree(p);
	assertTrue(btree.size() == 0, "Empty");

	Graph empty_graph;
	std::vector<NodeLabel> empty_candidates;
	empty_graph.addNode();
	empty_graph.finalize();

	std::vector<Operation<NodeLabel> > updates;
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 10, 30});
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 20, 20});
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 30, 10});

	btree.apply_updates(updates, INSERTS_AND_DELETES);
	assertTrue(btree.size() == 3, "Insert into empty tree");
	assertTrue(btree.get_stats().leaves == 1, "Leave count");

	std::vector<Operation<NodeLabel>> minima;
	btree.find_pareto_minima(Label(0, 1000000), minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 3, "Three, as there are no dominated");

	updates.clear();
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 10, 22}); // non-dominated, domiantes (10, 30)
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 15, 25}); // dominated 
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 40, 10}); // dominated
	updates.push_back({Operation<NodeLabel>::INSERT, 0, 50, 5}); // non-dominated

	btree.apply_updates(updates, INSERTS_AND_DELETES);
	assertTrue(btree.size() == 7, "");

	minima.clear();
	btree.find_pareto_minima(Label(0, 1000000), minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 4, "4 non-dominated");

	btree.clear();
	assertTrue(btree.size() == 0, "Empty");
	assertTrue(btree.get_stats().leaves == 0, "Leave count");

	assertTrue(empty_candidates.empty(), "Candidates should be empty");
}

BOOST_AUTO_TEST_CASE(testParetoMinInInternalNode) {
	btree<NodeLabel, Label, LabelComparator, btree_test_set_traits> btree(p);
	assertTrue(btree.size() == 0, "Empty");

	Graph empty_graph;
	std::vector<NodeLabel> empty_candidates;
	empty_graph.addNode();
	empty_graph.finalize();

	std::vector<Operation<NodeLabel> > updates;
	for (int i=1; i<15; ++i) {
		for (int j=0; j<5; j++) {
			updates.push_back({Operation<NodeLabel>::INSERT, 0, i*10, i*10});
		}
	}
	btree.apply_updates(updates, INSERTS_AND_DELETES);
	assertTrue(btree.height() == 2, "In an optimal tree we need 2 inner node layers");
	assertTrue(btree.get_stats().leaves == 14, "Initial Leafcount");
	assertTrue(btree.size() == 70, "Tree size");

	std::vector<Operation<NodeLabel>> minima;
	btree.find_pareto_minima(Label(0, 1000000), minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 5, "Report all elements of the first (smallest leaf)");

	// overflow leaf with 100 pareto opt labels
	updates.clear();
	for (int i=0; i<100; ++i) {
		updates.push_back({Operation<NodeLabel>::INSERT, 0, 1, 0});
	}
	btree.apply_updates(updates, INSERTS_AND_DELETES);
	assertTrue(btree.size() == 170, "Tree size");

	minima.clear();
	btree.find_pareto_minima(Label(0, 1000000), minima, empty_candidates, empty_graph);
	assertTrue(minima.size() == 100, "Find all minima");

	btree.clear();
	assertTrue(btree.size() == 0, "Empty");
	assertTrue(btree.get_stats().leaves == 0, "Leave count");

	assertTrue(empty_candidates.empty(), "Candidates should be empty");
}
