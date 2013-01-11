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
#include <istream>
#include <iostream>
#include <ostream>
#include <memory>
#include <cstddef>
#include <assert.h>
#include <array>
#include <unistd.h>
#include <cmath>
#include <bitset>

// *** Debugging Macros
#ifdef BTREE_DEBUG
#define BTREE_ASSERT(x)         do { assert(x); } while(0)
#else
#define BTREE_ASSERT(x)         do { } while(0)
#endif

//#define BTREE_PRINT(x)          do { } while(0)
#define BTREE_PRINT(x)          do { (std::cout << x); } while(0)


/// The maximum of a and b. Used in some compile-time formulas.
#define BTREE_MAX(a,b)          ((a) < (b) ? (b) : (a))

// Widht of nodes given as number of cache-lines
#ifndef NODE_WIDTH
#define NODE_WIDTH 8
#endif
#ifndef DCACHE_LINESIZE
#define DCACHE_LINESIZE 64
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
    static const int    leafparameter_k = BTREE_MAX( 8, NODE_WIDTH * DCACHE_LINESIZE / (sizeof(_Key)) );
    static const int    branchingparameter_b = BTREE_MAX( 8, NODE_WIDTH * DCACHE_LINESIZE / (sizeof(_Key) + sizeof(void*)) );
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

    /// Size type used to count keys
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

    /// The header structure of each node in-memory. This structure is extended
    /// by inner_node or leaf_node.
    struct node {
        /// Level in the b-tree, if level == 0 -> leaf node
        level_type level;

        /// Number of key slotuse use, so number of valid children or data
        /// pointers
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

        /// Keys of children or data pointers
        key_type        slotkey[innerslotmax];

        /// Pointers to children
        node*           childid[innerslotmax+1];

        size_type       weight[innerslotmax+1];

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

        inline tree_stats()
            : itemcount(0), leaves(0), innernodes(0)
        { }

        /// Return the total number of nodes
        inline size_type nodes() const {
            return innernodes + leaves;
        }

        /// Return the average fill of leaves
        inline double avgfill_leaves() const {
            return static_cast<double>(itemcount) / (leaves * leafslotmax);
        }
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
    size_type updates_size;

    // Weight delta of currently running updates
    std::vector<size_type> weightdelta;

    // Leaves used during the current reconstruction effort
    std::vector<leaf_node*> leaves;


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
    }

private:
    // *** Convenient Key Comparison Functions Generated From key_less

    /// True if a <= b ? constructed from key_less()
    inline bool key_lessequal(const key_type &a, const key_type &b) const {
        return !key_less(b, a);
    }

    /// True if a >= b ? constructed from key_less()
    inline bool key_greaterequal(const key_type &a, const key_type &b) const {
        return !key_less(a, b);
    }

    /// True if a == b ? constructed from key_less(). This requires the <
    /// relation to be a total order, otherwise the B+ tree cannot be sorted.
    inline bool key_equal(const key_type &a, const key_type &b) const {
        return !key_less(a, b) && !key_less(b, a);
    }

private:
    // *** Node Object Allocation and Deallocation Functions

    /// Return an allocator for leaf_node objects
    typename leaf_node::alloc_type leaf_node_allocator() {
        return typename leaf_node::alloc_type(allocator);
    }

    /// Return an allocator for inner_node objects
    typename inner_node::alloc_type inner_node_allocator() {
        return typename inner_node::alloc_type(allocator);
    }

    /// Allocate and initialize a leaf node
    inline leaf_node* allocate_leaf() {
        leaf_node *n = new (leaf_node_allocator().allocate(1)) leaf_node();
        n->initialize();
        stats.leaves++;
        return n;
    }

    /// Allocate and initialize an inner node
    inline inner_node* allocate_inner(level_type level) {
        inner_node *n = new (inner_node_allocator().allocate(1)) inner_node();
        n->initialize(level);
        stats.innernodes++;
        return n;
    }

    /// Correctly free either inner or leaf node, destructs all contained key
    /// and value objects
    inline void free_node(node *n) {
        if (n->isleafnode()) {
            leaf_node *ln = static_cast<leaf_node*>(n);
            typename leaf_node::alloc_type a(leaf_node_allocator());
            a.destroy(ln);
            a.deallocate(ln, 1);
            stats.leaves--;
        } else {
            inner_node *in = static_cast<inner_node*>(n);
            typename inner_node::alloc_type a(inner_node_allocator());
            a.destroy(in);
            a.deallocate(in, 1);
            stats.innernodes--;
        }
    }

public:
    // *** Fast Destruction of the B+ Tree

    /// Frees all key/data pairs and all nodes of the tree
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

            for (width_type slot = 0; slot <= innernode->slotuse; ++slot) {
                clear_recursive(innernode->childid[slot]);
            }
        }
        free_node(n);
    }

public:
    // *** Access Functions to the Item Count

    /// Return the number of key/data pairs in the B+ tree
    inline size_type size() const {
        return stats.itemcount;
    }

    /// Returns true if there is at least one key/data pair in the B+ tree
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
        setOperationsAndComputeWeightDelta(_updates);
        size_type new_size = stats.itemcount = size() + weightdelta[updates_size];
        
        if (new_size == 0) {
            BTREE_PRINT("Update clears tree" << std::endl);
            clear(); // Tree will become empty. Just finish early.
            return;
        } 
        if (root == NULL) {
            root = allocate_leaf();
        }
        bool rebuild_needed = num_optimal_levels(new_size) != root->level;

        if (rebuild_needed) {
            allocate_new_nodes(new_size);
        }
        key_type router; // unused on this level
        update(root, router, /*rank*/0, /*update_begin*/0, updates_size, rebuild_needed);

        if (rebuild_needed) {
            build_tree_from_nodes(root, router);
        }
        print_node(std::cout, root, 0, true);


        /*if (designated_level != ) { 
            // need to reconstruct

            if (designated_level == 0) {
                // write all remaining elements into a single leaf
                node* newroot = NULL;
                std::cout << "Reconstruct Leaf " << std::endl;
                create_subtree(root, newroot, designated_level, router, 0, new_size);

                clear_recursive(root);
                root = newroot;
            } else {
                // Reconstruct entire tree
                inner_node* newroot = allocate_inner(designated_level);

                size_type subtreesize = designated_subtreesize(designated_level);
                width_type subtrees = num_subtrees(new_size, subtreesize);
                width_type slotuse = newroot->slotuse = subtrees - 1;

                size_type rank = 0;
                for (width_type i = 0; i < slotuse; ++i) {
                    newroot->weight[i] = subtreesize;
                    create_subtree(root, newroot->childid[i], designated_level-1, newroot->slotkey[i], rank, rank+subtreesize);
                    rank += subtreesize;
                }
                create_subtree(root, newroot->childid[slotuse], designated_level-1, router, rank, new_size);
                newroot->weight[slotuse] = new_size - rank;

                clear_recursive(root);
                root = newroot;

                print_node(std::cout, root, 0, true);
            }
        }*/
        if (traits::selfverify) {
            verify();
        }
    }

private:

    void setOperationsAndComputeWeightDelta(const std::vector<Operation<key_type>>& _updates) {
        updates = _updates.data();
        updates_size = _updates.size();

        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)
        weightdelta.clear();
        weightdelta.reserve(updates_size+1);
        auto result = weightdelta.begin();

        long val = 0; // exclusive prefix sum
        *result++ = val;
        for (size_type i = 0; i < updates_size; ++i) {
            *result++ = val = val + (updates[i].type == Operation<key_type>::INSERT ? 1 : -1);
        }
    }

    void allocate_new_nodes(size_type n) {
        BTREE_PRINT("Allocating new nodes for tree of size " << n << std::endl);
        size_type leaf_count = num_subtrees(n, designated_leafsize);
        leaves.resize(leaf_count);
        for (size_type i = 0; i < leaf_count; ++i) {
            leaves[i] = allocate_leaf();
        }
    }

    void build_tree_from_nodes(node*& new_tree, key_type& router) {
        BTREE_PRINT("Building new tree with " << leaves.size() << " leaves" << std::endl);
        BTREE_ASSERT(leaves.size() > 0);

        if (leaves.size() == 1) {
            new_tree = leaves[0];

        } else {
            // FIXME: needs to handle all different tree sizes not just leaves
            inner_node* result = allocate_inner(1);

            width_type slotuse = result->slotuse = leaves.size()-1;

            for (int i=0; i < slotuse; ++i) {
                result->childid[i] = leaves[i];
                result->slotkey[i] = leaves[i]->slotkey[leaves[i]->slotuse-1];
                result->weight[i] = leaves[i]->slotuse;
            }
            result->childid[slotuse] = leaves[slotuse];
            router = leaves[slotuse]->slotkey[leaves[slotuse]->slotuse];
            result->weight[slotuse] = leaves[slotuse]->slotuse;

            new_tree = result;
        }
    }

    void update(node*& node, key_type& router, const size_type rank, const size_type upd_begin, const size_type upd_end, const bool rewrite_subtree) {
        BTREE_PRINT("Applying updates [" << upd_begin << ", " << upd_end << ") to " << node << std::endl);

        if (node->isleafnode()) {
            if (rewrite_subtree) {
                write_updated_leaf_to_new_tree(node, rank, upd_begin, upd_end);
            } else {
                update_leaf_in_current_tree(node, router, upd_begin, upd_end);
            }

        } else {
            inner_node* inner = static_cast<inner_node*>(node);
            const level_type subtree_level = inner->level - 1;
            size_type subtree_rank = rank;

            // Distribute operations and find out which subtrees need rebalancing
            size_type subupd_begin = upd_begin;
            for (width_type i = 0; i < inner->slotuse; ++i) {
                size_type subupd_end = find_lower(subupd_begin, upd_end, inner->slotkey[i]);

                const size_type new_weight = inner->weight[i] = inner->weight[i] + weightdelta[subupd_end] - weightdelta[subupd_begin];

                if (subupd_begin != subupd_end) {
                    const bool rebalance_needed = new_weight < minweight(subtree_level) || new_weight > maxweight(subtree_level);
                    if (rebalance_needed) {
                        BTREE_PRINT("Rebalancing needed (but not implemented) " << std::endl);
                        // See root level for how to do it.
                    } else {
                        update(inner->childid[i], inner->slotkey[i], subtree_rank, subupd_begin, subupd_end, rebalance_needed);
                    }
                }
                subtree_rank += new_weight; 
                subupd_begin = subupd_end;
            }

            if (subupd_begin != upd_end) {
                const size_type new_weight = inner->weight[inner->slotuse] = inner->weight[inner->slotuse] + weightdelta[upd_end] - weightdelta[subupd_begin];
                const bool rebalance_needed = new_weight < minweight(subtree_level) || new_weight > maxweight(subtree_level);

                if (rebalance_needed) {
                    BTREE_PRINT("Rebalancing needed (but not implemented) " << std::endl);
                } else {
                    update(inner->childid[inner->slotuse], router, subtree_rank, subupd_begin, upd_end, rebalance_needed);
                }
            }

        }
        if (rewrite_subtree) {
            free_node(node);
        }

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

    static inline  width_type num_subtrees(size_type n, size_type subtreesize) {
        width_type num_subtrees = n / subtreesize;
        // Squeeze remaining elements into last subtree or place in own subtree? 
        // Choose what is closer to our designated subtree size
        size_type remaining = n % subtreesize;
        num_subtrees += ( remaining > subtreesize-(subtreesize+remaining)/2 );
        return num_subtrees;
    }

    static inline level_type num_optimal_levels(size_type n) {
        return (n <= leafslotmax) ? 0 : ceil( log(((double) n)/traits::leafparameter_k) / log(traits::branchingparameter_b) );
    }

    void write_updated_leaf_to_new_tree(node*& node, const size_type rank, const size_type begin, const size_type end) {
        BTREE_PRINT("Rewriting updated leaf " << node << " starting with rank " << rank);

        size_type leaf_number = rank / designated_leafsize;
        width_type offset_in_leaf = rank % designated_leafsize;

        width_type in = 0; // existing key to read
        width_type out = offset_in_leaf; // position where to write

        leaf_node* result = leaves[leaf_number];
        const leaf_node* leaf = static_cast<leaf_node*>(node);

        for (size_type i = begin; i != end; ++i) {
            switch (updates[i].type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (!key_equal(leaf->slotkey[in], updates[i].data)) {
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

        BTREE_PRINT(" as range [" << rank << ", " << (leaf_number-rank/designated_leafsize*designated_leafsize)
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
        while (in < leaf->slotuse) {
            result->slotkey[out++] = leaf->slotkey[in++];
        }
        result->slotuse = out;
        router = result->slotkey[out-1]; 

        BTREE_PRINT(": size " << leaf->slotuse << " -> " << result->slotuse << std::endl);

        spare_leaf = node;
        node = result;
    }


#ifdef BTREE_DEBUG
    /// Recursively descend down the tree and print out nodes.
    static void print_node(std::ostream &os, const node* node, level_type depth=0, bool recursive=false) {
        for(level_type i = 0; i < depth; i++) os << "  ";
        os << "node " << node << " level " << node->level << " slotuse " << node->slotuse << std::endl;

        if (node->isleafnode()) {
            const leaf_node *leafnode = static_cast<const leaf_node*>(node);
            for(level_type i = 0; i < depth; i++) os << "  ";

            for (width_type slot = 0; slot < leafnode->slotuse; ++slot) {
                os << leafnode->slotkey[slot] << "  "; // << "(data: " << leafnode->slotdata[slot] << ") ";
            }
            os << std::endl;
        } else {
            const inner_node *innernode = static_cast<const inner_node*>(node);
            for(level_type i = 0; i < depth; i++) os << "  ";

            for (width_type slot = 0; slot < innernode->slotuse; ++slot) {
                os << "(" << innernode->childid[slot] << ") " << innernode->slotkey[slot] << " ";
            }
            os << "(" << innernode->childid[innernode->slotuse] << ")" << std::endl;

            if (recursive) {
                for (width_type slot = 0; slot < innernode->slotuse + 1; ++slot) {
                    print_node(os, innernode->childid[slot], depth + 1, recursive);
                }
            }
        }
    }
#endif

public:
    // *** Verification of B+ Tree Invariants

    /// Run a thorough verification of all B+ tree invariants. The program
    /// aborts via assert() if something is wrong.
    void verify() const {
        key_type minkey, maxkey;
        tree_stats vstats;

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
        //BTREE_PRINT("verifynode " << n << std::endl);

        if (n->isleafnode()) {
            const leaf_node *leaf = static_cast<const leaf_node*>(n);

            assert( leaf == root || leaf->slotuse >= minweight(leaf->level) );
            assert( leaf->slotuse <= maxweight(leaf->level) );

            for(width_type slot = 0; slot < leaf->slotuse - 1; ++slot) {
                assert(key_lessequal(leaf->slotkey[slot], leaf->slotkey[slot + 1]));
            }

            *minkey = leaf->slotkey[0];
            *maxkey = leaf->slotkey[leaf->slotuse - 1];

            vstats.leaves++;
            vstats.itemcount += leaf->slotuse;
        }
        else { 
            const inner_node *inner = static_cast<const inner_node*>(n);
            vstats.innernodes++;

            size_type weight = 0;
            assert( inner == root || weight >= minweight(inner->level) );
            assert( weight <= maxweight(inner->level) );

            for(width_type slot = 0; slot < inner->slotuse - 1; ++slot) {
                assert(key_lessequal(inner->slotkey[slot], inner->slotkey[slot + 1]));
            }

            for(width_type slot = 0; slot <= inner->slotuse; ++slot) {
                const node *subnode = inner->childid[slot];
                key_type subminkey = key_type();
                key_type submaxkey = key_type();

                assert(subnode->level + 1 == inner->level);

                size_type itemcount = vstats.itemcount;
                verify_node(subnode, &subminkey, &submaxkey, vstats);

                assert(inner->weight[slot] == vstats.itemcount - itemcount);

                //BTREE_PRINT("verify subnode " << subnode << ": " << subminkey << " - " << submaxkey << std::endl);

                if (slot == 0) {
                    *minkey = subminkey;
                } else {
                    assert(key_greaterequal(subminkey, inner->slotkey[slot-1]));
                }

                if (slot == inner->slotuse) {
                    *maxkey = submaxkey;
                } else {
                    assert(key_equal(inner->slotkey[slot], submaxkey));
                }
            }
        }
    }
};


#endif // _BTREE_H_
