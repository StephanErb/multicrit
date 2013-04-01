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

#include "BTree_base.hpp"



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
class btree : 
    #ifdef COMPUTE_PARETO_MIN
        public btree_base<_Key, _MinKey, _Compare, _Traits, _Alloc>
    #else
        public btree_base<_Key, _Compare, _Traits, _Alloc>
    #endif

 {

private:
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
    typedef typename base::node             node;
    typedef typename base::leaf_node        leaf_node;
    typedef typename base::inner_node       inner_node;
    typedef typename base::inner_node_data  inner_node_data;
    typedef typename base::tree_stats       tree_stats;
    typedef typename base::UpdateDescriptor UpdateDescriptor;


public:
    typedef typename base::traits           traits;

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

private:
    using base::root;
    using base::stats;
    using base::allocator;
    using base::weightdelta;
    using base::batch_type;
    using base::updates;

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

protected:
    // *** Tree Object Data Members

    /// Pointer to spare leaf used for merging.
    node*       spare_leaf;
    
    // Leaves created during the current reconstruction effort
    std::vector<leaf_node*> leaves;

    // For each level, one array to store the updates needed to push down to the individual subtrees
    std::vector<UpdateDescriptor*> subtree_updates_per_level;


public:
    // *** Constructors and Destructor

    /// Default constructor initializing an empty B+ tree with the standard key
    /// comparison function
    explicit inline btree(const allocator_type &alloc=allocator_type())
        : base(alloc)
    {
        spare_leaf = allocate_leaf_without_count();
        weightdelta.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
    }

    /// Frees up all used B+ tree memory pages
    inline ~btree() {
        clear();
        free_node(spare_leaf);
        for (auto updates : subtree_updates_per_level) {
            delete[] updates;
        }
    }


public:
    static std::string name() {
        return "Sequential BTree";
    }

public:

    template<typename T>
    void apply_updates(const T& _updates, const OperationBatchType _batch_type) {
        size_type new_size = setOperationsAndComputeWeightDelta(_updates, _batch_type);
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
        inner_node_data fake_slot;
        fake_slot.childid = root;

        if (rebuild_needed) {
            for (level_type i = root->level; i <= level; ++i) {
                subtree_updates_per_level.push_back(new UpdateDescriptor[innerslotmax]);
            }
            BTREE_PRINT("Root-level rewrite session started for new level " << level << std::endl);
            allocate_new_leaves(new_size);
            rewrite(fake_slot, /*rank*/0, upd);
            create_subtree_from_leaves(fake_slot, 0, false, level, 0, new_size, leaves);
        } else {
            update(fake_slot, /*rank*/0, upd);
        }
        root = fake_slot.childid;

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

private:

    template<typename T>
    inline size_type setOperationsAndComputeWeightDelta(const T& _updates, const OperationBatchType _batch_type) {
        updates = _updates.data();
        batch_type = _batch_type;

        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)

        if (_batch_type == INSERTS_AND_DELETES) {
            long val = 0; // exclusive prefix sum
            weightdelta[0] = val;
            for (size_type i = 0; i < _updates.size();) {
                val = val + updates[i].type;
				weightdelta[++i] = val;
            }
            return size() + weightdelta[_updates.size()];
        } else {
            // We can use the shortcut based on the 
            return size() + _updates.size() * batch_type;
        }
    }


    inline void allocate_new_leaves(size_type n) {
        BTREE_PRINT("Allocating " << num_subtrees(n, designated_leafsize) << " new  nodes for tree of size " << n << std::endl);
        size_type leaf_count = num_subtrees(n, designated_leafsize);
        leaves.clear();
        leaves.resize(leaf_count, NULL);
    }

    void rewrite(inner_node_data& slot, const size_type rank, const UpdateDescriptor& upd) {
        if (slot.childid->isleafnode()) {
            write_updated_leaf_to_new_tree(slot.childid, rank, upd);
        } else {
            inner_node* inner = static_cast<inner_node*>(slot.childid);
            const size_type min_weight = minweight(inner->level-1);
            const size_type max_weight = maxweight(inner->level-1);
            UpdateDescriptor* subtree_updates = subtree_updates_per_level[inner->level];
            
            // Distribute operations and find out which subtrees need rebalancing
            const width_type last = inner->slotuse-1;
            size_type subupd_begin = upd.upd_begin;
            for (width_type i = 0; i < last; ++i) {
                size_type subupd_end = find_lower(subupd_begin, upd.upd_end, inner->slot[i].slotkey);
                scheduleSubTreeUpdate(i, inner->slot[i].weight, min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                subupd_begin = subupd_end;
            } 
            scheduleSubTreeUpdate(last, inner->slot[last].weight, min_weight, max_weight, subupd_begin, upd.upd_end, subtree_updates);

            size_type subtree_rank = rank;
            for (width_type i = 0; i < inner->slotuse; ++i) {
                rewrite(inner->slot[i], subtree_rank, subtree_updates[i]);
                subtree_rank += subtree_updates[i].weight;
            }
        }
        free_node(slot.childid);
    }

    void update(inner_node_data& slot, const size_type rank, const UpdateDescriptor& upd) {
        BTREE_PRINT("Applying updates [" << upd.upd_begin << ", " << upd.upd_end << ") to " << _node << " on level " << _node->level << ". Rewrite = " << rewrite_subtree << std::endl);

        if (slot.childid->isleafnode()) {
            update_leaf_in_current_tree(slot, upd);

        } else {
            inner_node* const inner = static_cast<inner_node*>(slot.childid);
            const size_type min_weight = minweight(inner->level-1);
            const size_type max_weight = maxweight(inner->level-1);
            UpdateDescriptor* subtree_updates = subtree_updates_per_level[inner->level];

            bool rebalancing_needed = false;

            // Distribute operations and find out which subtrees need rebalancing
            const width_type last = inner->slotuse-1;
            size_type subupd_begin = upd.upd_begin;
            for (width_type i = 0; i < last; ++i) {
                size_type subupd_end = find_lower(subupd_begin, upd.upd_end, inner->slot[i].slotkey);
                rebalancing_needed |= scheduleSubTreeUpdate(i, inner->slot[i].weight, min_weight, max_weight, subupd_begin, subupd_end, subtree_updates);
                subupd_begin = subupd_end;
            } 
            rebalancing_needed |= scheduleSubTreeUpdate(last, inner->slot[last].weight, min_weight, max_weight, subupd_begin, upd.upd_end, subtree_updates);

            // If no rewrite session is starting on this node, then just push down all updates
            if (!rebalancing_needed) {
                updateSubTreesInRange(inner, 0, inner->slotuse, rank, subtree_updates);
                set_min_element(slot, inner);
                update_router(slot.slotkey, inner->slot[inner->slotuse-1].slotkey);
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
                                clear_recursive(inner->slot[i].childid);
                            }
                        } else {
                            BTREE_PRINT("Rewrite session started on level " << inner->level << " of " << height() << " for subtrees [" << rebalancing_range_start << "," << in <<") of total weight " << weight_of_defective_range << std::endl);
                            allocate_new_leaves(weight_of_defective_range);
                            rewriteSubTreesInRange(inner, rebalancing_range_start, in, 0, subtree_updates);

                            result->slotuse = out;
                            inner_node_data fake_slot;
                            fake_slot.childid = result;
                            out += create_subtree_from_leaves(fake_slot, out, /*write into result node*/ true, result->level, 0, weight_of_defective_range, leaves);
                            result = static_cast<inner_node*>(fake_slot.childid);                            
                        }
                    } else {
                        BTREE_PRINT("Copying " << in << " to " << out << " " << inner->childid[in] << std::endl);
                        result->slot[out] = inner->slot[in];
                        result->slot[out].weight = subtree_updates[in].weight;
                        if (hasUpdates(subtree_updates[in])) {
                            update(result->slot[out], /*unused rank*/ -1, subtree_updates[in]);
                        }
                        ++out;
                        ++in;
                    }
                }
                result->slotuse = out;
                set_min_element(slot, result);
                update_router(slot.slotkey, result->slot[out-1].slotkey);
                free_node(slot.childid);
                slot.childid = result;
            }
        }
    }

    inline void rewriteSubTreesInRange(inner_node* node, const width_type begin, const width_type end, const size_type rank, const UpdateDescriptor* subtree_updates) {
        size_type subtree_rank = rank;
        for (width_type i = begin; i < end; ++i) {
            if (subtree_updates[i].weight == 0) {
                clear_recursive(node->slot[i].childid);
            } else {
                rewrite(node->slot[i], subtree_rank, subtree_updates[i]);
            }
            subtree_rank += subtree_updates[i].weight;
        }
    }

    inline void updateSubTreesInRange(inner_node* node, const width_type begin, const width_type end, const size_type rank, const UpdateDescriptor* subtree_updates) {
        size_type subtree_rank = rank;
        for (width_type i = begin; i < end; ++i) {
            if (hasUpdates(subtree_updates[i])) {
                update(node->slot[i], subtree_rank, subtree_updates[i]);
            	node->slot[i].weight = subtree_updates[i].weight;
            }
            subtree_rank += subtree_updates[i].weight;
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

        leaf_node* result = getOrCreateLeaf(leaf_number);
        const leaf_node* leaf = (leaf_node*)(node);

        for (size_type i = upd.upd_begin; i != upd.upd_end; ++i) {
            switch (updates[i].type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (key_less(leaf->slotkey[in], updates[i].data)) {
                    BTREE_ASSERT(in < leaf->slotuse);
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                        result = getOrCreateLeaf(++leaf_number);
                        out = 0;
                    }
                }
                ++in; // delete the element by jumping over it
                break;
            case Operation<key_type>::INSERT:
                while(in < leaf->slotuse && key_less(leaf->slotkey[in], updates[i].data)) {
                    result->slotkey[out++] = leaf->slotkey[in++];

                    if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                        result = getOrCreateLeaf(++leaf_number);
                        out = 0;
                    }
                }
                result->slotkey[out++] = updates[i].data;

                if (out == designated_leafsize && hasNextLeaf(leaf_number)) {
                    result = getOrCreateLeaf(++leaf_number);
                    out = 0;
                }
                break;
            }
        } 
        while (in < leaf->slotuse) {
            result->slotkey[out++] = leaf->slotkey[in++];

            if (out == designated_leafsize && hasNextLeaf(leaf_number) && in < leaf->slotuse) {
                result = getOrCreateLeaf(++leaf_number);
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

    inline leaf_node* getOrCreateLeaf(const size_type leaf_number) {
        if (leaves[leaf_number] == NULL) {
            leaves[leaf_number] = allocate_leaf();
        }
        return leaves[leaf_number];
    }

    void update_leaf_in_current_tree(inner_node_data& slot, const UpdateDescriptor& upd) {
        leaf_node* result = static_cast<leaf_node*>(spare_leaf);
        const leaf_node* leaf = static_cast<leaf_node*>(slot.childid);

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

        for (size_type i = upd.upd_begin; i != upd.upd_end; ++i) {
            const Operation<key_type>& op = updates[i];

            switch (op.type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (key_less(leaf->slotkey[in], op.data)) {
                    BTREE_ASSERT(in < in_slotuse);
                    #ifdef COMPUTE_PARETO_MIN
                        local_min = leaf->slotkey[in].second_weight < local_min->second_weight ? &leaf->slotkey[in] : local_min;
                    #endif 
                    result->slotkey[out++] = leaf->slotkey[in++];
                }
                ++in; // delete the element by jumping over it
                break;
            case Operation<key_type>::INSERT:
                while(in < in_slotuse && key_less(leaf->slotkey[in], op.data)) {
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
        assert(leaf->slotuse <= leafslotmax);
        while (in < in_slotuse) {
            #ifdef COMPUTE_PARETO_MIN
                local_min = leaf->slotkey[in].second_weight < local_min->second_weight ? &leaf->slotkey[in] : local_min;
            #endif
            result->slotkey[out++] = leaf->slotkey[in++];
        }
        assert(out <= leafslotmax);

        set_min_element(slot, *local_min);
        result->slotuse = out;
        update_router(slot.slotkey, result->slotkey[out-1]); 
        spare_leaf = slot.childid;
        slot.childid = result;

        BTREE_PRINT(": size " << leaf->slotuse << " -> " << result->slotuse << std::endl);
    }

};


#endif // _SEQUENTIAL_BTREE_H_
