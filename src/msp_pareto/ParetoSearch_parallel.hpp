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
 

#include "../options.hpp"

#include "../Label.hpp"
#include "../Graph.hpp"

#include "ParetoQueue_parallel.hpp"
#include "ParetoSearchStatistics.hpp"
#include "ParetoLabelSet.hpp"

#include "../tbx/parallel_sort.hpp"

#include "tbb/parallel_sort.h"
#include "tbb/concurrent_vector.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/task.h"
#include "tbb/atomic.h"

#include "assert.h"
#include "algorithm"


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

#ifdef BTREE_PARETO_LABELSET
	typedef BtreeParetoLabelSet<tbb::cache_aligned_allocator<Label>> ParetoLabelSet;
#else
	typedef VectorParetoLabelSet<tbb::cache_aligned_allocator<Label>> ParetoLabelSet;
#endif


template<typename labelset_slot=ParetoLabelSet>
class ParetoSearch {
private:

	typedef labelset_slot LabelSet;
	struct PaddedLabelSet : public LabelSet {
		char pad[DCACHE_LINESIZE - sizeof(LabelSet) % DCACHE_LINESIZE];
	};

	struct ThreadData {
		ThreadLocalWriteBuffer<NodeLabel> candidates;
		ThreadLocalWriteBuffer<Operation<NodeLabel>> updates;
		typename LabelSet::ThreadLocalLSData labelset_data;

		ThreadData(ParetoSearch* algo) 
		:	candidates(algo->candidates, algo->candidate_counter,
				NodeLabel(std::numeric_limits<NodeID>::max(), Label())),
		    updates(algo->updates, algo->update_counter, 
		    	Operation<NodeLabel>(Operation<NodeLabel>::INSERT, std::numeric_limits<NodeID>::max(), MAX_WEIGHT, MAX_WEIGHT)),
		    labelset_data(algo->labelsets[0])
		{}
	};	
	typedef tbb::enumerable_thread_specific< ThreadData, tbb::cache_aligned_allocator<ThreadData>, tbb::ets_key_per_instance > TLSData; 

	typedef ParallelBTreeParetoQueue<TLSData> ParetoQueue;
	typedef Operation<NodeLabel> Updates; 

	CACHE_ALIGNED Updates*   const updates;
	CACHE_ALIGNED NodeLabel* const candidates;
	CACHE_ALIGNED AtomicCounter update_counter;
	CACHE_ALIGNED AtomicCounter candidate_counter;

	std::vector<PaddedLabelSet> labelsets;
	TLSData tls_data;
	ParetoQueue pq;

	ParetoSearchStatistics<Label> stats;
	const Graph& graph;

	GroupOperationsByWeightAndNodeComperator<Operation<NodeLabel>> groupByWeight;
	GroupNodeLabelsByNodeComperator groupCandidates;
	GroupLabelsByWeightComperator groupLabels;

public:
	ParetoSearch(const Graph& graph_, const unsigned short num_threads):
		updates((Updates*) scalable_malloc(LARGE_ENOUGH_FOR_EVERYTHING * sizeof(Updates))),
		candidates((NodeLabel*) scalable_malloc(LARGE_ENOUGH_FOR_EVERYTHING * sizeof(NodeLabel))),
		labelsets(graph_.numberOfNodes()),
		tls_data([this](){ return this; }),
		pq(graph_, num_threads, tls_data),
		graph(graph_)
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_changes(101)
		#endif
	{ }

	~ParetoSearch() {
		scalable_free(updates);
		scalable_free(candidates);
	}

	void run(const NodeID node) {
		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count stop, start = tbb::tick_count::now();
		#endif

		tbb::auto_partitioner auto_part;
		tbb::affinity_partitioner candidates_part;
		tbb::auto_partitioner tree_part;

		pq.init(NodeLabel(node, Label(0,0)));
		labelsets[node].init(Label(0,0), tls_data.local().labelset_data);

		while (!pq.empty()) {
			update_counter = 0;
			candidate_counter = 0;
			stats.report(ITERATION, pq.size());

			pq.findParetoMinima(); // write pareto minima to updates & candidates vectors
			TIME_COMPONENT(timings[FIND_PARETO_MIN]);

			sortByNode(candidates, candidate_counter, auto_part, min_problem_size(candidate_counter, 512));
			TIME_COMPONENT(timings[SORT_CANDIDATES]);
			candidate_counter -= countGapsInThreadLocalCandidateBuckets();

			tbb::parallel_for(node_based_range(candidates, candidate_counter, min_problem_size(candidate_counter, 64)),
			[this](const node_based_range& r) {				
				#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
					typename TLSTimings::reference subtimings = tls_timings.local();
					tbb::tick_count stop, start = tbb::tick_count::now();
				#endif
				typename TLSData::reference tl = tls_data.local();

				size_t i = r.begin();
				const size_t end = r.end();
				while(i != end) {
					const size_t range_start = i;
					const NodeID node = candidates[i].node;
					auto& ls = labelsets[node];
					ls.prefetch(); // fetch label set while we prepare its candidate labels

					while (i != end && candidates[i].node == node) {
						++i;
					}
					std::sort(candidates+range_start, candidates+i, groupLabels);
					TIME_SUBCOMPONENT(subtimings.candidates_sort);

					ls.updateLabelSet(node, candidates+range_start, candidates+i, tl.updates, tl.labelset_data, stats);
					TIME_SUBCOMPONENT(subtimings.update_labelsets);
				}
			}, candidates_part);
			TIME_COMPONENT(timings[UPDATE_LABELSETS]);

			parallel_sort(updates, updates+update_counter, groupByWeight, auto_part, min_problem_size(update_counter, 512));
			TIME_COMPONENT(timings[SORT_UPDATES]);
			update_counter -= countGapsInThreadLocalUpdateBuckets();

			pq.applyUpdates(updates, update_counter, tree_part);
			TIME_COMPONENT(timings[PQ_UPDATE]);
		}		
	}

	inline void sortByNode(NodeLabel* candidates, const AtomicCounter& candidate_counter, tbb::auto_partitioner& auto_part, const size_t min_prob_size) {
		#ifdef RADIX_SORT
			parallel_radix_sort(candidates, candidate_counter, [](const NodeLabel& x) { return x.node; }, auto_part, min_prob_size);
		#else
			parallel_sort(candidates, candidates + candidate_counter, groupCandidates, auto_part, min_prob_size);
		#endif
	}

	inline size_t countGapsInThreadLocalUpdateBuckets() {
		size_t update_counter_size_diff = 0;
		for (auto& tl : tls_data) {
			update_counter_size_diff += tl.updates.reset();
		}
		return update_counter_size_diff;
	}

	inline size_t countGapsInThreadLocalCandidateBuckets() {
		size_t update_counter_size_diff = 0;
		for (auto& tl : tls_data) {
			update_counter_size_diff += tl.candidates.reset();
		}
		return update_counter_size_diff;
	}

	inline size_t min_problem_size(const size_t total, const double max=1.0) const {
		return std::max((total/pq.num_threads) / (log2(total/pq.num_threads + 1)+1), max);
	}

	/** 
	 * Range class which splits only one node boundaries (so that each label set is only updated by a single thread)
	 */
	class node_based_range {
	public:
		typedef size_t Value;
		typedef size_t size_type;
		typedef Value const_iterator;

	    node_based_range(const NodeLabel* candidates_, const AtomicCounter& candidate_counter_, const size_type grainsize_=1) : 
	        my_end(candidate_counter_), my_begin(0), my_grainsize(grainsize_), candidates(candidates_)
	    {}

	    node_based_range(node_based_range& r, tbb::split) : 
	        my_end(r.my_end), my_begin(r.split()), my_grainsize(r.my_grainsize), candidates(r.candidates)
	    {}

	    const_iterator	begin()		   const { return my_begin; }
	    const_iterator	end() 		   const { return my_end; }
	    size_type		size() 		   const { return size_type(my_end - my_begin); }
	    size_type		grainsize()    const { return my_grainsize; }
	    bool 			empty() 	   const { return !(my_begin < my_end);}
	    bool			is_divisible() const { return my_grainsize < size();}

	private:
	    Value my_end;
	    const Value my_begin;
   	    const size_type my_grainsize;
	   	const NodeLabel* candidates;

	    Value split() {
	    	// find middle by best effort
	        Value middle = my_begin + (my_end-my_begin)/2u;
	        const NodeID node = candidates[middle].node;
	        // correct until we find the first candidate belonging to another labelset
	        while (middle != my_end && candidates[middle].node == node) {
	        	++middle;
	        }
	        my_end = middle;
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
					timings[TL_CANDIDATES_SORT]  = std::max(subtimings.candidates_sort,  timings[TL_CANDIDATES_SORT]);
					timings[TL_UPDATE_LABELSETS] = std::max(subtimings.update_labelsets, timings[TL_UPDATE_LABELSETS]);
				}
			#endif
			std::cout << "# Subcomponent Timings:" << std::endl;
			std::cout << "#   " << timings[FIND_PARETO_MIN]  << " Find Pareto Min" << std::endl;
			std::cout << "#   " << timings[SORT_CANDIDATES]  << " Sort Candidates"  << std::endl;
			std::cout << "#   " << timings[UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				std::cout << "#       " << timings[TL_CANDIDATES_SORT]  << " Sort Candidate Labels " << std::endl;
				std::cout << "#       " << timings[TL_UPDATE_LABELSETS] << " Update Labelsets " << std::endl;
			#endif
			std::cout << "#   " << timings[SORT_UPDATES] << " Sort Updates"  << std::endl;
			std::cout << "#   " << timings[PQ_UPDATE]    << " Update PQ " << std::endl;
		#endif
		pq.printStatistics();
	}

	void printComponentTimings() const {
		#ifdef GATHER_SUBCOMPNENT_TIMING
			std::cout << pq.num_threads << " " << pq.num_threads << " " << timings[FIND_PARETO_MIN] << " " << timings[SORT_CANDIDATES] << " " << timings[UPDATE_LABELSETS] << " " << timings[SORT_UPDATES] << " " << timings[PQ_UPDATE];
		#endif
	}

	size_t size(NodeID node) const { return labelsets[node].size(); }
	typename LabelSet::iterator begin(NodeID node) { return labelsets[node].begin(); }
	typename LabelSet::iterator end(NodeID node) { return labelsets[node].end(); }
	typename LabelSet::const_iterator begin(NodeID node) const { return labelsets[node].begin(); }
	typename LabelSet::const_iterator end(NodeID node) const { return labelsets[node].end(); }

private:

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

};

#endif 
