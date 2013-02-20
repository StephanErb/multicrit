#ifndef PARALLEL_PARETO_QUEUE_H_
#define PARALLEL_PARETO_QUEUE_H_

#include <iostream>
#include <vector>
#include <deque>

#include "options.hpp"

#define COMPUTE_PARETO_MIN
#include "datastructures/BTree.hpp"

#include <algorithm>
#include <limits>
#include "utility/datastructure/graph/GraphMacros.h"

#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/scalable_allocator.h"
#include "tbb/task.h"
#include "tbb/concurrent_vector.h"


#ifndef PARETO_FIND_RECURSION_END_LEVEL
#define PARETO_FIND_RECURSION_END_LEVEL 2
#endif



template<typename type>
struct BTreeSetOrderer {
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


/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename graph_slot, typename data_type, typename label_type>
class ParallelBTreeParetoQueue : public btree<data_type, label_type, BTreeSetOrderer<data_type>> {
	friend class FindParetMinTask;
private:
	typedef btree<data_type, label_type, BTreeSetOrderer<data_type>> base_type;

	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;

	typedef typename data_type::weight_type weight_type;
	const data_type min_label;

	typedef typename base_type::node node;
	typedef typename base_type::inner_node inner_node;
	typedef typename base_type::leaf_node leaf_node;
	typedef typename base_type::width_type width_type;

	const graph_slot& graph;

	tbb::task_list root_tasks;


public:
	typedef std::vector< Operation<data_type>, tbb::scalable_allocator<Operation<data_type>> > OpVec; 
	typedef std::vector< NodeID, tbb::scalable_allocator<NodeID> > NodeVec;
	typedef std::vector< data_type, tbb::scalable_allocator<data_type> > MinimaVec;
	typedef std::vector< label_type, tbb::scalable_allocator<label_type> > CandLabelVec;
	typedef std::vector< label_type, tbb::scalable_allocator<label_type> > LabelVec; // When changing this type check the LabelSet struct size

	struct TimestampedCandidates {
		size_t timestamp;
		CandLabelVec candidates;
	};
	typedef std::vector< TimestampedCandidates > CandLabelVecVec;

	typedef tbb::enumerable_thread_specific< OpVec,           tbb::cache_aligned_allocator<OpVec>,           tbb::ets_key_per_instance > TLSUpdates; 
	typedef tbb::enumerable_thread_specific< CandLabelVecVec, tbb::cache_aligned_allocator<CandLabelVecVec>, tbb::ets_key_per_instance > TLSCandidates; 
	typedef tbb::enumerable_thread_specific< NodeVec,         tbb::cache_aligned_allocator<NodeVec>,         tbb::ets_key_per_instance > TLSAffected;
	typedef tbb::enumerable_thread_specific< MinimaVec,       tbb::cache_aligned_allocator<MinimaVec>,       tbb::ets_key_per_instance > TLSMinima; 
	typedef tbb::enumerable_thread_specific< LabelVec,        tbb::cache_aligned_allocator<LabelVec>,        tbb::ets_key_per_instance > TLSSpareLabelVec; 

	TLSUpdates tls_local_updates;
	TLSMinima tls_minima;
	TLSCandidates tls_candidates;
	TLSAffected tls_affected_nodes;
	TLSSpareLabelVec tls_spare_labelset;

	typedef unsigned short thread_count; // When changing this type check the LabelSet struct size
	const thread_count num_threads; 

	#define PE_COUNT 12
	struct LabelSet { // With these datatypes & settings: stuct occupies 2 cache lines (64bytes each) 
		LabelVec labels;
		tbb::atomic<thread_count> bufferlist_counter;
		CandLabelVec* bufferlists[PE_COUNT]; // FIXME: too small for large instances
	};
	std::vector<LabelSet> labelsets;

	 __attribute__ ((aligned (DCACHE_LINESIZE))) tbb::atomic<size_t> update_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) OpVec updates;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) tbb::atomic<size_t> affected_nodes_counter;
	 __attribute__ ((aligned (DCACHE_LINESIZE))) NodeVec affected_nodes;

	 tbb::atomic<size_t> current_timestamp;

	 #ifdef GATHER_SUBCOMPNENT_TIMING
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			struct tls_tim {
				double candidates_collection = 0;
				double candidates_sort = 0;
				double update_labelsets = 0;
				double copy_updates = 0;
				double find_pareto_min = 0;
				double group_pareto_min = 0;
				double write_pareto_min_updates = 0;
				double write_affected_nodes = 0;
			};
			typedef tbb::enumerable_thread_specific<tls_tim, tbb::cache_aligned_allocator<tls_tim>, tbb::ets_key_per_instance > TLSTimings;
			TLSTimings tls_timings;
		#endif
	#endif

public:

	ParallelBTreeParetoQueue(const graph_slot& _graph, const thread_count _num_threads)
		: base_type(_graph.numberOfNodes()), min_label(NodeID(0), typename data_type::label_type(std::numeric_limits<weight_type>::min(),
			std::numeric_limits<weight_type>::max())), graph(_graph), num_threads(_num_threads), labelsets(_graph.numberOfNodes())
	{
		assert(num_threads > 0);
		assert(PE_COUNT >= num_threads);

		updates.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
		affected_nodes.reserve(LARGE_ENOUGH_FOR_EVERYTHING);

		const typename label_type::weight_type min = std::numeric_limits<typename label_type::weight_type>::min();
		const typename label_type::weight_type max = std::numeric_limits<typename label_type::weight_type>::max();

		// add sentinals
		for (size_t i=0; i<labelsets.size(); ++i) {
			labelsets[i].labels.insert(labelsets[i].labels.begin(), label_type(min, max));
			labelsets[i].labels.insert(labelsets[i].labels.end(), label_type(max, min));
		}
	}

	void init(const data_type& data) {
		const std::vector<Operation<data_type>> upds = {{Operation<data_type>::INSERT, data}};
		base_type::apply_updates(upds);
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

	void findParetoMinima(const size_t _current_timestamp) {
		assert(update_counter == 0);
		assert(affected_nodes_counter == 0);

		current_timestamp = _current_timestamp;
		assert(current_timestamp > 0);

		if (base_type::root->level < PARETO_FIND_RECURSION_END_LEVEL) {
			findParetoMinAndDistribute(base_type::root, min_label);
		} else {
			const inner_node* const inner = (inner_node*) base_type::root;
			const width_type slotuse = inner->slotuse;

			const label_type* min = &min_label;
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
       	const label_type* const prefix_minima;
        ParallelBTreeParetoQueue* const tree;

    public:
		
		inline FindParetMinTask(const node* const _in_node, const label_type* const _prefix_minima, ParallelBTreeParetoQueue* const _tree)
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

				const label_type* min = prefix_minima;
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

	void findParetoMinAndDistribute(const node* const in_node, const label_type& prefix_minima) {
		typename TLSMinima::reference minima = tls_minima.local();

		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			typename TLSTimings::reference subtimings = tls_timings.local();
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif
		// Scan the tree while it is likely to be still in cache
		assert(minima.size() == 0);
		base_type::find_pareto_minima(in_node, prefix_minima, minima);
		assert(minima.size() > 0);

		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.find_pareto_min += (stop-start).seconds();
			start = stop;
		#endif
		// Schedule minima for deletion
		size_t upd_position = update_counter.fetch_and_add(minima.size());
		assert(upd_position + minima.size() < updates.capacity());
		for (const data_type& min : minima) {
			updates[upd_position++] = Operation<data_type>(Operation<data_type>::DELETE, min);
		}
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.write_pareto_min_updates += (stop-start).seconds();
			start = stop;
		#endif

		typename TLSAffected::reference locally_affected_nodes = tls_affected_nodes.local();
		typename TLSCandidates::reference local_candidates = tls_candidates.local();
		local_candidates.resize(graph.numberOfNodes());

		// Derive candidates & Place into buckets
		for (const data_type& min : minima) {
			FORALL_EDGES(graph, min.node, eid) {
				const Edge& edge = graph.getEdge(eid);
				TimestampedCandidates& tc = local_candidates[edge.target];

				if (current_timestamp != tc.timestamp) {
					assert(current_timestamp > tc.timestamp);
					// Prepare candidate list for this iteration
					tc.timestamp = current_timestamp;
					tc.candidates.clear();
					// Publish the candidate list
					LabelSet& ls = labelsets[edge.target];
					const thread_count position = ls.bufferlist_counter.fetch_and_increment();
					ls.bufferlists[position] = &(tc.candidates);
					if (position == 0) {
						// We were the first, so we are responsible!
						locally_affected_nodes.push_back(edge.target);
					}
					assert(position < num_threads);
				} else {
					assert(tc.candidates.size() > 0);
				}
				tc.candidates.push_back(createNewLabel(min, edge));
			}
		}
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.group_pareto_min += (stop-start).seconds();
			start = stop;
		#endif
		if (locally_affected_nodes.size() > 0) {
			// Move affected nodes to shared data structure
			const size_t position = affected_nodes_counter.fetch_and_add(locally_affected_nodes.size());
			assert(position + locally_affected_nodes.size() < affected_nodes.capacity());
			memcpy(affected_nodes.data() + position, locally_affected_nodes.data(), sizeof(NodeID) * locally_affected_nodes.size());
		}
		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			stop = tbb::tick_count::now();
			subtimings.write_affected_nodes += (stop-start).seconds();
			start = stop;
		#endif
		locally_affected_nodes.clear();
		minima.clear();


	}

	static label_type createNewLabel(const label_type& current_label, const Edge& edge) {
		return label_type(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

};


#endif