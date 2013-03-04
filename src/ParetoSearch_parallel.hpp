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
#include "Label.hpp"

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

	typedef ParallelBTreeParetoQueue<graph_slot> ParetoQueue;

	typedef typename ParetoQueue::LabelVec::iterator label_iter;
	typedef typename ParetoQueue::CandLabelVec::iterator const_label_iter;

	struct LabelInsPos {
		size_t pos;
		const Label& label;
	};

	typedef std::vector<LabelInsPos, tbb::cache_aligned_allocator<LabelInsPos> > LabelInsertionPositionVec;
	typedef tbb::enumerable_thread_specific< LabelInsertionPositionVec, tbb::cache_aligned_allocator<LabelInsertionPositionVec>, tbb::ets_key_per_instance > TLSLabInsPos; 
	TLSLabInsPos tls_insertion_positions;

	ParetoQueue pq;
	ParetoSearchStatistics<Label> stats;
	const graph_slot& graph;


	#ifdef GATHER_SUBCOMPNENT_TIMING
		enum Component {FIND_PARETO_MIN=0, UPDATE_LABELSETS=1, SORT_CANDIDATES=2, SORT_UPDATES=3, PQ_UPDATE=4, CLEAR_BUFFERS=9,
						TL_CANDIDATES_SORT=5, TL_UPDATE_LABELSETS=6, TL_FIND_PARETO_MIN=7, TL_WRITE_LOCAL_TO_SHARED=8};
		double timings[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	#endif

	#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
		std::vector<unsigned long> set_changes;
	#endif

	struct GroupByWeightComp {
		inline bool operator() (const Operation<NodeLabel>& i, const Operation<NodeLabel>& j) const {
			if (i.data.first_weight == j.data.first_weight) {
				if (i.data.second_weight == j.data.second_weight) {
					return i.data.node < j.data.node;
				}
				return i.data.second_weight < j.data.second_weight;
			}
			return i.data.first_weight < j.data.first_weight;
		}
	} groupByWeight;

	struct GroupNodeLabelsByNodeComp {
		inline bool operator() (const NodeLabel& i, const NodeLabel& j) const {
			if (i.node == j.node) {
				if (i.first_weight == j.first_weight) {
					return i.second_weight < j.second_weight;
				}
				return i.first_weight < j.first_weight;
			}
			return i.node < j.node;
		}
	} groupCandidates;

	struct GroupLabelsByNodeComp {
		inline bool operator() (const Label& i, const Label& j) const {
			if (i.first_weight == j.first_weight) {
				return i.second_weight < j.second_weight;
			}
			return i.first_weight < j.first_weight;
		}
	} groupLabels;

	static struct WeightLessComp {
		inline bool operator() (const Label& i, const Label& j) const {
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
	static inline size_t y_predecessor(const typename ParetoQueue::LabelVec& labelset, size_t i, const Label& new_label) {
		while (secondWeightGreaterOrEquals(labelset[i], new_label)) {
			++i;
		}
		return i;
	}

	static inline bool isDominated(label_iter begin, label_iter end, const Label& new_label, label_iter& iter) {
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

	template<class iter_type>
	static void updateLabelSet(const NodeID node, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::LabelVec& spare_labelset, const iter_type start, const iter_type end, typename ParetoQueue::OpVec& updates, LabelInsertionPositionVec& nondominated) {
		typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

		label_iter unprocessed_range_begin = labelset.begin();
		for (iter_type candidate = start; candidate != end; ++candidate) {
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
		updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, op.label)});
		const size_t first_nondominated = y_predecessor(labelset, op.pos, op.label);

		if (op.pos == first_nondominated) {
			labelset.insert(labelset.begin() + op.pos, op.label);
		} else {
			for (size_t i = op.pos; i != first_nondominated; ++i) {
				// schedule deletion of dominated labels
				updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, labelset[i])});
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
			updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, op.label)});

			if (op.pos == range_end) {
				// No elements dominated directly by us. Just insert here
				labelset.insert(labelset.begin() + op.pos, op.label);
			} else {
				for (size_t i = op.pos; i != range_end; ++i) {
					// schedule deletion of dominated labels
					updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, labelset[i])});
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
			updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, op.label)});

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
				updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, labelset[i])});
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
		pq.init(NodeLabel(node, Label(0,0)));

		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif

		while (!pq.empty()) {
			pq.update_counter = 0;
			pq.candidate_counter = 0;
			stats.report(ITERATION, pq.size());

			// write updates & candidates to thread locals in pq.*
			pq.findParetoMinima(); 
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[FIND_PARETO_MIN] += (stop-start).seconds();
				start = stop;
			#endif

			for (typename ParetoQueue::TLSData::reference tl : pq.tls_data) {
				if (!tl.candidates.empty()) {
					const size_t position = pq.candidate_counter.fetch_and_add(tl.candidates.size());
					assert(position + tl.candidates.size() < pq.candidates.capacity());
					memcpy(pq.candidates.data() + position, tl.candidates.data(), sizeof(Operation<NodeLabel>) * tl.candidates.size());
					tl.candidates.clear();
				}
			}
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[CLEAR_BUFFERS] += (stop-start).seconds();
				start = stop;
			#endif

			tbb::parallel_sort(pq.candidates.data(), pq.candidates.data() + pq.candidate_counter, groupCandidates);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[SORT_CANDIDATES] += (stop-start).seconds();
				start = stop;
			#endif

			tbb::parallel_for(candidate_range(&pq, 2*(DCACHE_LINESIZE / sizeof(NodeID))),
			[this](const candidate_range& r) {
				ParetoQueue& pq = this->pq;
				typename ParetoQueue::TLSData::reference tl = pq.tls_data.local();
				typename TLSLabInsPos::reference nondominated_labels = tls_insertion_positions.local();

				#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
					typename ParetoQueue::TLSTimings::reference subtimings = pq.tls_timings.local();
					tbb::tick_count start = tbb::tick_count::now();
					tbb::tick_count stop = tbb::tick_count::now();
				#endif

				size_t i = r.begin();
				while(i != r.end()) {
					const size_t range_start = i;
					const NodeID node = pq.candidates[i].node;
					while (i != r.end() && pq.candidates[i].node == node) {
						++i;
					}
					auto& ls = pq.labelsets[node];
					updateLabelSet(node, ls.labels, tl.spare_labelset, pq.candidates.begin()+range_start, pq.candidates.begin()+i, tl.updates, nondominated_labels);
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.update_labelsets += (stop-start).seconds();
						start = stop;
					#endif

					// Move thread local data to shared data structure. Move in cache sized blocks to prevent false sharing
					if (tl.updates.size() > BATCH_SIZE) {
						const size_t position = pq.update_counter.fetch_and_add(tl.updates.size());
						assert(position + tl.updates.size() < pq.updates.capacity());
						memcpy(pq.updates.data() + position, tl.updates.data(), sizeof(Operation<NodeLabel>) * tl.updates.size() );
						tl.updates.clear();
					}
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.write_local_to_shared += (stop-start).seconds();
						start = stop;
					#endif

				}
			});
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[UPDATE_LABELSETS] += (stop-start).seconds();
				start = stop;
			#endif

			for (typename ParetoQueue::TLSData::reference tl : pq.tls_data) {
				if (!tl.updates.empty()) {
					const size_t position = pq.update_counter.fetch_and_add(tl.updates.size());
					assert(position + tl.updates.size() < pq.updates.capacity());
					memcpy(pq.updates.data() + position, tl.updates.data(), sizeof(Operation<NodeLabel>) * tl.updates.size());
					tl.updates.clear();
				}
			}
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[CLEAR_BUFFERS] += (stop-start).seconds();
				start = stop;
			#endif

			tbb::parallel_sort(pq.updates.data(), pq.updates.data() + pq.update_counter, groupByWeight);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[SORT_UPDATES] += (stop-start).seconds();
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


	class candidate_range {
	public:
		typedef size_t Value;
	    typedef size_t size_type;
	    typedef Value const_iterator;

	    candidate_range() {}

	    candidate_range(ParetoQueue* _pq, size_type grainsize_=1) : 
	        my_end(_pq->candidate_counter), my_begin(0), my_grainsize(grainsize_), my_pq(_pq)
	    {}

	    const_iterator begin() const {return my_begin;}
	    const_iterator end() const {return my_end;}

	    size_type size() const {
	        return size_type(my_end-my_begin);
	    }
	    size_type grainsize() const {return my_grainsize;}

	    bool empty() const {return !(my_begin<my_end);}
	    bool is_divisible() const {return my_grainsize<size();}

	    candidate_range(candidate_range& r, tbb::split) : 
	        my_end(r.my_end),
	        my_begin(do_split(r)),
	        my_grainsize(r.my_grainsize),
	        my_pq(r.my_pq)
	    {}

	private:
	    Value my_end;
	    Value my_begin;
	    size_type my_grainsize;
	    ParetoQueue* my_pq;

	    static Value do_split( candidate_range& r ) {
	        Value middle = r.my_begin + (r.my_end-r.my_begin)/2u;
	        const NodeID node = r.my_pq->candidates[middle].node;
	        while (middle < r.my_end && r.my_pq->candidates[middle].node == node) {
	        	++middle;
	        }
	        r.my_end = middle;
	        return middle;
	    }
	};

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
					timings[TL_CANDIDATES_SORT] = std::max(subtimings.candidates_sort, timings[TL_CANDIDATES_SORT]);
					timings[TL_FIND_PARETO_MIN] = std::max(subtimings.find_pareto_min, timings[TL_FIND_PARETO_MIN]);
					timings[TL_UPDATE_LABELSETS] = std::max(subtimings.update_labelsets, timings[TL_UPDATE_LABELSETS]);
					timings[TL_WRITE_LOCAL_TO_SHARED] = std::max(subtimings.write_local_to_shared, timings[TL_WRITE_LOCAL_TO_SHARED]);
				}
			#endif
			std::cout << "Subcomponent Timings:" << std::endl;
			std::cout << "  " << timings[FIND_PARETO_MIN]  << " Find Pareto Min" << std::endl;
			std::cout << "  " << timings[SORT_CANDIDATES] << " Sort Candidates"  << std::endl;
			std::cout << "  " << timings[UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				std::cout << "      " << timings[TL_CANDIDATES_SORT] << " Sort Candidate Labels " << std::endl;
				std::cout << "      " << timings[TL_UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			#endif
			std::cout << "  " << timings[SORT_UPDATES] << " Sort Updates"  << std::endl;
			std::cout << "  " << timings[PQ_UPDATE] << " Update PQ " << std::endl;
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				std::cout << "  " << timings[TL_WRITE_LOCAL_TO_SHARED] << " Writing thread local data to shared memory"  << std::endl;
				std::cout << "  " << timings[CLEAR_BUFFERS] << " Copying remainig data from thread local buffers"  << std::endl;
			#endif
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
