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

#ifndef _BTREE_H_
#define _BTREE_H

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

#include "tbb/parallel_scan.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/partitioner.h"
#include "tbb/atomic.h"


// *** Debugging Macros
#ifdef BTREE_DEBUG
#define BTREE_PRINT(x)          do { (std::cout << x); } while(0)
#define BTREE_ASSERT(x)         do { assert(x); } while(0)
#else
#define BTREE_PRINT(x)          do { } while(0)
#define BTREE_ASSERT(x)         do { } while(0)
#endif

/// The maximum of a and b. Used in some compile-time formulas.
#define BTREE_MAX(a,b)          ((a) < (b) ? (b) : (a))

// Width of nodes given as number of cache-lines
#ifndef INNER_NODE_WIDTH
#define INNER_NODE_WIDTH 12
#endif
#ifndef LEAF_NODE_WIDTH
#define LEAF_NODE_WIDTH 12
#endif
#ifndef DCACHE_LINESIZE
#define DCACHE_LINESIZE 64
#endif

// Number of leaves that need to be written before we try perform it in parallel
#ifndef REWRITE_THRESHOLD
#define REWRITE_THRESHOLD 2
#endif

template<typename data_type>
struct Operation {
    enum OpType {INSERT, DELETE};
    OpType type;
    data_type data;
};

template <typename _Key>
struct btree_default_traits {
    /// If true, the tree will self verify it's invariants after each insert()
    /// or erase(). The header must have been compiled with BTREE_DEBUG defined.
    static const bool   selfverify = false;

    /// Configure nodes to have a fixed size of 8 cache lines. 
    static const int    leafparameter_k = BTREE_MAX( 8, LEAF_NODE_WIDTH * DCACHE_LINESIZE / (sizeof(_Key)) );
    static const int    branchingparameter_b = BTREE_MAX( 8,  (INNER_NODE_WIDTH * DCACHE_LINESIZE / (sizeof(_Key) + sizeof(size_t) + sizeof(void*)))/4 );
};

/** 
 * Basic class implementing a base B+ tree data structure in memory.
 */
template <typename _Key,
          typename _Compare = std::less<_Key>,
          typename _Traits = btree_default_traits<_Key>,
          typename _Alloc = std::allocator<_Key>>
class btree {
public:
    /// The key type of the B+ tree. This is stored  in inner nodes and leaves
    typedef _Key                        key_type;
    
    /// Key comparison function object
    typedef _Compare                    key_compare;
    
    /// Traits object used to define more parameters of the B+ tree
    typedef _Traits                     traits;
   
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

private:
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

    struct inner_node : public node {
        /// Define an related allocator for the inner_node structs.
        typedef typename _Alloc::template rebind<inner_node>::other alloc_type;

        /// Heighest key in the subtree with the same slot index
        key_type        slotkey[innerslotmax];

        /// Weight (total number of keys) of the subtree 
        size_type       weight[innerslotmax];

        /// Pointers to children
        node*           childid[innerslotmax];

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

    static size_type minweight(level_type level) {
        return ipow(traits::branchingparameter_b, level) * traits::leafparameter_k / 4;
    }

    static size_type maxweight(level_type level) {
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

public:
    // *** Small Statistics Structure

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

private:
    // *** Tree Object Data Members

    /// Pointer to the B+ tree's root node, either leaf or inner node
    node*       root;

    /// Pointer to spare leaf used for merging.
    node*       spare_leaf;

    /// Other small statistics about the B+ tree
    tree_stats  stats;

    /// Key comparison object. More comparison functions are generated from
    /// this < relation.
    static key_compare key_less;

    /// Memory allocator.
    allocator_type allocator;

    // Currently running updates
    const Operation<key_type>* updates;
    size_type updates_size;

    // Weight delta of currently running updates
    std::vector<signed long> weightdelta;

    // Leaves created during the current reconstruction effort
    std::vector<leaf_node*> leaves;

    struct UpdateDescriptor {
        bool rebalancing_needed;
        size_type weight;
        size_type upd_begin;
        size_type upd_end;
    };

    // For each level, one array to store the updates needed to push down to the individual subtrees
    std::vector<UpdateDescriptor*> subtree_updates_per_level;


public:
    // *** Constructors and Destructor

    /// Default constructor initializing an empty B+ tree with the standard key
    /// comparison function
    explicit inline btree(const allocator_type &alloc = allocator_type())
        : root(NULL), allocator(alloc) 
    {
        spare_leaf = allocate_leaf();
        stats = tree_stats(); // reset stats after leaf creation
    }

    /// Frees up all used B+ tree memory pages
    inline ~btree() {
        clear();
        free_node(spare_leaf);
        for (auto updates : subtree_updates_per_level) {
            delete[] updates;
        }
    }

private:
    // *** Convenient Key Comparison Functions Generated From key_less

    /// True if a <= b
    static inline bool key_lessequal(const key_type &a, const key_type &b) {
        return !key_less(b, a);
    }

    /// True if a >= b
    static inline bool key_greaterequal(const key_type &a, const key_type &b) {
        return !key_less(a, b);
    }

    /// True if a == b. This requires the < relation to be a total order,
    /// otherwise the B+ tree cannot be sorted.
    static inline bool key_equal(const key_type &a, const key_type &b) {
        return !key_less(a, b) && !key_less(b, a);
    }

private:
    // *** Node Object Allocation and Deallocation Functions

    typename leaf_node::alloc_type leaf_node_allocator() {
        return typename leaf_node::alloc_type(allocator);
    }

    typename inner_node::alloc_type inner_node_allocator() {
        return typename inner_node::alloc_type(allocator);
    }

    inline leaf_node* allocate_leaf() {
        leaf_node *n = new (leaf_node_allocator().allocate(1)) leaf_node();
        n->initialize();
        if (stats.gather_stats) stats.leaves.fetch_and_increment();
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

    void clear() {
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
                clear_recursive(innernode->childid[slot]);
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

public:

    void apply_updates(const std::vector<Operation<key_type>>& _updates) {
        size_type new_size = setOperationsAndComputeWeightDelta(_updates);
        stats.itemcount = new_size;
        
        if (new_size == 0) {
            clear(); // Tree will become empty. Just finish early.
            return;
        } 
        if (root == NULL) {
            root = allocate_leaf();
        }
        level_type level = num_optimal_levels(new_size);
        bool rebuild_needed = (level < root->level && size() < minweight(root->level)) || size() > maxweight(root->level);

        if (rebuild_needed) {
            for (level_type i = root->level; i <= level; ++i) {
                subtree_updates_per_level.push_back(new UpdateDescriptor[innerslotmax]);
            }
            allocate_new_leaves(new_size);
        }
        key_type router; // unused on this level
        update(root, router, /*rank*/0, /*update_begin*/0, updates_size, rebuild_needed);

        if (rebuild_needed) {
            create_subtree_from_leaves(root, false, level, router, 0, new_size);
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
        PrefixSum(TOut* out_, const TIn* in_)
            : sum(0), out(out_), in(in_) {}

        PrefixSum(PrefixSum& b, tbb::split)
            : sum(0), out(b.out), in(b.in) {}

        template<typename Tag>
        void operator() (const tbb::blocked_range<size_type>& r, Tag) {
            TOut temp = sum;
            if (Tag::is_final_scan()) {
                for(size_type i=r.begin(); i<r.end(); ++i) {
                    temp = temp + (in[i].type == TIn::INSERT ? 1 : -1);
                    out[i+1] = temp;
                }
            } else {
                for(size_type i=r.begin(); i<r.end(); ++i) {
                    temp = temp + (in[i].type == TIn::INSERT ? 1 : -1);
                }
            }
            sum = temp;
        }
        void reverse_join(PrefixSum& a) {sum = a.sum + sum;}
        void assign(PrefixSum& b) {sum = b.sum;}
        TOut get_sum() const { return sum; }
    };

    size_type setOperationsAndComputeWeightDelta(const std::vector<Operation<key_type>>& _updates) {
        updates = _updates.data();
        updates_size = _updates.size();

        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)
        weightdelta.clear();

        weightdelta.reserve(updates_size+1);
        weightdelta[0] = 0;
        PrefixSum<Operation<key_type>, signed long> body(weightdelta.data(), _updates.data());
        tbb::parallel_scan(tbb::blocked_range<size_type>(0, _updates.size()), body);

        return size() + body.get_sum();

    }

    void allocate_new_leaves(const size_type n) {
        BTREE_PRINT("Allocating new nodes for tree of size " << n << std::endl);
        const size_type leaf_count = num_subtrees(n, designated_leafsize);
        leaves.resize(leaf_count);
        tbb::parallel_for(tbb::blocked_range<size_type>(0, leaf_count),
            [this, &leaf_count, &n](const tbb::blocked_range<size_type>& r) {
                for (size_type i = r.begin(); i < r.end(); ++i) {
                    leaf_node* leaf = this->allocate_leaf();
                    const size_type last_leaf = leaf_count-1;
                    leaf->slotuse = (i == last_leaf) ? n - last_leaf*designated_leafsize : designated_leafsize;
                    this->leaves[i] = leaf;
                }
            }
        );
    }

    width_type create_subtree_from_leaves(node*& out_node, const bool reuse_outnode, const level_type level, key_type& router, const size_type rank_begin, const size_type rank_end) {
        BTREE_ASSERT(rank_end - rank_begin > 0);
        BTREE_PRINT("Creating tree on level " << level << " for range [" << rank_begin << ", " << rank_end << ")" << std::endl);

        if (level == 0) { // reached leaf level
            leaf_node* result = leaves[rank_begin / designated_leafsize];
            BTREE_ASSERT(rank_end - rank_begin == result->slotuse);
            router = result->slotkey[result->slotuse-1];
            out_node = result;
            return 1;
        } else {
            const size_type n = rank_end - rank_begin;
            const size_type designated_treesize = designated_subtreesize(level);
            const width_type subtrees = num_subtrees(n, designated_treesize);

            BTREE_PRINT("Creating inner node on level " << level << " with " << subtrees << " subtrees of desiganted size " 
                << designated_treesize << std::endl);

            inner_node* result = reuse_outnode ? static_cast<inner_node*>(out_node) : allocate_inner(level);
            const width_type old_slotuse = reuse_outnode ? result->slotuse : 0;
            const width_type new_slotuse = subtrees+old_slotuse;

            BTREE_ASSERT(new_slotuse <= innerslotmax);

            size_type rank = rank_begin;
            for (width_type i = old_slotuse; i < new_slotuse; ++i) {
                const size_type weight = (i != new_slotuse-1) ? designated_treesize : (rank_end - rank);
                result->weight[i] = weight;
                create_subtree_from_leaves(result->childid[i], false, level-1, result->slotkey[i], rank, rank+weight);
                rank += weight;
            }
            result->slotuse = new_slotuse;
            router = result->slotkey[new_slotuse-1];
            out_node = result;
            return subtrees;
         }
    }

    void update(node*& _node, key_type& router, const size_type rank, const size_type upd_begin, const size_type upd_end, const bool rewrite_subtree) {
        BTREE_PRINT("Applying updates [" << upd_begin << ", " << upd_end << ") to " << _node << " on level " << _node->level << ". Rewrite = " << rewrite_subtree << std::endl);

        if (_node->isleafnode()) {
            if (rewrite_subtree) {
                write_updated_leaf_to_new_tree(_node, rank, upd_begin, upd_end);
            } else {
                update_leaf_in_current_tree(_node, router, upd_begin, upd_end);
            }

        } else {
            inner_node* inner = static_cast<inner_node*>(_node);
            UpdateDescriptor* subtree_updates = subtree_updates_per_level[inner->level];

            const size_type min_weight = minweight(inner->level-1);
            const size_type max_weight = maxweight(inner->level-1);

            bool rebalancing_needed = false;

            // Distribute operations and find out which subtrees need rebalancing
            width_type last = inner->slotuse-1;
            size_type subupd_begin = upd_begin;
            for (width_type i = 0; i < last; ++i) {
                size_type subupd_end = find_lower(subupd_begin, upd_end, inner->slotkey[i]);

                rebalancing_needed |= scheduleSubTreeUpdate(i, inner->weight[i], min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                subupd_begin = subupd_end;
            } 
            rebalancing_needed |= scheduleSubTreeUpdate(last, inner->weight[last], min_weight, max_weight, subupd_begin, upd_end, subtree_updates);


            // If no rewrite session is starting on this node, then just push down all updates
            if (!rebalancing_needed || rewrite_subtree) {
                updateSubTreesInRange(inner, 0, inner->slotuse, rank, rewrite_subtree, subtree_updates);
                router = inner->slotkey[inner->slotuse-1];
            } else {
                BTREE_PRINT("Rewrite session started for " << inner << " on level " << inner->level << std::endl);
                // Need to perform rebalancing.
                size_type designated_treesize = designated_subtreesize(inner->level);

                inner_node* result = allocate_inner(inner->level);
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
                    if (weight_of_defective_range > 0) {
                        BTREE_PRINT("Rewrite session started on level " << inner->level << " of " << height() << " for subtree count " << in-rebalancing_range_start<< std::endl);
                        allocate_new_leaves(weight_of_defective_range);
                        updateSubTreesInRange(inner, rebalancing_range_start, in, 0, true, subtree_updates);

                        result->slotuse = out;
                        node* result_as_node = result;
                        key_type unused_router;
                        out += create_subtree_from_leaves(result_as_node, /*write into result node*/ true, result->level, unused_router, 0, weight_of_defective_range);
                        result = static_cast<inner_node*>(result_as_node);

                    } else {
                        BTREE_PRINT("Copying " << in << " to " << out << " " << inner->childid[in] << std::endl);
                        if (hasUpdates(in, subtree_updates)) {
                            update(inner->childid[in], result->slotkey[out], /*unused rank*/ -1, subtree_updates[in].upd_begin,
                                subtree_updates[in].upd_end, false);
                        } else {
                            result->slotkey[out] = inner->slotkey[in];
                        }
                        result->weight[out] = subtree_updates[in].weight;
                        result->childid[out] = inner->childid[in];

                        ++out;
                        ++in;
                    }
                }
                router = result->slotkey[out-1];
                result->slotuse = out;
                free_node(_node);
                _node = result;
            }
        }
        if (rewrite_subtree) {
            free_node(_node);
        }
    }

    void updateSubTreesInRange(inner_node* node, const width_type begin, const width_type end, const size_type rank, const bool rewrite_subtree, const UpdateDescriptor* subtree_updates) {
        size_type subtree_rank = rank;
        for (width_type i = begin; i < end; ++i) {
            if (subtree_updates[i].weight > 0 && (rewrite_subtree || hasUpdates(i, subtree_updates))) {
                update(node->childid[i], node->slotkey[i], subtree_rank, subtree_updates[i].upd_begin,
                    subtree_updates[i].upd_end, rewrite_subtree);
            }
            subtree_rank += subtree_updates[i].weight;
            node->weight[i] = subtree_updates[i].weight;
        }
    }

    static inline bool hasUpdates(const width_type i, const UpdateDescriptor* subtree_updates) {
        return subtree_updates[i].upd_begin != subtree_updates[i].upd_end;
    }

    inline bool scheduleSubTreeUpdate(const width_type i, const size_type& weight, const size_type minweight,
            const size_type maxweight, const size_type subupd_begin, const size_type subupd_end, UpdateDescriptor* subtree_updates) const {
        subtree_updates[i].upd_begin = subupd_begin;
        subtree_updates[i].upd_end = subupd_end;
        subtree_updates[i].weight = weight + weightdelta[subupd_end] - weightdelta[subupd_begin];
        subtree_updates[i].rebalancing_needed =  subtree_updates[i].weight < minweight || subtree_updates[i].weight > maxweight;
        return subtree_updates[i].rebalancing_needed;
    }

    inline int find_lower(const size_type begin, const size_type end, const key_type& key) const {
        int lo = begin;
        int hi = end - 1;

        while(lo < hi) {
            size_type mid = (lo + hi) >> 1; // FIXME: Potential integer overflow

            if (key_less(key, updates[mid].data)) {
                hi = mid - 1;
            } else {
                lo = mid + 1;
            }
        }
        hi += (hi < 0 || key_lessequal(updates[hi].data, key));
        return hi;
    }

    static inline size_type designated_subtreesize(level_type level) {
        return (maxweight(level-1) + minweight(level-1)) / 2;
    }

    static inline  size_type num_subtrees(size_type n, size_type subtreesize) {
        size_type num_subtrees = n / subtreesize;
        // Squeeze remaining elements into last subtree or place in own subtree? 
        // Choose what is closer to our designated subtree size
        size_type remaining = n % subtreesize;
        const size_type diff_in_single_tree_case = remaining;
        const size_type diff_in_extra_tree_case = subtreesize-remaining;

        num_subtrees += diff_in_single_tree_case >= diff_in_extra_tree_case;
        return num_subtrees;
        
    }

    static inline level_type num_optimal_levels(size_type n) {
        if (n <= leafslotmax) {
            return 0;
        } else {
            level_type opt_levels = ceil( log(2 * ((double) n)/traits::leafparameter_k) / log(traits::branchingparameter_b) );
            opt_levels -= num_subtrees(n, designated_subtreesize(opt_levels)) == 1;
            return opt_levels;
        }
    }

    void write_updated_leaf_to_new_tree(const node* const node, const size_type rank, const size_type begin, const size_type end) {
        if (weightdelta[end] - weightdelta[begin] < designated_leafsize * REWRITE_THRESHOLD) {
            write_updated_leaf_to_new_tree_sequential(node, /*read start index*/0, rank, begin, end, /*write remaining*/ true);
        } else {
            const leaf_node* const leaf = (leaf_node*)(node);

            tbb::parallel_for(tbb::blocked_range<size_type>(begin, end),
                [&, this](const tbb::blocked_range<size_type>& r) {
                    const signed long delta = this->weightdelta[r.begin()] - this->weightdelta[begin];
                    const width_type key_index = find_index_of_lower_key(leaf, updates[r.begin()].data);
                    const size_type corrected_rank = rank + key_index + delta;
                    this->write_updated_leaf_to_new_tree_sequential(node, key_index, corrected_rank, r.begin(), r.end(), r.end() == end);
                }
            );
        }
    }

    static inline int find_index_of_lower_key(const leaf_node* const leaf, const key_type& key) {
        int lo = 0;
        int hi = leaf->slotuse - 1;

        while(lo < hi) {
            size_type mid = (lo + hi) >> 1; // FIXME: Potential integer overflow

            if (key_less(key, leaf->slotkey[mid])) {
                hi = mid - 1;
            } else {
                lo = mid + 1;
            }
        }
        hi += (hi < 0 || key_lessequal(leaf->slotkey[hi], key));
        return hi;
    }


    void write_updated_leaf_to_new_tree_sequential(const node* const node, width_type in, const size_type rank, const size_type begin, const size_type end, const bool write_remaining) {
        BTREE_PRINT("Rewriting updated leaf " << node << " starting with rank " << rank);

        size_type leaf_number = rank / designated_leafsize;
        width_type offset_in_leaf = rank % designated_leafsize;

        if (leaf_number >= leaves.size()) {
            // elements are squeezed into the previous leaf
            leaf_number = leaves.size()-1; 
            offset_in_leaf = rank - leaf_number*designated_leafsize;
        }

        // in is the existing key to read
        width_type out = offset_in_leaf; // position where to write


        leaf_node* result = leaves[leaf_number];
        const leaf_node* const leaf = (leaf_node*)(node);

        for (size_type i = begin; i != end; ++i) {
            switch (updates[i].type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (!key_equal(leaf->slotkey[in], updates[i].data)) {
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                        result = leaves[++leaf_number];
                        out = 0;
                    }
                }
                ++in; // delete the element by jumping over it
                break;
            case Operation<key_type>::INSERT:
                while(in < leaf->slotuse && key_less(leaf->slotkey[in], updates[i].data)) {
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                        result = leaves[++leaf_number];
                        out = 0;
                    }
                }
                result->slotkey[out++] = updates[i].data;

                if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                    result = leaves[++leaf_number];
                    out = 0;
                }
                break;
            }
        } 
        if (write_remaining) {
            while (in < leaf->slotuse) {
                result->slotkey[out++] = leaf->slotkey[in++];

                if (out == designated_leafsize && hasNextLeaf(leaf_number) && in < leaf->slotuse) {
                    result = leaves[++leaf_number];
                    out = 0;
                }
            }
        }

        BTREE_PRINT(" as range [" << rank << ", " << ((leaf_number-(rank/designated_leafsize))*designated_leafsize)
            + out << ") into " << leaves.size() << " leaves " << std::endl);
    }

    inline bool hasNextLeaf(size_type leaf_count) const {
        return leaf_count+1 < leaves.size();
    }

    void update_leaf_in_current_tree(node*& node, key_type& router, const size_type begin, const size_type end) {
        width_type in = 0; // existing key to read
        width_type out = 0; // position where to write

        leaf_node* result = static_cast<leaf_node*>(spare_leaf);
        const leaf_node* leaf = static_cast<leaf_node*>(node);

        BTREE_PRINT("Updating leaf from " << node << " to " << result);

        for (size_type i = begin; i != end; ++i) {
            switch (updates[i].type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (!key_equal(leaf->slotkey[in], updates[i].data)) {
                    result->slotkey[out++] = leaf->slotkey[in++];
                }
                ++in; // delete the element by jumping over it
                break;
            case Operation<key_type>::INSERT:
                while(in < leaf->slotuse && key_less(leaf->slotkey[in], updates[i].data)) {
                    result->slotkey[out++] = leaf->slotkey[in++];
                }
                result->slotkey[out++] = updates[i].data;
                break;
            }
        }
        memcpy(result->slotkey + out, leaf->slotkey + in, sizeof(key_type) * (leaf->slotuse - in));

        result->slotuse = out + leaf->slotuse - in;
        router = result->slotkey[result->slotuse-1]; 

        BTREE_PRINT(": size " << leaf->slotuse << " -> " << result->slotuse << std::endl);

        spare_leaf = node;
        node = result;
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
                std::cout  << "(" << innernode->childid[slot] << ": " << innernode->weight[slot] << ") " << innernode->slotkey[slot] << " ";
            }
            std::cout << std::endl;

            if (recursive) {
                for (width_type slot = 0; slot < innernode->slotuse; ++slot) {
                    print_node(innernode->childid[slot], depth + 1, recursive);
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
                if (!key_lessequal(inner->slotkey[slot], inner->slotkey[slot + 1])) {
                    print_node(inner, 0, true);
                }
                assert(key_lessequal(inner->slotkey[slot], inner->slotkey[slot + 1]));
            }

            for(width_type slot = 0; slot < inner->slotuse; ++slot) {
                const node *subnode = inner->childid[slot];
                key_type subminkey = key_type();
                key_type submaxkey = key_type();

                assert(subnode->level + 1 == inner->level);

                if ((inner != root && !(inner->weight[slot] >= minweight(inner->level-1))) || !(inner->weight[slot] <= maxweight(inner->level-1))) {
                    std::cout << inner->weight[slot] << " min " << minweight(inner->level-1) << " max " <<maxweight(inner->level-1) << std::endl;
                    print_node(inner, 0, true);
                }
                assert( inner == root || inner->weight[slot] >= minweight(inner->level-1) );
                assert( inner->weight[slot] <= maxweight(inner->level-1) );

                size_type itemcount = vstats.itemcount;
                verify_node(subnode, &subminkey, &submaxkey, vstats);

                assert(inner->weight[slot] == vstats.itemcount - itemcount);

                BTREE_PRINT("verify subnode " << subnode << ": " << subminkey << " - " << submaxkey << std::endl);

                if (slot == 0) {
                    *minkey = subminkey;
                } else {
                    assert(key_greaterequal(subminkey, inner->slotkey[slot-1]));
                }
                assert(key_equal(inner->slotkey[slot], submaxkey));
            }
            *maxkey = inner->slotkey[inner->slotuse-1];
        }
    }
};


#endif // _BTREE_H_
