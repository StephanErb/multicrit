#undef NDEBUG
#define BTREE_DEBUG

#include "BTree.hpp"
#include <iostream>
#include <algorithm>

template <typename _Key>
struct btree_test_set_traits {
    static const bool   selfverify = true;
    static const int    leafparameter_k = 16;
    static const int    branchingparameter_b = 8;
};

void assertTrue(bool cond, std::string msg) {
	if (!cond) {
		std::cout << "FAILED: " << msg << std::endl;
		exit(-1);
	}
}

void testBulkUpdatesOfSingleLeaf() {
	btree<int, std::less<int>, btree_test_set_traits<int>> btree;
	assertTrue(btree.size() == 0, "Empty");

	std::vector<Operation<int> > updates;
	updates.push_back({Operation<int>::INSERT, 10});
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::INSERT, 30});

	btree.applyUpdates(updates);
	assertTrue(btree.size() == 3, "Insert into empty tree");

	updates.clear();
	updates.push_back({Operation<int>::INSERT, 5});
	updates.push_back({Operation<int>::INSERT, 15});
	updates.push_back({Operation<int>::INSERT, 15});
	updates.push_back({Operation<int>::INSERT, 25});
	updates.push_back({Operation<int>::INSERT, 35});

	btree.applyUpdates(updates);
	assertTrue(btree.size() == 8, "Insert into spaces");

	updates.clear();
	updates.push_back({Operation<int>::DELETE, 10});
	updates.push_back({Operation<int>::DELETE, 20});
	updates.push_back({Operation<int>::DELETE, 30});

	btree.applyUpdates(updates);
	assertTrue(btree.size() == 5, "Delete original elements");

	updates.clear();
	updates.push_back({Operation<int>::DELETE, 5});
	updates.push_back({Operation<int>::DELETE, 15});
	updates.push_back({Operation<int>::DELETE, 15});
	updates.push_back({Operation<int>::DELETE, 25});
	updates.push_back({Operation<int>::DELETE, 35});

	btree.applyUpdates(updates);
	assertTrue(btree.size() == 0, "Empty");
}

int main() {
	testBulkUpdatesOfSingleLeaf();

	std::cout << "Tests passed successfully." << std::endl;
	return 0;
}