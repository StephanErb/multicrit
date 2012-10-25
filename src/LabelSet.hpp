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


template<typename label_type_slot>
class LabelSet {
public: 
	typedef label_type_slot label_type;
	typedef typename std::vector<label_type>::iterator iterator;
	typedef typename std::vector<label_type>::const_iterator const_iterator;
protected:
	// sorted by increasing x (first_weight) and thus decreasing y (second_weight)
	std::vector<label_type> labels;

	/** First label where the x-coord is truely smaller */
	iterator x_predecessor(iterator iter, const label_type& new_label) {
		while (iter->first_weight < new_label.first_weight) {
			++iter;
		}
		return --iter;
	}

	/** First label where the y-coord is truely smaller */
	iterator y_predecessor(iterator iter, const label_type& new_label) {
		while (iter->second_weight >= new_label.second_weight) {
			++iter;
		}
		return iter;
	}

	void printLabels() {
		std::cout << "All labels:";
		for (iterator i=labels.begin(); i != labels.end(); ++i) {
			std::cout << " (" <<  i->first_weight << "," << i->second_weight << ")";
		}
		std::cout << std::endl;
	}

public:

	LabelSet() {
		typename label_type::weight_type min = std::numeric_limits<typename label_type::weight_type>::min();
		typename label_type::weight_type max = std::numeric_limits<typename label_type::weight_type>::max();
		// add sentinals
		labels.push_back(label_type(min, max));
		labels.push_back(label_type(max, min));
	}

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
		return true;
	}

	/* Subtraction used to hide the sentinals */
	std::size_t size() const { return labels.size()-2; }

	iterator begin() { return labels.begin()+1; }
	const_iterator begin() const { return labels.begin()+1; }
	iterator end() { return labels.end()-1; }
	const_iterator end() const { return labels.end()-1; }
};

#endif 
