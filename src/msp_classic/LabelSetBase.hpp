/*
 * A labelset stores and manages all non-dominated labels of a node.
 *  
 * Author: Stephan Erb
 */
#ifndef SEQ_LABELSET_BASE_H_
#define SEQ_LABELSET_BASE_H_

#include "../options.hpp"

#include <set>
#include <vector>

#include <iostream>
#include <limits>
#include <algorithm>

/**
 * Common functions & types shared by all label set implementations.
 */
template<typename label_type, typename label_type_extended>
class LabelSetBase {
public:
	typedef uint64_t Priority;

	static inline Priority computePriority(const label_type& label) {
		#ifdef PRIORITY_SUM
			return (uint64_t) label.first_weight + (uint64_t) label.second_weight;
		#endif
		#ifdef PRIORITY_LEX
			return label.combinedWeight();
		#endif
		#ifdef PRIORITY_MAX
			return std::max(label.first_weight, label.second_weight);
		#endif
	}

#ifdef TREE_SET
	struct SetOrderer {
  		inline bool operator() (const label_type_extended& i, const label_type_extended& j) const {
  			return i.first_weight < j.first_weight;
  		}
	};
	typedef std::set<label_type_extended, SetOrderer> Set;
#else
	typedef std::vector<label_type_extended> Set;
#endif

	typedef typename Set::iterator iterator;
	typedef typename Set::const_iterator const_iterator;

protected:
	static struct WeightLessComp {
		inline bool operator() (const label_type& i, const label_type& j) const {
			return i.first_weight < j.first_weight;
		}
	} firstWeightLess;

	static inline bool secondWeightGreaterOrEquals(const label_type& i, const label_type& j) {
		return i.second_weight >= j.second_weight;
	}

	/** First label where the x-coord is truly smaller */
	static inline iterator x_predecessor(Set& labels, const label_type& new_label) {
#ifdef TREE_SET
		return --labels.lower_bound(label_type_extended(new_label));
#else
		return --std::lower_bound(labels.begin(), labels.end(), new_label, firstWeightLess);
#endif	
	}

	/** First label where the y-coord is truly smaller */
	static inline iterator y_predecessor(const iterator& begin, const label_type& new_label) {
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

	static inline bool isDominated(Set& labels, const label_type& new_label, iterator& iter) {
		// Short cut domination check with the last element
		iter = labels.begin() + labels.size()-2; // element before the closing sentinal
		if (iter->first_weight < new_label.first_weight) {
			if (iter->second_weight <= new_label.second_weight) {
				return true; // new label is dominated
			} else {
				++iter;
				return false;
			}
		}
		// Exact check via binary search
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

	static inline void insertAndRemoveDominated(Set &labels, const label_type_extended& new_label, iterator iter) {
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
	explicit LabelWithFlag(const label_type_slot& label):
		label_type_slot(label.first_weight, label.second_weight),
		permanent(false)
	{}
	explicit LabelWithFlag(const typename label_type_slot::weight_type first, const typename label_type_slot::weight_type second, const bool permanent_):
		label_type_slot(first, second),
		permanent(permanent_)
	{}
    bool mutable permanent;
};

// Extend label type with a priority queue handle
template<typename label_type_slot>
class LabelWithHandle : public label_type_slot {
public:
	inline explicit LabelWithHandle(const label_type_slot& label, const size_t _handle=0):
		label_type_slot(label.first_weight, label.second_weight),
		handle(_handle)
	{}
	inline explicit LabelWithHandle(label_type_slot&& label, const size_t _handle=0):
		label_type_slot(label.first_weight, label.second_weight),
		handle(_handle)
	{}
	inline explicit LabelWithHandle(const typename label_type_slot::weight_type first, const typename label_type_slot::weight_type second, const size_t handle):
		label_type_slot(first, second),
		handle(handle)
	{}
    size_t mutable handle;
};

#endif 
