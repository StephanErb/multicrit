#ifndef PARETO_QUEUE_H_
#define PARETO_QUEUE_H_

#include <iostream>
#include <vector>

#include "options.hpp"

#define COMPUTE_PARETO_MIN
#include "datastructures/BTree_sequential.hpp"
#undef COMPUTE_PARETO_MIN
#include "Label.hpp"

#include <algorithm>
#include <limits>
#include "utility/datastructure/graph/GraphMacros.h"


/**
 * Queue storing all temporary labels of all nodes.
 */

class VectorParetoQueue {
private:

	typedef std::vector<NodeLabel> QueueType;
	QueueType labels;
	QueueType temp;
	
	typedef typename QueueType::iterator iterator;
	typedef typename QueueType::const_iterator const_iterator;
	typedef typename Label::weight_type weight_type;

	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> pq_inserts;
		std::vector<unsigned long> pq_deletes;
	#endif

public:

	VectorParetoQueue()
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		: pq_inserts(101), pq_deletes(101)
		#endif
	{
		const weight_type min = std::numeric_limits<weight_type>::min();
		const weight_type max = std::numeric_limits<weight_type>::max();

		labels.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
		temp.reserve(LARGE_ENOUGH_FOR_EVERYTHING);

		// add sentinals
		labels.insert(labels.begin(), NodeLabel(NodeID(0), Label(min, max)));
		labels.insert(labels.end(), NodeLabel(NodeID(0), Label(max, min)));
	}

	void init(const NodeLabel& data) {
		labels.insert(++labels.begin(), data);
	}

    template<typename upd_sequence_type, typename cand_sequence_type, typename graph_type>
	void findParetoMinima(upd_sequence_type& updates, cand_sequence_type& candidates, const graph_type& graph) const {
		const_iterator iter = ++labels.begin(); // ignore the sentinal
		const_iterator end = --labels.end();  // ignore the sentinal

		const_iterator min = iter;
		while (iter != end) {
			if (iter->second_weight < min->second_weight ||
					(iter->first_weight == min->first_weight && iter->second_weight == min->second_weight)) {
				min = iter;
				updates.emplace_back(Operation<NodeLabel>::DELETE, *iter);
				FORALL_EDGES(graph, iter->node, eid) {
                	const auto& edge = graph.getEdge(eid);
                    candidates.emplace_back(edge.target, iter->first_weight + edge.first_weight, 
                    	                                 iter->second_weight + edge.second_weight);
                }
			}
			++iter;
		}
	}

	void applyUpdates(const std::vector<Operation<NodeLabel> >& updates) {
		typedef typename std::vector<Operation<NodeLabel> >::const_iterator u_iterator;

		iterator label_iter = labels.begin();
		u_iterator update_iter = updates.begin();

		temp.clear();

		while (update_iter != updates.end()) {
			switch (update_iter->type) {
			case Operation<NodeLabel>::DELETE:
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
	        case Operation<NodeLabel>::INSERT:
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
class BTreeParetoQueue {
private:


	typedef btree<NodeLabel, Label, GroupNodeLablesByWeightAndNodeComperator> QueueType;
	QueueType labels;

	typedef typename Label::weight_type weight_type;
	const Label min_label;
	
public:

	BTreeParetoQueue()
		: min_label(std::numeric_limits<weight_type>::min(), std::numeric_limits<weight_type>::max())
	{}

	void init(const NodeLabel& data) {
		std::vector<Operation<NodeLabel>> upds;
		upds.emplace_back(Operation<NodeLabel>::INSERT, data);
		labels.apply_updates(upds, INSERTS_ONLY);
	}

    template<typename upd_sequence_type, typename cand_sequence_type, typename graph_type>
	void findParetoMinima(upd_sequence_type& updates, cand_sequence_type& candidates, const graph_type& graph) const {
		labels.find_pareto_minima(min_label, updates, candidates, graph);
	}

	void applyUpdates(const std::vector<Operation<NodeLabel>>& updates) {
		labels.apply_updates(updates, INSERTS_AND_DELETES);
	}

	bool empty() {
		return labels.empty();
	}

	size_t size() {
		return labels.size();
	}

	void printStatistics() {
		std::cout << "# " << QueueType::name() << ": " << std::endl;
		std::cout << "#  inner slots size [" << QueueType::innerslotmin << ", " << QueueType::innerslotmax << "]" << std::endl;
		std::cout << "#  leaf slots size [" << QueueType::leafslotmin << ", " << QueueType::leafslotmax << "]" << std::endl;
	}
};

class ParetoQueue : public PARETO_QUEUE {
public:
	ParetoQueue():
		PARETO_QUEUE()
	 {}
};


#endif
