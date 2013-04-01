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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USAlevel
 */

#ifndef _SEQUENTIAL_BTREE_H_
#define _SEQUENTIAL_BTREE_H_

#include "BTree_base.hpp"
#include "tbb/scalable_allocator.h"


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
        typename _Alloc = tbb::scalable_allocator<_Key>>
class btree : 
    #ifdef COMPUTE_PARETO_MIN
        public btree_base<_Key, _MinKey, _Compare, _Traits, _Alloc>
    #else
        public btree_base<_Key, _Compare, _Traits, _Alloc>
    #endif

 {

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
    typedef typename base::node             node;
    typedef typename base::leaf_node        leaf_node;
    typedef typename base::inner_node       inner_node;
    typedef typename base::inner_node_data  inner_node_data;
    typedef typename base::tree_stats       tree_stats;
    typedef typename base::UpdateDescriptor UpdateDescriptor;
    typedef typename base::leaf_list        leaf_list;

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
    using base::leaves;

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
    using base::find_pareto_minima;
    using base::rewrite;
    using base::update;
    using base::get_resized_leaves_array;

public:
    // *** Constructors and Destructor

    /// Default constructor initializing an empty B+ tree with the standard key
    /// comparison function
    explicit inline btree(const allocator_type &alloc=allocator_type())
        : base(alloc)
    {
        weightdelta.reserve(LARGE_ENOUGH_FOR_EVERYTHING);
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
            BTREE_PRINT("Root-level rewrite session started for new level " << level << std::endl);
            auto& leaves = get_resized_leaves_array(new_size);
            rewrite(fake_slot.childid, /*rank*/0, upd, leaves);
            create_subtree_from_leaves(fake_slot, 0, false, level, 0, new_size, leaves);
        } else {
            update(fake_slot, 0, _updates.size());
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

};


#endif // _SEQUENTIAL_BTREE_H_
