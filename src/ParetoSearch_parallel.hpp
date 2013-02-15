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

	typedef typename std::vector<Label, tbb::scalable_allocator<Label>> LabelVec;
	typedef typename LabelVec::iterator label_iter;
	typedef typename LabelVec::const_iterator const_label_iter;

	typedef tbb::enumerable_thread_specific< tbb::auto_partitioner, tbb::cache_aligned_allocator<tbb::auto_partitioner>, tbb::ets_key_per_instance > TLSPartitioner;
	TLSPartitioner tls_partitioner;


    // Permanent and temporary labels per node.
	std::vector<LabelVec> labels;

	const graph_slot& graph;

	ParetoQueue pq;
	ParetoSearchStatistics<Label> stats;

	tbb::atomic<size_t> update_counter;
	typename std::vector<Operation<Data>> updates;

	#ifdef GATHER_SUBCOMPNENT_TIMING
		enum Component {FIND_PARETO_MIN_AND_BUCKETSORT=0, UPDATE_LABELSETS=1, SORT=2, PQ_UPDATE=3, CANDIDATES_COLLECTION=4,
						CANDIDATES_SORT=5, UPDATE_ACTUAL_LABELSET=6, COPY_UPDATES=7};
		double timings[8] = {0, 0, 0, 0, 0, 0, 0, 0};

		#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
			struct tls_tim {
				double candidates_collection = 0;
				double candidates_sort = 0;
				double update_labelsets = 0;
				double copy_updates = 0;
			};
			typedef tbb::enumerable_thread_specific<tls_tim, tbb::cache_aligned_allocator<tls_tim>, tbb::ets_key_per_instance > TLSTimings;
			TLSTimings tls_timings;
		#endif
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

	static struct WeightLessComp {
		bool operator() (const Label& i, const Label& j) const {
			return i.first_weight < j.first_weight;
		}
	} firstWeightLess;

	static bool secondWeightGreaterOrEquals(const Label& i, const Label& j) {
		return i.second_weight >= j.second_weight;
	}

	/** First label where the x-coord is truly smaller */
	static label_iter x_predecessor(label_iter begin, label_iter end, const Label& new_label) {
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

	void updateLabelSet(const NodeID node, LabelVec& labelset, const const_cand_iter start, const const_cand_iter end, typename ParetoQueue::OpVec& updates) {
		typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

		label_iter labelset_iter = labelset.begin();
		for (const_cand_iter candidate = start; candidate != end; ++candidate) {
			Label new_label = *candidate;
			// short cut dominated check among candidates
			if (new_label.second_weight >= min) { 
				stats.report(LABEL_DOMINATED);
				stats.report(DOMINATION_SHORTCUT);
				continue; 
			}
			label_iter iter;
			if (isDominated(labelset_iter, labelset.end(), new_label, iter)) {
				stats.report(LABEL_DOMINATED);
				min = iter->second_weight; 
				continue;
			}
			min = new_label.second_weight;
			stats.report(LABEL_NONDOMINATED);
			updates.push_back(Operation<Data>(Operation<Data>::INSERT, Data(node, new_label)));

			label_iter first_nondominated = y_predecessor(iter, new_label);
			if (iter == first_nondominated) {

				#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
					set_changes[(int)(100*((first_nondominated - labelset.begin())/(double)labelset.size()) + 0.5)]++;
				#endif

				// delete range is empty, so just insert
				labelset_iter = labelset.insert(first_nondominated, new_label);
			} else {
				// schedule deletion of dominated labels
				for (label_iter i = iter; i != first_nondominated; ++i) {

					#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
						if (i != iter) set_changes[(int)(100*((first_nondominated - labelset.begin())/(double)labelset.size()) + 0.5)]++;
					#endif

					updates.push_back(Operation<Data>(Operation<Data>::DELETE, Data(node, *i)));
				}
				// replace first dominated label and remove the rest
				*iter = new_label;
				labelset_iter = labelset.erase(++iter, first_nondominated);
			}
		}
	}


public:
	ParetoSearch(const graph_slot& graph_, const unsigned short num_threads):
		labels(graph_.numberOfNodes()),
		graph(graph_),
		pq(graph_, num_threads)
		#ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
			,set_changes(101)
		#endif
	{
		const typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::min();
		const typename Label::weight_type max = std::numeric_limits<typename Label::weight_type>::max();

		// add sentinals
		for (size_t i=0; i<labels.size(); ++i) {
			labels[i].insert(labels[i].begin(), Label(min, max));
			labels[i].insert(labels[i].end(), Label(max, min));		
		}

		updates.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
	}

	void run(const NodeID node) {
		tbb::task_list tasks;
		pq.init(Data(node, Label(0,0)));

		#ifdef GATHER_SUBCOMPNENT_TIMING
			tbb::tick_count start = tbb::tick_count::now();
			tbb::tick_count stop = tbb::tick_count::now();
		#endif

		while (!pq.empty()) {
			update_counter = 0;
			stats.report(ITERATION, pq.size());

			// write minimas & candidates to thread locals in pq.*
			pq.findParetoMinima(); 
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[FIND_PARETO_MIN_AND_BUCKETSORT] += (stop-start).seconds();
				start = stop;
			#endif

			// Spawn tasks that use these candidates to update the labelsets
			for (typename ParetoQueue::TLSAffected::reference affected :  pq.tls_affected_nodes) {
				if (!affected.nodes.empty()) {
					auto& task = *new(tbb::task::allocate_root()) LabelSetUpdateTask(this, affected.nodes);
					task.set_affinity(affected.affinity);
					tasks.push_back(task);
				}
			}
			tbb::task::spawn_root_and_wait(tasks);
			tasks.clear();
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[UPDATE_LABELSETS] += (stop-start).seconds();
				start = stop;
			#endif

			// Due to loadbalancing in the parallel_for, there may be some remaining elements that still need to be copied
			for (typename ParetoQueue::TLSUpdates::reference local_updates : pq.tls_local_updates) {
				if (!local_updates.empty()) {
					assert(update_counter + local_updates.size() < updates.capacity());
					memcpy(updates.data() + update_counter, local_updates.data(), sizeof(Operation<Data>) * local_updates.size());
					update_counter += local_updates.size();
					local_updates.clear();
				}
			}
			tbb::parallel_sort(updates.data(), updates.data() + update_counter, groupByWeight);
			#ifdef GATHER_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				timings[SORT] += (stop-start).seconds();
				start = stop;
			#endif

			pq.applyUpdates(updates.data(), update_counter);
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
		std::cout << stats.toString(labels) << std::endl;
		#ifdef GATHER_SUBCOMPNENT_TIMING
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				for (typename TLSTimings::reference subtimings : tls_timings) {
					timings[CANDIDATES_COLLECTION] = std::max(subtimings.candidates_collection, timings[CANDIDATES_COLLECTION]);
					timings[CANDIDATES_SORT] = std::max(subtimings.candidates_sort, timings[CANDIDATES_SORT]);
					timings[UPDATE_ACTUAL_LABELSET] = std::max(subtimings.update_labelsets, timings[UPDATE_ACTUAL_LABELSET]);
					timings[COPY_UPDATES] += subtimings.copy_updates;
				}
			#endif
			std::cout << "Subcomponent Timings:" << std::endl;
			std::cout << "  " << timings[FIND_PARETO_MIN_AND_BUCKETSORT]  << " Find pareto min & bucket sort" << std::endl;
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
	size_t size(NodeID node) {return labels[node].size()-2; }

	label_iter begin(NodeID node) { return ++labels[node].begin(); }
	const_label_iter begin(NodeID node) const { return ++labels[node].begin(); }
	label_iter end(NodeID node) { return --labels[node].end(); }
	const_label_iter end(NodeID node) const { return --labels[node].end(); }


private:

	class LabelSetUpdateTask : public tbb::task {

		ParetoSearch<graph_slot>* const ps;
		typename ParetoQueue::NodeVec& locally_affected_nodes;

		static struct GroupByNodeComp {
			bool operator() (const Label& i, const Label& j) const {
				if (i.first_weight == j.first_weight) {
						return i.second_weight < j.second_weight;
					}
				return i.first_weight < j.first_weight;
			}
		} groupLabels;

	public:
		
		inline LabelSetUpdateTask(ParetoSearch<graph_slot>* const _ps, typename ParetoQueue::NodeVec& _locally_affected_nodes)
			: ps(_ps), locally_affected_nodes(_locally_affected_nodes)
		{}

		tbb::task* execute() {
			tbb::parallel_for(tbb::blocked_range<size_t>(0, locally_affected_nodes.size()),
				[&](const tbb::blocked_range<size_t>& r) {

				typename ParetoQueue::TLSUpdates::reference local_updates = ps->pq.tls_local_updates.local();
				const size_t last_node_index = locally_affected_nodes.size()-1;

				#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
					typename TLSTimings::reference subtimings = ps->tls_timings.local();
					tbb::tick_count start = tbb::tick_count::now();
					tbb::tick_count stop = tbb::tick_count::now();
				#endif

				for (size_t i = r.begin(); i != r.end(); ++i) {	
					const NodeID node = locally_affected_nodes[last_node_index - i];
					// Collect candidates
					const typename ParetoQueue::thread_count count = ps->pq.candidate_bufferlist_counter[node];
					ps->pq.candidate_bufferlist_counter[node] = 0;
		
					typename ParetoQueue::CandLabelVec* candidates = ps->pq.candidate_bufferlist[node * ps->pq.num_threads + 0];	
					assert(candidates->size() > 0);
					for (typename ParetoQueue::thread_count j=1; j < count; ++j) {
						typename ParetoQueue::CandLabelVec* c = ps->pq.candidate_bufferlist[node * ps->pq.num_threads + j];
						candidates->insert(candidates->end(), std::make_move_iterator(c->begin()), std::make_move_iterator(c->end()));
						assert(c->size() > 0);
						c->clear();
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

					ps->updateLabelSet(node, ps->labels[node], candidates->cbegin(), candidates->cend(), local_updates);
					candidates->clear();
					#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
						stop = tbb::tick_count::now();
						subtimings.update_labelsets += (stop-start).seconds();
						start = stop;
					#endif
				}
			}, ps->tls_partitioner.local());

			// Copy updates to globally shared data structure
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				typename TLSTimings::reference subtimings = ps->tls_timings.local();
				tbb::tick_count start = tbb::tick_count::now();
				tbb::tick_count stop = tbb::tick_count::now();
			#endif
			typename ParetoQueue::TLSUpdates::reference local_updates = ps->pq.tls_local_updates.local();
			const size_t position = ps->update_counter.fetch_and_add(local_updates.size());
			assert(position + local_updates.size() < ps->updates.capacity());
			memcpy(ps->updates.data() + position, local_updates.data(), sizeof(Operation<Data>) * local_updates.size());
			#ifdef GATHER_SUB_SUBCOMPNENT_TIMING
				stop = tbb::tick_count::now();
				subtimings.copy_updates += (stop-start).seconds();
				start = stop;
			#endif

			local_updates.clear();
			locally_affected_nodes.clear();
			return NULL;
		}
	};
};


#endif 
