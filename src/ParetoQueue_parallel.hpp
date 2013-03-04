#ifndef PARALLEL_PARETO_QUEUE_H_
#define PARALLEL_PARETO_QUEUE_H_

#include <iostream>
#include <vector>
#include <deque>

#include "options.hpp"

#define COMPUTE_PARETO_MIN
#include "datastructures/BTree_parallel.hpp"

#include <algorithm>
#include <limits>
#include "utility/datastructure/graph/GraphMacros.h"
#include "Label.hpp"

#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/scalable_allocator.h"
#include "tbb/task.h"
#include "tbb/concurrent_vector.h"


#ifndef PARETO_FIND_RECURSION_END_LEVEL
#define PARETO_FIND_RECURSION_END_LEVEL 2
#endif

#ifndef BATCH_SIZE
#define BATCH_SIZE 256
#endif

#define ROUND_DOWN(x, s) ((x) & ~((s)-1))


template<typename type>
struct BTreeSetOrderer {
	inline bool operator() (const type& i, const type& j) const {
		if (i.first_weight == j.first_weight) {
			if (i.second_weight == j.second_weight) {
				return i.node < j.node;
			}
			return i.second_weight < j.second_weight;
		}
		return i.first_weight < j.first_weight;
	}
};

/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename graph_slot>
class ParallelBTreeParetoQueue : public btree<NodeLabel, Label, BTreeSetOrderer<NodeLabel>> {
	friend class FindParetMinTask;
private:
	typedef btree<NodeLabel, Label, BTreeSetOrderer<NodeLabel>> base_type;

	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;

	typedef typename Label::weight_type weight_type;
	const Label min_label;

	typedef typename base_type::node node;
	typedef typename base_type::inner_node inner_node;
	typedef typename base_type::leaf_node leaf_node;
	typedef typename base_type::width_type width_type;

	const graph_slot& graph;

	tbb::task_list root_tasks;

public:
	typedef std::vector< Operation<NodeLabel>, tbb::cache_aligned_allocator<Operation<NodeLabel>> > OpVec; 
	typedef std::vector< NodeLabel, tbb::cache_aligned_allocator<Label> > CandLabelVec;
	typedef std::vector< Label, tbb::cache_aligned_allocator<Label> > LabelVec; 

	struct ThreadData {
		OpVec updates;
		CandLabelVec candidates;
		LabelVec spare_labelset;
	};	
	typedef tbb::enumerable_thread_specific< ThreadData, tbb::cache_aligned_allocator<ThreadData>, tbb::ets_key_per_instance > TLSData; 
	TLSData tls_data;

	typedef unsigned short thread_count;
	const thread_count num_threads; 

	struct LabelSet { 
		LabelVec labels;
	};
	struct PaddedLabelSet : public LabelSet {
		char pad[DCACHE_LINESIZE - sizeof(LabelSet) % DCACHE_LINESIZE];
	};

	std::vector<PaddedLabelSet> labelsets;

	 __attribute__ ((aligned (DCACHE_LINESIZE))) tbb::atomic<size_t> update_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) OpVec updates;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) tbb::atomic<size_t> candidate_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) CandLabelVec candidates;


	 #ifdef GATHER_SUBCOMPNENT_TIMING
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			struct tls_tim {
				double candidates_sort = 0;
				double update_labelsets = 0;
				double find_pareto_min = 0;
				double write_local_to_shared = 0;
				double create_candidates = 0;
			};
			typedef tbb::enumerable_thread_specific<tls_tim, tbb::cache_aligned_allocator<tls_tim>, tbb::ets_key_per_instance > TLSTimings;
			TLSTimings tls_timings;
		#endif
	#endif

public:

	ParallelBTreeParetoQueue(const graph_slot& _graph, const thread_count _num_threads)
		: min_label(std::numeric_limits<weight_type>::min(), std::numeric_limits<weight_type>::max()),
			graph(_graph), num_threads(_num_threads), labelsets(_graph.numberOfNodes())
	{
		assert(num_threads > 0);

		updates.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
		candidates.reserve(LARGE_ENOUGH_FOR_EVERYTHING);

		const typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::min();
		const typename Label::weight_type max = std::numeric_limits<typename Label::weight_type>::max();

		// add sentinals
		for (size_t i=0; i<labelsets.size(); ++i) {
			labelsets[i].labels.insert(labelsets[i].labels.begin(), Label(min, max));
			labelsets[i].labels.insert(labelsets[i].labels.end(), Label(max, min));
		}
	}

	void init(const NodeLabel& data) {
		Operation<NodeLabel> op = {Operation<NodeLabel>::INSERT, data};
		base_type::apply_updates(&op, 1);
	}

	template<typename T>
	void applyUpdates(const T* updates, const size_t update_count) {
		base_type::apply_updates(updates, update_count);
	}

	bool empty() {
		return base_type::empty();
	}

	size_t size() {
		return base_type::size();
	}

	void printStatistics() {
		std::cout << base_type::name() << " (k=" << base_type::traits::leafparameter_k << ", b=" << base_type::traits::branchingparameter_b << "):" << std::endl;
		std::cout << "  inner slots size [" << base_type::innerslotmin << ", " << base_type::innerslotmax << "]. Bytes: " << base_type::innernodebytesize << std::endl;
		std::cout << "  leaf slots size [" << base_type::leafslotmin << ", " << base_type::leafslotmax << "]. Bytes: " << base_type::leafnodebytesize << std::endl;
	}

	void findParetoMinima() {
		if (base_type::root->level < PARETO_FIND_RECURSION_END_LEVEL) {
			findParetoMinAndDistribute(base_type::root, min_label);
		} else {
			const inner_node* const inner = (inner_node*) base_type::root;
			const width_type slotuse = inner->slotuse;

			const Label* min = &min_label;
			for (width_type i = 0; i<slotuse; ++i) {
				 if (inner->slot[i].minimum.second_weight < min->second_weight ||
						(inner->slot[i].minimum.first_weight == min->first_weight && inner->slot[i].minimum.second_weight == min->second_weight)) {
					root_tasks.push_back(*new(tbb::task::allocate_root()) FindParetMinTask(inner->slot[i].childid, min, this));
					min = &inner->slot[i].minimum;
				}
			}
			tbb::task::spawn_root_and_wait(root_tasks);
			root_tasks.clear();
		}
	}

    class FindParetMinTask : public tbb::task {
       	const node* const in_node;
       	const Label* const prefix_minima;
        ParallelBTreeParetoQueue* const tree;

    public:
		
		inline FindParetMinTask(const node* const _in_node, const Label* const _prefix_minima, ParallelBTreeParetoQueue* const _tree)
			: in_node(_in_node), prefix_minima(_prefix_minima), tree(_tree)
		{ }

		tbb::task* execute() {
			if (in_node->level < PARETO_FIND_RECURSION_END_LEVEL) {
				tree->findParetoMinAndDistribute(in_node, *prefix_minima);
				return NULL;
			} else {
				const inner_node* const inner = (inner_node*) in_node;
				const width_type slotuse = inner->slotuse;

				tbb::task_list tasks;
				width_type task_count = 0;
				auto& c = *new(allocate_continuation()) tbb::empty_task(); // our parent will wait for this one

				const Label* min = prefix_minima;
				for (width_type i = 0; i<slotuse; ++i) {
				 if (inner->slot[i].minimum.second_weight < min->second_weight ||
						(inner->slot[i].minimum.first_weight == min->first_weight && inner->slot[i].minimum.second_weight == min->second_weight)) {
						tasks.push_back(*new(c.allocate_child()) FindParetMinTask(inner->slot[i].childid, min, tree));
						++task_count;
						min = &inner->slot[i].minimum;
					}
				}
				c.set_ref_count(task_count);
				auto& task = tasks.pop_front();
				spawn(tasks);
				return &task;
			}
		}
    };

	void findParetoMinAndDistribute(const node* const in_node, const Label& prefix_minima) {
		typename TLSData::reference tl = tls_data.local();

		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			typename TLSTimings::reference subtimings = tls_timings.local();
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif
		// Scan the tree while it is likely to be still in cache
		const size_t pre_size = tl.updates.size();
		base_type::find_pareto_minima(in_node, prefix_minima, tl.updates);
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.find_pareto_min += (stop-start).seconds();
			start = stop;
		#endif

		// Derive candidates 
		for (size_t i=pre_size; i < tl.updates.size(); ++i) {
			const NodeLabel& min = tl.updates[i].data;
			FORALL_EDGES(graph, min.node, eid) {
				const Edge& edge = graph.getEdge(eid);
				tl.candidates.push_back(NodeLabel(edge.target, createNewLabel(min, edge)));
			}
		}
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.create_candidates += (stop-start).seconds();
			start = stop;
		#endif

		// Move thread local data to shared data structure. Move in cache sized blocks to prevent false sharing
		if (tl.candidates.size() > BATCH_SIZE) {
			const size_t aligned_size = ROUND_DOWN(tl.candidates.size(), DCACHE_LINESIZE / sizeof(CandLabelVec::value_type));
			const size_t remaining = tl.candidates.size() - aligned_size;
			const size_t position = candidate_counter.fetch_and_add(aligned_size);
			assert(position + tl.candidates.size() < candidates.capacity());
			memcpy(candidates.data() + position, tl.candidates.data()+remaining, sizeof(CandLabelVec::value_type) * aligned_size);
			tl.candidates.resize(remaining);
		}
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.write_local_to_shared += (stop-start).seconds();
			start = stop;
		#endif
	}

	static inline Label createNewLabel(const Label& current_label, const Edge& edge) {
		return Label(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

};


#endif