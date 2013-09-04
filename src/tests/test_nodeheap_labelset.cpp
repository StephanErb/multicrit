#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE numeral_conversion_tests
#include <boost/test/auto_unit_test.hpp>

#include "../msp_classic/NodeHeapLabelSet.hpp"
#include "../Label.hpp"
#include <iostream>
#include <algorithm>


void assertTrue(bool cond, std::string msg) {
	BOOST_REQUIRE_MESSAGE(cond, msg);
}

template<class LabelSet> 
bool contains(const LabelSet& set, const Label label) {
	return std::find(set.begin(), set.end(), label) != set.end();
}

template<class LabelSet> 
void testSimpleInsertion(LabelSet set) {
	assertTrue(set.size() == 0, "Should be empty");

	Label label = Label(1,10);
	set.add(label);
	assertTrue(set.size() == 1, "Should only contain single label");
	assertTrue(contains(set, label), "Should contain single label");

	set.add(Label(10,1));
	assertTrue(set.size() == 2, "Should have added non-dominated label");

	set.add(Label(10,1));
	assertTrue(set.size() == 2, "Should not have added duplicated label");

	set.add(Label(5,5));
	assertTrue(set.size() == 3, "Should have fitted non-dominated into sequence");

	set.add(Label(5,6));
	assertTrue(set.size() == 3, "Should not have added dominated label (x-coord conflict)");

	set.add(Label(6,6));
	assertTrue(set.size() == 3, "Should not have added dominated label (in field)");

	set.add(Label(4,6));
	assertTrue(set.size() == 4, "Should have fitted non-dominated into sequence");

	set.add(Label(8,4));
	assertTrue(set.size() == 5, "Should have fitted non-dominated into sequence");

	set.add(Label(7,3));
	assertTrue(set.size() == 5, "Should have replaced label");
	assertTrue(contains(set, Label(7,3)), "Should have added new label");
	assertTrue(!contains(set, Label(8,4)), "Should have removed dominated label");

	set.add(Label(4,3));
	assertTrue(set.size() == 3, "Should have replaced range of dominated labels");
	assertTrue(contains(set, Label(4,3)), "Label should have remained");
	assertTrue(contains(set, Label(10,1)), "Label should have remained");
	assertTrue(contains(set, Label(1,10)), "Label should have remained");
}

template<class LabelSet> 
void testAdditionForBorderlineCase1(LabelSet set) {
	assertTrue(set.size() == 0, "Should be empty");
	set.add(Label(5,6));
	assertTrue(set.size() == 1, "Should have fitted non-dominated into sequence");
	set.add(Label(5,5));
	assertTrue(set.size() == 1, "Should have removed dominated label with x-coord conflict");
	assertTrue(contains(set, Label(5,5)), "Label should have remained");
}

template<class LabelSet> 
void testAdditionForBorderlineCase2(LabelSet set) {
	assertTrue(set.size() == 0, "Should be empty");
	set.add(Label(5,6));
	assertTrue(set.size() == 1, "Should have fitted non-dominated into sequence");
	set.add(Label(4,6));
	assertTrue(set.size() == 1, "Should have removed dominated label with y-coord conflict");
	assertTrue(contains(set, Label(4,6)), "Label should have remained");
}

template<class LabelSet> 
void testAdditionForBorderlineCase3(LabelSet set) {
	assertTrue(set.size() == 0, "Should be empty");
	set.add(Label(4,6));
	assertTrue(set.size() == 1, "Should have fitted non-dominated into sequence");
	set.add(Label(5,6));
	assertTrue(set.size() == 1, "Should not have added dominated label with y-coord conflict");
	assertTrue(contains(set, Label(4,6)), "Original label should have remained");
}

template<class LabelSet> 
void testBestLabelInteraction(LabelSet set) {
	assertTrue(!set.hasTemporaryLabels(), "Should be empty");

	set.add(Label(4,5));
	assertTrue(set.hasTemporaryLabels(), "Should not be empty");
	assertTrue(set.getBestTemporaryLabel() == Label(4,5), "Should find initial best label");

	set.add(Label(4,1));
	assertTrue(set.getBestTemporaryLabel() == Label(4,1), "Should handle if old best label is dominated");

	set.add(Label(2,2));
	assertTrue(set.hasTemporaryLabels(), "");
	assertTrue(set.getBestTemporaryLabel() == Label(2,2), "Use better label");
	set.markBestLabelAsPermantent();
	assertTrue(set.hasTemporaryLabels(), "");
	assertTrue(set.getBestTemporaryLabel() == Label(4,1), "Use next best temporary");
	set.markBestLabelAsPermantent();

	assertTrue(!set.hasTemporaryLabels(), "Should be empty");
}


BOOST_AUTO_TEST_CASE(testSimpleInsertion_NaiveLabelSet) {
	testSimpleInsertion(NaiveLabelSet<Label>());
}
BOOST_AUTO_TEST_CASE(testBorderlineCases_NaiveLabelSet) {
	testAdditionForBorderlineCase1(NaiveLabelSet<Label>());
	testAdditionForBorderlineCase2(NaiveLabelSet<Label>());
	testAdditionForBorderlineCase3(NaiveLabelSet<Label>());
}
BOOST_AUTO_TEST_CASE(testBestLabelInteraction_NaiveLabelSet) {
	testBestLabelInteraction(NaiveLabelSet<Label>());
}


BOOST_AUTO_TEST_CASE(testSimpleInsertion_HeapLabelSet) {
	testSimpleInsertion(HeapLabelSet<Label>());
}
BOOST_AUTO_TEST_CASE(testBorderlineCases_HeapLabelSet) {
	testAdditionForBorderlineCase1(HeapLabelSet<Label>());
	testAdditionForBorderlineCase2(HeapLabelSet<Label>());
	testAdditionForBorderlineCase3(HeapLabelSet<Label>());
}
BOOST_AUTO_TEST_CASE(testBestLabelInteraction_HeapLabelSet) {
	testBestLabelInteraction(HeapLabelSet<Label>());
}
