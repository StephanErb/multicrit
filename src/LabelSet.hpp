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


template<typename label_type_slot>
class LabelSet {
public: 

	// Extend label type with flag to mark if the label is still 
	// tentative/temporary or already permanent.
	class label_type : public label_type_slot {
	public:

		label_type() {}

		label_type(const label_type_slot& label):
			label_type_slot(label.first_weight, label.second_weight),
			permanent(false)
		{}

		label_type(const typename label_type::weight_type first, const typename label_type::weight_type second, const bool permanent_):
			label_type_slot(first, second),
			permanent(permanent_)
		{}

	    bool permanent;
	};
	typedef typename std::vector<label_type>::iterator iterator;
	typedef typename std::vector<label_type>::const_iterator const_iterator;
	typedef unsigned int Priority;

protected:

	// sorted by increasing x (first_weight) and thus decreasing y (second_weight)
	std::vector<label_type> labels;

	size_t perm_label_counter;

	label_type best_label;
	Priority best_label_priority; 

	static bool firstWeightLess(const label_type& i, const label_type& j) {
		return i.first_weight < j.first_weight;
	}

	static bool secondWeightGreaterOrEquals(const label_type& i, const label_type& j) {
		return i.second_weight >= j.second_weight;
	}

	/** First label where the x-coord is truly smaller */
	iterator x_predecessor(iterator iter, const label_type& new_label) {
		return std::lower_bound(labels.begin(), labels.end(), new_label, firstWeightLess)-1;
	}

	/** First label where the y-coord is truly smaller */
	iterator y_predecessor(iterator iter, const label_type& new_label) {
		return std::lower_bound(iter, labels.end(), new_label, secondWeightGreaterOrEquals);
	}

	void updateBestLabelIfNeeded(const label_type& label) {
		Priority priority = computePriority(label);
		if (priority >= best_label_priority) {
			best_label_priority = priority;
			best_label = label;
		}
	}

	void printLabels() {
		std::cout << "All labels:";
		for (iterator i=labels.begin(); i != labels.end(); ++i) {
			std::cout << " (" <<  i->first_weight << "," << i->second_weight << ")";
			if (!i->permanent) {
				std::cout << "?";
			}
		}
		std::cout << std::endl;
	}

public:

	LabelSet() {
		const typename label_type::weight_type min = std::numeric_limits<typename label_type::weight_type>::min();
		const typename label_type::weight_type max = std::numeric_limits<typename label_type::weight_type>::max();

		// add sentinals
		labels.push_back(label_type(min, max, /*permanent*/ true));
		labels.push_back(label_type(max, min, /*permanent*/ true));

		best_label = label_type(max, min, /*permanent*/ true); // one of the sentinals
		best_label_priority = computePriority(best_label);

		perm_label_counter = 2; // sentinals
	}

	// Return true if the new_label is non-dominated and has been added
	// as a temporary label to this label set. 
	bool add(const label_type& new_label) {
		iterator iter = x_predecessor(labels.begin(), new_label);

		if (iter->second_weight <= new_label.second_weight) {
			return false; // new label is dominated
		}
		++iter; // move to element with greater or equal x-coordinate

		if (iter->first_weight == new_label.first_weight && 
			iter->second_weight <= new_label.second_weight) {
			// new label is dominated by the (single) pre-existing element with the same
			// x-coordinate.
			return false; 
		}
		iterator first_nondominated = y_predecessor(iter, new_label);

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

		// Scan all labels: Find new best label and mark the old one as permanent.
		for (iterator i = labels.begin(); i != labels.end(); ++i) {
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

	Priority getPriorityOfBestTemporaryLabel() {
		return best_label_priority;
	}

	label_type getBestTemporaryLabel() {
		return best_label;
	}

	static Priority computePriority(const label_type& label) {
		return label.first_weight + label.second_weight;
	}

	/* Subtraction used to hide the sentinals */
	std::size_t size() const { return labels.size()-2; }

	iterator begin() { return labels.begin()+1; }
	const_iterator begin() const { return labels.begin()+1; }
	iterator end() { return labels.end()-1; }
	const_iterator end() const { return labels.end()-1; }

};

#endif 
