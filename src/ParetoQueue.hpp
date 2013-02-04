#ifndef PARETO_QUEUE_H_
#define PARETO_QUEUE_H_

#include <iostream>
#include <vector>

#include "options.hpp"

#define COMPUTE_PARETO_MIN
#include "datastructures/BTree.hpp"

#include <algorithm>
#include <limits>
#include "utility/datastructure/graph/GraphMacros.h"


/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename data_type, typename min_key_type=data_type>
class VectorParetoQueue {
private:

	typedef std::vector<data_type> QueueType;
	QueueType labels;
	QueueType temp;
	
	typedef typename QueueType::iterator iterator;
	typedef typename QueueType::const_iterator const_iterator;
	typedef typename data_type::weight_type weight_type;

	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> pq_inserts;
		std::vector<unsigned long> pq_deletes;
	#endif

	static void printLabels(const std::string msg, const const_iterator begin, const const_iterator end) {
		std::cout << msg;
		for (const_iterator i = begin; i != end; ++i) {
			std::cout << " (" << i->node << ": " << i->first_weight << "," << i->second_weight << ")";
		}
		std::cout << std::endl;
	}

public:

	VectorParetoQueue(const size_t node_count)
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		: pq_inserts(101), pq_deletes(101)
		#endif
	{
		const weight_type min = std::numeric_limits<weight_type>::min();
		const weight_type max = std::numeric_limits<weight_type>::max();

		labels.reserve(node_count); // rough approximiation
		temp.reserve(node_count);

		// add sentinals
		labels.insert(labels.begin(), data_type(NodeID(0), typename data_type::label_type(min, max)));
		labels.insert(labels.end(), data_type(NodeID(0), typename data_type::label_type(max, min)));
	}

	void init(const data_type& data) {
		labels.insert(++labels.begin(), data);
	}

	void findParetoMinima(std::vector<data_type>& minima) const {
		const_iterator iter = ++labels.begin(); // ignore the sentinal
		const_iterator end = --labels.end();  // ignore the sentinal

		data_type min = *iter;
		while (iter != end) {
			if (iter->second_weight < min.second_weight ||
					(iter->first_weight == min.first_weight && iter->second_weight == min.second_weight)) {
				min = *iter;
				minima.push_back(*iter);
			}
			++iter;
		}
	}

	void applyUpdates(const std::vector<Operation<data_type> >& updates) {
		typedef typename std::vector<Operation<data_type> >::const_iterator u_iterator;

		iterator label_iter = labels.begin();
		u_iterator update_iter = updates.begin();

		temp.clear();

		while (update_iter != updates.end()) {
			switch (update_iter->type) {
			case Operation<data_type>::DELETE:
			  	// We know the element is in here
				while (label_iter->first_weight != update_iter->data.first_weight ||
						label_iter->second_weight != update_iter->data.second_weight ||
						label_iter->node != update_iter->data.node) {
					temp.push_back(*label_iter++);
				}
				#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
					pq_deletes[(int)(100*((label_iter - labels.begin())/(double)labels.size()) + 0.5)]++;
				#endif
				++label_iter; // delete the element by jumping over it
                ++update_iter;
                continue;
	        case Operation<data_type>::INSERT:
				// insertion bounded by sentinal
				while (label_iter->first_weight < update_iter->data.first_weight) {
					temp.push_back(*label_iter++);
				}
				while (label_iter->first_weight == update_iter->data.first_weight
						&& label_iter->second_weight < update_iter->data.second_weight) {
					temp.push_back(*label_iter++);
				}
				while (label_iter->first_weight == update_iter->data.first_weight
						&& label_iter->second_weight == update_iter->data.second_weight
						&& label_iter->node < update_iter->data.node) {
					temp.push_back(*label_iter++);
				}
				#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
					pq_inserts[(int)(100*((label_iter - labels.begin())/(double)labels.size()) + 0.5)]++;
				#endif
				temp.push_back(update_iter->data);
				++update_iter;
				continue;
	        }
		}
		std::copy(label_iter, labels.end(), std::back_inserter(temp));
	    labels.swap(temp);
	}

	void printStatistics() {
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			std::cout << "# ParetoQueue Insertions, Deletions" << std::endl;
			for (size_t i=0; i < 101; ++i) {
				std::cout << i << " " << pq_inserts[i] << " " << pq_deletes[i] << std::endl;
			}
		#endif
	}

	bool empty() {
		return size() == 0;
	}

	size_t size() {
		return labels.size() -2;
	}
};

/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename data_type, typename min_key_type=data_type>
class BTreeParetoQueue {
private:

	template<typename type>
	struct SetOrderer {
  		bool operator() (const type& i, const type& j) const {
  			if (i.first_weight == j.first_weight) {
  				if (i.second_weight == j.second_weight) {
  					return i.node < j.node;
  				}
  				return i.second_weight < j.second_weight;
  			}
  			return i.first_weight < j.first_weight;
  		}
	};
	typedef btree<data_type, min_key_type, SetOrderer<data_type>> QueueType;
	QueueType labels;

	typedef typename data_type::weight_type weight_type;
	data_type min_label;
	
public:

	BTreeParetoQueue(const size_t node_count)
		: labels(node_count)
	{
		const weight_type min = std::numeric_limits<weight_type>::min();
		const weight_type max = std::numeric_limits<weight_type>::max();

		min_label = data_type(NodeID(0), typename data_type::label_type(min, max));
	}

	void init(const data_type& data) {
		const std::vector<Operation<data_type>> upds = {{Operation<data_type>::INSERT, data}};
		labels.apply_updates(upds);
	}

	void findParetoMinima(std::vector<data_type>& minima) const {
		labels.find_pareto_minima(min_label, minima);
	}

	void applyUpdates(const std::vector<Operation<data_type>>& updates) {
		labels.apply_updates(updates);
	}

	bool empty() {
		return labels.empty();
	}

	size_t size() {
		return labels.size();
	}

	void printStatistics() {
		std::cout << QueueType::name() << ": " << std::endl;
		std::cout << "  inner slots size [" << QueueType::innerslotmin << ", " << QueueType::innerslotmax << "]" << std::endl;
		std::cout << "  leaf slots size [" << QueueType::leafslotmin << ", " << QueueType::leafslotmax << "]" << std::endl;
	}
};

template<typename data_type, typename min_key_type>
class ParetoQueue : public PARETO_QUEUE<data_type, min_key_type> {
public:
	ParetoQueue(const size_t node_count):
		PARETO_QUEUE<data_type>(node_count)
	 {}
};


#endif