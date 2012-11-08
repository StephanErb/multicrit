/*
 * A labelset stores and manages all non-dominated labels of a node.
 *  
 * Author: Stephan Erb
 */
#ifndef LABELSET_H_
#define LABELSET_H_

#include "options.hpp"
#ifdef TREE_SET
	#include <set>
#else
 	#include <vector>
#endif

#include <iostream>
#include <limits>
#include <algorithm>
#include "utility/datastructure/graph/GraphTypes.hpp"


/**
 * Common functions & types shared by all label set implementations.
 */
template<typename label_type_slot>
class LabelSetBase {
public:
	typedef unsigned int Priority;

	static Priority computePriority(const label_type_slot& label) {
		#ifdef PRIORITY_SUM
			return label.first_weight + label.second_weight;
		#endif
		#ifdef PRIORITY_LEX
			return label.first_weight * MAX_COST + label.second_weight;
		#endif
		#ifdef PRIORITY_MAX
			return std::max(label.first_weight, label.second_weight);
		#endif
	}

#ifdef TREE_SET
	struct SetOrderer {
  		bool operator() (const label_type_slot& i, const label_type_slot& j) const {
  			return i.first_weight < j.first_weight;
  		}
	};
	typedef std::set<label_type_slot, SetOrderer> Set;
#else
	typedef std::vector<label_type_slot> Set;
#endif

	typedef typename Set::iterator iterator;
	typedef typename Set::const_iterator const_iterator;

protected:
	static bool firstWeightLess(const label_type_slot& i, const label_type_slot& j)  {
		return i.first_weight < j.first_weight;
	}

	static bool secondWeightGreaterOrEquals(const label_type_slot& i, const label_type_slot& j) {
		return i.second_weight >= j.second_weight;
	}

	/** First label where the x-coord is truly smaller */
	static iterator x_predecessor(Set& labels, const label_type_slot& new_label) {
#ifdef TREE_SET
		return --labels.lower_bound(new_label);
#else
		return --std::lower_bound(labels.begin(), labels.end(), new_label, firstWeightLess);
#endif	
	}

	/** First label where the y-coord is truly smaller */
	static iterator y_predecessor(const iterator& begin, const label_type_slot& new_label) {
		iterator i = begin;
		while (secondWeightGreaterOrEquals(*i, new_label)) {
			++i;
		}
		return i;
	}

	static void printLabels(iterator begin, iterator end) {
		std::cout << "All labels:";
		for (iterator i = begin; i != end; ++i) {
			std::cout << " (" <<  i->first_weight << "," << i->second_weight << ")";
			if (!i->permanent) {
				std::cout << "?";
			}
		}
		std::cout << std::endl;
	}

	static bool isDominated(Set& labels, const label_type_slot& new_label, iterator& iter) {
		iter = x_predecessor(labels, new_label);

		if (iter->second_weight <= new_label.second_weight) {
			return true; // new label is dominated
		}
		++iter; // move to element with greater or equal x-coordinate

		if (iter->first_weight == new_label.first_weight && 
			iter->second_weight <= new_label.second_weight) {
			// new label is dominated by the (single) pre-existing element with the same
			// x-coordinate.
			return true; 
		}
		return false;
	}

	static void insertAndRemoveDominated(Set &labels, const label_type_slot& new_label, iterator iter) {
		iterator first_nondominated = y_predecessor(iter, new_label);

#ifdef TREE_SET
		labels.erase(iter--, first_nondominated);
		labels.insert(iter, new_label);
#else
		if (iter == first_nondominated) {
			// delete range is empty, so just insert
			labels.insert(first_nondominated, new_label);
		} else {
			// replace first dominated label and remove the rest
			*iter = new_label;
			labels.erase(++iter, first_nondominated);
		}
#endif
	}
};


// Extend label type with a flag to mark if the label is still 
// tentative/temporary or already permanent.
template<typename label_type_slot>
class LabelWithFlag : public label_type_slot {
public:
	LabelWithFlag() {}
	LabelWithFlag(const label_type_slot& label):
		label_type_slot(label.first_weight, label.second_weight),
		permanent(false)
	{}
	LabelWithFlag(const typename label_type_slot::weight_type first, const typename label_type_slot::weight_type second, const bool permanent_):
		label_type_slot(first, second),
		permanent(permanent_)
	{}
    bool mutable permanent;
};


/**
  * Naive implementation: Find the next best label of this node by scanning all labels.
  */
template<typename label_type_slot>
class NaiveLabelSet : public LabelSetBase<LabelWithFlag<label_type_slot> > {
public: 
	typedef LabelWithFlag<label_type_slot> label_type;

protected:
	typedef LabelSetBase<label_type> B;

	// sorted by increasing x (first_weight) and thus decreasing y (second_weight)
	typename B::Set labels;
	size_t perm_label_counter;

	label_type best_label;
	typename B::Priority best_label_priority; 

	void updateBestLabelIfNeeded(const label_type& label) {
		typename B::Priority priority = B::computePriority(label);
		if (priority < best_label_priority) {
			best_label_priority = priority;
			best_label = label;
		}
	}

public:
	NaiveLabelSet() {
		const typename label_type::weight_type min = std::numeric_limits<typename label_type::weight_type>::min();
		const typename label_type::weight_type max = std::numeric_limits<typename label_type::weight_type>::max();

		// add sentinals
		labels.insert(labels.begin(), label_type(min, max, /*permanent*/ true));
		labels.insert(labels.end(), label_type(max, min, /*permanent*/ true));

		best_label = *labels.begin(); // one of the sentinals
		best_label_priority = B::computePriority(best_label);

		perm_label_counter = 2; // sentinals
	}

	// Return true if the new_label is non-dominated and has been added
	// as a temporary label to this label set. 
	bool add(const label_type& new_label) {
		typename B::iterator iter;
		if (B::isDominated(labels, new_label, iter)) {
			return false;
		}
		B::insertAndRemoveDominated(labels, new_label, iter);
		updateBestLabelIfNeeded(new_label);
		return true;
	}

	void markBestLabelAsPermantent() {
		label_type old_best_label = best_label;
		best_label_priority = B::computePriority(*labels.begin()); // prio of sentinal

		// Scan all labels: Find new best label and mark the old one as permanent.
		for (typename B::iterator i = labels.begin(); i != labels.end(); ++i) {
			if (i->permanent) {
				continue;
			}
			if (i->first_weight == old_best_label.first_weight && i->second_weight == old_best_label.second_weight) {
				i->permanent = true;
				++perm_label_counter;
				continue;
			}
			updateBestLabelIfNeeded(*i);
		}
	}

	bool hasTemporaryLabels() const {
		return labels.size() > perm_label_counter;
	}

	typename B::Priority getPriorityOfBestTemporaryLabel() const {
		return best_label_priority;
	}

	label_type getBestTemporaryLabel() const {
		return best_label;
	}

	/* Subtraction used to hide the sentinals */
	std::size_t size() const { return labels.size()-2; }

	typename B::iterator begin() { return labels.begin(); }
	typename B::const_iterator begin() const { return labels.begin(); }
	typename B::iterator end() { return labels.end(); }
	typename B::const_iterator end() const { return labels.end(); }
};



/**
  * Similar to the naive implementation, but temporary labels and 
  * permanent labels are kept in different sets, limiting the set
  * of elements that have to be scanned constanly.
  */
template<typename label_type_slot>
class SplittedNaiveLabelSet : public LabelSetBase<label_type_slot > {
public: 
	typedef label_type_slot label_type;

protected:
	typedef LabelSetBase<label_type> B;

	// sorted by increasing x (first_weight) and thus decreasing y (second_weight)
	typename B::Set perm_labels;
	typename B::Set temp_labels;

	label_type best_label;
	typename B::Priority best_label_priority; 

	void updateBestLabelIfNeeded(const label_type& label) {
		typename B::Priority priority = B::computePriority(label);
		if (priority < best_label_priority) {
			best_label_priority = priority;
			best_label = label;
		}
	}

public:
	SplittedNaiveLabelSet() {
		const typename label_type::weight_type min = std::numeric_limits<typename label_type::weight_type>::min();
		const typename label_type::weight_type max = std::numeric_limits<typename label_type::weight_type>::max();

		// add sentinals
		perm_labels.insert(perm_labels.begin(), label_type(min, max));
		perm_labels.insert(perm_labels.end(), label_type(max, min));

		temp_labels.insert(temp_labels.begin(), label_type(min, max));
		temp_labels.insert(temp_labels.end(), label_type(max, min));

		best_label = *temp_labels.begin(); // one of the sentinals
		best_label_priority = B::computePriority(best_label);
	}

	// Return true if the new_label is non-dominated and has been added
	// as a temporary label to this label set. 
	bool add(const label_type& new_label) {
		typename B::iterator iter;
		if (B::isDominated(temp_labels, new_label, iter)) {
			return false;
		}
		typename B::iterator unused;
		if (B::isDominated(perm_labels, new_label, unused)) {
			return false;
		}
		B::insertAndRemoveDominated(temp_labels, new_label, iter);
		updateBestLabelIfNeeded(new_label);
		return true;
	}

	void markBestLabelAsPermantent() {
		if (!hasTemporaryLabels()) {
			return;
		}
		best_label_priority = B::computePriority(*temp_labels.begin()); // prio of sentinal
		const label_type old_best_label = best_label;
		typename B::iterator i = temp_labels.begin();

#ifdef TREE_SET
		perm_labels.insert(best_label);
		while (i != temp_labels.end()) {
			if (i->first_weight == old_best_label.first_weight && i->second_weight == old_best_label.second_weight) {
				temp_labels.erase(i--);
			}
			updateBestLabelIfNeeded(*i);
			++i;
		}
#else
		perm_labels.insert(std::lower_bound(perm_labels.begin(), perm_labels.end(), best_label, B::firstWeightLess), best_label);
		while (i != temp_labels.end()) {
			if (i->first_weight == old_best_label.first_weight && i->second_weight == old_best_label.second_weight) {
				i = temp_labels.erase(i);
			}
			updateBestLabelIfNeeded(*i);
			++i;
		}	
#endif 
	}

	bool hasTemporaryLabels() const {
		return temp_labels.size() > 2;
	}

	typename B::Priority getPriorityOfBestTemporaryLabel() const {
		return best_label_priority;
	}

	label_type getBestTemporaryLabel() const {
		return best_label;
	}

	/* Subtraction used to hide the sentinals */
	std::size_t size() const { return perm_labels.size() + temp_labels.size()-4; }

	typename B::iterator begin() { return perm_labels.begin(); }
	typename B::const_iterator begin() const { return perm_labels.begin(); }
	typename B::iterator end() { return perm_labels.end(); }
	typename B::const_iterator end() const { return perm_labels.end(); }
};


// Extend label type with a priority queue handle
template<typename label_type_slot>
class LabelWithHandle : public label_type_slot {
public:
	LabelWithHandle() {}
	LabelWithHandle(const label_type_slot& label):
		label_type_slot(label.first_weight, label.second_weight),
		handle(0)
	{}
	LabelWithHandle(const typename label_type_slot::weight_type first, const typename label_type_slot::weight_type second, const size_t handle):
		label_type_slot(first, second),
		handle(handle)
	{}
    size_t mutable handle;
};


/**
  * LabelSet as used by the SharedHeapLabelSettingAlgorithm
  */
template<typename label_type_slot, typename heap_type_slot>
class SharedHeapLabelSet : public LabelSetBase<LabelWithHandle<label_type_slot> > {
public: 
	typedef LabelWithHandle<label_type_slot> label_type;

protected:
	typedef LabelSetBase<label_type> B;
	typedef utility::datastructure::NodeID NodeID;
	typedef typename heap_type_slot::data_type Data;

	// sorted by increasing x (first_weight) and thus decreasing y (second_weight)
	typename B::Set labels;


public:
	SharedHeapLabelSet() {
		const typename label_type::weight_type min = std::numeric_limits<typename label_type::weight_type>::min();
		const typename label_type::weight_type max = std::numeric_limits<typename label_type::weight_type>::max();

		// add sentinals
		labels.insert(labels.begin(), label_type(min, max, /*permanent*/ true));
		labels.insert(labels.end(), label_type(max, min, /*permanent*/ true));
	}

	// Return true if the new_label is non-dominated and has been added
	// as a temporary label to this label set. 
	bool add(const NodeID& node, const label_type& new_label, heap_type_slot& heap) {
		typename B::iterator iter;
		if (B::isDominated(labels, new_label, iter)) {
			return false;
		}

		typename B::iterator first_nondominated = B::y_predecessor(iter, new_label);

#ifdef TREE_SET

		labels.erase(iter--, first_nondominated);
		labels.insert(iter, new_label);
#else
		if (iter == first_nondominated) {
			// delete range is empty, so just insert
			new_label.handle = heap.push(B::computePriority(new_label), Data(node, new_label));
			labels.insert(first_nondominated, new_label);
		} else {
			// replace first dominated label and remove the rest
			iter->first_weight = new_label.first_weight;
			iter->second_weight = new_label.second_weight;
			heap.decreaseKey(iter->handle, B::computePriority(new_label));
			heap.getUserData(iter->handle) = Data(node, new_label);

			for (typename B::iterator i = ++iter; i != first_nondominated; ++i) {
				heap.deleteNode(i->handle);
			}
			labels.erase(iter, first_nondominated);
		}
#endif
		return true;
	}

	/* Subtraction used to hide the sentinals */
	std::size_t size() const { return labels.size()-2; }

	typename B::iterator begin() { return labels.begin(); }
	typename B::const_iterator begin() const { return labels.begin(); }
	typename B::iterator end() { return labels.end(); }
	typename B::const_iterator end() const { return labels.end(); }
};



template<typename label_type_slot>
class LabelSet : public LABEL_SET<label_type_slot> {};

#endif 
