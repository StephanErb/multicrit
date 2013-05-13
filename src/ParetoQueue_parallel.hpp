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

#define ROUND_DOWN(x, s) ((x) & ~((s)-1))

template<typename _value_type, typename vector_type, typename counter_type>
class BufferedSharedVector {
private:
	vector_type* vec;
	counter_type* counter;

	inline void alloc(size_t delta) {
		current = counter->fetch_and_add(delta);
		end = current + delta;
	}

public:
	typedef _value_type value_type;
	size_t current = 0;
	size_t end = 0;

	inline void setup(vector_type& _vec, counter_type& _counter) {
		vec = &_vec;
		counter = &_counter;
	}
	inline void reset() {
		current = end = 0;
	}
	template<typename ...Args>
	inline void emplace_back(Args&& ...args) {
		if (current == end) {
			alloc(BATCH_SIZE);
			for (size_t i = current; i < end; ++i) {
				vec->data()[i].node = std::numeric_limits<unsigned int>::max();
			}
		}
		assert(vec->capacity() > current);
		vec->data()[current++] = value_type(std::forward<Args>(args)...);
	}
};

template<typename _value_type, typename vector_type, typename counter_type>
class BufferedSharedOpVector {
private:
	vector_type* vec;
	counter_type* counter;

	inline void alloc(size_t delta) {
		current = counter->fetch_and_add(delta);
		end = current + delta;
	}

public:
	typedef _value_type value_type;
	size_t current = 0;
	size_t end = 0;

	inline void setup(vector_type& _vec, counter_type& _counter) {
		vec = &_vec;
		counter = &_counter;
	}
	inline void reset() {
		current = end = 0;
	}
	template<typename ...Args>
	inline void emplace_back(Args&& ...args) {
		if (current == end) {
			alloc(BATCH_SIZE);
			for (size_t i = current; i < end; ++i) {
				vec->data()[i].data.first_weight = std::numeric_limits<unsigned int>::max();
				vec->data()[i].data.second_weight = std::numeric_limits<unsigned int>::max();
			}
		}
		assert(vec->capacity() > current);
		vec->data()[current++] = value_type(std::forward<Args>(args)...);
	}
};


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
	typedef typename base_type::inner_node_data inner_node_data;
	typedef typename base_type::leaf_node leaf_node;
	typedef typename base_type::width_type width_type;

	using base_type::num_threads;
	using base_type::min_problem_size;

	const graph_slot& graph;

public:

	struct GroupLabelsByWeightComp {
		inline bool operator() (const Label& i, const Label& j) const {
			return i.combined() < j.combined();
		}
	};

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

	 __attribute__ ((aligned (DCACHE_LINESIZE))) tbb::atomic<size_t> update_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) OpVec updates;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) tbb::atomic<size_t> candidate_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) CandLabelVec candidates;


	struct ThreadData {
		BufferedSharedVector<NodeLabel, CandLabelVec, tbb::atomic<size_t>> candidates;
		BufferedSharedOpVector<Operation<NodeLabel>, OpVec, tbb::atomic<size_t>> updates;
		#ifdef BTREE_PARETO_LABELSET
			typename LabelSet::ThreadLocalLSData labelset_data;
		#endif
	};	
	typedef tbb::enumerable_thread_specific< ThreadData, tbb::cache_aligned_allocator<ThreadData>, tbb::ets_key_per_instance > TLSData; 
	TLSData tls_data;


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

	ParallelBTreeParetoQueue(const graph_slot& _graph, const base_type::thread_count _num_threads)
		: base_type(_num_threads), min_label(std::numeric_limits<weight_type>::min(), std::numeric_limits<weight_type>::max()),
			graph(_graph), labelsets(_graph.numberOfNodes())
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

		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			typename TLSTimings::reference subtimings = tls_timings.local();
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif
		tl.updates.setup(updates, update_counter);
		tl.candidates.setup(candidates, candidate_counter);

		base_type::find_pareto_minima(in_node, prefix_minima, tl.updates, tl.candidates, graph);
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.find_pareto_min += (stop-start).seconds();
			start = stop;
		#endif
	}


};


#endif
