#ifndef PARALLEL_PARETO_QUEUE_H_
#define PARALLEL_PARETO_QUEUE_H_

#include <iostream>
#include <vector>
#include <deque>

#include "options.hpp"

#ifndef PARALLEL_BUILD
#define PARALLEL_BUILD
#endif

#define COMPUTE_PARETO_MIN
#include "datastructures/BTree_parallel.hpp"
#undef COMPUTE_PARETO_MIN

#include "datastructures/ThreadLocalWriteBuffer.hpp"
#include "ParetoLabelSet_sequential.hpp"

#include <algorithm>
#include <limits>
#include "utility/datastructure/graph/GraphMacros.h"
#include "Label.hpp"

#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/scalable_allocator.h"
#include "tbb/task.h"
#include "tbb/concurrent_vector.h"



/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename graph_slot>
class ParallelBTreeParetoQueue : public btree<NodeLabel, Label, GroupNodeLablesByWeightAndNodeComperator> {
	friend class FindParetMinTask;
private:
	typedef btree<NodeLabel, Label, GroupNodeLablesByWeightAndNodeComperator> base_type;

	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;

	typedef typename Label::weight_type weight_type;
	const Label min_label;

	typedef typename base_type::node node;
	typedef typename base_type::inner_node inner_node;
	typedef typename base_type::inner_node_data inner_node_data;
	typedef typename base_type::leaf_node leaf_node;
	typedef typename base_type::width_type width_type;

	using base_type::min_problem_size;

	const graph_slot& graph;

public:

	using base_type::num_threads;

	#ifdef BTREE_PARETO_LABELSET
		typedef BtreeParetoLabelSet<Label, GroupLabelsByWeightComp, tbb::cache_aligned_allocator<Label>> LabelSet;
	#else
		typedef VectorParetoLabelSet<tbb::cache_aligned_allocator<Label>> LabelSet;
	#endif

	typedef std::vector< Operation<NodeLabel>, tbb::cache_aligned_allocator<Operation<NodeLabel>> > OpVec; 
	typedef std::vector< NodeLabel, tbb::cache_aligned_allocator<Label> > CandLabelVec;

	struct PaddedLabelSet : public LabelSet {
		char pad[DCACHE_LINESIZE - sizeof(LabelSet) % DCACHE_LINESIZE];
	};

	std::vector<PaddedLabelSet> labelsets;

	 __attribute__ ((aligned (DCACHE_LINESIZE))) AtomicCounter update_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) OpVec updates;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) AtomicCounter candidate_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) CandLabelVec candidates;


	struct ThreadData {
		ThreadLocalWriteBuffer<NodeLabel> candidates;
		ThreadLocalWriteBuffer<Operation<NodeLabel>> updates;
		typename LabelSet::ThreadLocalLSData labelset_data;

		ThreadData(ParallelBTreeParetoQueue* pq) 
		:	candidates(pq->candidates.data(), pq->candidate_counter,
				NodeLabel(std::numeric_limits<NodeID>::max(), Label())),
		    updates(pq->updates.data(), pq->update_counter, 
		    	Operation<NodeLabel>(Operation<NodeLabel>::INSERT, std::numeric_limits<NodeID>::max(), 
		    		std::numeric_limits<Label::weight_type>::max(), std::numeric_limits<Label::weight_type>::max()))
		{}
	};	
	typedef tbb::enumerable_thread_specific< ThreadData, tbb::cache_aligned_allocator<ThreadData>, tbb::ets_key_per_instance > TLSData; 
	TLSData tls_data;


public:

	ParallelBTreeParetoQueue(const graph_slot& _graph, const base_type::thread_count _num_threads)
		: base_type(_num_threads), min_label(std::numeric_limits<weight_type>::min(), std::numeric_limits<weight_type>::max()),
			graph(_graph), labelsets(_graph.numberOfNodes()), tls_data([this](){ return this; })
	{
		updates.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
		candidates.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
	}

	void init(const NodeLabel& data) {
		Operation<NodeLabel> op = {Operation<NodeLabel>::INSERT, data};
		auto ap = tbb::auto_partitioner();
		base_type::apply_updates(&op, 1, INSERTS_ONLY, ap);
	}

	template<typename T, typename Partitioner>
	void applyUpdates(const T* updates, const size_t update_count, Partitioner& partitioner) {
		base_type::apply_updates(updates, update_count, INSERTS_AND_DELETES, partitioner);
	}

	bool empty() {
		return base_type::empty();
	}

	size_t size() {
		return base_type::size();
	}

	void printStatistics() {
		std::cout << "# " << base_type::name() << " (k=" << base_type::traits::leafparameter_k << ", b=" << base_type::traits::branchingparameter_b << "):" << std::endl;
		std::cout << "#   inner slots size [" << base_type::innerslotmin << ", " << base_type::innerslotmax << "]. Bytes: " << base_type::innernodebytesize << std::endl;
		std::cout << "#   leaf slots size [" << base_type::leafslotmin << ", " << base_type::leafslotmax << "]. Bytes: " << base_type::leafnodebytesize << std::endl;
	}

	void findParetoMinima() {
		// Adaptive cut-off; Taken from the MCSTL implementation
        min_problem_size = std::max((base_type::size()/num_threads) / (log2(base_type::size()/num_threads + 1)+1), base_type::maxweight(1)*1.0);

		if (base_type::size() <= min_problem_size) {
			findParetoMinAndDistribute(base_type::root, min_label);
		} else {
			assert(!base_type::root->isleafnode());
			const inner_node* const inner = (inner_node*) base_type::root;
			const width_type slotuse = inner->slotuse;

			tbb::task_list root_tasks;
			const Label* min = &min_label;
			for (width_type i = 0; i<slotuse; ++i) {
				const Label& l = inner->slot[i].minimum;
				if (l.second_weight < min->second_weight || (l.first_weight == min->first_weight && l.second_weight == min->second_weight)) {
					root_tasks.push_back(*new(tbb::task::allocate_root()) FindParetMinTask(inner->slot[i], min, this, &base_type::subtree_affinity[i]));
					min = &l;
				}
			}
			tbb::task::spawn_root_and_wait(root_tasks);
		}
	}

    class FindParetMinTask : public tbb::task {
       	const inner_node_data& slot;
       	const Label* const prefix_minima;
        ParallelBTreeParetoQueue* const tree;
        tbb::task::affinity_id* affinity;

    public:
		
		inline FindParetMinTask(const inner_node_data& _slot, const Label* const _prefix_minima, ParallelBTreeParetoQueue* const _tree)
			: slot(_slot), prefix_minima(_prefix_minima), tree(_tree), affinity(NULL)
		{ }

		inline FindParetMinTask(const inner_node_data& _slot, const Label* const _prefix_minima, ParallelBTreeParetoQueue* const _tree, tbb::task::affinity_id* _affinity)
			: slot(_slot), prefix_minima(_prefix_minima), tree(_tree), affinity(_affinity)
		{ 
			set_affinity(*affinity);
		}

		void note_affinity(tbb::task::affinity_id id) {
            if (affinity != NULL && *affinity != id) {
                *affinity = id;
            }
        }

		tbb::task* execute() {
			if (slot.weight <= tree->min_problem_size) {
				tree->findParetoMinAndDistribute(slot.childid, *prefix_minima);
				return NULL;
			} else {
				assert(!slot.childid->isleafnode());
				const inner_node* const inner = (inner_node*) slot.childid;
				const width_type slotuse = inner->slotuse;

				tbb::task_list tasks;
				width_type task_count = 0;
				auto& c = *new(allocate_continuation()) tbb::empty_task(); // our parent will wait for this one

				const Label* min = prefix_minima;
				for (width_type i = 0; i<slotuse; ++i) {
					const Label& l = inner->slot[i].minimum;
					if (l.second_weight < min->second_weight || (l.first_weight == min->first_weight && l.second_weight == min->second_weight)) {
						tasks.push_back(*new(c.allocate_child()) FindParetMinTask(inner->slot[i], min, tree));
						++task_count;
						min = &l;
					}
				}
				c.set_ref_count(task_count);
				auto& task = tasks.pop_front();
				spawn(tasks);
				return &task;
			}
		}
    };

	inline void findParetoMinAndDistribute(const node* const in_node, const Label& prefix_minima) {
		typename TLSData::reference tl = tls_data.local();
		base_type::find_pareto_minima(in_node, prefix_minima, tl.updates, tl.candidates, graph);
	}
};


#endif
