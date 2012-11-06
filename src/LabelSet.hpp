/*
 * A labelset stores and manages all non-dominated labels of a node.
 *  
 * Author: Stephan Erb
 */
#ifndef LABELSET_H_
#define LABELSET_H_

#include <vector>
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

	typedef std::vector<label_type_slot> Set;
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
	static iterator x_predecessor(const iterator begin, const iterator end, const label_type_slot& new_label) {
		return std::lower_bound(begin, end, new_label, firstWeightLess)-1;
	}

	/** First label where the y-coord is truly smaller */
	static iterator y_predecessor(const iterator begin, const iterator end, const label_type_slot& new_label) {
		return std::lower_bound(begin, end, new_label, secondWeightGreaterOrEquals);
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
		iter = x_predecessor(labels.begin(), labels.end(), new_label);

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
    bool permanent;
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
		labels.push_back(label_type(min, max, /*permanent*/ true));
		labels.push_back(label_type(max, min, /*permanent*/ true));

		best_label = labels[0]; // one of the sentinals
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

		if (iter == first_nondominated) {
			// delete range is empty, so just insert
			labels.insert(first_nondominated, new_label);
		} else {
			// replace first dominated label and remove the rest
			*iter = new_label;
			labels.erase(++iter, first_nondominated);
		}
		updateBestLabelIfNeeded(new_label);
		return true;
	}

	// FIXME: The following implementations is very naive and thus horribly slow
	void markBestLabelAsPermantent() {
		label_type old_best_label = best_label;
		best_label_priority = B::computePriority(labels[0]); // prio of sentinal

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

	typename B::iterator begin() { return labels.begin()+1; }
	typename B::const_iterator begin() const { return labels.begin()+1; }
	typename B::iterator end() { return labels.end()-1; }
	typename B::const_iterator end() const { return labels.end()-1; }
};


template<typename label_type_slot>
class LabelSet : public NaiveLabelSet<label_type_slot> {};

#endif 
