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

#ifndef _SEQUENTIAL_BTREE_H_
#define _SEQUENTIAL_BTREE_H_

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
#define INNER_NODE_WIDTH 8
#endif
#ifndef LEAF_NODE_WIDTH
#define LEAF_NODE_WIDTH 8
#endif

template<typename data_type>
struct Operation {
    enum OpType {INSERT=1, DELETE=-1};
    OpType type;
    data_type data;
    Operation(const OpType& x, const data_type& y) : type(x), data(y) {}
};

template <typename _Key, typename _MinKey>
struct btree_default_traits {
    /// If true, the tree will self verify it's invariants after each insert()
    /// or erase(). The header must have been compiled with BTREE_DEBUG defined.
    static const bool   selfverify = false;

    /// Configure nodes to have a fixed size of X cache lines. 
    static const int    leafparameter_k = BTREE_MAX( 4, (LEAF_NODE_WIDTH * DCACHE_LINESIZE - 2*sizeof(unsigned short)) / (sizeof(_Key)) );
    static const int    branchingparameter_b = BTREE_MAX( 4, ((INNER_NODE_WIDTH * DCACHE_LINESIZE - 2*sizeof(unsigned short)) / (sizeof(_Key) + sizeof(_MinKey) + sizeof(size_t) + sizeof(void*)))/4 );
};

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
        typename _Alloc = std::allocator<_Key>>
class btree {
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

        /// Heighest key in the subtree with the same slot index
        min_key_type        minimum[innerslotmax];

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
        size_type       itemcount;

        /// Number of leaves in the B+ tree
        size_type       leaves;

        /// Number of inner nodes in the B+ tree
        size_type       innernodes;

        /// Return the total number of nodes
        inline size_type nodes() const {
            return innernodes + leaves;
        }

        /// Return the average fill of leaves
        inline double avgfill_leaves() const {
            return static_cast<double>(itemcount) / (leaves * leafslotmax);
        }

        inline tree_stats()
            : itemcount(0), leaves(0), innernodes(0)
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
    key_compare key_less;

    /// Memory allocator.
    allocator_type allocator;

    // Currently running updates
    const Operation<key_type>* updates;
    
    // Leaves created during the current reconstruction effort
    std::vector<leaf_node*> leaves;

    // Weight delta of currently running updates
    typedef std::vector<signed long> weightdelta_list;
    weightdelta_list weightdelta;

    struct UpdateDescriptor {
        bool rebalancing_needed;
        size_type weight;
        size_type upd_begin;
        size_type upd_end;
    };

    typedef std::vector<Operation<key_type>> update_list;

    // For each level, one array to store the updates needed to push down to the individual subtrees
    std::vector<UpdateDescriptor*> subtree_updates_per_level;


public:
    // *** Constructors and Destructor

    /// Default constructor initializing an empty B+ tree with the standard key
    /// comparison function
    explicit inline btree(size_type size_hint=0, const allocator_type &alloc = allocator_type())
        : root(NULL), allocator(alloc)
    {
        spare_leaf = allocate_leaf_without_count();
        weightdelta.reserve(size_hint+1);
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

    typename leaf_node::alloc_type leaf_node_allocator() {
        return typename leaf_node::alloc_type(allocator);
    }

    typename inner_node::alloc_type inner_node_allocator() {
        return typename inner_node::alloc_type(allocator);
    }

    inline leaf_node* allocate_leaf() {
        leaf_node *n = new (leaf_node_allocator().allocate(1)) leaf_node();
        n->initialize();
        if (stats.gather_stats) stats.leaves++;
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
        if (stats.gather_stats) stats.innernodes++;
        return n;
    }

    inline void free_node(node *n) {
        if (n->isleafnode()) {
            leaf_node *ln = static_cast<leaf_node*>(n);
            typename leaf_node::alloc_type a(leaf_node_allocator());
            a.destroy(ln);
            a.deallocate(ln, 1);
            if (stats.gather_stats) stats.leaves--;
        } else {
            inner_node *in = static_cast<inner_node*>(n);
            typename inner_node::alloc_type a(inner_node_allocator());
            a.destroy(in);
            a.deallocate(in, 1);
            if (stats.gather_stats) stats.innernodes--;
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

    static std::string name() {
        return "Sequential BTree";
    }

public:

    void apply_updates(const update_list& _updates) {
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

        UpdateDescriptor upd = {rebuild_needed, new_size, 0, _updates.size()};

        if (rebuild_needed) {
            for (level_type i = root->level; i <= level; ++i) {
                subtree_updates_per_level.push_back(new UpdateDescriptor[innerslotmax]);
            }
            BTREE_PRINT("Root-level rewrite session started for new level " << level << std::endl);
            allocate_new_leaves(new_size);
        }
        key_type router; // unused on this level
        min_key_type min_key; // unused on this level
        update(root, router, min_key, /*rank*/0, upd, rebuild_needed);

        if (rebuild_needed) {
            create_subtree_from_leaves(root, false, level, router, min_key, 0, new_size);
        }
        #ifdef BTREE_DEBUG
            print_node(root, 0, true);
        #endif

        if (traits::selfverify) {
            verify();
        }
    }

    void find_pareto_minima(const min_key_type& prefix_minima, std::vector<key_type>& minima) const {
        BTREE_ASSERT(minima.empty());
        find_pareto_minima(root, prefix_minima, minima);
    }

private:

#ifdef COMPUTE_PARETO_MIN
    void find_pareto_minima(const node* const node, const min_key_type& prefix_minima, std::vector<key_type>& minima) const {
        if (node->isleafnode()) {
            const leaf_node* const leaf = (leaf_node*) node;
            width_type slotuse = leaf->slotuse;

            min_key_type min = prefix_minima;
            for (width_type i = 0; i<slotuse; ++i) {
                if (leaf->slotkey[i].second_weight < min.second_weight ||
                        (leaf->slotkey[i].first_weight == min.first_weight && leaf->slotkey[i].second_weight == min.second_weight)) {
                    minima.push_back(leaf->slotkey[i]);
                    min = leaf->slotkey[i];
                }
            }
        } else {
            const inner_node* const inner = (inner_node*) node;
            width_type slotuse = inner->slotuse;

            min_key_type min = prefix_minima;
            for (width_type i = 0; i<slotuse; ++i) {
                if (inner->minimum[i].second_weight < min.second_weight ||
                        (inner->minimum[i].first_weight == min.first_weight && inner->minimum[i].second_weight == min.second_weight)) {
                    find_pareto_minima(inner->childid[i], min, minima);
                    min = inner->minimum[i];
                }
            }
        }
    }

    static inline void set_min_element(min_key_type& min_key, const leaf_node* const node, width_type size) {
        min_key = *std::min_element(node->slotkey, node->slotkey+size,
            [](const min_key_type& i, const min_key_type& j) { return i.second_weight < j.second_weight; });
    }

    static inline void set_min_element(min_key_type& min_key, const inner_node* const node, width_type size) {
        min_key = *std::min_element(node->minimum, node->minimum+size,
            [](const min_key_type& i, const min_key_type& j) { return i.second_weight < j.second_weight; });
    }
#else 
    void find_pareto_minima(const node* const, const min_key_type&, std::vector<key_type>&) const {
        std::cout << "Pareto Min Feature disabled" << std::endl;
    }
    static inline void set_min_element(min_key_type&, const node* const, width_type) {

    }
#endif

private:

    size_type setOperationsAndComputeWeightDelta(const update_list& _updates) {
        updates = _updates.data();

        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)

        if (size() == 0) {
            return _updates.size(); // Shortcut, we know this is a bulk insertion and don't need any weights
        } else {
            weightdelta.reserve(_updates.size() + 1);

            long val = 0; // exclusive prefix sum
            weightdelta[0] = val;
            for (size_type i = 0; i < _updates.size();) {
                val = val + updates[i].type;
				weightdelta[++i] = val;
            }
            return size() + weightdelta[_updates.size()];
        }
    }

    void allocate_new_leaves(size_type n) {
        BTREE_PRINT("Allocating " << num_subtrees(n, designated_leafsize) << " new  nodes for tree of size " << n << std::endl);
        size_type leaf_count = num_subtrees(n, designated_leafsize);
        leaves.clear();
        leaves.reserve(leaf_count);
        for (size_type i = 0; i < leaf_count; ++i) {
            leaves.push_back(allocate_leaf());
        }
    }

    width_type create_subtree_from_leaves(node*& out_node, const bool reuse_outnode, const level_type level, key_type& router, min_key_type& min_key, size_type rank_begin, size_type rank_end) {
        BTREE_ASSERT(rank_end - rank_begin > 0);
        BTREE_PRINT("Creating tree on level " << level << " for range [" << rank_begin << ", " << rank_end << ")" << std::endl);

        if (level == 0) { // reached leaf level
            leaf_node* result = leaves[rank_begin / designated_leafsize];
            width_type slotuse = rank_end - rank_begin;
            BTREE_ASSERT(slotuse == result->slotuse);
            set_min_element(min_key, result, slotuse);
            router = result->slotkey[slotuse-1];
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
            result->slotuse = new_slotuse;
            out_node = result;

            BTREE_ASSERT(new_slotuse <= innerslotmax);

            size_type rank = rank_begin;
            for (width_type i = old_slotuse; i < new_slotuse; ++i) {
                const size_type weight = (i != new_slotuse-1) ? designated_treesize : (rank_end - rank);
                result->weight[i] = weight;
                create_subtree_from_leaves(result->childid[i], false, level-1, result->slotkey[i], result->minimum[i], rank, rank+weight);
                rank += weight;
            }
            set_min_element(min_key, result, new_slotuse);
            router = result->slotkey[new_slotuse-1];

            return subtrees;
         }
    }

    void update(node*& _node, key_type& router, min_key_type& min_key, const size_type rank, const UpdateDescriptor& upd, const bool rewrite_subtree) {
        BTREE_PRINT("Applying updates [" << upd.upd_begin << ", " << upd.upd_end << ") to " << _node << " on level " << _node->level << ". Rewrite = " << rewrite_subtree << std::endl);

        if (_node->isleafnode()) {
            if (rewrite_subtree) {
                write_updated_leaf_to_new_tree(_node, rank, upd);
            } else {
                update_leaf_in_current_tree(_node, router, min_key, upd);
            }

        } else {
            inner_node* inner = static_cast<inner_node*>(_node);
            UpdateDescriptor* subtree_updates = subtree_updates_per_level[inner->level];

            const size_type min_weight = minweight(inner->level-1);
            const size_type max_weight = maxweight(inner->level-1);

            bool rebalancing_needed = false;

            // Distribute operations and find out which subtrees need rebalancing
            width_type last = inner->slotuse-1;
            size_type subupd_begin = upd.upd_begin;
            for (width_type i = 0; i < last; ++i) {
                size_type subupd_end = find_lower(subupd_begin, upd.upd_end, inner->slotkey[i]);

                rebalancing_needed |= scheduleSubTreeUpdate(i, inner->weight[i], min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                subupd_begin = subupd_end;
            } 
            rebalancing_needed |= scheduleSubTreeUpdate(last, inner->weight[last], min_weight, max_weight, subupd_begin, upd.upd_end, subtree_updates);

            // If no rewrite session is starting on this node, then just push down all updates
            if (!rebalancing_needed || rewrite_subtree) {
                updateSubTreesInRange(inner, 0, inner->slotuse, rank, rewrite_subtree, subtree_updates);
                set_min_element(min_key, inner, inner->slotuse);
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
                    if (openrebalancing_region) {
                        if (weight_of_defective_range == 0) {
                            BTREE_PRINT("Deleting entire subtree range" << std::endl);
                            for (width_type i = rebalancing_range_start; i < in; ++i) {
                                clear_recursive(inner->childid[i]);
                            }
                        } else {
                            BTREE_PRINT("Rewrite session started on level " << inner->level << " of " << height() << " for subtrees [" << rebalancing_range_start << "," << in <<") of total weight " << weight_of_defective_range << std::endl);
                            allocate_new_leaves(weight_of_defective_range);
                            updateSubTreesInRange(inner, rebalancing_range_start, in, 0, true, subtree_updates);

                            result->slotuse = out;
                            node* result_as_node = result;
                            key_type unused_router;
                            min_key_type unused_min_key;
                            out += create_subtree_from_leaves(result_as_node, /*write into result node*/ true, result->level, unused_router, unused_min_key, 0, weight_of_defective_range);
                            result = static_cast<inner_node*>(result_as_node);                            
                        }
                    } else {
                        BTREE_PRINT("Copying " << in << " to " << out << " " << inner->childid[in] << std::endl);
                        result->weight[out] = subtree_updates[in].weight;
                        result->childid[out] = inner->childid[in];
                        result->minimum[out] = inner->minimum[in];
                        if (hasUpdates(subtree_updates[in])) {
                            update(result->childid[out], result->slotkey[out], result->minimum[out], /*unused rank*/ -1, subtree_updates[in], false);
                        } else {
                            result->slotkey[out] = inner->slotkey[in];
                        }

                        ++out;
                        ++in;
                    }
                }
                set_min_element(min_key, result, out);
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

    inline void updateSubTreesInRange(inner_node* node, const width_type begin, const width_type end, const size_type rank, const bool rewrite_subtree, const UpdateDescriptor* subtree_updates) {
        size_type subtree_rank = rank;
        for (width_type i = begin; i < end; ++i) {
            if (subtree_updates[i].weight == 0) {
                clear_recursive(node->childid[i]);
            } else if (rewrite_subtree || hasUpdates(subtree_updates[i])) {
                update(node->childid[i], node->slotkey[i], node->minimum[i], subtree_rank, subtree_updates[i], rewrite_subtree);
            	node->weight[i] = subtree_updates[i].weight;
            }
            subtree_rank += subtree_updates[i].weight;
        }
    }

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

    static inline  size_type num_subtrees(const size_type n, const size_type subtreesize) {
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

    void write_updated_leaf_to_new_tree(node*& node, const size_type rank, const UpdateDescriptor& upd) {
        BTREE_PRINT("Rewriting updated leaf " << node << " starting with rank " << rank << " using upd range [" << upd.upd_begin << "," << upd.upd_end << ")");

        size_type leaf_number = rank / designated_leafsize;
        width_type offset_in_leaf = rank % designated_leafsize;

        if (leaf_number >= leaves.size()) {
            // elements are squeezed into the previous leaf
            leaf_number = leaves.size()-1; 
            offset_in_leaf = rank - leaf_number*designated_leafsize;
        }
        BTREE_PRINT(". From leaf " << leaf_number << ": " << offset_in_leaf);

        width_type in = 0; // existing key to read
        width_type out = offset_in_leaf; // position where to write

        leaf_node* result = leaves[leaf_number];
        const leaf_node* leaf = (leaf_node*)(node);

        for (size_type i = upd.upd_begin; i != upd.upd_end; ++i) {
            switch (updates[i].type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (!key_equal(leaf->slotkey[in], updates[i].data)) {
                    BTREE_ASSERT(in < leaf->slotuse);
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                        result->slotuse = out;
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
                        result->slotuse = out;
                        result = leaves[++leaf_number];
                        out = 0;
                    }
                }
                result->slotkey[out++] = updates[i].data;

                if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                    result->slotuse = out;
                    result = leaves[++leaf_number];
                    out = 0;
                }
                break;
            }
        } 
        while (in < leaf->slotuse) {
            result->slotkey[out++] = leaf->slotkey[in++];

            if (out == designated_leafsize && hasNextLeaf(leaf_number) && in < leaf->slotuse) {
                result->slotuse = out;
                result = leaves[++leaf_number];
                out = 0;
            }
        }
        result->slotuse = out;

        BTREE_PRINT(" to leaf " << leaf_number << ": " << out);


        BTREE_PRINT(", writing range [" << rank << ", " << ((leaf_number-(rank/designated_leafsize))*designated_leafsize)
            + out << ") into " << leaves.size() << " leaves " << std::endl);
    }

    inline bool hasNextLeaf(size_type leaf_count) const {
        return leaf_count+1 < leaves.size();
    }

    void update_leaf_in_current_tree(node*& node, key_type& router, min_key_type& min_key, const UpdateDescriptor& upd) {
        width_type in = 0; // existing key to read
        width_type out = 0; // position where to write

        leaf_node* result = static_cast<leaf_node*>(spare_leaf);
        const leaf_node* leaf = static_cast<leaf_node*>(node);

        BTREE_PRINT("Updating leaf from " << node << " to " << result);

        for (size_type i = upd.upd_begin; i != upd.upd_end; ++i) {

            switch (updates[i].type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (!key_equal(leaf->slotkey[in], updates[i].data)) {
                    BTREE_ASSERT(in < leaf->slotuse);
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
        set_min_element(min_key, result, result->slotuse);
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


#endif // _SEQUENTIAL_BTREE_H_
