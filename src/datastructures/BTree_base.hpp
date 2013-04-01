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

#include "tbb/atomic.h"


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

#ifndef BRANCHING_PARAMETER_B
#define BRANCHING_PARAMETER_B 8
#endif
#ifndef LEAF_PARAMETER_K
#define LEAF_PARAMETER_K 1024
#endif

enum OperationBatchType {INSERTS_ONLY=1, DELETES_ONLY=-1, INSERTS_AND_DELETES=2};

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
    /// If true, the tree will self verify it's invariants after each insert()
    /// or erase(). The header must have been compiled with BTREE_DEBUG defined.
    static const bool   selfverify = false;

    /// Configure nodes to have a fixed size of X cache lines. 
    static const unsigned int leafparameter_k = BTREE_MAX( 8, LEAF_PARAMETER_K );
    static const unsigned int branchingparameter_b = BTREE_MAX( 8, BRANCHING_PARAMETER_B );
};

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
class btree_base {
private:
    btree_base(const btree_base &){}    // do not copy
    void operator=(const btree_base&){} // do not copy
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

    typedef signed long weightdelta_type;
    typedef typename _Alloc::template rebind<weightdelta_type>::other weightdelta_listalloc_type;

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

    explicit inline btree_base(const allocator_type &alloc=allocator_type())
        : root(NULL), allocator(alloc)
    { }

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

    struct UpdateDescriptor {
        bool rebalancing_needed;
        size_type weight;
        size_type upd_begin;
        size_type upd_end;
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

protected:
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

protected:
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

protected:
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


    static inline bool hasUpdates(const UpdateDescriptor& update) {
        return update.upd_begin != update.upd_end;
    }

    inline weightdelta_type getWeightDelta(const size_type upd_begin, const size_type upd_end) const {
        if (batch_type == INSERTS_AND_DELETES) {
            return weightdelta[upd_end]-weightdelta[upd_begin];
        } else {
            return (upd_end - upd_begin) * batch_type;
        }
    }

    inline bool scheduleSubTreeUpdate(const width_type i, const size_type& weight, const size_type minweight,
            const size_type maxweight, const size_type subupd_begin, const size_type subupd_end, UpdateDescriptor* subtree_updates) const {
        subtree_updates[i].upd_begin = subupd_begin;
        subtree_updates[i].upd_end = subupd_end;
        subtree_updates[i].weight = weight + getWeightDelta(subupd_begin, subupd_end);
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
            return 1; // hack to prevent division by zero when the root node runs empty
                      // Instead of a single leaf we will now get an inner node pointing to the single leaf
        } else {
            return ceil( log((8.0*n)/(5.0*traits::leafparameter_k)) / log(traits::branchingparameter_b) );
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

protected:

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


    template<class leaf_list>
    width_type create_subtree_from_leaves(inner_node_data& slot, const width_type old_slotuse, const bool reuse_node, const level_type level, size_type rank_begin, size_type rank_end, const leaf_list& leaves) {
        BTREE_ASSERT(rank_end - rank_begin > 0);
        BTREE_PRINT("Creating tree on level " << level << " for range [" << rank_begin << ", " << rank_end << ")" << std::endl);

        if (level == 0) { // reached leaf level
            // Just re-use the pre-alloced and filled leaf
            leaf_node* result = leaves[rank_begin / designated_leafsize];
            result->slotuse = rank_end - rank_begin;
            set_min_element(slot, result);
            update_router(slot.slotkey, result->slotkey[result->slotuse-1]);
            slot.childid = result;
            return 1;
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
                result = allocate_inner(level);
                result->slotuse = new_slotuse;
                slot.childid = result;
            }
            BTREE_ASSERT(new_slotuse <= innerslotmax);

            size_type rank = rank_begin;
            for (width_type i = old_slotuse; i < new_slotuse; ++i) {
                const size_type weight = (i != new_slotuse-1) ? designated_treesize : (rank_end - rank);
                result->slot[i].weight = weight;
                create_subtree_from_leaves(result->slot[i], 0, false, level-1, rank, rank+weight, leaves);
                rank += weight;
            }
            set_min_element(slot, result);
            update_router(slot.slotkey, result->slot[new_slotuse-1].slotkey);

            return subtrees;
         }
    }


protected:
    /// Other small statistics about the B+ tree
    tree_stats  stats;

    /// Pointer to the B+ tree's root node, either leaf or inner node
    node*       root;

    /// Key comparison object. More comparison functions are generated from
    /// this < relation.
    key_compare key_less;

    /// Memory allocator.
    allocator_type allocator;

    // Currently running updates
    const Operation<key_type>* updates;
    // Classifies the set of runninng updates
    OperationBatchType batch_type;

    // Weight delta of currently running updates
    typedef std::vector<weightdelta_type, weightdelta_listalloc_type> weightdelta_list;
    weightdelta_list weightdelta;

};