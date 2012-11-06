/*
 * A labelset stores and manages all non-dominated labels of a node.
 *  
 * Author: Stephan Erb
 */
#ifndef LABELSET_H_
#define LABELSET_H_

//#define TREE_SET
#ifdef TREE_SET
	#include <set>
#else
 	#include <vector>
#endif

#include <iostream>
#include <limits>
#include <algorithm>


/**
 * Common functions & types shared by all label set implementations.
 */
template<typename label_type_slot>
class LabelSetBase {
public:
	typedef unsigned int Priority;

	static Priority computePriority(const label_type_slot& label) {
		return label.first_weight  + label.second_weight;
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
		iterator iter = labels.lower_bound(new_label);
		--iter;
		return iter;
#else
		return std::lower_bound(labels.begin(), labels.end(), new_label, firstWeightLess)-1;
#endif	
	}

	/** First label where the y-coord is truly smaller */
	static iterator y_predecessor(const iterator begin, const iterator end, const label_type_slot& new_label) {
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
		typename B::iterator first_nondominated = B::y_predecessor(iter, labels.end(), new_label);

#ifdef TREE_SET
		labels.erase(iter, first_nondominated);
		labels.insert(new_label);
#else
		if (iter == first_nondominated) {
			// delete range is empty, so just insert
			labels.insert(new_label);
		} else {
			// replace first dominated label and remove the rest
			*iter = new_label;
			labels.erase(++iter, first_nondominated);
		}
#endif
		updateBestLabelIfNeeded(new_label);
		return true;
	}

	// FIXME: The following implementations is very naive and thus horribly slow
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


template<typename label_type_slot>
class LabelSet : public NaiveLabelSet<label_type_slot> {};

#endif 
