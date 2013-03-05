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

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <cstddef>
#include <assert.h>
#include <array>
#include <unistd.h>
#include <cmath>
#include <string.h>
#include "../utility/datastructure/NullData.hpp"
#include "../options.hpp"

#include "../tbb/cache_aligned_blocked_range.hpp"

#include "tbb/parallel_scan.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/partitioner.h"
#include "tbb/atomic.h"
#include "tbb/task.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/scalable_allocator.h"


// *** Debugging Macros
#ifdef BTREE_DEBUG
#define BTREE_PRINT(x)          do { (std::cout << x); } while(0)
#else
#define BTREE_PRINT(x)          do { } while(0)
#endif

#ifndef NDEBUG
#define BTREE_ASSERT(x)         do { assert(x); } while(0)
#else
#define BTREE_ASSERT(x)         do { } while(0)
#endif

/// The maximum of a and b. Used in some compile-time formulas.
#define BTREE_MAX(a,b)          ((a) < (b) ? (b) : (a))

// Width of nodes given as number of cache-lines
#ifndef INNER_NODE_WIDTH
#define INNER_NODE_WIDTH 7
#endif
#ifndef LEAF_NODE_WIDTH
#define LEAF_NODE_WIDTH 64
#endif


template<typename data_type>
struct Operation {
    enum OpType {INSERT=1, DELETE=-1};
    OpType type;
    data_type data;
    Operation(const OpType& x, const data_type& y) : type(x), data(y) {}
    Operation() {}

};

template <typename _Key, typename _MinKey>
struct btree_default_traits {
private:
    struct slot {
        _Key        slotkey;
        size_t      weight;
        void*       childid;
    #ifdef COMPUTE_PARETO_MIN
        _MinKey     minimum;
    #endif
    };
public:
    /// If true, the tree will self verify it's invariants after each insert()
    /// or erase(). The header must have been compiled with BTREE_DEBUG defined.
    static const bool   selfverify = false;

    /// Configure nodes to have a fixed size of X cache lines. 
    static const int    leafparameter_k = BTREE_MAX( 4, (LEAF_NODE_WIDTH * DCACHE_LINESIZE - 2*sizeof(unsigned short)) / (sizeof(_Key)) );
    static const int    branchingparameter_b = BTREE_MAX( 4, ((INNER_NODE_WIDTH * DCACHE_LINESIZE - 2*sizeof(unsigned short)) / sizeof(slot))/4 );
};

// Number of leaves that need to be written before we try perform it in parallel
#ifndef REWRITE_THRESHOLD
#define REWRITE_THRESHOLD 8
#endif

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
class btree {
    friend class TreeCreationTask;
    friend class TreeRewriteTask;
    friend class TreeUpdateTask;
private:
    btree(const btree &){}         // do not copy
    void operator=(const btree&){} // do not copy
public:
    /// The key type of the B+ tree. This is stored  in inner nodes and leaves
    typedef _Key                        key_type;
    
    /// Key comparison function object
    typedef _Compare                    key_compare;
    
    /// Traits object used to define more parameters of the B+ tree
    typedef _Traits                     traits;

    // Key data required to find pareto minima (subset of key_type)
#ifdef COMPUTE_PARETO_MIN
    typedef _MinKey                     min_key_type;
#else 
    typedef utility::NullData           min_key_type;
#endif

    /// STL allocator for tree nodes
    typedef _Alloc                      allocator_type;

    typedef size_t                      size_type;
    typedef unsigned short              level_type;
    typedef unsigned short              width_type;

public:
    // *** Static Constant Options and Values of the B+ Tree

    /// The number of key/data slots in each leaf
    static const width_type         leafslotmax =  traits::leafparameter_k;
    static const width_type         leafslotmin =  traits::leafparameter_k / 4;

    // The number of keys per leaf in a perfectly re-balanced tree
    static const width_type         designated_leafsize = (leafslotmax + leafslotmin) / 2;

    /// The number of key slots in each inner node,
    static const width_type         innerslotmax =  traits::branchingparameter_b * 4;
    static const width_type         innerslotmin =  traits::branchingparameter_b / 4;

protected:
    // *** Node Classes for In-Memory Nodes

    struct node {
        /// Level in the b-tree, if level == 0 -> leaf node
        level_type level;

        width_type slotuse;

        /// Delayed initialisation of constructed node
        inline void initialize(const level_type l) {
            level = l;
            slotuse = 0;
        }

        inline bool isleafnode() const {
            return (level == 0);
        }

    };

    struct inner_node_data {
        /// Heighest key in the subtree with the same slot index
        key_type        slotkey;
        /// Weight (total number of keys) of the subtree 
        size_type       weight;
        /// Pointers to children
        node*           childid;
    #ifdef COMPUTE_PARETO_MIN
        /// Heighest key in the subtree with the same slot index
        min_key_type    minimum; 
    #endif
    };

    struct inner_node : public node {
        /// Define an related allocator for the inner_node structs.
        typedef typename _Alloc::template rebind<inner_node>::other alloc_type;

        inner_node_data slot[innerslotmax];

        /// Set variables to initial values
        inline void initialize(const level_type l) {
            node::initialize(l);
        }
    };

    struct leaf_node : public node {
        /// Define an related allocator for the leaf_node structs.
        typedef typename _Alloc::template rebind<leaf_node>::other alloc_type;

        /// Keys of children or data pointers
        key_type        slotkey[leafslotmax];

        /// Set variables to initial values
        inline void initialize() {
            node::initialize(0);
        }
    };

    static size_type minweight(const level_type level) {
        return ipow(traits::branchingparameter_b, level) * traits::leafparameter_k / 4;
    }

    static size_type maxweight(const level_type level) {
        return ipow(traits::branchingparameter_b, level) * traits::leafparameter_k;
    }

    static inline size_type ipow(int base, int exp) {
        size_type result = 1;
        while (exp > 0) {
            if (exp & 1) {
              result *= base;
            }
            exp >>= 1;
            base *= base;
        }
        return result;
    }

    static const size_type innernodebytesize = sizeof(inner_node);
    static const size_type leafnodebytesize  = sizeof(leaf_node);

public:

    /** A small struct containing basic statistics about the B+ tree. It can be
     * fetched using get_stats(). */
    struct tree_stats {
        /// Number of items in the B+ tree
        size_type                    itemcount;

        /// Number of leaves in the B+ tree
        tbb::atomic<size_type>       leaves;

        /// Number of inner nodes in the B+ tree
        tbb::atomic<size_type>       innernodes;

        /// Return the total number of nodes
        inline size_type nodes() const {
            return innernodes + leaves;
        }

        /// Return the average fill of leaves
        inline double avgfill_leaves() const {
            return static_cast<double>(itemcount) / (leaves * leafslotmax);
        }

        inline tree_stats()
            : itemcount(0), leaves(), innernodes()
        { }

#ifdef NDEBUG
        static const bool gather_stats = false;
#else 
        static const bool gather_stats = true;
#endif
    };

protected:
    // *** Tree Object Data Members

    /// Pointer to the B+ tree's root node, either leaf or inner node
    node*       root;

    /// Pointer to spare leaf used for merging.
    typedef tbb::enumerable_thread_specific<leaf_node*, tbb::cache_aligned_allocator<leaf_node*>, tbb::ets_key_per_instance> SpareLeafPerThread; 
    SpareLeafPerThread spare_leaves;

    /// Other small statistics about the B+ tree
    tree_stats  stats;

    /// Key comparison object. More comparison functions are generated from
    /// this < relation.
    key_compare key_less;

    /// Memory allocator.
    allocator_type allocator;

    // Currently running updates
    const Operation<key_type>* updates;

    // Weight delta of currently running updates
    typedef typename _Alloc::template rebind<signed long>::other weightdelta_listalloc_type;
    typedef std::vector<signed long, weightdelta_listalloc_type> weightdelta_list;
    weightdelta_list weightdelta;

    struct UpdateDescriptor {
        bool rebalancing_needed;
        size_type weight;
        size_type upd_begin;
        size_type upd_end;
    };

    UpdateDescriptor root_subtree_updates[innerslotmax];
    tbb::task_list root_tasks;

    struct PaddedAtomic : public tbb::atomic<leaf_node*> {
        // Not used. Missing cache locality seems worse than false sharing!
        //char pad[DCACHE_LINESIZE - sizeof(tbb::atomic<leaf_node*>) % DCACHE_LINESIZE];
    };
    typedef typename _Alloc::template rebind<PaddedAtomic>::other leaf_listalloc_type;
    typedef std::vector<PaddedAtomic, leaf_listalloc_type> leaf_list;


public:
    // *** Constructors and Destructor

    /// Default constructor initializing an empty B+ tree with the standard key
    /// comparison function
    explicit inline btree(const allocator_type &alloc=allocator_type())
        : root(NULL), allocator(alloc)
    {
        weightdelta.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
    }

    /// Frees up all used B+ tree memory pages
    inline ~btree() {
        clear();
        for (auto leaf : spare_leaves) {
            free_node(leaf);
        }
    }

private:
    // *** Convenient Key Comparison Functions Generated From key_less

    /// True if a <= b
    inline bool key_lessequal(const key_type &a, const key_type &b) const {
        return !key_less(b, a);
    }

    /// True if a >= b
    inline bool key_greaterequal(const key_type &a, const key_type &b) const {
        return !key_less(a, b);
    }

    /// True if a == b. This requires the < relation to be a total order,
    /// otherwise the B+ tree cannot be sorted.
    inline bool key_equal(const key_type &a, const key_type &b) const {
        return !key_less(a, b) && !key_less(b, a);
    }

private:
    // *** Node Object Allocation and Deallocation Functions

    inline typename leaf_node::alloc_type leaf_node_allocator() {
        return typename leaf_node::alloc_type(allocator);
    }

    inline typename inner_node::alloc_type inner_node_allocator() {
        return typename inner_node::alloc_type(allocator);
    }

    inline leaf_node* allocate_leaf() {
        leaf_node *n = new (leaf_node_allocator().allocate(1)) leaf_node();
        n->initialize();
        if (stats.gather_stats) stats.leaves.fetch_and_increment();
        return n;
    }

    inline leaf_node* allocate_leaf_without_count() {
        leaf_node *n = new (leaf_node_allocator().allocate(1)) leaf_node();
        n->initialize();
        return n;
    }

    inline inner_node* allocate_inner(level_type level) {
        inner_node *n = new (inner_node_allocator().allocate(1)) inner_node();
        n->initialize(level);
        if (stats.gather_stats) stats.innernodes.fetch_and_increment();
        return n;
    }

    inline void free_node(node *n) {
        if (n->isleafnode()) {
            leaf_node *ln = static_cast<leaf_node*>(n);
            typename leaf_node::alloc_type a(leaf_node_allocator());
            a.destroy(ln);
            a.deallocate(ln, 1);
            if (stats.gather_stats) stats.leaves.fetch_and_decrement();
        } else {
            inner_node *in = static_cast<inner_node*>(n);
            typename inner_node::alloc_type a(inner_node_allocator());
            a.destroy(in);
            a.deallocate(in, 1);
            if (stats.gather_stats) stats.innernodes.fetch_and_decrement();
        }
    }

public:
    // *** Fast Destruction of the B+ Tree

    inline void clear() {
        if (root) {
            clear_recursive(root);
            root = NULL;
        }
        BTREE_ASSERT(stats.innernodes == 0);
        BTREE_ASSERT(stats.leaves == 0);
    }

private:
    /// Recursively free up nodes
    void clear_recursive(node* n) {
        if (!n->isleafnode()) {
            inner_node *innernode = static_cast<inner_node*>(n);

            for (width_type slot = 0; slot < innernode->slotuse; ++slot) {
                clear_recursive(innernode->slot[slot].childid);
            }
        }
        free_node(n);
    }

public:
    // *** Access Functions to the Item Count

    inline size_type size() const {
        return stats.itemcount;
    }

    inline bool empty() const {
        return (size() == size_type(0));
    }

    inline level_type height() const {
        if (root == NULL) {
            return 0; 
        } else {
            return root->level;
        }
    }

    /// Return a const reference to the current statistics.
    inline const struct tree_stats& get_stats() const {
        return stats;
    }

    static std::string name() {
        return "Parallel BTree";
    }

public:

    template<typename T>
    void apply_updates(const T& _updates) {
        auto ap = tbb::auto_partitioner();
        apply_updates(_updates.data(), _updates.size(), ap);
    }

    template<typename T, typename Partitioner>
    void apply_updates(const T* _updates, const size_t update_count, Partitioner& partitioner) {
        const size_type new_size = setOperationsAndComputeWeightDelta(_updates, update_count, partitioner);
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
            node* new_root = NULL; 
            UpdateDescriptor upd = {rebuild_needed, new_size, 0, update_count};
            TreeRootCreationTask& task = *new(tbb::task::allocate_root()) TreeRootCreationTask(new_root, 0, level, new_size, this);
            task.subtasks.push_back(*new(task.allocate_child()) TreeRewriteTask(root, 0, upd, task.leaves, this));
            task.subtask_count = 1;
            tbb::task::spawn_root_and_wait(task);
            root = new_root;
        } else {
            inner_node_data fake_slot;
            fake_slot.childid = root;

            if (root->isleafnode()) {
                TreeUpdateTask& task = *new(tbb::task::allocate_root()) TreeUpdateTask(fake_slot, 0, update_count, this);
                tbb::task::spawn_root_and_wait(task);
            } else {
                // The common case (and thus partially inlined here)
                inner_node* const inner = static_cast<inner_node*>(root);
                const size_type min_weight = minweight(level-1);
                const size_type max_weight = maxweight(level-1);
                bool rebalancing_needed = false;

                // Distribute operations and find out which subtrees need rebalancing
                const width_type last = inner->slotuse-1;
                size_type subupd_begin = 0;
                for (width_type i = 0; i < last; ++i) {
                    size_type subupd_end = find_lower(subupd_begin, update_count, inner->slot[i].slotkey);
                    rebalancing_needed |= scheduleSubTreeUpdate(i, inner->slot[i].weight, min_weight, max_weight, subupd_begin, subupd_end, root_subtree_updates);
                    subupd_begin = subupd_end;
                } 
                rebalancing_needed |= scheduleSubTreeUpdate(last, inner->slot[last].weight, min_weight, max_weight, subupd_begin, update_count, root_subtree_updates);

                if (!rebalancing_needed) {
                    // No rebalancing needed at all (this is the common case). Push updates to subtrees to update them parallel
                    for (width_type i = 0; i < inner->slotuse; ++i) {
                        if (hasUpdates(root_subtree_updates[i])) {
                            root_tasks.push_back(*new(tbb::task::allocate_root()) TreeUpdateTask(inner->slot[i], root_subtree_updates[i].upd_begin, root_subtree_updates[i].upd_end, this));
                            inner->slot[i].weight = root_subtree_updates[i].weight;
                        }
                    }
                    tbb::task::spawn_root_and_wait(root_tasks);
                    root_tasks.clear();
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

    template<typename sequence_type>
    void find_pareto_minima(const min_key_type& prefix_minima, sequence_type& minima) const {
        BTREE_ASSERT(minima.empty());
        find_pareto_minima(root, prefix_minima, minima);
    }

protected:

#ifdef COMPUTE_PARETO_MIN

    template<typename sequence_type>
    inline void find_pareto_minima(const node* const node, const min_key_type& prefix_minima, sequence_type& minima) const {
        if (node->isleafnode()) {
            const leaf_node* const leaf = (leaf_node*) node;
            const width_type slotuse = leaf->slotuse;

            const min_key_type* min = &prefix_minima;
            for (width_type i = 0; i<slotuse; ++i) {
                if (leaf->slotkey[i].second_weight < min->second_weight ||
                        (leaf->slotkey[i].first_weight == min->first_weight && leaf->slotkey[i].second_weight == min->second_weight)) {
                    minima.push_back({Operation<key_type>::DELETE, leaf->slotkey[i]});
                    min = &leaf->slotkey[i];
                }
            }
        } else {
            const inner_node* const inner = (inner_node*) node;
            const width_type slotuse = inner->slotuse;

            const min_key_type* min = &prefix_minima;
            for (width_type i = 0; i<slotuse; ++i) {
                if (inner->slot[i].minimum.second_weight < min->second_weight ||
                        (inner->slot[i].minimum.first_weight == min->first_weight && inner->slot[i].minimum.second_weight == min->second_weight)) {
                    find_pareto_minima(inner->slot[i].childid, *min, minima);
                    min = &inner->slot[i].minimum;
                }
            }
        }
    }

    static inline void set_min_element(inner_node_data& slot, const leaf_node* const node) {
        slot.minimum = *std::min_element(node->slotkey, node->slotkey+node->slotuse,
            [](const min_key_type& i, const min_key_type& j) { return i.second_weight < j.second_weight; });
    }

    static inline void set_min_element(inner_node_data& slot, const min_key_type& local) {
        slot.minimum = local;
    }

    static inline void set_min_element(inner_node_data& slot, const inner_node* const node) {
        slot.minimum = std::min_element(node->slot, node->slot+node->slotuse,
            [](const inner_node_data& i, const inner_node_data& j) { return i.minimum.second_weight < j.minimum.second_weight; })->minimum;
    }
#else 
    void find_pareto_minima(const node* const, const min_key_type&, std::vector<key_type>&) const {
        std::cout << "Pareto Min Feature disabled" << std::endl;
    }
    static inline void set_min_element(inner_node_data&, const leaf_node* const) {
    }
    static inline void set_min_element(inner_node_data&, const min_key_type&) {
    }
#endif


private:

    template<typename TIn, typename TOut>
    class PrefixSum {
        TOut sum;
        TOut* const out;
        const TIn* const in;
        const signed char all_ops_identical;
    public:
        inline PrefixSum(TOut* _out, const TIn* _in, const signed char _all_ops_identical)
            : sum(0), out(_out), in(_in), all_ops_identical(_all_ops_identical) {}

        inline PrefixSum(const PrefixSum& b, tbb::split)
            : sum(0), out(b.out), in(b.in), all_ops_identical(b.all_ops_identical) {}

        template<typename Tag>
        void operator() (const cache_aligned_blocked_range<size_type>& r, Tag) {

            if (!all_ops_identical) {
                // Mixed insertions and deletions. Need to perform full prefix sum.
                TOut temp = sum;
                if (Tag::is_final_scan()) {
                    for(size_type i=r.begin(); i<r.end(); ++i) {
                        temp += in[i].type;
                        out[i] = temp;
                    }
                } else {
                    for(size_type i=r.begin(); i<r.end(); ++i) {
                        temp += in[i].type;
                    }
                }
                sum = temp;                
            } else {
                // All operations are either deletes or all are inserts
                if (Tag::is_final_scan()) {
                    for(size_type i=r.begin(); i<r.end(); ++i) {
                        out[i] = (i+1) * all_ops_identical;
                    }
                }
                sum += all_ops_identical * (r.end() - r.begin());  
            }
        }
        void reverse_join(const PrefixSum& a) {sum = a.sum + sum;}
        void assign(const PrefixSum& b) {sum = b.sum;}
        TOut get_sum() const { return sum; }
    };
    
    template<typename T, typename Partitioner>
    inline size_type setOperationsAndComputeWeightDelta(const T* _updates, const size_t update_count, Partitioner& partitioner) {
        updates = _updates;

        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)
        weightdelta[0] = 0;
        // If the tree is empty, then the updates can only contain insertions.
        const signed char all_ops_identical = size() == 0; 

        PrefixSum<Operation<key_type>, signed long> body(weightdelta.data()+1, _updates, all_ops_identical);
        parallel_scan(cache_aligned_blocked_range<size_type>(0, update_count, traits::leafparameter_k), body, partitioner);

        return size() + body.get_sum();
    }

    template<typename Range, typename Body, typename Partitioner>
    void parallel_scan( const Range& range, Body& body, Partitioner& partitioner ) {
        tbb::internal::start_scan<Range,Body,Partitioner>::run(range,body, partitioner);
    }

    class TreeRootCreationTask : public tbb::task {
        node*& out_node;
        const width_type old_slotuse;
        const level_type level;
        const size_type size;
        btree* const tree;

    public:

        tbb::task_list subtasks;
        width_type subtask_count;

        leaf_list leaves;
        const width_type subtrees;


        inline TreeRootCreationTask(node*& _out_node, const width_type _old_slotuse, const level_type _level, const size_type _size, btree* const _tree) 
            : out_node(_out_node), old_slotuse(_old_slotuse), level(_level), size(_size), tree(_tree), subtrees(num_subtrees(_size, designated_subtreesize(_level)))
        { }

        tbb::task* execute() {
            const size_type subtrees = num_subtrees(size, designated_leafsize);
            leaves.reserve(next_power_of_two(subtrees)); // use next power of two to allow efficient caching by the allocator
            leaves.resize(subtrees); // but use actual size

            // Fill leaves via TreeRewriteTask
            set_ref_count(subtask_count+1);
            spawn_and_wait_for_all(subtasks);

            if (size > 0) {
                // Reconstruct new tree from the filled leaves
                inner_node_data fake_slot;
                fake_slot.childid = out_node;

                TreeCreationTask& task = *new(allocate_child()) TreeCreationTask(fake_slot, old_slotuse, out_node != NULL, level, 0, size, leaves, tree);
                set_ref_count(2);
                spawn_and_wait_for_all(task); // wait to keep the leaves data structure alive
                out_node = fake_slot.childid;

            }
            return NULL;
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
    };


    class TreeCreationTask: public tbb::task {
        inner_node_data& slot;
        const width_type old_slotuse;
        const bool reuse_node;

        const level_type level;
        const size_type rank_begin;
        const size_type rank_end;

        btree* const tree;
        const leaf_list& leaves;

    public:


        inline TreeCreationTask(inner_node_data& _slot, const width_type _old_slotuse, const bool _reuse_node, const level_type _level, const size_type _rank_begin, const size_type _rank_end, const leaf_list& _leaves, btree* const _tree) 
            : slot(_slot), old_slotuse(_old_slotuse), reuse_node(_reuse_node), level(_level), rank_begin(_rank_begin), rank_end(_rank_end), tree(_tree), leaves(_leaves)
        { }

        tbb::task* execute() {
            if (level == 0) {
                // Just re-use the pre-alloced and filled leaf
                leaf_node* result = leaves[rank_begin / designated_leafsize];
                result->slotuse = rank_end - rank_begin;
                set_min_element(slot, result);
                tree->update_router(slot.slotkey, result->slotkey[result->slotuse-1]);
                slot.childid = result;
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
        leaf_list& leaves;
        btree* const tree;

    public:

        TreeRewriteTask(node* const _source_node, const size_type _rank, const UpdateDescriptor _upd, leaf_list& _leaves, btree* const _tree)
            : source_node(_source_node), rank(_rank), upd(_upd), leaves(_leaves), tree(_tree)
        {}

        tbb::task* execute() {
           BTREE_PRINT("Rewriting tree " << source_node << " on level " << source_node->level << " while applying updates [" << upd.upd_begin << ", " << upd.upd_end << ")" << std::endl);

            if (upd.weight == 0) {
                BTREE_PRINT("Deleting subtree " << source_node << std::endl);
                tree->clear_recursive(source_node);
                return NULL; 
            } else if (source_node->isleafnode()) {
                if (resultTreeIsSmall()) {
                    write_updated_leaf_to_new_tree(/*start index*/0, rank, upd.upd_begin, upd.upd_end);
                } else {
                    const leaf_node* const leaf = (leaf_node*)(source_node);

                    tbb::parallel_for(cache_aligned_blocked_range<size_type>(upd.upd_begin, upd.upd_end, traits::leafparameter_k),
                        [&, this](const cache_aligned_blocked_range<size_type>& r) {
                            
                            if (r.begin() == upd.upd_begin) {
                                this->write_updated_leaf_to_new_tree(/*start index*/0, rank, r.begin(), r.end());
                            } else {
                                const signed long delta = tree->weightdelta[r.begin()] - tree->weightdelta[upd.upd_begin];
                                const width_type key_index = this->find_index_of_lower_key(leaf, tree->updates[r.begin()].data);
                                const size_type corrected_rank = rank + key_index + delta;
                                this->write_updated_leaf_to_new_tree(key_index, corrected_rank, r.begin(), r.end());
                            }
                        }
                    );
                }
                tree->free_node(source_node);
                return NULL;
            } else {
                const inner_node* const inner = (inner_node*) source_node;
                const size_type min_weight = minweight(inner->level-1);
                const size_type max_weight = maxweight(inner->level-1);
                UpdateDescriptor subtree_updates[inner->slotuse];
                
                // Distribute operations and find out which subtrees need rebalancing
                const width_type last = inner->slotuse-1;
                size_type subupd_begin = upd.upd_begin;
                for (width_type i = 0; i < last; ++i) {
                    size_type subupd_end = tree->find_lower(subupd_begin, upd.upd_end, inner->slot[i].slotkey);

                    tree->scheduleSubTreeUpdate(i, inner->slot[i].weight, min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                    subupd_begin = subupd_end;
                } 
                tree->scheduleSubTreeUpdate(last, inner->slot[last].weight, min_weight, max_weight, subupd_begin, upd.upd_end, subtree_updates);

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

        inline bool resultTreeIsSmall() const {
            return tree->weightdelta[upd.upd_end] - tree->weightdelta[upd.upd_begin] < designated_leafsize * REWRITE_THRESHOLD;
        }

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

        // in is the existing key where to start reading in the source_node
        void write_updated_leaf_to_new_tree(width_type in, const size_type& rank, const size_type& upd_begin, const size_type& upd_end) {
            BTREE_PRINT("Rewriting updated leaf " << source_node << " starting with rank " << rank << " and in " << in << " using upd range [" << upd_begin << "," << upd_end << ")" << std::endl);

            size_type leaf_number = rank / designated_leafsize;
            width_type offset_in_leaf = rank % designated_leafsize;

            if (leaf_number >= leaves.size()) {
                // elements are squeezed into the previous leaf
                leaf_number = leaves.size()-1; 
                offset_in_leaf = rank - leaf_number*designated_leafsize;
            }
            width_type out = offset_in_leaf; // position where to write

            leaf_node* result = getOrCreateLeaf(leaf_number);
            const leaf_node* const leaf = (leaf_node*)(source_node);
            const width_type in_slotuse = leaf->slotuse;

            for (size_type i = upd_begin; i != upd_end; ++i) {
                switch (tree->updates[i].type) {
                case Operation<key_type>::DELETE:
                    // We know the element is in here, so no bounds checks
                    while (tree->key_less(leaf->slotkey[in], tree->updates[i].data)) {
                        BTREE_ASSERT(in < in_slotuse);
                        result->slotkey[out++] = leaf->slotkey[in++];

                        if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                            result = getOrCreateLeaf(++leaf_number);
                            out = 0;
                        }
                    }
                    ++in; // delete the element by jumping over it
                    break;
                case Operation<key_type>::INSERT:
                    while(in < in_slotuse && tree->key_less(leaf->slotkey[in], tree->updates[i].data)) {
                        result->slotkey[out++] = leaf->slotkey[in++];

                        if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                            result = getOrCreateLeaf(++leaf_number);
                            out = 0;
                        }
                    }
                    result->slotkey[out++] = tree->updates[i].data;

                    if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                        result = getOrCreateLeaf(++leaf_number);
                        out = 0;
                    }
                    break;
                }
            } 
            if (upd_end == upd.upd_end) {
                // Reached the total end of the update range. Have to write the remaining elements
                while (in < in_slotuse) {
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number) && in < in_slotuse) {
                        result = getOrCreateLeaf(++leaf_number);
                        out = 0;
                    }
                }
            }
            BTREE_PRINT("   as range [" << rank << ", " << ((leaf_number-(rank/designated_leafsize))*designated_leafsize)
                + out << ") into " << leaves.size() << " leaves " << std::endl);
        }

        inline bool hasNextLeaf(const size_type leaf_count) const {
            return leaf_count+1 < leaves.size();
        }

        leaf_node* getOrCreateLeaf(size_type leaf_number) {
            if (leaves[leaf_number] == NULL) {

                leaf_node* tmp = tree->allocate_leaf();
                if (leaves[leaf_number].compare_and_swap(tmp, NULL) != NULL)
                    // Another thread installed the value, so throw away mine.
                    tree->free_node(tmp);
            }
            return leaves[leaf_number];
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

    public:
        inline TreeUpdateTask(inner_node_data& _slot, const size_type _upd_begin, const size_type _upd_end, btree* const _tree)
            : slot(_slot), upd_begin(_upd_begin), upd_end(_upd_end), tree(_tree)
        {}

        tbb::task* execute() {
            BTREE_PRINT("Applying updates [" << upd_begin << ", " << upd_end << ") to " << slot.childid   << " on level " << slot.childid->level << std::endl);
            BTREE_ASSERT(upd_begin != upd_end);

            if (slot.childid->isleafnode()) {
                bool exists;
                typename SpareLeafPerThread::reference spare_leaf = tree->spare_leaves.local(exists);
                leaf_node* const result = exists ? spare_leaf : tree->allocate_leaf_without_count();
                update_leaf_in_current_tree(result, static_cast<leaf_node*>(slot.childid));
                spare_leaf = static_cast<leaf_node*>(slot.childid);
                slot.childid = result;
                return NULL;
            } else {
                inner_node* const inner = static_cast<inner_node*>(slot.childid);
                const size_type min_weight = minweight(inner->level-1);
                const size_type max_weight = maxweight(inner->level-1);
                UpdateDescriptor subtree_updates[inner->slotuse];

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

                if (!rebalancing_needed) {
                    auto& c = *new(allocate_continuation()) NodeFinalizerContinuationTask(slot, tree);

                    // No rebalancing needed at all (this is the common case). Push updates to subtrees to update them parallel
                    for (width_type i = 0; i < inner->slotuse; ++i) {
                        if (hasUpdates(subtree_updates[i])) {
                            tasks.push_back(*new(c.allocate_child()) TreeUpdateTask(inner->slot[i], subtree_updates[i].upd_begin, subtree_updates[i].upd_end, tree));
                            ++task_count;                            
                            inner->slot[i].weight = subtree_updates[i].weight;
                        }
                    }
                    c.set_ref_count(task_count);
                    auto& task = tasks.pop_front();
                    spawn(tasks);
                    return &task;
                } else {
                    BTREE_PRINT("Rewrite session started for " << inner << " on level " << inner->level << std::endl);

                    // Need to perform rebalancing.
                    const size_type designated_treesize = designated_subtreesize(inner->level);

                    inner_node* const result = tree->allocate_inner(inner->level);
                    width_type in = 0; // current slot in input tree
                    width_type out = 0;

                    // FIXME: Use continuation here...

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
                            TreeRootCreationTask& task = *new(allocate_child()) TreeRootCreationTask(result_as_node, out, inner->level, weight_of_defective_range, tree);

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
                                tasks.push_back(*new(allocate_child()) TreeUpdateTask(result->slot[out], subtree_updates[in].upd_begin, subtree_updates[in].upd_end, tree));
                                ++task_count;
                            }
                            ++out;
                            ++in;
                        }
                    }
                    result->slotuse = out;
                    set_ref_count(task_count+1);
                    spawn_and_wait_for_all(tasks);
                    set_min_element(slot, result);
                    tree->update_router(slot.slotkey, result->slot[out-1].slotkey);
                    tree->free_node(slot.childid);
                    slot.childid = result;

                    return NULL;
                }
            }
        }

    private:

        void update_leaf_in_current_tree(leaf_node* const result, const leaf_node* const leaf) {
            width_type in = 0; // existing key to read
            width_type out = 0; // position where to write
            const width_type in_slotuse = leaf->slotuse;

            #ifdef COMPUTE_PARETO_MIN
                const min_key_type local_min_dummy(0, std::numeric_limits<typename min_key_type::weight_type>::max());
                const min_key_type* local_min =&local_min_dummy;
            #else 
                min_key_type* local_min;
            #endif

            BTREE_PRINT("Updating leaf from " << leaf << " to " << result);

            for (size_type i = upd_begin; i != upd_end; ++i) {
                const Operation<key_type>& op = tree->updates[i];

                switch (op.type) {
                case Operation<key_type>::DELETE:
                    // We know the element is in here, so no bounds checks
                    while (tree->key_less(leaf->slotkey[in], op.data)) {
                        BTREE_ASSERT(in < in_slotuse);
                        #ifdef COMPUTE_PARETO_MIN
                            local_min = leaf->slotkey[in].second_weight < local_min->second_weight ? &leaf->slotkey[in] : local_min;
                        #endif 
                        result->slotkey[out++] = leaf->slotkey[in++];
                    }
                    ++in; // delete the element by jumping over it
                    break;
                case Operation<key_type>::INSERT:
                    while(in < in_slotuse && tree->key_less(leaf->slotkey[in], op.data)) {
                        #ifdef COMPUTE_PARETO_MIN
                            local_min = leaf->slotkey[in].second_weight < local_min->second_weight ? &leaf->slotkey[in] : local_min;
                        #endif
                        result->slotkey[out++] = leaf->slotkey[in++];
                    }
                     #ifdef COMPUTE_PARETO_MIN
                        local_min = op.data.second_weight < local_min->second_weight ? &op.data : local_min;
                    #endif
                    result->slotkey[out++] = op.data;
                    break;
                }
            }
            assert(leaf->slotuse <= tree->leafslotmax);
            while (in < in_slotuse) {
                #ifdef COMPUTE_PARETO_MIN
                    local_min = leaf->slotkey[in].second_weight < local_min->second_weight ? &leaf->slotkey[in] : local_min;
                #endif
                result->slotkey[out++] = leaf->slotkey[in++];
            }
            assert(out <= tree->leafslotmax);

            set_min_element(slot, *local_min);
            result->slotuse = out;
            tree->update_router(slot.slotkey, result->slotkey[out-1]); 

            BTREE_PRINT(": size " << leaf->slotuse << " -> " << result->slotuse << std::endl);
        }

    };

    static inline bool hasUpdates(const UpdateDescriptor& update) {
        return update.upd_begin != update.upd_end;
    }

    inline bool scheduleSubTreeUpdate(const width_type i, const size_type& weight, const size_type minweight,
            const size_type maxweight, const size_type subupd_begin, const size_type subupd_end, UpdateDescriptor* subtree_updates) const {
        subtree_updates[i].upd_begin = subupd_begin;
        subtree_updates[i].upd_end = subupd_end;
        subtree_updates[i].weight = weight + weightdelta[subupd_end] - weightdelta[subupd_begin];
        subtree_updates[i].rebalancing_needed =  subtree_updates[i].weight < minweight || subtree_updates[i].weight > maxweight;
        return subtree_updates[i].rebalancing_needed;
    }

    inline int find_lower(int lo, int hi, const key_type& key) const {
        while (lo < hi) {
            int mid = (lo + hi) >> 1;

            if (key_less(key, updates[mid].data)) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return hi;
    }

    static inline size_type designated_subtreesize(const level_type level) {
        size_type num_to_round = (maxweight(level-1) + minweight(level-1)) / 2;

        size_type remaining = num_to_round % designated_leafsize;
        if (remaining == 0) {
            return num_to_round;  
        } else {
            const size_type diff_in_single_tree_case = remaining;
            const size_type diff_in_extra_tree_case = designated_leafsize-remaining;

            return num_to_round - remaining + designated_leafsize * (diff_in_single_tree_case >= diff_in_extra_tree_case);
        }
    }

    static inline size_type num_subtrees(const size_type n, const size_type subtreesize) {
        size_type num_subtrees = n / subtreesize;
        // Squeeze remaining elements into last subtree or place in own subtree? 
        // Choose what is closer to our designated subtree size
        size_type remaining = n % subtreesize;
        const size_type diff_in_single_tree_case = remaining;
        const size_type diff_in_extra_tree_case = subtreesize-remaining;

        num_subtrees += diff_in_single_tree_case >= diff_in_extra_tree_case;
        num_subtrees += n > 0 && num_subtrees == 0; // but have at least enough space to hold all elements
        return num_subtrees;
    }

    static inline level_type num_optimal_levels(const size_type n) {
        if (n <= designated_leafsize) {
            return 1;
        } else {
            return ceil( log(2 * ((double) n)/traits::leafparameter_k) / log(traits::branchingparameter_b) );
        }
    }

    inline void update_router(key_type& router, const key_type& new_router) const {
        if (!key_equal(router, new_router)) {
            router = new_router;
        }
    }

    /// Recursively descend down the tree and print out nodes.
    static void print_node(const node* node, level_type depth=0, bool recursive=false) {
        for(level_type i = 0; i < depth; i++) std::cout  << "  ";
        std::cout << "node " << node << " level " << node->level << " slotuse " << node->slotuse << std::endl;

        if (node->isleafnode()) {
            const leaf_node *leafnode = static_cast<const leaf_node*>(node);
            for(level_type i = 0; i < depth; i++) std::cout  << "  ";

            for (width_type slot = 0; slot < leafnode->slotuse; ++slot) {
                std::cout << leafnode->slotkey[slot] << "  "; // << "(data: " << leafnode->slotdata[slot] << ") ";
            }
            std::cout  << std::endl;
        } else {
            const inner_node *innernode = static_cast<const inner_node*>(node);
            for(level_type i = 0; i < depth; i++) std::cout  << "  ";

            for (width_type slot = 0; slot < innernode->slotuse; ++slot) {
                std::cout  << "(" << innernode->slot[slot].childid << ": " << innernode->slot[slot].weight << ") " << innernode->slot[slot].slotkey << " ";
            }
            std::cout << std::endl;

            if (recursive) {
                for (width_type slot = 0; slot < innernode->slotuse; ++slot) {
                    print_node(innernode->slot[slot].childid, depth + 1, recursive);
                }
            }
        }
    }


public:
    // *** Verification of B+ Tree Invariants

    /// Run a thorough verification of all B+ tree invariants. The program
    /// aborts via assert() if something is wrong.
    void verify() const {
        key_type minkey, maxkey;
        tree_stats vstats;

        #ifdef NDEBUG
            std::cout << "WARNING: Trying to verify, but all assertions have been disabled" << std::endl;
        #endif

        if (root) {
            verify_node(root, &minkey, &maxkey, vstats);
            assert( vstats.itemcount == stats.itemcount );
            assert( vstats.leaves == stats.leaves );
            assert( vstats.innernodes == stats.innernodes );
        }
    }

private:

    /// Recursively descend down the tree and verify each node
    void verify_node(const node* n, key_type* minkey, key_type* maxkey, tree_stats &vstats) const {
        BTREE_PRINT("verifynode " << n << std::endl);

        if (n->isleafnode()) {
            const leaf_node *leaf = static_cast<const leaf_node*>(n);

            for(width_type slot = 0; slot < leaf->slotuse - 1; ++slot) {
                assert(key_lessequal(leaf->slotkey[slot], leaf->slotkey[slot + 1]));
            }
            if ((leaf != root && !(leaf->slotuse >= minweight(leaf->level))) || !( leaf->slotuse <= maxweight(leaf->level))) {
                std::cout << leaf->slotuse << " min " << minweight(0) << " max " <<maxweight(0) << std::endl;
                print_node(leaf);
            }
            assert( leaf == root || leaf->slotuse >= minweight(leaf->level) );
            assert( leaf->slotuse <= maxweight(leaf->level) );

            *minkey = leaf->slotkey[0];
            *maxkey = leaf->slotkey[leaf->slotuse - 1];

            vstats.leaves++;
            vstats.itemcount += leaf->slotuse;
        }
        else { 
            const inner_node *inner = static_cast<const inner_node*>(n);
            vstats.innernodes++;

            for(width_type slot = 0; slot < inner->slotuse-1; ++slot) {
                if (!key_lessequal(inner->slot[slot].slotkey, inner->slot[slot + 1].slotkey)) {
                    print_node(inner, 0, true);
                }
                assert(key_lessequal(inner->slot[slot].slotkey, inner->slot[slot + 1].slotkey));
            }

            for(width_type slot = 0; slot < inner->slotuse; ++slot) {
                const node *subnode = inner->slot[slot].childid;
                key_type subminkey = key_type();
                key_type submaxkey = key_type();

                assert(subnode->level + 1 == inner->level);

                if ((inner != root && !(inner->slot[slot].weight >= minweight(inner->level-1))) || !(inner->slot[slot].weight <= maxweight(inner->level-1))) {
                    std::cout << inner->slot[slot].weight << " min " << minweight(inner->level-1) << " max " <<maxweight(inner->level-1) << std::endl;
                    print_node(inner, 0, true);
                }
                assert( inner == root || inner->slot[slot].weight >= minweight(inner->level-1) );
                assert( inner->slot[slot].weight <= maxweight(inner->level-1) );

                size_type itemcount = vstats.itemcount;
                verify_node(subnode, &subminkey, &submaxkey, vstats);

                assert(inner->slot[slot].weight == vstats.itemcount - itemcount);

                BTREE_PRINT("verify subnode " << subnode << ": " << subminkey << " - " << submaxkey << std::endl);

                if (slot == 0) {
                    *minkey = subminkey;
                } else {
                    assert(key_greaterequal(subminkey, inner->slot[slot-1].slotkey));
                }
                assert(key_equal(inner->slot[slot].slotkey, submaxkey));
            }
            *maxkey = inner->slot[inner->slotuse-1].slotkey;
        }
    }
};


#endif // _PARALLEL_BTREE_H_
