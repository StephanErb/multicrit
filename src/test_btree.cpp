#undef NDEBUG // uncomment to enable assertions
//#define BTREE_DEBUG // uncomment to enable debug print

#include "BTree.hpp"
#include <iostream>
#include <algorithm>

template <typename _Key>
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

void testBulkUpdatesOfSingleLeaf() {
	btree<int, std::less<int>, btree_test_set_traits<int>> btree;
	assertTrue(btree.size() == 0, "Empty");

	std::vector<Operation<int> > updates;
	updates.push_back({Operation<int>::INSERT, 10});
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::INSERT, 30});

	btree.apply_updates(updates);
	assertTrue(btree.size() == 3, "Insert into empty tree");
	assertTrue(btree.get_stats().leaves == 1, "Leave count");

	updates.clear();
	updates.push_back({Operation<int>::INSERT, 5});
	updates.push_back({Operation<int>::INSERT, 15});
	updates.push_back({Operation<int>::INSERT, 15});
	updates.push_back({Operation<int>::INSERT, 25});
	updates.push_back({Operation<int>::INSERT, 35});

	btree.apply_updates(updates);
	assertTrue(btree.size() == 8, "Insert into spaces");
	assertTrue(btree.get_stats().leaves == 1, "Leave count");

	updates.clear();
	updates.push_back({Operation<int>::DELETE, 10});
	updates.push_back({Operation<int>::DELETE, 20});
	updates.push_back({Operation<int>::DELETE, 30});

	btree.apply_updates(updates);
	assertTrue(btree.size() == 5, "Delete original elements");
	assertTrue(btree.get_stats().leaves == 1, "Leave count");

	updates.clear();
	updates.push_back({Operation<int>::DELETE, 5});
	updates.push_back({Operation<int>::DELETE, 15});
	updates.push_back({Operation<int>::DELETE, 15});
	updates.push_back({Operation<int>::DELETE, 25});
	updates.push_back({Operation<int>::DELETE, 35});

	btree.apply_updates(updates);
	assertTrue(btree.size() == 0, "Empty");
	assertTrue(btree.get_stats().leaves == 0, "Leave count");
}

void testSplitLeafInto2() {
	btree<int, std::less<int>, btree_test_set_traits<int>> btree;
	assertTrue(btree.size() == 0, "Empty");

	std::vector<Operation<int> > updates;
	updates.push_back({Operation<int>::INSERT, 10});
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::INSERT, 30});
	updates.push_back({Operation<int>::INSERT, 40});
	updates.push_back({Operation<int>::INSERT, 50});
	updates.push_back({Operation<int>::INSERT, 60});
	updates.push_back({Operation<int>::INSERT, 70});
	updates.push_back({Operation<int>::INSERT, 80});

	btree.apply_updates(updates);
	assertTrue(btree.height() == 0, "8 elements fit into a leaf");
	assertTrue(btree.get_stats().leaves == 1, "Leave count");

	updates.clear();
	updates.push_back({Operation<int>::INSERT, 5});

	btree.apply_updates(updates);
	assertTrue(btree.height() == 1, "9 elements need 2 leaves");
	assertTrue(btree.get_stats().leaves == 2, "Leave count after overflow");
	assertTrue(btree.get_stats().innernodes == 1, "Innernode count");
	assertTrue(btree.get_stats().itemcount == 9, "Item count");
}

void testSplitLeafInto3() {
	btree<int, std::less<int>, btree_test_set_traits<int>> btree;
	assertTrue(btree.size() == 0, "Empty");

	std::vector<Operation<int> > updates;
	updates.push_back({Operation<int>::INSERT, 10});
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::INSERT, 30});
	updates.push_back({Operation<int>::INSERT, 40});
	updates.push_back({Operation<int>::INSERT, 50});
	updates.push_back({Operation<int>::INSERT, 60});
	updates.push_back({Operation<int>::INSERT, 70});
	updates.push_back({Operation<int>::INSERT, 80});

	btree.apply_updates(updates);
	assertTrue(btree.height() == 0, "8 elements fit into a leaf");
	assertTrue(btree.get_stats().leaves == 1, "Leafcount");

	updates.clear();
	updates.push_back({Operation<int>::INSERT, 1});
	updates.push_back({Operation<int>::INSERT, 2});
	updates.push_back({Operation<int>::INSERT, 3});
	updates.push_back({Operation<int>::INSERT, 4});
	updates.push_back({Operation<int>::INSERT, 5});

	btree.apply_updates(updates);
	assertTrue(btree.height() == 1, "13 elements need 3 leaves");
	assertTrue(btree.get_stats().leaves == 3, "Leafcount");

	updates.clear();
	updates.push_back({Operation<int>::INSERT, 0});
	updates.push_back({Operation<int>::INSERT, 6});
	updates.push_back({Operation<int>::INSERT, 7});
	updates.push_back({Operation<int>::DELETE, 20});
	updates.push_back({Operation<int>::INSERT, 49}); // new router 
	updates.push_back({Operation<int>::DELETE, 50}); // delete existing router after 2nd subtree 
	updates.push_back({Operation<int>::INSERT, 51});
	updates.push_back({Operation<int>::INSERT, 72});
	updates.push_back({Operation<int>::INSERT, 81});

	btree.apply_updates(updates);
	assertTrue(btree.height() == 1, "All elements still fit into 3 leaves");
	assertTrue(btree.get_stats().leaves == 3, "Leafcount");
	assertTrue(btree.get_stats().avgfill_leaves() == 0.75, "Should have 6 elements per leaf");
	assertTrue(btree.size() == 3*6, "Total element count");
}

void testSplitDeepRebalancingInsert() {
	btree<int, std::less<int>, btree_test_set_traits<int>> btree;
	assertTrue(btree.size() == 0, "Empty");

	std::vector<Operation<int> > updates;
	for (int i=0; i<14; ++i) {
		for (int j=0; j<5; j++) {
			updates.push_back({Operation<int>::INSERT, i*10});
		}
	}
	btree.apply_updates(updates);
	assertTrue(btree.height() == 2, "In an optimal tree we need 2 inner node layers");
	assertTrue(btree.get_stats().leaves == 14, "Initial Leafcount");


	// Underflow one leaf. Element should be moved to neihgbor node
	updates.clear();
	updates.push_back({Operation<int>::DELETE, 40});
	updates.push_back({Operation<int>::DELETE, 40});
	updates.push_back({Operation<int>::DELETE, 40});
	updates.push_back({Operation<int>::DELETE, 40});
	btree.apply_updates(updates);

	assertTrue(btree.height() == 2, "In an optimal tree we need 2 inner node layers");
	assertTrue(btree.get_stats().leaves == 13, "Leafcount after leaf underflow");

	// Overflow one leaf 
	updates.clear();
	updates.push_back({Operation<int>::INSERT, 40});
 	updates.push_back({Operation<int>::INSERT, 40});
	updates.push_back({Operation<int>::INSERT, 40});
	updates.push_back({Operation<int>::INSERT, 40});
	btree.apply_updates(updates);
	assertTrue(btree.height() == 2, "In an optimal tree we need 2 inner node layers");
	assertTrue(btree.get_stats().leaves == 14, "Leafcount after leaf overflow");


	// Overflow one leaf and underflow its neighbor
	updates.clear();
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::INSERT, 20});
	updates.push_back({Operation<int>::DELETE, 30});
	updates.push_back({Operation<int>::DELETE, 30});
	updates.push_back({Operation<int>::DELETE, 30});
	updates.push_back({Operation<int>::DELETE, 30});
	btree.apply_updates(updates);
	assertTrue(btree.height() == 2, "In an optimal tree we need 2 inner node layers");
	assertTrue(btree.get_stats().leaves == 14, "Leafcount after leaf overflow & underflow");

	// Bulk insertion into leaf which oveflows the parent node
	updates.clear();
	for (int i=0; i<45; ++i) {
		updates.push_back({Operation<int>::INSERT, 95});
	}
	btree.apply_updates(updates);
	assertTrue(btree.get_stats().innernodes == 1 + 3, "Innernode count after heavy leaf overflow");
}

int main() {
	testBulkUpdatesOfSingleLeaf();
	testSplitLeafInto2();
	testSplitLeafInto3();
	testSplitDeepRebalancingInsert();

	std::cout << "Tests passed successfully." << std::endl;
	return 0;
}