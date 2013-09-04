/*
 * A labelset stores and manages all non-dominated labels of a node.
 *  
 * Author: Stephan Erb
 */
#ifndef SEQ_SHAREDHEAP_LABELSET_H_
#define SEQ_SHAREDHEAP_LABELSET_H_

#include "../options.hpp"
#include "../Graph.hpp"
#include "LabelSetBase.hpp"


/**
  * LabelSet as used by the SharedHeapLabelSettingAlgorithm
  */
template<typename label_type_slot, typename heap_type_slot>
class SharedHeapLabelSet : public LabelSetBase<label_type_slot, LabelWithHandle<label_type_slot> > {
public: 
	typedef LabelWithHandle<label_type_slot> label_type_extended;
	typedef label_type_slot label_type;


protected:
	typedef LabelSetBase<label_type, label_type_extended> B;
	typedef typename heap_type_slot::data_type Data;

	// sorted by increasing x (first_weight) and thus decreasing y (second_weight)
	typename B::Set labels;

public:
	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> set_insertions;
		std::vector<unsigned long> set_dominations;
	#endif

	SharedHeapLabelSet()
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		   : set_insertions(101), set_dominations(101)
		#endif
	{
		const typename label_type::weight_type min = std::numeric_limits<typename label_type::weight_type>::min();
		const typename label_type::weight_type max = std::numeric_limits<typename label_type::weight_type>::max();

		// add sentinals
		labels.reserve(INITIAL_LABELSET_SIZE);
		labels.insert(labels.begin(), label_type_extended(min, max, 0));
		labels.insert(labels.end(), label_type_extended(max, min, 0));
	}

	// Return true if the new_label is non-dominated and has been added
	// as a temporary label to this label set. 
	bool add(const NodeID& node, const label_type&& label, heap_type_slot& heap) {
		typename B::iterator iter;
		if (B::isDominated(labels, label, iter)) {
			#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
				const double size = labels.size()-2; // without both sentinals
				const double position = iter - (labels.begin() + 1); // without leading sentinal 
				if (size > 0) set_dominations[(int) (100.0 * position / size)]++;
			#endif
			return false;
		}
		typename B::iterator first_nondominated = B::y_predecessor(iter, label);

		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			const double size = labels.size()-2; // without both sentinals
			const double position = iter - (labels.begin() + 1); // without leading sentinal 
			if (size > 0) set_insertions[(int) (100.0 * position / size)]++;
		#endif
		if (iter == first_nondominated) {
			// delete range is empty, so just insert
			labels.emplace(first_nondominated, label, heap.push(B::computePriority(label), Data(node, label)));
		} else {
			// replace first dominated label and remove the rest
			iter->first_weight = label.first_weight;
			iter->second_weight = label.second_weight;
			heap.decreaseKey(iter->handle, B::computePriority(label));
			heap.getUserData(iter->handle) = Data(node, label);

			for (typename B::iterator i = ++iter; i != first_nondominated; ++i) {
				heap.deleteNode(i->handle);
			}
			labels.erase(iter, first_nondominated);
		}
		return true;
	}

	void init(const label_type& label) {
		label_type_extended new_label(label);
		labels.insert(++labels.begin(), new_label);
	}

	/* Subtraction used to hide the sentinals */
	std::size_t size() const { return labels.size()-2; }

	typename B::iterator begin() { return ++labels.begin(); }
	typename B::const_iterator begin() const { return ++labels.begin(); }
	typename B::iterator end() { return --labels.end(); }
	typename B::const_iterator end() const { return --labels.end(); }
};


#endif 
