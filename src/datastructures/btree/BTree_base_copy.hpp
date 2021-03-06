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

/* ######## FIXME ###########
 *   Exact copy of BTree_base but used differently as we don't want that COMPUTE_PARETO_MIN is defined. 
 *   We need to make that configurable as template parameter so that we can drop the ugly copy here. 
 */
#include "BTree_base.hpp"

#ifndef _BASE_COPY_BTREE_COMMON_H_
#define _BASE_COPY_BTREE_COMMON_H_

template <typename _Key,
        typename _Compare,
        typename _Traits,
        typename _Alloc>
class btree_base_copy {
private:
    btree_base_copy(const btree_base_copy &){}    // do not copy
    void operator=(const btree_base_copy&){} // do not copy
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

protected:

    typedef btree_base_copy<_Key, _Compare, _Traits, _Alloc> btree_self_type;

    explicit inline btree_base_copy(const allocator_type &alloc=allocator_type())
        : root(NULL), allocator(alloc) { 
    }

    inline ~btree_base_copy() {
        clear();
    }

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
    typedef std::array<UpdateDescriptor, innerslotmax> UpdateDescriptorArray;

    static size_type minweight(const level_type level) {
        return maxweight(level) / 4; // when changing: also modify direct computations!
    }

    static size_type maxweight(const level_type level) {
        return weight(level);
    }

    static size_type midweight(const level_type level) {
        return weight(level) * 0.625 /* which is 5/8*/;
    }

    static size_type weight(const level_type level) {
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

    typedef signed long weightdelta_type;
    typedef typename _Alloc::template rebind<weightdelta_type>::other weightdelta_listalloc_type;
    typedef std::vector<weightdelta_type, weightdelta_listalloc_type> weightdelta_list;

    struct PaddedAtomic : public tbb::atomic<leaf_node*> {
        //char pad[DCACHE_LINESIZE - sizeof(tbb::atomic<leaf_node*>) % DCACHE_LINESIZE];
    };

    typedef typename _Alloc::template rebind<PaddedAtomic>::other padded_leaf_list_alloc_type;
    typedef std::vector<PaddedAtomic, padded_leaf_list_alloc_type> padded_leaf_list; 

    typedef typename _Alloc::template rebind<leaf_node*>::other leaf_list_alloc_type;
    typedef std::vector<leaf_node*, leaf_list_alloc_type> leaf_list; 

    typedef typename std::vector<Operation<key_type>> local_update_list;

    struct ThreadLocalLSData {
        weightdelta_list weightdelta;
        leaf_list leaves;
        leaf_node* spare_leaf;
        inner_node* spare_inner;
        UpdateDescriptorArray subtree_updates_per_level[MAX_TREE_LEVEL];
        local_update_list local_updates;
        btree_self_type& btree;

        ThreadLocalLSData(btree_self_type& _btree) : btree(_btree) {
            weightdelta.reserve(LARGE_ENOUGH_FOR_MOST);
            local_updates.reserve(LARGE_ENOUGH_FOR_MOST);
            leaves.reserve(LARGE_ENOUGH_FOR_MOST);

            spare_leaf = btree.allocate_leaf_without_count();
            spare_inner = btree.allocate_inner_without_count(0);
        }

        ~ThreadLocalLSData() {
            btree.free_node_without_count(spare_inner);
            btree.free_node_without_count(spare_leaf);
        }

    };

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

    inline inner_node* allocate_inner_without_count(level_type level) {
        inner_node *n = new (inner_node_allocator().allocate(1)) inner_node();
        n->initialize(level);
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

    inline void free_node_without_count(node *n) {
        if (n->isleafnode()) {
            leaf_node *ln = static_cast<leaf_node*>(n);
            typename leaf_node::alloc_type a(leaf_node_allocator());
            a.destroy(ln);
            a.deallocate(ln, 1);
        } else {
            inner_node *in = static_cast<inner_node*>(n);
            typename inner_node::alloc_type a(inner_node_allocator());
            a.destroy(in);
            a.deallocate(in, 1);
        }
    }

public:
    // *** Fast Destruction of the B+ Tree

    inline void clear() {
        if (root) {
            clear_recursive(root);
            stats.itemcount = 0;
            root = NULL;
        }
        BTREE_ASSERT(stats.itemcount == 0);
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
            return tls_data->weightdelta[upd_end]-tls_data->weightdelta[upd_begin];
        } else {
            return (upd_end - upd_begin) * batch_type;
        }
    }

    inline bool scheduleSubTreeUpdate(const width_type i, const size_type& weight, const size_type minweight,
            const size_type maxweight, const size_type subupd_begin, const size_type subupd_end, UpdateDescriptorArray& subtree_updates) const {
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
        const size_type num_to_round = midweight(level-1);

        const size_type remaining = num_to_round % designated_leafsize;
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
        const size_type remaining = n % subtreesize;
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
            //if ((leaf != root && !(leaf->slotuse >= minweight(leaf->level))) || !( leaf->slotuse <= maxweight(leaf->level))) {
             //   std::cout << leaf->slotuse << " min " << minweight(0) << " max " <<maxweight(0) << std::endl;
              //  print_node(leaf);
            //}
            //assert( leaf == root || leaf->slotuse >= minweight(leaf->level) );
            //assert( leaf->slotuse <= maxweight(leaf->level) );

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

                //if ((inner != root && !(inner->slot[slot].weight >= minweight(inner->level-1))) || !(inner->slot[slot].weight <= maxweight(inner->level-1))) {
                //    std::cout << inner->slot[slot].weight << " min " << minweight(inner->level-1) << " max " <<maxweight(inner->level-1) << std::endl;
                //    print_node(inner, 0, true);
                //}
                //assert( inner == root || inner->slot[slot].weight >= minweight(inner->level-1) );
                //assert( inner->slot[slot].weight <= maxweight(inner->level-1) );

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

    template<class leaf_list> 
    width_type create_subtree_from_leaves(inner_node_data& slot, const width_type old_slotuse, const bool reuse_node, const level_type level, size_type rank_begin, size_type rank_end, const leaf_list& leaves) {
        BTREE_ASSERT(rank_end - rank_begin > 0);
        BTREE_PRINT("Creating tree on level " << level << " for range [" << rank_begin << ", " << rank_end << ")" << std::endl);

        if (level == 0) { // reached leaf level
            // Just re-use the pre-alloced and filled leaf
            leaf_node* result = leaves[rank_begin / designated_leafsize];
            result->slotuse = rank_end - rank_begin;
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
            for (width_type i = old_slotuse; i < new_slotuse-1; ++i) {
                result->slot[i].weight = designated_treesize;
                create_subtree_from_leaves(result->slot[i], 0, false, level-1, rank, rank+designated_treesize, leaves);
                rank += designated_treesize;
            }
            result->slot[new_slotuse-1].weight = rank_end - rank;
            create_subtree_from_leaves(result->slot[new_slotuse-1], 0, false, level-1, rank, rank_end, leaves);

            update_router(slot.slotkey, result->slot[new_slotuse-1].slotkey);

            return subtrees;
         }
    }

    inline leaf_list& get_resized_leaves_array(size_type n) {
        BTREE_PRINT("Allocating " << num_subtrees(n, designated_leafsize) << " new  nodes for tree of size " << n << std::endl);
        const size_type leaf_count = num_subtrees(n, designated_leafsize);

        auto& ret = tls_data->leaves;
        ret.clear();
        ret.resize(leaf_count, NULL);
        return ret;
    }

    template<class leaf_list> 
    void rewrite(node* const source_node, const size_type rank, const size_type upd_begin, const size_type upd_end, leaf_list& leaves) {
        if (source_node->isleafnode()) {
            write_updated_leaf_to_new_tree(source_node, 0, rank, upd_begin, upd_end, leaves, true);
        } else {
            inner_node* inner = static_cast<inner_node*>(source_node);
            auto& subtree_updates = tls_data->subtree_updates_per_level[inner->level];
            
            // Distribute operations and find out which subtrees need rebalancing
            const width_type last = inner->slotuse-1;
            size_type subupd_begin = upd_begin;
            for (width_type i = 0; i < last; ++i) {
                size_type subupd_end = find_lower(subupd_begin, upd_end, inner->slot[i].slotkey);
                scheduleSubTreeUpdate(i, inner->slot[i].weight, 0, 0, subupd_begin, subupd_end, subtree_updates);
                subupd_begin = subupd_end;
            } 
            scheduleSubTreeUpdate(last, inner->slot[last].weight, 0, 0, subupd_begin, upd_end, subtree_updates);

            rewriteSubTreesInRange(inner, 0, inner->slotuse, rank, subtree_updates, leaves);
        }
        free_node(source_node);
    }

    void update(inner_node_data& slot, const size_type upd_begin, const size_type upd_end) {
        BTREE_PRINT("Applying updates [" << upd_begin << ", " << upd_end << ") to " << slot.childid << " on level " << slot.childid->level << std::endl);

        if (slot.childid->isleafnode()) {
            update_leaf_in_current_tree(slot, upd_begin, upd_end);

        } else {
            inner_node* const inner = static_cast<inner_node*>(slot.childid);
            const size_type max_weight = maxweight(inner->level-1);
            const size_type min_weight = max_weight / 4;
            auto& subtree_updates = tls_data->subtree_updates_per_level[inner->level];

            bool rebalancing_needed = false;
            // Distribute operations and find out which subtrees need rebalancing
            const width_type lastslot = inner->slotuse-1;
            const width_type slotuse = inner->slotuse;

            size_type subupd_begin = upd_begin;
            for (width_type i = 0; i < lastslot; ++i) {
                size_type subupd_end = find_lower(subupd_begin, upd_end, inner->slot[i].slotkey);
                rebalancing_needed |= scheduleSubTreeUpdate(i, inner->slot[i].weight, min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                subupd_begin = subupd_end;
            } 
            rebalancing_needed |= scheduleSubTreeUpdate(lastslot, inner->slot[lastslot].weight, min_weight, max_weight, subupd_begin, upd_end, subtree_updates);

            // If no rewrite session is starting on this node, then just push down all updates
            if (!rebalancing_needed) {
                updateSubTreesInRange(inner, 0, slotuse, subtree_updates);
                update_router(slot.slotkey, inner->slot[lastslot].slotkey);
            } else {
                BTREE_PRINT("Rewrite session started for " << inner << " on level " << inner->level << std::endl);
                // Need to perform rebalancing.
                size_type designated_treesize = designated_subtreesize(inner->level);

                inner_node* const result = tls_data->spare_inner;
                result->initialize(inner->level);

                width_type in = 0; // current slot in input tree
                width_type out = 0;

                while (in < slotuse) {
                    width_type rebalancing_range_start = in;
                    size_type weight_of_defective_range = 0;
                    bool openrebalancing_region = false;

                    // Find non-empty consecutive run of subtrees that need rebalancing
                    while (in < slotuse && (subtree_updates[in].rebalancing_needed 
                            || (openrebalancing_region && weight_of_defective_range != 0 && weight_of_defective_range < designated_treesize))) {
                        openrebalancing_region = true;
                        subtree_updates[in].rebalancing_needed = true;
                        weight_of_defective_range += subtree_updates[in].weight;
                        ++in;
                    }
                    if (openrebalancing_region) {
                        if (weight_of_defective_range == 0) {
                            BTREE_PRINT("Deleting entire subtree range" << std::endl);
                            for (width_type i = rebalancing_range_start; i < in; ++i) {
                                clear_recursive(inner->slot[i].childid);
                            }
                        } else {
                            BTREE_PRINT("Rewrite session started on level " << inner->level << " of " << height() << " for subtrees [" << rebalancing_range_start << "," << in <<") of total weight " << weight_of_defective_range << std::endl);
                            auto& leaves = get_resized_leaves_array(weight_of_defective_range);
                            rewriteSubTreesInRange(inner, rebalancing_range_start, in, 0, subtree_updates, leaves);

                            result->slotuse = out;
                            inner_node_data fake_slot;
                            fake_slot.childid = result;
                            out += create_subtree_from_leaves(fake_slot, out, /*write into result node*/ true, result->level, 0, weight_of_defective_range, leaves);
                            BTREE_ASSERT(result == fake_slot.childid);                            
                        }
                    } else {
                        BTREE_PRINT("Copying " << in << " to " << out << " " << inner->slot[in].childid << std::endl);
                        assert(!subtree_updates[in].rebalancing_needed);
                        result->slot[out] = inner->slot[in];
                        result->slot[out].weight = subtree_updates[in].weight;
                        // reuse weight variable to store the new index of the current tree; we will update it in a second pass
                        subtree_updates[in].weight = out;
                        ++out;
                        ++in;
                    }
                }
                result->slotuse = out;

                // Setup spare_inner to be used by nested rebalancing operations
                tls_data->spare_inner = static_cast<inner_node*>(slot.childid);

                // In a second pass, apply updates to subtrees not needing rebalancing on THIS level
                // (but which may need to be updated recursively and therefore require the spare_inner)
                for (width_type i = 0; i < slotuse; ++i) {
                    if (!subtree_updates[i].rebalancing_needed && hasUpdates(subtree_updates[i])) {
                        const width_type mapped_slot = subtree_updates[i].weight;
                        update(result->slot[mapped_slot], subtree_updates[i].upd_begin, subtree_updates[i].upd_end);
                    }
                }
                result->slotuse = out;
                update_router(slot.slotkey, result->slot[result->slotuse-1].slotkey);
                slot.childid = result;
            }
        }
    }

    template<class leaf_list> 
    inline void rewriteSubTreesInRange(inner_node* node, const width_type begin, const width_type end, const size_type rank, const UpdateDescriptorArray& subtree_updates, leaf_list& leaves) {
        size_type subtree_rank = rank;
        for (width_type i = begin; i < end; ++i) {
            if (subtree_updates[i].weight == 0) {
                clear_recursive(node->slot[i].childid);
            } else {
                rewrite(node->slot[i].childid, subtree_rank, subtree_updates[i].upd_begin, subtree_updates[i].upd_end, leaves);
            }
            subtree_rank += subtree_updates[i].weight;
        }
    }

    inline void updateSubTreesInRange(inner_node* node, const width_type begin, const width_type end, const UpdateDescriptorArray& subtree_updates) {
        for (width_type i = begin; i < end; ++i) {
            if (hasUpdates(subtree_updates[i])) {
                update(node->slot[i], subtree_updates[i].upd_begin, subtree_updates[i].upd_end);
                node->slot[i].weight = subtree_updates[i].weight;
            }
        }
    }
   
    template<class leaf_list> 
    inline void write_updated_leaf_to_new_tree(const node* const node, width_type in, const size_type rank, const size_type upd_begin, const size_type upd_end, leaf_list& leaves, const bool is_last) {
        BTREE_PRINT("Rewriting updated leaf " << node << " starting with rank " << rank << " and in " << in << " using upd range [" << upd_begin << "," << upd_end << ")");

        size_type leaf_number = rank / designated_leafsize;
        width_type offset_in_leaf = rank % designated_leafsize;

        if (leaf_number >= leaves.size()) {
            // elements are squeezed into the previous leaf
            leaf_number = leaves.size()-1; 
            offset_in_leaf = rank - leaf_number*designated_leafsize;
        }
        BTREE_PRINT(". From leaf " << leaf_number << ": " << offset_in_leaf);

        width_type out = offset_in_leaf; // position where to write

        leaf_node* result = getOrCreateLeaf(leaf_number, leaves);
        const leaf_node* const leaf = (leaf_node*)(node);
        const width_type in_slotuse = leaf->slotuse;

        for (size_type i = upd_begin; i != upd_end; ++i) {
            const Operation<key_type>& op = updates[i];

            switch (op.type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (key_less(leaf->slotkey[in], op.data)) {
                    BTREE_ASSERT(in < in_slotuse);
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number, leaves)) {
                        result = getOrCreateLeaf(++leaf_number, leaves);
                        out = 0;
                    }
                }
                ++in; // delete the element by jumping over it
                continue;
            case Operation<key_type>::INSERT:
                while(in < in_slotuse && key_less(leaf->slotkey[in], op.data)) {
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number, leaves)) {
                        result = getOrCreateLeaf(++leaf_number, leaves);
                        out = 0;
                    }
                }
                result->slotkey[out++] = op.data;

                if (out == designated_leafsize && hasNextLeaf(leaf_number, leaves)) {
                    result = getOrCreateLeaf(++leaf_number, leaves);
                    out = 0;
                }
                continue;
            }
        } 
        // Reached the end of the update range. Have to write the remaining elements
        size_type next_update = upd_end;
        while (in < in_slotuse && (is_last || key_less(leaf->slotkey[in], updates[next_update].data))) {
            result->slotkey[out++] = leaf->slotkey[in++];

            if (out == designated_leafsize && hasNextLeaf(leaf_number, leaves) && in < in_slotuse) {
                result = getOrCreateLeaf(++leaf_number, leaves);
                out = 0;
            }
        }
        BTREE_PRINT("   as range [" << rank << ", " << ((leaf_number-(rank/designated_leafsize))*designated_leafsize)
            + out << ") into " << leaves.size() << " leaves " << std::endl);
    }

    template<class leaf_list> 
    inline bool hasNextLeaf(const size_type leaf_count, const leaf_list& leaves) const {
        return leaf_count+1 < leaves.size();
    }


    inline leaf_node* getOrCreateLeaf(const size_type leaf_number, padded_leaf_list& leaves) {
        if (leaves[leaf_number] == NULL) {
            leaf_node* tmp = allocate_leaf();
            if (leaves[leaf_number].compare_and_swap(tmp, NULL) == NULL) {
                return tmp;
            } else {
                // Another thread installed the value, so throw away mine.
                free_node(tmp);
            }
        }
        return leaves[leaf_number];
    }

    inline leaf_node* getOrCreateLeaf(const size_type leaf_number, leaf_list& leaves) {
        if (leaves[leaf_number] == NULL) {
            leaves[leaf_number] = allocate_leaf();
        }
        return leaves[leaf_number];
    }

    inline void update_leaf_in_current_tree(inner_node_data& slot, const size_type upd_begin, const size_type upd_end) {
        const leaf_node* const leaf = static_cast<leaf_node*>(slot.childid);
        leaf_node* const result = tls_data->spare_leaf;

        width_type in = 0; // existing key to read
        width_type out = 0; // position where to write
        const width_type in_slotuse = leaf->slotuse;
        BTREE_PRINT("Updating leaf from " << leaf << " to " << result);

        for (size_type i = upd_begin; i != upd_end; ++i) {
            const Operation<key_type>& op = updates[i];

            switch (op.type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (key_less(leaf->slotkey[in], op.data)) {
                    BTREE_ASSERT(in < in_slotuse);
                    result->slotkey[out++] = leaf->slotkey[in++];
                }
                ++in; // delete the element by jumping over it
                continue;
            case Operation<key_type>::INSERT:
                while(in < in_slotuse && key_less(leaf->slotkey[in], op.data)) {
                    result->slotkey[out++] = leaf->slotkey[in++];
                }
                result->slotkey[out++] = op.data;
                continue;
            }
        }
        assert(leaf->slotuse <= leafslotmax);
        const size_t diff = in_slotuse - in;
        memcpy(result->slotkey + out, leaf->slotkey + in, sizeof(key_type)*diff);
        out += diff;
        assert(out <= leafslotmax);

        result->slotuse = out;
        update_router(slot.slotkey, result->slotkey[out-1]); 
        tls_data->spare_leaf = static_cast<leaf_node*>(slot.childid);
        slot.childid = result;

        BTREE_PRINT(": size " << leaf->slotuse << " -> " << result->slotuse << std::endl);
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

    ThreadLocalLSData* tls_data;

};

#endif
