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
		size_t pos;
		const Label& label;
	};

	typedef std::vector<LabelInsPos, tbb::cache_aligned_allocator<LabelInsPos> > LabelInsertionPositionVec;
	typedef tbb::enumerable_thread_specific< LabelInsertionPositionVec, tbb::cache_aligned_allocator<LabelInsertionPositionVec>, tbb::ets_key_per_instance > TLSLabInsPos; 
	TLSLabInsPos tls_insertion_positions;

	typedef tbb::enumerable_thread_specific< typename ParetoQueue::CandLabelVec, tbb::cache_aligned_allocator<typename ParetoQueue::CandLabelVec>, tbb::ets_key_per_instance > TLSLocalCandidates; 
	TLSLocalCandidates tls_local_candidates;
	

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
	static size_t y_predecessor(const typename ParetoQueue::LabelVec& labelset, size_t i, const Label& new_label) {
		while (secondWeightGreaterOrEquals(labelset[i], new_label)) {
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
			labelset_simple_insert(node, nondominated.front(), labelset, updates);
			break;
		default:
			// Sentinal: (max position, any label)
			nondominated.push_back({labelset.size(), nondominated.front().label});

			if (is_update_at_the_end(labelset, nondominated.front().pos)) {
				labelset_bulk_update(node, nondominated, labelset, updates);
			} else {
				labelset_rewrite(node, nondominated, labelset, spare_labelset, updates);
			}
		}
		nondominated.clear();
	}

	static inline void labelset_simple_insert(const NodeID node, const LabelInsPos& op, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::OpVec& updates) {
		updates.push_back({Operation<Data>::INSERT, Data(node, op.label)});
		const size_t first_nondominated = y_predecessor(labelset, op.pos, op.label);

		if (op.pos == first_nondominated) {
			labelset.insert(labelset.begin() + op.pos, op.label);
		} else {
			for (size_t i = op.pos; i != first_nondominated; ++i) {
				// schedule deletion of dominated labels
				updates.push_back({Operation<Data>::DELETE, Data(node, labelset[i])});
			}
			// replace first dominated label and remove the rest
			labelset[op.pos] = op.label;
			labelset.erase(labelset.begin() + op.pos + 1, labelset.begin() + first_nondominated);
		}
	}

	static inline void labelset_bulk_update(const NodeID node, LabelInsertionPositionVec& nondominated, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::OpVec& updates) {
		size_t delta = 0;
		for (size_t i = 0; i < nondominated.size()-1; ++i) {
			LabelInsPos& op = nondominated[i];
			op.pos += delta;

			const size_t first_nondominated = y_predecessor(labelset, op.pos, op.label);
			const size_t range_end = std::min(first_nondominated, nondominated[i+1].pos+delta);
			updates.push_back({Operation<Data>::INSERT, Data(node, op.label)});

			if (op.pos == range_end) {
				// No elements dominated directly by us. Just insert here
				labelset.insert(labelset.begin() + op.pos, op.label);
			} else {
				for (size_t i = op.pos; i != range_end; ++i) {
					// schedule deletion of dominated labels
					updates.push_back({Operation<Data>::DELETE, Data(node, labelset[i])});
				}
				// replace first dominated label and remove the rest
				labelset[op.pos] = op.label;
				labelset.erase(labelset.begin() + op.pos + 1, labelset.begin() + range_end);
			}
			delta += op.pos - range_end + 1;
		}
	}

	static inline void labelset_rewrite(const NodeID node, const LabelInsertionPositionVec& nondominated, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::LabelVec& spare_labelset, typename ParetoQueue::OpVec& updates) {
		size_t copied_until = 0;
		// power of two to allow efficient caching by the allocator
		spare_labelset.reserve(next_power_of_two(labelset.size() + nondominated.size()));

		for (size_t i = 0; i < nondominated.size()-1; ++i) {
			const LabelInsPos& op = nondominated[i];
			const size_t first_nondominated = y_predecessor(labelset, op.pos, op.label);
			updates.push_back({Operation<Data>::INSERT, Data(node, op.label)});

			// Move all non-affected labels
			if (copied_until < op.pos)	{
				spare_labelset.insert(spare_labelset.end(), labelset.begin() + copied_until, labelset.begin() + op.pos);
				copied_until = op.pos;
			}
			// Insert the new label
			spare_labelset.push_back(op.label);

			// Delete all dominated labels by
			size_t range_end = std::min(first_nondominated, nondominated[i+1].pos);
			for (size_t i = op.pos; i != range_end; ++i) {
				updates.push_back({Operation<Data>::DELETE, Data(node, labelset[i])});
			}
			copied_until = range_end;
		}
		// copy remaining labels
		spare_labelset.insert(spare_labelset.end(),  labelset.begin() + copied_until, labelset.end());
		labelset.swap(spare_labelset);
		spare_labelset.clear();
	}

	static inline bool is_update_at_the_end(const typename ParetoQueue::LabelVec& labelset, const size_t pos) {
		return labelset.size() > 8 || pos / (double) labelset.size() > 0.75;
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

			for (typename ParetoQueue::TLSAffected::reference a : pq.tls_affected_nodes) {
				if (!a.empty()) {
					const size_t position = pq.affected_nodes_counter.fetch_and_add(a.size());
					assert(position + a.size() < pq.affected_nodes.capacity());
					memcpy(pq.affected_nodes.data() + position, a.data(), sizeof(NodeID) * a.size());
					a.clear();
				}
			}

			tbb::parallel_for(tbb::blocked_range<size_t>(0, pq.affected_nodes_counter, 2*(DCACHE_LINESIZE / sizeof(NodeID))),
				[this](const tbb::blocked_range<size_t>& r) {

				ParetoQueue& pq = this->pq;
				typename ParetoQueue::TLSUpdates::reference local_updates = pq.tls_local_updates.local();
				typename ParetoQueue::TLSSpareLabelVec::reference spare_labelset = pq.tls_spare_labelset.local();
				typename TLSLabInsPos::reference nondominated_labels = tls_insertion_positions.local();
				typename TLSLocalCandidates::reference candidates = tls_local_candidates.local();

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

					for (typename ParetoQueue::thread_count j=0; j < count; ++j) {
						const typename ParetoQueue::CandLabelVec* c = ls.bufferlists[j];
						assert(c->size() > 0);
						candidates.insert(candidates.end(), c->cbegin(), c->cend());
					}
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.candidates_collection += (stop-start).seconds();
						start = stop;
					#endif

					// Batch process labels belonging to the same target node
					std::sort(candidates.begin(), candidates.end(), groupLabels);
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.candidates_sort += (stop-start).seconds();
						start = stop;
					#endif
					updateLabelSet(node, ls.labels, spare_labelset, candidates.cbegin(), candidates.cend(), local_updates, nondominated_labels);
					candidates.clear();
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.update_labelsets += (stop-start).seconds();
						start = stop;
					#endif

				}
				if (local_updates.size() > 512) {
					// Copy updates to globally shared data structure
					const size_t position = pq.update_counter.fetch_and_add(local_updates.size());
					assert(position + local_updates.size() < pq.updates.capacity());
					memcpy(pq.updates.data() + position, local_updates.data(), sizeof(Operation<Data>) * local_updates.size());
					local_updates.clear();

					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.copy_updates += (stop-start).seconds();
						start = stop;
					#endif
				}
			}, ap);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[UPDATE_LABELSETS] += (stop-start).seconds();
				start = stop;
			#endif

			for (typename ParetoQueue::TLSUpdates::reference u : pq.tls_local_updates) {
				if (!u.empty()) {
					const size_t position = pq.update_counter.fetch_and_add(u.size());
					assert(position + u.size() < pq.updates.capacity());
					memcpy(pq.updates.data() + position, u.data(), sizeof(Operation<Data>) * u.size());
					u.clear();
				}
			}

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
