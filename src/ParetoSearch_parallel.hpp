/*
 * A implementation of a bi-objective shortest path label setting algorithm
 * that always relaxes _all_ globaly pareto optimal labels instead of just one.
 *  
 * Author: Stephan Erb
 */
#ifndef PARALLEL_PARETO_SEARCH_H_
#define PARALLEL_PARETO_SEARCH_H_

#ifndef PARALLEL_BUILD
#define PARALLEL_BUILD
#endif
 
#include "utility/datastructure/graph/GraphMacros.h"
#include "options.hpp"
#include "ParetoQueue_parallel.hpp"
#include "ParetoSearchStatistics.hpp"
#include "assert.h"
#include "Label.hpp"
#include "algorithm"

#include "tbb/parallel_sort.hpp"

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
	#define TIME_COMPONENT(target) do { stop = tbb::tick_count::now(); target += (stop-start).seconds(); start = stop; } while(0)
#else
	#define TIME_COMPONENT(target) do { } while(0)
#endif
#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
	#define TIME_SUBCOMPONENT(target) TIME_COMPONENT(target)
#else
	#define TIME_SUBCOMPONENT(target) do { } while(0)
#endif


template<typename graph_slot>
class ParetoSearch {
	friend class LabelSetUpdateTask;
private:
	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;

	typedef ParallelBTreeParetoQueue<graph_slot> ParetoQueue;
	ParetoQueue pq;
	ParetoSearchStatistics<Label> stats;
	const graph_slot& graph;

	#ifdef GATHER_SUBCOMPNENT_TIMING
		enum Component {FIND_PARETO_MIN=0, UPDATE_LABELSETS=1, SORT_CANDIDATES=2, SORT_UPDATES=3, PQ_UPDATE=4,
						TL_CANDIDATES_SORT=5, TL_UPDATE_LABELSETS=6 };
		double timings[7] = {0, 0, 0, 0, 0, 0, 0};

		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			struct tls_tim {
				double candidates_sort = 0;
				double update_labelsets = 0;
			};
			typedef tbb::enumerable_thread_specific<tls_tim, tbb::cache_aligned_allocator<tls_tim>, tbb::ets_key_per_instance > TLSTimings;
			TLSTimings tls_timings;
		#endif
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
			return i.node < j.node;
		}
	} groupCandidates;

	struct GroupLabelsByNodeComp {
		inline bool operator() (const Label& i, const Label& j) const {
            return i.combined() < j.combined(); 
		}
	} groupLabels;


public:
	ParetoSearch(const graph_slot& graph_, const unsigned short num_threads):
		pq(graph_, num_threads),
		graph(graph_)
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_changes(101)
		#endif
	{ 
		#ifdef BTREE_PARETO_LABELSET
			typename ParetoQueue::TLSData::reference tl = pq.tls_data.local();
			tl.labelset_data.spare_leaf = pq.labelsets[0].allocate_leaf_without_count();
			tl.labelset_data.spare_inner = pq.labelsets[0].allocate_inner_without_count(0);
		#endif 
	}

	~ParetoSearch() {
		#ifdef BTREE_PARETO_LABELSET
			for (auto& tl : pq.tls_data) {
				if (tl.labelset_data.spare_leaf != NULL) {
					pq.labelsets[0].free_node_without_count(tl.labelset_data.spare_leaf);
					pq.labelsets[0].free_node_without_count(tl.labelset_data.spare_inner);
				}
			}
		#endif
	}

	void run(const NodeID node) {
		pq.init(NodeLabel(node, Label(0,0)));
		pq.labelsets[node].init(Label(0,0), pq.tls_data.local().labelset_data);

		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif

		tbb::auto_partitioner auto_part;
		tbb::affinity_partitioner candidates_aff_part;
		tbb::auto_partitioner tree_aff_part;

		while (!pq.empty()) {
			pq.update_counter = 0;
			pq.candidate_counter = 0;
			stats.report(ITERATION, pq.size());

			// write updates & candidates to thread locals in pq.*
			pq.findParetoMinima(); 
			TIME_COMPONENT(timings[FIND_PARETO_MIN]);

			// Count gaps moved to the end via sorting. Then we can ignore them
			size_t candidate_counter_size_diff = 0;
			for (typename ParetoQueue::TLSData::reference tl : pq.tls_data) {
				candidate_counter_size_diff += (tl.candidates.end - tl.candidates.current);
				tl.candidates.reset();
			}

			sort(pq.candidates, pq.candidate_counter, auto_part);
			TIME_COMPONENT(timings[SORT_CANDIDATES]);

			pq.candidate_counter -= candidate_counter_size_diff;
			tbb::parallel_for(candidate_range(&pq, min_problem_size(pq.candidate_counter, 64)),
			[this](const candidate_range& r) {
				ParetoQueue& pq = this->pq;
				typename ParetoQueue::TLSData::reference tl = pq.tls_data.local();
				#ifdef BTREE_PARETO_LABELSET
					if (tl.labelset_data.spare_leaf == NULL) {
						tl.labelset_data.spare_leaf = pq.labelsets[pq.candidates[r.begin()].node].allocate_leaf_without_count();
						tl.labelset_data.spare_inner = pq.labelsets[pq.candidates[r.begin()].node].allocate_inner_without_count(0);
					}
				#endif
				
				#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
					typename TLSTimings::reference subtimings = this->tls_timings.local();
					tbb::tick_count start = tbb::tick_count::now();
					tbb::tick_count stop = tbb::tick_count::now();
				#endif

				size_t i = r.begin();
				const size_t end = r.end();
				while(i != end) {
					const size_t range_start = i;
					const NodeID node = pq.candidates[i].node;
					auto& ls = pq.labelsets[node];
					ls.prefetch();
					while (i != end && pq.candidates[i].node == node) {
						++i;
					}
					std::sort(pq.candidates.begin()+range_start, pq.candidates.begin()+i, groupLabels);
					TIME_SUBCOMPONENT(subtimings.candidates_sort);

					ls.updateLabelSet(node, pq.candidates.cbegin()+range_start, pq.candidates.cbegin()+i, tl.updates, tl.labelset_data, stats);
					TIME_SUBCOMPONENT(subtimings.update_labelsets);
				}
			}, candidates_aff_part);
			TIME_COMPONENT(timings[UPDATE_LABELSETS]);

			// Count gaps moved to the end via sorting. Then we can ignore them
			size_t update_counter_size_diff = 0;
			for (typename ParetoQueue::TLSData::reference tl : pq.tls_data) {
				update_counter_size_diff += (tl.updates.end - tl.updates.current);
				tl.updates.reset();
			}

			parallel_sort(pq.updates.data(), pq.updates.data() + pq.update_counter, groupByWeight, auto_part, min_problem_size(pq.update_counter, 512));
			TIME_COMPONENT(timings[SORT_UPDATES]);

			pq.update_counter -= update_counter_size_diff;
			pq.applyUpdates(pq.updates.data(), pq.update_counter, tree_aff_part);
			TIME_COMPONENT(timings[PQ_UPDATE]);
		}		
	}

	inline void sort(typename ParetoQueue::CandLabelVec& candidates, const AtomicCounter& candidate_counter, tbb::auto_partitioner& auto_part) {
		const auto min_prob_size = min_problem_size(candidate_counter, 512);
		#ifdef RADIX_SORT
			parallel_radix_sort(candidates.data(), candidate_counter, [](const NodeLabel& x) { return x.node; }, auto_part, min_prob_size);
		#else
			parallel_sort(candidates.data(), candidates.data() + candidate_counter, groupCandidates, auto_part, min_prob_size);
		#endif
	}

	inline size_t min_problem_size(size_t total, const double max=1.0) const {
		return std::max((total/pq.num_threads) / (log2(total/pq.num_threads + 1)+1), max);
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
				for (typename TLSTimings::reference subtimings : tls_timings) {
					timings[TL_CANDIDATES_SORT] = std::max(subtimings.candidates_sort, timings[TL_CANDIDATES_SORT]);
					timings[TL_UPDATE_LABELSETS] = std::max(subtimings.update_labelsets, timings[TL_UPDATE_LABELSETS]);
				}
			#endif
			std::cout << "# Subcomponent Timings:" << std::endl;
			std::cout << "#   " << timings[FIND_PARETO_MIN]  << " Find Pareto Min" << std::endl;
			std::cout << "#   " << timings[SORT_CANDIDATES] << " Sort Candidates"  << std::endl;
			std::cout << "#   " << timings[UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				std::cout << "#       " << timings[TL_CANDIDATES_SORT] << " Sort Candidate Labels " << std::endl;
				std::cout << "#       " << timings[TL_UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			#endif
			std::cout << "#   " << timings[SORT_UPDATES] << " Sort Updates"  << std::endl;
			std::cout << "#   " << timings[PQ_UPDATE] << " Update PQ " << std::endl;
		#endif
		pq.printStatistics();
	}

	void printComponentTimings() const {
		#ifdef GATHER_SUBCOMPNENT_TIMING
			std::cout << pq.num_threads << " " << pq.num_threads << " " << timings[FIND_PARETO_MIN] << " " << timings[SORT_CANDIDATES] << " " << timings[UPDATE_LABELSETS] << " " << timings[SORT_UPDATES] << " " << timings[PQ_UPDATE];
		#endif
	}

	size_t size(NodeID node) const { return pq.labelsets[node].size(); }
	typename ParetoQueue::LabelSet::label_iter begin(NodeID node) { return pq.labelsets[node].begin(); }
	typename ParetoQueue::LabelSet::label_iter end(NodeID node) { return pq.labelsets[node].end(); }
	typename ParetoQueue::LabelSet::const_label_iter begin(NodeID node) const { return pq.labelsets[node].begin(); }
	typename ParetoQueue::LabelSet::const_label_iter end(NodeID node) const { return pq.labelsets[node].end(); }


};


#endif 
