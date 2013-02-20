/*
 * A implementation of a bi-objective shortest path label setting algorithm
 * that always relaxes _all_ globaly pareto optimal labels instead of just one.
 *  
 * Author: Stephan Erb
 */
#ifndef PARALLEL_PARETO_SEARCH_H_
#define PARALLEL_PARETO_SEARCH_H_

#include "utility/datastructure/graph/GraphMacros.h"
#include "options.hpp"
#include "ParetoQueue_parallel.hpp"
#include "ParetoSearchStatistics.hpp"
#include "assert.h"

#include "tbb/parallel_sort.h"
#include "tbb/concurrent_vector.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/task.h"
#include "tbb/atomic.h"

#ifdef GATHER_SUBCOMPNENT_TIMING
#include "tbb/tick_count.h"
#endif


template<typename graph_slot>
class ParetoSearch {
	friend class LabelSetUpdateTask;
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;
	typedef typename Edge::edge_data Label;

	struct Data : public Label {
		NodeID node;
  		typedef Label label_type;
 		Data(const NodeID& x, const Label& y) : Label(y), node(x) {}
 		Data() : Label(0,0), node(0) {}

 		friend std::ostream& operator<<(std::ostream& os, const Data& data) {
			os << " (" << data.node << ": " << data.first_weight << "," << data.second_weight << ")";
			return os;
		}
	};
	typedef ParallelBTreeParetoQueue<graph_slot, Data, Label> ParetoQueue;
	typedef typename ParetoQueue::CandLabelVec::const_iterator const_cand_iter;

	typedef typename ParetoQueue::LabelVec::iterator label_iter;
	typedef typename ParetoQueue::LabelVec::const_iterator const_label_iter;

	struct LabelInsPos {
		const size_t pos;
		const Label& label;
	};

	typedef std::vector<LabelInsPos> LabelInsertionPositionVec;
	typedef tbb::enumerable_thread_specific< LabelInsertionPositionVec, tbb::cache_aligned_allocator<LabelInsertionPositionVec>, tbb::ets_key_per_instance > TLSLabInsPos; 
	TLSLabInsPos tls_insertion_positions;

	ParetoQueue pq;
	ParetoSearchStatistics<Label> stats;

	const graph_slot& graph;


	 #ifdef GATHER_SUBCOMPNENT_TIMING
		enum Component {FIND_PARETO_MIN_AND_BUCKETSORT=0, UPDATE_LABELSETS=1, SORT=2, PQ_UPDATE=3, CANDIDATES_COLLECTION=4,
						CANDIDATES_SORT=5, UPDATE_ACTUAL_LABELSET=6, COPY_UPDATES=7, FIND_PARETO_MIN=8, GROUP_PARETO_MIN=9, WRITE_PARETO_MIN_UPDATES=10,
						WRITE_AFFECTED_NODES=11};
		double timings[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	#endif

	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> set_changes;
	#endif

	struct GroupByWeightComp {
		bool operator() (const Operation<Data>& i, const Operation<Data>& j) const {
			if (i.data.first_weight == j.data.first_weight) {
				if (i.data.second_weight == j.data.second_weight) {
					return i.data.node < j.data.node;
				}
				return i.data.second_weight < j.data.second_weight;
			}
			return i.data.first_weight < j.data.first_weight;
		}
	} groupByWeight;

	static struct GroupByNodeComp {
		bool operator() (const Label& i, const Label& j) const {
			if (i.first_weight == j.first_weight) {
					return i.second_weight < j.second_weight;
				}
			return i.first_weight < j.first_weight;
		}
	} groupLabels;

	static struct WeightLessComp {
		bool operator() (const Label& i, const Label& j) const {
			return i.first_weight < j.first_weight;
		}
	} firstWeightLess;

	static inline bool secondWeightGreaterOrEquals(const Label& i, const Label& j) {
		return i.second_weight >= j.second_weight;
	}

	/** First label where the x-coord is truly smaller */
	static inline label_iter x_predecessor(label_iter begin, label_iter end, const Label& new_label) {
		return --std::lower_bound(begin, end, new_label, firstWeightLess);
	}

	/** First label where the y-coord is truly smaller */
	static label_iter y_predecessor(const label_iter& begin, const Label& new_label) {
		label_iter i = begin;
		while (secondWeightGreaterOrEquals(*i, new_label)) {
			++i;
		}
		return i;
	}

	static bool isDominated(label_iter begin, label_iter end, const Label& new_label, label_iter& iter) {
		iter = x_predecessor(begin, end, new_label);

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

	static void updateLabelSet(const NodeID node, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::LabelVec& spare_labelset, const const_cand_iter start, const const_cand_iter end, typename ParetoQueue::OpVec& updates, LabelInsertionPositionVec& nondominated) {
		typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

		label_iter unprocessed_range_begin = labelset.begin();
		for (const_cand_iter candidate = start; candidate != end; ++candidate) {
			// short cut dominated check among candidates
			if (candidate->second_weight >= min) { 
				continue; 
			}
			label_iter iter;
			if (isDominated(unprocessed_range_begin, labelset.end(), *candidate, iter)) {
				min = iter->second_weight; 
			} else {
				min = candidate->second_weight;
				nondominated.push_back({(size_t)(iter-labelset.begin()), *candidate});
			}
			unprocessed_range_begin = iter;
		}
		switch (nondominated.size()) {
		case 0:
			break; // nothing to insert
		case 1:
			simple_insert(node, nondominated.front(), labelset, updates);
			break;
		default: 
			if (is_update_at_the_end(labelset, nondominated.front().pos)) {
				for (auto i=nondominated.rbegin(); i !=  nondominated.rend(); i++) {
					simple_insert(node, *i, labelset, updates);
				}
			} else {
				bool replaced = false;
				label_iter copied_until_iter = labelset.begin();
				label_iter deleted_until_iter = labelset.begin();

				for (LabelInsPos& op : nondominated) {
					const label_iter op_iter = labelset.begin() + op.pos;
					updates.push_back({Operation<Data>::INSERT, Data(node, op.label)});
					const label_iter first_nondominated = y_predecessor(op_iter, op.label);

					if (!replaced && (first_nondominated - op_iter) == 1 && deleted_until_iter != first_nondominated) {
						// Simple case. We can just replace the dominated label with the new one
						updates.push_back({Operation<Data>::DELETE, Data(node, *op_iter)});
						*op_iter = op.label;
						deleted_until_iter = first_nondominated;
					} else {

						// This update is more complicated. We will have to rewrite our entire labelset
						if (!replaced) {
							replaced = true;
							// power óf to to allow efficient caching by the allocator
							spare_labelset.reserve(next_power_of_two(labelset.size() + nondominated.size()));
						}
						// Move all non-affected labels
						if (copied_until_iter < op_iter) {
							spare_labelset.insert(spare_labelset.end(), copied_until_iter, op_iter);
						}
						// Insert the new label
						spare_labelset.push_back(op.label);

						// Delete all newly dominated labels by
						deleted_until_iter = deleted_until_iter > op_iter ? deleted_until_iter : op_iter;
						for (label_iter i = deleted_until_iter; i != first_nondominated; ++i) {
							updates.push_back({Operation<Data>::DELETE, Data(node, *i)});
						}
						copied_until_iter = first_nondominated;
						deleted_until_iter = first_nondominated;
					}
				}
				if (replaced) {
					// copy remaining labels
					spare_labelset.insert(spare_labelset.end(), copied_until_iter, labelset.end());
					labelset.swap(spare_labelset);
					spare_labelset.clear();
				}
			}
		}
		nondominated.clear();
	}

	static inline void simple_insert(const NodeID node, const LabelInsPos& op, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::OpVec& updates) {
		const label_iter op_iter = labelset.begin() + op.pos;
		updates.push_back({Operation<Data>::INSERT, Data(node, op.label)});
		const label_iter first_nondominated = y_predecessor(op_iter, op.label);

		if (op_iter == first_nondominated) {
			labelset.insert(op_iter, op.label);
		} else {
			for (label_iter i = op_iter; i != first_nondominated; ++i) {
				// schedule deletion of dominated labels
				updates.push_back({Operation<Data>::DELETE, Data(node, *i)});
			}
			// replace first dominated label and remove the rest
			*op_iter = op.label;
			labelset.erase(op_iter+1, first_nondominated);
		}
	}


	static inline bool is_update_at_the_end(const typename ParetoQueue::LabelVec& labelset, const size_t pos) {
		return labelset.empty() || pos / (double) labelset.size() > 0.70;
	}

	// http://graphics.stanford.edu/~seander/bithacks.html
	static inline unsigned int next_power_of_two(unsigned int v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

public:
	ParetoSearch(const graph_slot& graph_, const unsigned short num_threads):
		pq(graph_, num_threads),
		graph(graph_)
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_changes(101)
		#endif
	{ }

	void run(const NodeID node) {
		pq.init(Data(node, Label(0,0)));

		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif

		tbb::auto_partitioner ap; 

		size_t iter_timestamp = 0;
		while (!pq.empty()) {
			pq.update_counter = 0;
			pq.affected_nodes_counter = 0;
			stats.report(ITERATION, pq.size());

			// write minimas & candidates to thread locals in pq.*
			pq.findParetoMinima(++iter_timestamp); 
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[FIND_PARETO_MIN_AND_BUCKETSORT] += (stop-start).seconds();
				start = stop;
			#endif

			tbb::parallel_for(tbb::blocked_range<size_t>(0, pq.affected_nodes_counter, 2*(DCACHE_LINESIZE / sizeof(NodeID))),
				[this](const tbb::blocked_range<size_t>& r) {

				ParetoQueue& pq = this->pq;
				typename ParetoQueue::TLSUpdates::reference local_updates = pq.tls_local_updates.local();
				typename ParetoQueue::TLSSpareLabelVec::reference spare_labelset = pq.tls_spare_labelset.local();
				typename TLSLabInsPos::reference nondominated_labels = tls_insertion_positions.local();

				#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
					typename ParetoQueue::TLSTimings::reference subtimings = pq.tls_timings.local();
					tbb::tick_count start = tbb::tick_count::now();
					tbb::tick_count stop = tbb::tick_count::now();
				#endif

				for (size_t i = r.begin(); i != r.end(); ++i) {	
					const NodeID node = pq.affected_nodes[i];
					typename ParetoQueue::LabelSet& ls = pq.labelsets[node];

					// Collect candidates
					const typename ParetoQueue::thread_count count = ls.bufferlist_counter;
					ls.bufferlist_counter = 0;
					assert(count > 0);

					typename ParetoQueue::CandLabelVec* candidates = ls.bufferlists[0];
					assert(candidates->size() > 0);
					for (typename ParetoQueue::thread_count j=1; j < count; ++j) {
						const typename ParetoQueue::CandLabelVec* c = ls.bufferlists[j];
						candidates->insert(candidates->end(), c->cbegin(), c->cend());
						assert(c->size() > 0);
					}
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.candidates_collection += (stop-start).seconds();
						start = stop;
					#endif

					// Batch process labels belonging to the same target node
					std::sort(candidates->begin(), candidates->end(), groupLabels);
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.candidates_sort += (stop-start).seconds();
						start = stop;
					#endif
					updateLabelSet(node, ls.labels, spare_labelset, candidates->cbegin(), candidates->cend(), local_updates, nondominated_labels);
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.update_labelsets += (stop-start).seconds();
						start = stop;
					#endif

				}
				if (local_updates.size() > 0) {
					// Copy updates to globally shared data structure
					const size_t position = pq.update_counter.fetch_and_add(local_updates.size());
					assert(position + local_updates.size() < pq.updates.capacity());
					memcpy(pq.updates.data() + position, local_updates.data(), sizeof(Operation<Data>) * local_updates.size());

					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.copy_updates += (stop-start).seconds();
						start = stop;
					#endif
					local_updates.clear();
				}


			}, ap);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[UPDATE_LABELSETS] += (stop-start).seconds();
				start = stop;
			#endif

			tbb::parallel_sort(pq.updates.data(), pq.updates.data() + pq.update_counter, groupByWeight);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[SORT] += (stop-start).seconds();
				start = stop;
			#endif

			pq.applyUpdates(pq.updates.data(), pq.update_counter);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[PQ_UPDATE] += (stop-start).seconds();
				start = stop;
			#endif
		}		
	}

	void printStatistics() {
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			std::cout << "# LabelSet Modifications" << std::endl;
			for (size_t i=0; i < 101; ++i) {
				std::cout << i << " " << set_changes[i] << std::endl;
			}
		#endif
		std::cout << stats.toString() << std::endl;
		#ifdef GATHER_SUBCOMPNENT_TIMING
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				for (typename ParetoQueue::TLSTimings::reference subtimings : pq.tls_timings) {
					timings[WRITE_PARETO_MIN_UPDATES] = std::max(subtimings.write_pareto_min_updates, timings[WRITE_PARETO_MIN_UPDATES]);
					timings[WRITE_AFFECTED_NODES] = std::max(subtimings.write_affected_nodes, timings[WRITE_AFFECTED_NODES]);
					timings[FIND_PARETO_MIN] = std::max(subtimings.find_pareto_min, timings[FIND_PARETO_MIN]);
					timings[GROUP_PARETO_MIN] = std::max(subtimings.group_pareto_min, timings[GROUP_PARETO_MIN]);
					timings[CANDIDATES_COLLECTION] = std::max(subtimings.candidates_collection, timings[CANDIDATES_COLLECTION]);
					timings[CANDIDATES_SORT] = std::max(subtimings.candidates_sort, timings[CANDIDATES_SORT]);
					timings[UPDATE_ACTUAL_LABELSET] = std::max(subtimings.update_labelsets, timings[UPDATE_ACTUAL_LABELSET]);
					timings[COPY_UPDATES] += subtimings.copy_updates;
				}
			#endif
			std::cout << "Subcomponent Timings:" << std::endl;
			std::cout << "  " << timings[FIND_PARETO_MIN_AND_BUCKETSORT]  << " Find & Group Pareto Min" << std::endl;
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				std::cout << "      " << timings[FIND_PARETO_MIN_AND_BUCKETSORT] - (timings[FIND_PARETO_MIN] + timings[WRITE_PARETO_MIN_UPDATES]
					+ timings[GROUP_PARETO_MIN] + timings[WRITE_AFFECTED_NODES]) << " Recursive TBB Tasks" << std::endl;
				std::cout << "      " << timings[FIND_PARETO_MIN] << " Find Pareto Min " << std::endl;
				std::cout << "      " << timings[WRITE_PARETO_MIN_UPDATES] << " Copy Updates " << std::endl;
				std::cout << "      " << timings[GROUP_PARETO_MIN] << " Group Pareto Min  " << std::endl;
				std::cout << "      " << timings[WRITE_AFFECTED_NODES] << " Copy Affected Nodes  " << std::endl;
			#endif
			std::cout << "  " << timings[UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				std::cout << "      " << timings[CANDIDATES_COLLECTION] << " Collect Candidate Labels " << std::endl;
				std::cout << "      " << timings[CANDIDATES_SORT] << " Sort Candidate Labels " << std::endl;
				std::cout << "      " << timings[UPDATE_ACTUAL_LABELSET] << " Update Labelsets " << std::endl;
				std::cout << "      " << timings[COPY_UPDATES] << " Copy Updates " << std::endl;
			#endif
			std::cout << "  " << timings[SORT] << " Sort Updates"  << std::endl;
			std::cout << "  " << timings[PQ_UPDATE] << " Update PQ " << std::endl;
		#endif
		pq.printStatistics();
	}

	// Subtraction / addition used to hide the sentinals
	size_t size(NodeID node) {return pq.labelsets[node].labels.size()-2; }

	label_iter begin(NodeID node) { return ++pq.labelsets[node].labels.begin(); }
	const_label_iter begin(NodeID node) const { return ++pq.labelsets[node].labels.begin(); }
	label_iter end(NodeID node) { return --pq.labelsets[node].labels.end(); }
	const_label_iter end(NodeID node) const { return --pq.labelsets[node].labels.end(); }

};


#endif 
