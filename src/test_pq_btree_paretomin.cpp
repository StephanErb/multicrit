#undef NDEBUG // uncomment to enable assertions
//#define BTREE_DEBUG // uncomment to enable debug print
#define TBB_USE_DEBUG 1
#define TBB_USE_ASSERT 1
#define TBB_USE_THREADING_TOOLS 1

#define COMPUTE_PARETO_MIN

#ifdef SEQUENTIAL_BTREE
#include "BTree.hpp"
#else
#include "ParallelBTree.hpp"
#endif

#include <iostream>
#include <algorithm>

struct btree_test_set_traits {
    static const bool   selfverify = true;
    static const int    leafparameter_k = 8;
    static const int    branchingparameter_b = 8;
};

void assertTrue(bool cond, std::string msg) {
	if (!cond) {
		std::cout << "FAILED: " << msg << std::endl;
		exit(-1);
	}
}

struct Label {
	Label(int first=0, int second=0): first_weight(first), second_weight(second) {};
	typedef unsigned int weight_type;
	weight_type first_weight;
	weight_type second_weight;
	bool operator==(const Label& other) const {
		return first_weight == other.first_weight && second_weight == other.second_weight;
	}
	friend std::ostream& operator<<(std::ostream& os, const Label& label) {
			os << "{" << label.first_weight << ", " << label.second_weight  << "}";
			return os;
	}
};

struct LabelComparator {
	bool operator() (const Label& i, const Label& j) const {
		if (i.first_weight == j.first_weight) {
			return i.second_weight < j.second_weight;
		}
		return i.first_weight < j.first_weight;
	}
};

void testParetoMinInLeaf() {
	btree<Label, Label, LabelComparator, btree_test_set_traits> btree;
	assertTrue(btree.size() == 0, "Empty");

	std::vector<Operation<Label> > updates;
	updates.push_back({Operation<Label>::INSERT, Label(10, 30)});
	updates.push_back({Operation<Label>::INSERT, Label(20, 20)});
	updates.push_back({Operation<Label>::INSERT, Label(30, 10)});

	btree.apply_updates(updates);
	assertTrue(btree.size() == 3, "Insert into empty tree");
	assertTrue(btree.get_stats().leaves == 1, "Leave count");

	std::vector<Label> minima;
	btree.find_pareto_minima(Label(0, 1000000), minima);
	assertTrue(minima.size() == 3, "Three, as there are no dominated");

	updates.clear();
	updates.push_back({Operation<Label>::INSERT, Label(10, 22)}); // non-dominated, domiantes (10, 30)
	updates.push_back({Operation<Label>::INSERT, Label(15, 25)}); // dominated 
	updates.push_back({Operation<Label>::INSERT, Label(40, 10)}); // dominated
	updates.push_back({Operation<Label>::INSERT, Label(50, 5)}); // non-dominated

	btree.apply_updates(updates);
	assertTrue(btree.size() == 7, "");

	minima.clear();
	btree.find_pareto_minima(Label(0, 1000000), minima);
	assertTrue(minima.size() == 4, "4 non-dominated");
}

void testParetoMinInInternalNode() {
	btree<Label, Label, LabelComparator, btree_test_set_traits> btree;
	assertTrue(btree.size() == 0, "Empty");

	std::vector<Operation<Label> > updates;
	for (int i=1; i<15; ++i) {
		for (int j=0; j<5; j++) {
			updates.push_back({Operation<Label>::INSERT, Label(i*10, i*10)});
		}
	}
	btree.apply_updates(updates);
	assertTrue(btree.height() == 2, "In an optimal tree we need 2 inner node layers");
	assertTrue(btree.get_stats().leaves == 14, "Initial Leafcount");
	assertTrue(btree.size() == 70, "Tree size");

	std::vector<Label> minima;
	btree.find_pareto_minima(Label(0, 1000000), minima);
	assertTrue(minima.size() == 5, "Report all elements of the first (smallest leaf)");

	// overflow leaf with 100 pareto opt labels
	updates.clear();
	for (int i=0; i<100; ++i) {
		updates.push_back({Operation<Label>::INSERT, Label(1, 0)});
	}
	btree.apply_updates(updates);
	assertTrue(btree.size() == 170, "Tree size");

	minima.clear();
	btree.find_pareto_minima(Label(0, 1000000), minima);
	assertTrue(minima.size() == 100, "Find all minima");
}



int main() {
	testParetoMinInLeaf();
	testParetoMinInInternalNode();

	std::cout << "Tests passed successfully." << std::endl;
	return 0;
}