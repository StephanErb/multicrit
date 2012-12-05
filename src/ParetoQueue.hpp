#ifndef PARETO_QUEUE_H_
#define PARETO_QUEUE_H_

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <limits>
#include "utility/datastructure/graph/GraphMacros.h"

template<typename data_type>
struct Operation {
	enum OpType {INSERT, DELETE};
	OpType type;
	data_type data;
 	explicit Operation(const OpType& x, const data_type& y) : type(x), data(y) {}
};

/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename data_type>
class SequentialVectorParetoQueue {
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

	SequentialVectorParetoQueue(const size_t node_count)
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
template<typename data_type>
class SequentialTreeParetoQueue {
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
	typedef std::set<data_type, SetOrderer<data_type> > QueueType;
	QueueType labels;
	
	typedef typename QueueType::iterator iterator;
	typedef typename QueueType::const_iterator const_iterator;
	typedef typename data_type::weight_type weight_type;

	static void printLabels(const std::string msg, const const_iterator begin, const const_iterator end) {
		std::cout << msg;
		for (const_iterator i = begin; i != end; ++i) {
			std::cout << " (" << i->node << ": " << i->first_weight << "," << i->second_weight << ")";
		}
		std::cout << std::endl;
	}

public:

	SequentialTreeParetoQueue(const size_t node_count) {

	}

	void init(const data_type& data) {
		labels.insert(data);
	}

	void findParetoMinima(std::vector<data_type>& minima) const {
		const_iterator iter = labels.begin();
		const_iterator end = labels.end();

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
		u_iterator update_iter = updates.begin();

		while (update_iter != updates.end()) {
			switch (update_iter->type) {
			case Operation<data_type>::DELETE:
			  	labels.erase(update_iter->data);
				++update_iter;
				continue;
			case Operation<data_type>::INSERT:
				labels.insert(update_iter->data);
				++update_iter;
				continue;
	        }
		}
	}

	bool empty() {
		return labels.empty();
	}

	size_t size() {
		return labels.size();
	}
};

#ifdef TREE_PARETO_QUEUE
	template<typename data_type>
	class SequentialParetoQueue : public SequentialTreeParetoQueue<data_type> {
	public:
		SequentialParetoQueue(const size_t node_count):
			SequentialTreeParetoQueue<data_type>(node_count)
		 {}
	};
#else 
	template<typename data_type>
	class SequentialParetoQueue : public SequentialVectorParetoQueue<data_type> {
	public:
		SequentialParetoQueue(const size_t node_count):
			SequentialVectorParetoQueue<data_type>(node_count)
		 {}
	};
#endif

#endif