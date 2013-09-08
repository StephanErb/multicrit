/**
 * Weight-balanced B-Tree with batch updates, partially based on:
 *
 * STX B+ Tree Template Classes v0.8.6
 * Copyright (C) 2008-2011 Timo Bingmann
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _PARALLEL_BTREE_H_
#define _PARALLEL_BTREE_H_

#include "BTree_base.hpp"

#include "tbb/parallel_scan.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/partitioner.h"
#include "tbb/task.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/scalable_allocator.h"
#include "tbb/tbb_thread.h"

#include "../../tbx/cache_aligned_blocked_range.hpp"


#define TREE_PAR_CUTOFF_LEVEL 1


/** 
 * Basic class implementing a base B+ tree data structure in memory.
 */
template <typename _Key,
    #ifdef COMPUTE_PARETO_MIN
          typename _MinKey,
          typename _Compare,
          typename _Traits = btree_default_traits<_Key, _MinKey>,
    #else
          typename _Compare = std::less<_Key>,
          typename _Traits = btree_default_traits<_Key, utility::NullData>,
    #endif
        typename _Alloc = tbb::cache_aligned_allocator<_Key>>
class btree : 
    #ifdef COMPUTE_PARETO_MIN
        public btree_base<_Key, _MinKey, _Compare, _Traits, _Alloc>
    #else
        public btree_base<_Key, _Compare, _Traits, _Alloc>
    #endif

 {
    friend class TreeCreationTask;
    friend class TreeRewriteTask;
    friend class TreeUpdateTask;

protected:
    #ifdef COMPUTE_PARETO_MIN
        typedef btree_base<_Key, _MinKey, _Compare, _Traits, _Alloc> base;
    #else
        typedef btree_base<_Key, _Compare, _Traits, _Alloc> base;
    #endif   

    typedef typename base::key_type         key_type;
    typedef typename base::key_compare      key_compare;
    typedef typename base::min_key_type     min_key_type;
    typedef typename base::allocator_type   allocator_type;
    typedef typename base::size_type        size_type;
    typedef typename base::level_type       level_type;
    typedef typename base::width_type       width_type;
    typedef typename base::leaf_node        leaf_node;
    typedef typename base::node             node;
    typedef typename base::inner_node       inner_node;
    typedef typename base::inner_node_data  inner_node_data;
    typedef typename base::tree_stats       tree_stats;
    typedef typename base::UpdateDescriptor UpdateDescriptor;
    typedef typename base::padded_leaf_list padded_leaf_list;
    typedef typename base::DataPerThread    DataPerThread;
    typedef typename base::UpdateDescriptorArray UpdateDescriptorArray;

public:
    typedef typename base::traits           traits;
    typedef typename base::thread_count     thread_count;

    using base::leafslotmax;
    using base::leafslotmin;
    using base::designated_leafsize;
    using base::innerslotmax;
    using base::innerslotmin;
    using base::minweight;
    using base::maxweight;

    using base::size;
    using base::clear;
    using base::verify;

protected:
    using base::root;
    using base::stats;
    using base::allocator;
    using base::weightdelta;
    using base::batch_type;
    using base::updates;
    using base::tls_data;

    using base::key_less;
    using base::key_lessequal;
    using base::key_greaterequal;
    using base::key_equal;
    using base::allocate_leaf;
    using base::allocate_inner;
    using base::allocate_leaf_without_count;
    using base::leaf_node_allocator;
    using base::inner_node_allocator;
    using base::free_node;
    using base::clear_recursive;
    using base::num_optimal_levels;
    using base::find_lower;
    using base::hasUpdates;
    using base::scheduleSubTreeUpdate;
    using base::set_min_element;
    using base::num_subtrees;
    using base::designated_subtreesize;
    using base::update_router;
    using base::create_subtree_from_leaves;
    using base::rewrite;
    using base::update;
    using base::get_resized_leaves_array;
    using base::write_updated_leaf_to_new_tree;
    using base::print_node;

protected:
    // *** Tree Object Data Members

    size_type min_problem_size;
    tbb::task::affinity_id subtree_affinity[innerslotmax] = {0};

    const thread_count num_threads; 

public:
    // *** Constructors and Destructor

    /// Default constructor initializing an empty B+ tree with the standard key
    /// comparison function
    explicit inline btree(const thread_count _num_threads, const allocator_type &alloc=allocator_type())
        : base(alloc), num_threads(_num_threads)
    {
        assert(num_threads > 0);
        weightdelta.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
    }

public:
    static std::string name() {
        return "Parallel BTree";
    }

public:

    template<typename T>
    void apply_updates(const T& _updates, const OperationBatchType _batch_type) {
        auto ap = tbb::auto_partitioner();
        apply_updates(_updates.data(), _updates.size(), _batch_type, ap);
    }

    template<typename T, typename Partitioner>
    void apply_updates(const T* _updates, const size_t update_count, const OperationBatchType _batch_type, Partitioner& partitioner) {
        const size_type new_size = setOperationsAndComputeWeightDelta(_updates, update_count, _batch_type, partitioner);
        stats.itemcount = new_size;
        
        if (new_size == 0) {
            clear(); // Tree will become empty. Just finish early.
            return;
        }
        if (root == NULL) {
            root = allocate_leaf();
        }

        const level_type level = num_optimal_levels(new_size);
        bool rebuild_needed = (level < root->level && size() < minweight(root->level)) || size() > maxweight(root->level);

        if (rebuild_needed) {
            BTREE_PRINT("Root-level rewrite session started for new level " << level << " with elements: " << new_size << std::endl);
            UpdateDescriptor upd = {rebuild_needed, new_size, 0, update_count};
            TreeRootCreationTask& task = *new(tbb::task::allocate_root()) TreeRootCreationTask(level, new_size, this);
            task.subtasks.push_back(*new(task.allocate_child()) TreeRewriteTask(root, 0, upd, task.leaves, this));
            task.subtask_count = 1;
            tbb::task::spawn_root_and_wait(task);
            root = task.result_node;
        } else {
            inner_node_data fake_slot;
            fake_slot.childid = root;

            if (root->isleafnode()) {
                TreeUpdateTask& task = *new(tbb::task::allocate_root()) TreeUpdateTask(fake_slot, 0, update_count, this);
                tbb::task::spawn_root_and_wait(task);
            } else {
                // The common case (and thus partially inlined here)
                inner_node* const inner = static_cast<inner_node*>(root);
                const size_type max_weight = maxweight(level-1);
                const size_type min_weight = max_weight / 4;
                auto& subtree_updates = tls_data.local().subtree_updates_per_level[MAX_TREE_LEVEL-1];
                bool rebalancing_needed = false;

                // Distribute operations and find out which subtrees need rebalancing
                const width_type last = inner->slotuse-1;
                size_type subupd_begin = 0;
                for (width_type i = 0; i < last; ++i) {
                    size_type subupd_end = find_lower(subupd_begin, update_count, inner->slot[i].slotkey);
                    rebalancing_needed |= scheduleSubTreeUpdate(i, inner->slot[i].weight, min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                    subupd_begin = subupd_end;
                } 
                rebalancing_needed |= scheduleSubTreeUpdate(last, inner->slot[last].weight, min_weight, max_weight, subupd_begin, update_count, subtree_updates);

                if (!rebalancing_needed) {
                    // No rebalancing needed at all (this is the common case). Push updates to subtrees to update them parallel
                    tbb::task_list root_tasks;
                    for (width_type i = 0; i < inner->slotuse; ++i) {
                        if (hasUpdates(subtree_updates[i])) {
                            root_tasks.push_back(*new(tbb::task::allocate_root()) TreeUpdateTask(inner->slot[i], subtree_updates[i].upd_begin, subtree_updates[i].upd_end, this, &subtree_affinity[i]));
                            inner->slot[i].weight = subtree_updates[i].weight;
                        }
                    }
                    tbb::task::spawn_root_and_wait(root_tasks);
                } else {
                    TreeUpdateTask& task = *new(tbb::task::allocate_root()) TreeUpdateTask(fake_slot, 0, update_count, this);
                    tbb::task::spawn_root_and_wait(task);
                }
            }
            root = fake_slot.childid;
        }

        #ifdef BTREE_DEBUG
            print_node(root, 0, true);
        #endif

        if (traits::selfverify) {
            verify();
        }
    }


private:

    template<typename TIn, typename TOut>
    class PrefixSum {
        TOut sum;
        TOut* const out;
        const TIn* const in;
    public:
        inline PrefixSum(TOut* _out, const TIn* _in)
            : sum(0), out(_out), in(_in) {}

        inline PrefixSum(const PrefixSum& b, tbb::split)
            : sum(0), out(b.out), in(b.in) {}

        template<typename Tag>
        void operator() (const cache_aligned_blocked_range<size_type>& r, Tag) {
            // Mixed insertions and deletions. Need to perform full prefix sum.
            const auto end = r.end();
            const auto begin = r.begin();
            if (Tag::is_final_scan()) {
                TOut temp = sum;

                for(size_type i=begin; i<end; ++i) {
                    temp += in[i].type;
                    out[i] = temp;
                }
                sum = temp;                
            } else {
                TOut temp = sum;
                for(size_type i=begin; i<end; ++i) {
                    temp += in[i].type;
                }
                sum = temp;                
            }
        }
        void reverse_join(const PrefixSum& a) {sum = a.sum + sum;}
        void assign(const PrefixSum& b) {sum = b.sum;}
        TOut get_sum() const { return sum; }
    };
    
    template<typename T, typename Partitioner>
    inline size_type setOperationsAndComputeWeightDelta(const T* _updates, const size_t update_count, const OperationBatchType _batch_type, Partitioner& partitioner) {
        updates = _updates;
        batch_type = _batch_type;

        // Adaptive cut-off; Taken from the MCSTL implementation
        min_problem_size = std::max((update_count/num_threads) / (log2(update_count/num_threads + 1)+1), 1.0);

        if (_batch_type == INSERTS_AND_DELETES) {
            // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
            // computes the weight delta realized by the updates in range [begin, end)
            weightdelta.reserve(update_count);
            weightdelta[0] = 0;

            PrefixSum<Operation<key_type>, signed long> body(weightdelta.data()+1, _updates);
            parallel_scan(cache_aligned_blocked_range<size_type>(0, update_count, min_problem_size), body, partitioner);

            return size() + body.get_sum(); 
        } else {
            // We can use the shortcut based on the 
            return size() + update_count * batch_type;
        }

    }

    template<typename Range, typename Body, typename Partitioner>
    void parallel_scan( const Range& range, Body& body, Partitioner& partitioner ) {
        tbb::internal::start_scan<Range,Body,Partitioner>::run(range,body, partitioner);
    }

    class TreeRootCreationTask : public tbb::task {
        node* in_node;
        const width_type old_slotuse;
        const level_type level;
        const size_type size;
        btree* const tree;
        short continuation_mode;
        inner_node_data fake_slot;

    public:
        tbb::task_list subtasks;
        width_type subtask_count;

        padded_leaf_list leaves;
        const width_type subtrees;

        node* result_node;

        inline TreeRootCreationTask(const level_type _level, const size_type _size, btree* const _tree) 
            : in_node(NULL), old_slotuse(0), level(_level), size(_size), tree(_tree), continuation_mode(0), subtrees(num_subtrees(_size, designated_subtreesize(_level)))
        { }

        inline TreeRootCreationTask(node* _node, const width_type _old_slotuse, const level_type _level, const size_type _size, btree* const _tree) 
            : in_node(_node), old_slotuse(_old_slotuse), level(_level), size(_size), tree(_tree), continuation_mode(0), subtrees(num_subtrees(_size, designated_subtreesize(_level)))
        { }

        tbb::task* execute() {
            if (continuation_mode == 0) {
                // Allocate & fill leaves via TreeRewriteTask
                const size_type subtrees = num_subtrees(size, designated_leafsize);
                leaves.resize(subtrees); // but use actual size

                continuation_mode++;
                recycle_as_continuation();
                set_ref_count(subtask_count);
                auto& task = subtasks.pop_front();
                spawn(subtasks);
                return &task;

            } else if (continuation_mode == 1 && size != 0) {
                // Reconstruct new tree from the filled leaves
                // Keep this task alive (as a continuation) so that the leaves data structure is still available to subtasks
                fake_slot.childid = in_node;
                continuation_mode++;
                recycle_as_continuation();
                TreeCreationTask& task = *new(allocate_child()) TreeCreationTask(fake_slot, old_slotuse, in_node != NULL, level, 0, size, leaves, tree);
                set_ref_count(1);
                return &task;
            } else {
                result_node = fake_slot.childid;
                return NULL;
            }
        }
    };


    class TreeCreationTask: public tbb::task {
        inner_node_data& slot;
        const width_type old_slotuse;
        const bool reuse_node;

        const level_type level;
        const size_type rank_begin;
        const size_type rank_end;

        btree* const tree;
        const padded_leaf_list& leaves;

    public:

        inline TreeCreationTask(inner_node_data& _slot, const width_type _old_slotuse, const bool _reuse_node, const level_type _level, const size_type _rank_begin, const size_type _rank_end, const padded_leaf_list& _leaves, btree* const _tree) 
            : slot(_slot), old_slotuse(_old_slotuse), reuse_node(_reuse_node), level(_level), rank_begin(_rank_begin), rank_end(_rank_end), tree(_tree), leaves(_leaves)
        { }

        tbb::task* execute() {
            if (level <= TREE_PAR_CUTOFF_LEVEL) {
                tree->create_subtree_from_leaves(slot, old_slotuse, reuse_node, level, rank_begin, rank_end, leaves);
                return NULL;
            } else {
                const size_type designated_treesize = designated_subtreesize(level);
                const width_type subtrees = num_subtrees(rank_end - rank_begin, designated_treesize);

                BTREE_PRINT((reuse_node ? "Filling" : "Creating") << " inner node on level " << level << " with " << subtrees << " subtrees of desiganted size " 
                    << designated_treesize << std::endl);

                const width_type new_slotuse = subtrees+old_slotuse;

                inner_node* result = NULL;
                if (reuse_node) {
                    result = static_cast<inner_node*>(slot.childid);
                } else {
                    result = tree->allocate_inner(level);
                    result->slotuse = new_slotuse;
                    slot.childid = result;
                }
                BTREE_ASSERT(new_slotuse <= innerslotmax);
                tbb::task_list tasks;

                auto& c = *new(allocate_continuation()) NodeFinalizerContinuationTask(slot, tree);
                c.set_ref_count(subtrees);

                size_type rank = rank_begin;
                for (width_type i = old_slotuse; i < new_slotuse-1; ++i) {
                    result->slot[i].weight = designated_treesize;
                    tasks.push_back(*new(c.allocate_child()) TreeCreationTask(
                        result->slot[i], 0, false, level-1, rank, rank+designated_treesize, leaves, tree));
                    rank += designated_treesize;
                }
                // Use scheduler bypass for the last task
                result->slot[new_slotuse-1].weight = rank_end - rank;
                auto& task = *new(c.allocate_child()) TreeCreationTask(
                        result->slot[new_slotuse-1], 0, false, level-1, rank, rank_end, leaves, tree);
                spawn(tasks);
                return &task;
            }
        }
    };

    class NodeFinalizerContinuationTask: public tbb::task {
        inner_node_data& slot;
        const btree* const tree;

    public:

        NodeFinalizerContinuationTask(inner_node_data& _slot, const btree* const _tree)
            : slot(_slot), tree(_tree)
        { }

        tbb::task* execute() {
            inner_node* const inner = (inner_node*) slot.childid;
            set_min_element(slot, inner);
            tree->update_router(slot.slotkey, inner->slot[inner->slotuse-1].slotkey);
            return NULL;
        }
    };

    /**
     * Task that applys the given updates to the given nodes by writing the updated tree to the newly created leaves.
     */
    class TreeRewriteTask: public tbb::task {

        node* const source_node;
        const size_type rank;
        const UpdateDescriptor upd;
        padded_leaf_list& leaves;
        btree* const tree;

    public:

        TreeRewriteTask(node* const _source_node, const size_type _rank, const UpdateDescriptor _upd, padded_leaf_list& _leaves, btree* const _tree)
            : source_node(_source_node), rank(_rank), upd(_upd), leaves(_leaves), tree(_tree)
        {}

        tbb::task* execute() {
           BTREE_PRINT("Rewriting tree " << source_node << " on level " << source_node->level << " while applying updates [" << upd.upd_begin << ", " << upd.upd_end << ")" << std::endl);

            if (upd.weight == 0) {
                BTREE_PRINT("Deleting subtree " << source_node << std::endl);
                if (source_node->level <= TREE_PAR_CUTOFF_LEVEL) {
                    tree->clear_recursive(source_node);
                    return NULL;
                } else {
                    auto& c = *new(allocate_continuation()) tbb::empty_task(); // our parent will wait for this one
                    c.set_ref_count(1);
                    return new(c.allocate_child()) TreeDeletionTask(source_node, tree);
                }
            } else if (upd.upd_end - upd.upd_begin < tree->min_problem_size) {
                tree->rewrite(source_node, rank, upd.upd_begin, upd.upd_end, leaves);
                return NULL;
            } else if (source_node->isleafnode()) {
                tbb::parallel_for(cache_aligned_blocked_range<size_type>(upd.upd_begin, upd.upd_end, tree->min_problem_size),
                    [&, this](const cache_aligned_blocked_range<size_type>& r) {
                        const leaf_node* const leaf = (leaf_node*)(source_node);
                        if (r.begin() == upd.upd_begin) {
                            tree->write_updated_leaf_to_new_tree(source_node, /*start index*/0, rank, r.begin(), r.end(), leaves, r.end() == upd.upd_end);
                        } else {
                            const signed long delta = tree->getWeightDelta(upd.upd_begin, r.begin());
                            const width_type key_index = this->find_index_of_lower_key(leaf, tree->updates[r.begin()].data);
                            const size_type corrected_rank = rank + key_index + delta;
                            tree->write_updated_leaf_to_new_tree(source_node, key_index, corrected_rank, r.begin(), r.end(), leaves, r.end() == upd.upd_end);
                        }
                    }
                );
                tree->free_node(source_node);
                return NULL;
            } else {
                const inner_node* const inner = (inner_node*) source_node;
                auto& subtree_updates = tree->tls_data.local().subtree_updates_per_level[MAX_TREE_LEVEL-1];
                
                // Distribute operations and find out which subtrees need rebalancing
                const width_type last = inner->slotuse-1;
                size_type subupd_begin = upd.upd_begin;
                for (width_type i = 0; i < last; ++i) {
                    size_type subupd_end = tree->find_lower(subupd_begin, upd.upd_end, inner->slot[i].slotkey);

                    tree->scheduleSubTreeUpdate(i, inner->slot[i].weight, 0, 0, subupd_begin, subupd_end, subtree_updates);
                    subupd_begin = subupd_end;
                } 
                tree->scheduleSubTreeUpdate(last, inner->slot[last].weight, 0, 0, subupd_begin, upd.upd_end, subtree_updates);

                // Push updates to subtrees and rewrite them in parallel
                tbb::task_list tasks;

                auto& c = *new(allocate_continuation()) tbb::empty_task(); // our parent will wait for this one

                size_type subtree_rank = rank;
                for (width_type i = 0; i < inner->slotuse; ++i) {
                    tasks.push_back(*new(c.allocate_child()) TreeRewriteTask(inner->slot[i].childid, subtree_rank, subtree_updates[i], leaves, tree));
                    subtree_rank += subtree_updates[i].weight;
                }
                c.set_ref_count(inner->slotuse);
                auto& task = tasks.pop_front(); // use scheduler bypass for one task
                spawn(tasks);
                tree->free_node(source_node);
                return &task;
            }
        }

    private:

        inline int find_index_of_lower_key(const leaf_node* const leaf, const key_type& key) const {
            int lo = 0;
            int hi = leaf->slotuse;

            while (lo < hi) {
                int mid = (lo + hi) >> 1;

                if (tree->key_lessequal(key, leaf->slotkey[mid])) {
                    hi = mid;
                } else {
                    lo = mid + 1;
                }
            }
            return hi;
        }
    };


    class TreeDeletionTask: public tbb::task {

        node* to_delete;
        btree* const tree;

    public:
        inline TreeDeletionTask(node* _to_delete, btree* const _tree)
            : to_delete(_to_delete), tree(_tree)
        {}

        tbb::task* execute() {
            if (to_delete->level <= TREE_PAR_CUTOFF_LEVEL) {
                tree->clear_recursive(to_delete);
                return NULL;
            } else {
                tbb::task_list tasks;
                auto& c = *new(allocate_continuation()) tbb::empty_task(); // our parent will wait for this one

                inner_node* const inner = static_cast<inner_node*>(to_delete);
                for (width_type i = 0; i < inner->slotuse; ++i) {
                    tasks.push_back(*new(c.allocate_child()) TreeDeletionTask(inner->slot[i].childid, tree));
                }
                c.set_ref_count(inner->slotuse);
                tree->free_node(inner);
                auto& task = tasks.pop_front();
                spawn(tasks);
                return &task;
            }
        }
    };


    /**
     * Task that applys the given updates to the given nodes. Will restructure subtrees if needed. 
     */
    class TreeUpdateTask: public tbb::task {

        inner_node_data& slot;
        const size_type upd_begin;
        const size_type upd_end;
        btree* const tree;
        tbb::task::affinity_id* affinity;

    public:
        inline TreeUpdateTask(inner_node_data& _slot, const size_type _upd_begin, const size_type _upd_end, btree* const _tree)
            : slot(_slot), upd_begin(_upd_begin), upd_end(_upd_end), tree(_tree), affinity(NULL)
        {}

        inline TreeUpdateTask(inner_node_data& _slot, const size_type _upd_begin, const size_type _upd_end, btree* const _tree, tbb::task::affinity_id* _affinity)
            : slot(_slot), upd_begin(_upd_begin), upd_end(_upd_end), tree(_tree), affinity(_affinity)
        {
            set_affinity(*affinity);
        }

        void note_affinity(tbb::task::affinity_id id) {
            if (affinity != NULL && *affinity != id) {
                *affinity = id;
            }
        }

        tbb::task* execute() {
            BTREE_PRINT("Applying updates [" << upd_begin << ", " << upd_end << ") to " << slot.childid   << " on level " << slot.childid->level << std::endl);
            BTREE_ASSERT(upd_begin != upd_end);

            if (upd_end - upd_begin < tree->min_problem_size || slot.childid->isleafnode()) {
                tree->update(slot, upd_begin, upd_end);
                return NULL;
            } else {
                inner_node* const inner = static_cast<inner_node*>(slot.childid);
                const size_type max_weight = maxweight(inner->level-1);
                const size_type min_weight = max_weight / 4;
                auto& subtree_updates = tree->tls_data.local().subtree_updates_per_level[MAX_TREE_LEVEL-1];
                bool rebalancing_needed = false;

                // Distribute operations and find out which subtrees need rebalancing
                const width_type last = inner->slotuse-1;
                size_type subupd_begin = upd_begin;
                for (width_type i = 0; i < last; ++i) {
                    size_type subupd_end = tree->find_lower(subupd_begin, upd_end, inner->slot[i].slotkey);
                    rebalancing_needed |= tree->scheduleSubTreeUpdate(i, inner->slot[i].weight, min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                    subupd_begin = subupd_end;
                } 
                rebalancing_needed |= tree->scheduleSubTreeUpdate(last, inner->slot[last].weight, min_weight, max_weight, subupd_begin, upd_end, subtree_updates);

                tbb::task_list tasks;
                width_type task_count = 0;
                auto& c = *new(allocate_continuation()) NodeFinalizerContinuationTask(slot, tree);

                if (!rebalancing_needed) {
                    // No rebalancing needed at all (this is the common case). Push updates to subtrees to update them parallel
                    for (width_type i = 0; i < inner->slotuse; ++i) {
                        if (hasUpdates(subtree_updates[i])) {
                            tasks.push_back(*new(c.allocate_child()) TreeUpdateTask(inner->slot[i], subtree_updates[i].upd_begin, subtree_updates[i].upd_end, tree));
                            ++task_count;                            
                            inner->slot[i].weight = subtree_updates[i].weight;
                        }
                    }
                    
                } else {
                    BTREE_PRINT("Rewrite session started for " << inner << " on level " << inner->level << std::endl);

                    // Need to perform rebalancing.
                    const size_type designated_treesize = designated_subtreesize(inner->level);

                    auto& spare_inner = tree->tls_data.local().spare_inner;
                    inner_node* const result = spare_inner != NULL ? spare_inner : tree->allocate_inner_without_count(0);
                    result->initialize(inner->level);
                    width_type in = 0; // current slot in input tree
                    width_type out = 0;

                    while (in < inner->slotuse) {
                        width_type rebalancing_range_start = in;
                        size_type weight_of_defective_range = 0;
                        bool openrebalancing_region = false;

                        // Find non-empty consecutive run of subtrees that need rebalancing
                        while (in < inner->slotuse && (subtree_updates[in].rebalancing_needed 
                                || (openrebalancing_region && weight_of_defective_range != 0 && weight_of_defective_range < designated_treesize))) {
                            openrebalancing_region = true;
                            weight_of_defective_range += subtree_updates[in].weight;
                            ++in;
                        }
                        if (openrebalancing_region) {
                            BTREE_PRINT("Rewrite session started on level " << inner->level << " of " << tree->height() << " for subtrees [" << rebalancing_range_start << "," << in <<") of total weight " << weight_of_defective_range << std::endl);
                           
                            node* result_as_node = static_cast<node*>(result);
                            TreeRootCreationTask& task = *new(c.allocate_child()) TreeRootCreationTask(result_as_node, out, inner->level, weight_of_defective_range, tree);

                            size_type subtree_rank = 0;
                            for (width_type i = rebalancing_range_start; i < in; ++i) {
                                task.subtasks.push_back(*new(task.allocate_child()) TreeRewriteTask(inner->slot[i].childid, subtree_rank, subtree_updates[i], task.leaves, tree));
                                subtree_rank += subtree_updates[i].weight;
                            }
                            task.subtask_count = in - rebalancing_range_start;
                            out += task.subtrees;
                            tasks.push_back(task);
                            ++task_count;
                        } else {
                            BTREE_PRINT("Copying " << inner->slot[in].childid << " from " << in << " to " << out << " in " << result << std::endl);

                            result->slot[out] = inner->slot[in];
                            result->slot[out].weight = subtree_updates[in].weight;
                            if (hasUpdates(subtree_updates[in])) {
                                tasks.push_back(*new(c.allocate_child()) TreeUpdateTask(result->slot[out], subtree_updates[in].upd_begin, subtree_updates[in].upd_end, tree));
                                ++task_count;
                            }
                            ++out;
                            ++in;
                        }
                    }
                    result->slotuse = out;
                    spare_inner = static_cast<inner_node*>(slot.childid);
                    slot.childid = result;
                }
                c.set_ref_count(task_count);
                auto& task = tasks.pop_front();
                spawn(tasks);
                return &task;
            }
        }
    };

};


#endif // _PARALLEL_BTREE_H_
