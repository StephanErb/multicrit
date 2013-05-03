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

#ifndef _PARETO_LABELSET_H_
#define _PARETO_LABELSET_H_

//#define BTREE_DEBUG

#include "datastructures/BTree_base_copy.hpp"
#include "tbb/scalable_allocator.h"

#include <iostream>
#include <limits>
#include <algorithm>

#include "Label.hpp"
#include "ParetoSearchStatistics.hpp"


#ifndef LS_LEAF_PARAMETER_K
#define LS_LEAF_PARAMETER_K 64
#endif

#ifndef LS_BRANCHING_PARAMETER_B
#define LS_BRANCHING_PARAMETER_B 32
#endif


struct labelset_default_traits {
    /// If true, the tree will self verify it's invariants after each insert()
    /// or erase(). The header must have been compiled with BTREE_DEBUG defined.
    static const bool   selfverify = false;

    /// Configure nodes to have a fixed size of X cache lines. 
    static const unsigned int leafparameter_k = BTREE_MAX( 8, LS_LEAF_PARAMETER_K );
    static const unsigned int branchingparameter_b = BTREE_MAX( 8, LS_BRANCHING_PARAMETER_B );
};


/** 
 * Basic class implementing a base B+ tree data structure in memory.
 */
template <typename _Key,
          typename _Compare,
          typename _Alloc>
class BtreeParetoLabelSet : public btree_base_copy<_Key, _Compare, labelset_default_traits, _Alloc> {

protected:
    typedef btree_base_copy<_Key, _Compare, labelset_default_traits, _Alloc> base;

    typedef typename base::key_type         key_type;
    typedef typename base::key_compare      key_compare;
    typedef typename base::allocator_type   allocator_type;
    typedef typename base::size_type        size_type;
    typedef typename base::level_type       level_type;
    typedef typename base::width_type       width_type;
    typedef typename base::inner_node_data  inner_node_data;
    typedef typename base::tree_stats       tree_stats;


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

    // Exported to allow external init    
    using base::allocate_leaf_without_count;
    using base::allocate_inner_without_count;
    using base::free_node_without_count;
    typedef typename base::node             node;
    typedef typename base::leaf_node        leaf_node;
    typedef typename base::inner_node       inner_node;
    typedef typename base::UpdateDescriptor UpdateDescriptor;
    typedef typename base::leaf_list        leaf_list;
    typedef typename base::weightdelta_list weightdelta_list;
    typedef typename base::UpdateDescriptorArray UpdateDescriptorArray;
    typedef typename base::ThreadLocalLSData ThreadLocalLSData;


private:
    using base::root;
    using base::stats;
    using base::allocator;
    using base::batch_type;
    using base::updates;
    using base::tls_data;

    using base::key_less;
    using base::key_lessequal;
    using base::key_greaterequal;
    using base::key_equal;
    using base::allocate_leaf;
    using base::allocate_inner;
    using base::leaf_node_allocator;
    using base::inner_node_allocator;
    using base::clear_recursive;
    using base::num_optimal_levels;
    using base::find_lower;
    using base::hasUpdates;
    using base::scheduleSubTreeUpdate;
    using base::num_subtrees;
    using base::designated_subtreesize;
    using base::update_router;
    using base::create_subtree_from_leaves;
    using base::rewrite;
    using base::update;
    using base::get_resized_leaves_array;
    using base::print_node;
    using base::update_leaf_in_current_tree;

public:
    // *** Constructors and Destructor

    explicit inline BtreeParetoLabelSet(const allocator_type &alloc=allocator_type())
        : base(alloc)
    { 
        root = allocate_leaf();
    }

public:

    struct GroupByWeightComp {
        inline bool operator() (const Operation<Label>& i, const Operation<Label>& j) const {
            return i.data.combined() < j.data.combined(); 
        }
    } groupByWeight;

    template<class NodeID, class candidates_iter_type, class PQUpdates>
    void updateLabelSet(const NodeID node, const candidates_iter_type start, const candidates_iter_type end, PQUpdates& pq_updates, ThreadLocalLSData& _data) {
        tls_data = &_data; // pupulate with current (thread local) data structures

        inner_node_data fake_slot;
        fake_slot.childid = root;
        fake_slot.weight = stats.itemcount;
        batch_type = INSERTS_ONLY;
        generateUpdates(node, fake_slot, start, end, pq_updates, std::numeric_limits<typename Label::weight_type>::max());
        stats.itemcount = fake_slot.weight;
        root = fake_slot.childid;

        if (tls_data->local_updates.size() > 0) {
            assert(std::is_sorted(tls_data->local_updates.begin(), tls_data->local_updates.end(), groupByWeight));
            apply_updates(tls_data->local_updates, batch_type);
            tls_data->local_updates.clear();
        }
    }

    inline void prefetch() const {
        #ifdef PREFETCH_LABELSETS
            __builtin_prefetch(root);
        #endif
    }

    void init(const Label& data, ThreadLocalLSData& _data) {
        tls_data = &_data; // pupulate with current (thread local) data structures
        std::vector<Operation<Label>> upds;
        upds.emplace_back(Operation<Label>::INSERT, data);
        apply_updates(upds, INSERTS_ONLY);
    }

private:

    template<class NodeID, class candidates_iter_type, class PQUpdates>
    inline Label::weight_type generateUpdates(const NodeID node, inner_node_data& slot, const candidates_iter_type start, const candidates_iter_type end, PQUpdates& pq_updates, Label::weight_type min) {

        if (slot.childid->isleafnode()) {
            return generateLeafUpdates(node, slot, start, end, pq_updates, min);
        } else {
            inner_node* const inner = static_cast<inner_node*>(slot.childid);
            const width_type slotuse = inner->slotuse;

            bool continue_check = false;
            auto sub_start = start;
            width_type i = 0;

            while (i < slotuse && key_less(inner->slot[i].slotkey, *sub_start)) {
                min = inner->slot[i].slotkey.second_weight;
                ++i;
            }
            while (i < slotuse && (sub_start != end || continue_check)) {
                // move over dominated labels
                while (sub_start != end && sub_start->second_weight >= min) {
                    ++sub_start;
                }
                // find labels falling into the current subtree/slot
                auto sub_end = sub_start;
                while (sub_end != end && key_less(*sub_end, inner->slot[i].slotkey)) {
                    ++sub_end;
                }
                if (sub_start != sub_end || continue_check) {
                    const width_type pre_weight = inner->slot[i].weight;
                    min = generateUpdates(node, inner->slot[i], sub_start, sub_end, pq_updates, min);
                    const signed short diff = inner->slot[i].weight - pre_weight;
                    slot.weight += diff;
                    // if the last element (router) is dominated, also check for dominance in the next leaf
                    continue_check = min <= inner->slot[i].slotkey.second_weight;
                    sub_start = sub_end;
                }
                min = std::min(min, inner->slot[i].slotkey.second_weight);
                ++i;
            }
            update_router(slot.slotkey, inner->slot[slotuse-1].slotkey);
            min = std::min(min, slot.slotkey.second_weight);

            // Generate updates for all elements larger than the largest router key
            auto& local_upds = tls_data->local_updates;
            for (candidates_iter_type candidate = sub_start; candidate != end; ++candidate) {
                const Label& new_label = *candidate;
                if (new_label.second_weight >= min) {
                    continue; 
                }
                min = new_label.second_weight;
                local_upds.emplace_back(Operation<Label>::INSERT, new_label);
                pq_updates.emplace_back(Operation<NodeLabel>::INSERT, node, new_label);
            }
        } 
        return min;
    }

    static inline bool isDominated(const leaf_node* const leaf, width_type& iii, const Label& new_label) {
        const width_type slotuse = leaf->slotuse;

        width_type i = iii;
        // Find x-predecessor
        while (i < slotuse && leaf->slotkey[i].first_weight < new_label.first_weight) {
            ++i;
        }
        if (i > 0) {
            --i; // move to predecessor
            // Dominated by x-predecessor?
            if (leaf->slotkey[i].second_weight <= new_label.second_weight) {
                ////std::cout << "  Predecessor " << std::endl;
                iii = i;
                return true;
            }
            ++i; // move to element with greater / equal x-coordinate
        }
        if (i < slotuse && 
            leaf->slotkey[i].first_weight == new_label.first_weight && 
            leaf->slotkey[i].second_weight <= new_label.second_weight) {
            iii = i;
            return true; 
        }
        iii = i;
        return false;
    }

    template<class NodeID, class candidates_iter_type, class PQUpdates>
    inline Label::weight_type generateLeafUpdates(const NodeID node, inner_node_data& slot, const candidates_iter_type start, const candidates_iter_type end, PQUpdates& pq_updates, Label::weight_type min) {
        auto& local_upds = tls_data->local_updates;

        const leaf_node* const leaf = static_cast<leaf_node*>(slot.childid);
        const width_type slotuse = leaf->slotuse;
        width_type i = 0;

        const size_t pre_upd_count = local_upds.size();
        size_t new_weight = slot.weight;

        // Delete elements by a new labels inserted into a previous leaf
        while (i < slotuse && leaf->slotkey[i].second_weight >= min) {
            batch_type = INSERTS_AND_DELETES;
            pq_updates.emplace_back(Operation<NodeLabel>::DELETE, node, leaf->slotkey[i]);
            local_upds.emplace_back(Operation<Label>::DELETE, leaf->slotkey[i]);
            --new_weight;
            ++i;
        }

        // Iterate of candidate labels and check dominance.
        for (candidates_iter_type candidate = start; candidate != end; ++candidate) {
            const Label& new_label = *candidate;
            // short cut dominated check among candidates
            if (new_label.second_weight >= min) {
                continue; 
            }
            if (isDominated(leaf, i, new_label)) {
                min = leaf->slotkey[i].second_weight;
            } else {
                min = new_label.second_weight;

                // Find proper insertion position. Might be required (but often isn't) if a previous dominance check was too greedy
                auto iter = --local_upds.end();
                while (iter != --local_upds.begin() && iter->data.first_weight >= new_label.first_weight) {
                    --iter;
                }
                local_upds.insert(++iter, {Operation<Label>::INSERT, new_label});
                pq_updates.emplace_back(Operation<NodeLabel>::INSERT, node, new_label);
                ++new_weight;

                while (i < slotuse && leaf->slotkey[i].second_weight >= min) {
                    batch_type = INSERTS_AND_DELETES;
                    local_upds.emplace_back(Operation<Label>::DELETE, leaf->slotkey[i]);
                    pq_updates.emplace_back(Operation<NodeLabel>::DELETE, node, leaf->slotkey[i]);
                    --new_weight;
                    ++i;
                }
            }
        }

        // If all updates fit into the leaf: Update directly without walking the root-to-leaf path again
        const size_t post_upd_count = local_upds.size();
        if (pre_upd_count !=  post_upd_count && new_weight >= leafslotmin && new_weight <= leafslotmax) {
            slot.weight = new_weight;
            setOperationsAndComputeWeightDelta(local_upds, batch_type, pre_upd_count);
            update_leaf_in_current_tree(slot, pre_upd_count, post_upd_count);
            local_upds.resize(pre_upd_count);
        }
        return min;
    }

    template<typename T>
    inline void apply_updates(const T& _updates, const OperationBatchType _batch_type) {
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

        inner_node_data fake_slot;
        fake_slot.childid = root;

        if (rebuild_needed) {
            BTREE_PRINT("Root-level rewrite session started for new level " << level << std::endl);
            auto& leaves = get_resized_leaves_array(new_size);
            rewrite(fake_slot.childid, 0, 0, _updates.size(), leaves);
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

    template<typename T>
    inline size_type setOperationsAndComputeWeightDelta(const T& _updates, const OperationBatchType _batch_type, const size_t start=0) {
        updates = _updates.data();
        batch_type = _batch_type;
        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)
        if (_batch_type == INSERTS_AND_DELETES) {
            long val = 0; // exclusive prefix sum
            auto& weightdelta = tls_data->weightdelta;
            assert(weightdelta.capacity() > _updates.size());
            weightdelta[0] = val;
            for (size_type i = start; i < _updates.size();) {
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



template <typename _Alloc>
class VectorParetoLabelSet {
public:
    typedef std::vector<Label, _Alloc> LabelVec;
    typedef typename LabelVec::iterator label_iter;
    typedef typename LabelVec::const_iterator const_label_iter;
private:
    LabelVec labels;
public:

    #ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
        std::vector<unsigned long> set_insertions;
        std::vector<unsigned long> set_dominations;
    #endif

    VectorParetoLabelSet() 
        #ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
            : set_insertions(101)
            , set_dominations(101)
        #endif
    {
        const typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::min();
        const typename Label::weight_type max = std::numeric_limits<typename Label::weight_type>::max();

        labels.reserve(INITIAL_LABELSET_SIZE);
        labels.insert(labels.begin(), Label(min, max));
        labels.insert(labels.end(), Label(max, min));
    }

    template<class NodeID, class candidates_iter_type, class Stats, class PQUpdates>
    void updateLabelSet(const NodeID node, const candidates_iter_type start, const candidates_iter_type end, PQUpdates& updates, Stats& stats) {
        typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();
        int modifications = 0;

        size_t previous_first_nondominated = 0;  // of the previous iteration
        bool deferred_insertion = false;
        size_t deferred_ins_pos_start = 0;                    // where to start writing the current open candidate range 
        candidates_iter_type deferred_ins_cand_start = start; // first element in the currently open candidate range

        for (candidates_iter_type candidate = start; candidate != end; ++candidate) {
            const Label& new_label = *candidate;
            // short cut dominated check among candidates
            if (new_label.second_weight >= min) {
                stats.report(LABEL_DOMINATED);
                stats.report(DOMINATION_SHORTCUT);
                perform_deferred_insertion(deferred_insertion, deferred_ins_pos_start, previous_first_nondominated, deferred_ins_cand_start, candidate);
                continue; 
            }
            label_iter iter;
            if (isDominated(previous_first_nondominated, new_label, iter)) {
                stats.report(LABEL_DOMINATED);
                min = iter->second_weight; 
                #ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
                    const double size = labels.size()-2; // without both sentinals
                    const double position = iter - (labels.begin() + 1); // without leading sentinal 
                    if (size > 0) set_dominations[(int) (100.0 * position / size)]++;
                #endif  
                perform_deferred_insertion(deferred_insertion, deferred_ins_pos_start, previous_first_nondominated, deferred_ins_cand_start, candidate);
                continue;
            }
            // Log upcoming insertion
            ++modifications;
            stats.report(LABEL_NONDOMINATED);
            #ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
                const double size = labels.size()-2; // without both sentinals
                const double position = iter - (labels.begin() + 1); // without leading sentinal 
                if (size > 0) set_insertions[(int) (100.0 * position / size)]++;
            #endif  

            min = new_label.second_weight;
            
            // Schedule PQ updates && Find affected range
            updates.emplace_back(Operation<NodeLabel>::INSERT, node, new_label);
            size_t insertion_pos = (size_t)(iter - labels.begin());
            size_t first_nondominated = insertion_pos; /* will point to first label where the y-coord is truly smaller */
            while (secondWeightGreaterOrEquals(labels[first_nondominated], new_label)) { 
                updates.emplace_back(Operation<NodeLabel>::DELETE, node, labels[first_nondominated++]);
            }

            const size_t insertion_distance = insertion_pos - previous_first_nondominated;
            if (insertion_distance > 0) {
                const size_t delta = perform_deferred_insertion(deferred_insertion, deferred_ins_pos_start, previous_first_nondominated, deferred_ins_cand_start, candidate);
                // Move the gap so that we can use it for the next (deferred) insertion
                memmove(labels.data() + deferred_ins_pos_start, labels.data() + previous_first_nondominated, sizeof(Label)*insertion_distance);

                insertion_pos += delta;
                first_nondominated += delta;
                start_deferred_insertion(deferred_insertion, deferred_ins_pos_start, insertion_pos, previous_first_nondominated, deferred_ins_cand_start, candidate);

            } else if (!deferred_insertion) {
                start_deferred_insertion(deferred_insertion, deferred_ins_pos_start, insertion_pos, previous_first_nondominated, deferred_ins_cand_start, candidate);
            }
            previous_first_nondominated = first_nondominated;
        }
        perform_deferred_insertion(deferred_insertion, deferred_ins_pos_start, previous_first_nondominated, deferred_ins_cand_start, end);
        labels.erase(labels.begin()+deferred_ins_pos_start, labels.begin()+previous_first_nondominated);

        assert(std::is_sorted(labels.begin(), labels.end(), groupByWeight) ||
            (labels[1].first_weight == 0 && labels[1].second_weight == 0)); // Hack: Do not fail on the start node, initiated with (0,0) 

        stats.report(CANDIDATE_LABELS_PER_NODE, end - start);
        stats.report(LS_MODIFICATIONS_PER_NODE, modifications);
    }

    inline void prefetch() const {
        #ifdef PREFETCH_LABELSETS
            const size_t size = labels.size();
            __builtin_prefetch(&(labels[size * 0.5]));
            __builtin_prefetch(&(labels[size * 0.75]));
            __builtin_prefetch(&(labels[size * 0.25]));
            __builtin_prefetch(&(labels[size * 1.0]));
        #endif
    }

    void init(const Label& label) {
        labels.insert(++labels.begin(), label);
    }

    // Accessors, corrected for internal sentinals
    size_t size() const { return labels.size() - 2; };
    label_iter begin() { return ++labels.begin(); }
    const_label_iter begin() const { return ++labels.begin(); }
    label_iter end() { return --labels.end(); }
    const_label_iter end() const { return --labels.end(); }


private:

    template<class candidates_iter_type>
    inline void start_deferred_insertion(bool& deferred_insertion, size_t& deferred_ins_pos_start, const size_t& insertion_pos, const size_t& previous_first_nondominated, candidates_iter_type& deferred_ins_cand_start, const candidates_iter_type& candidate) const {
        // insertion_pos + correction to make it point into the gap
        deferred_ins_pos_start = insertion_pos - (previous_first_nondominated - deferred_ins_pos_start);
        deferred_ins_cand_start = candidate;
        deferred_insertion = true;
    }

    template<class candidates_iter_type>
    inline size_t perform_deferred_insertion(bool& deferred_insertion, size_t& deferred_ins_pos_start, size_t& previous_first_nondominated, candidates_iter_type& deferred_ins_cand_start, const candidates_iter_type& candidate) {
        if (deferred_insertion) {
            deferred_insertion = false;

            const size_t deletion_gap_size = previous_first_nondominated - deferred_ins_pos_start;
            const size_t elements_moved_into_gap = std::min((size_t)(candidate - deferred_ins_cand_start), deletion_gap_size);
            for (size_t i=0; i < elements_moved_into_gap; ++i)  {
                labels[deferred_ins_pos_start++] = *deferred_ins_cand_start++;
            }
            // Gap space is filled. Insert remaining elements
            const size_t remaining_elements = (size_t) (candidate - deferred_ins_cand_start);
            labels.insert(labels.begin()+deferred_ins_pos_start, deferred_ins_cand_start, candidate);
            deferred_ins_pos_start += remaining_elements;
            previous_first_nondominated += remaining_elements;
            
            return remaining_elements;
        }
        return 0; 
    }

    struct GroupByWeightComp {
        inline bool operator() (const Label& i, const Label& j) const {
            return i.combined() < j.combined(); 
        }
    } groupByWeight;

    static struct WeightLessComp {
        inline bool operator() (const Label& i, const Label& j) const {
            return i.first_weight < j.first_weight;
        }
    } firstWeightLess;

    static inline bool secondWeightGreaterOrEquals(const Label& i, const Label& j) {
        return i.second_weight >= j.second_weight;
    }

    /** First label where the x-coord is truly smaller */
    static inline label_iter x_predecessor(label_iter begin, label_iter end, const Label& new_label) {
        return --std::lower_bound(begin, end, new_label, firstWeightLess);
    }

    inline bool isDominated(const size_t begin, const Label& new_label, label_iter& iter) {
        iter = x_predecessor(labels.begin() + begin, labels.end(), new_label);

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

};

/******************************************************************************
 * Slightly advanced altertnative vector labelset. Uses a two-pass approach but
 * does not seem to be any better
 ******************************************************************************
    struct LabelInsPos {
        size_t pos;
        const Label& label;
    };

    typedef std::vector<LabelInsPos, tbb::cache_aligned_allocator<LabelInsPos> > LabelInsertionPositionVec;
    typedef tbb::enumerable_thread_specific< LabelInsertionPositionVec, tbb::cache_aligned_allocator<LabelInsertionPositionVec>, tbb::ets_key_per_instance > TLSLabInsPos; 
    TLSLabInsPos tls_insertion_positions;

    static struct WeightLessComp {
        inline bool operator() (const Label& i, const Label& j) const {
            return i.first_weight < j.first_weight;
        }
    } firstWeightLess;

    static inline bool secondWeightGreaterOrEquals(const Label& i, const Label& j) {
        return i.second_weight >= j.second_weight;
    }

    // First label where the x-coord is truly smaller
    static inline label_iter x_predecessor(label_iter begin, label_iter end, const Label& new_label) {
        return --std::lower_bound(begin, end, new_label, firstWeightLess);
    }

    // First label where the y-coord is truly smaller
    static inline size_t y_predecessor(const typename ParetoQueue::LabelVec& labelset, size_t i, const Label& new_label) {
        while (secondWeightGreaterOrEquals(labelset[i], new_label)) {
            ++i;
        }
        return i;
    }

    static inline bool isDominated(label_iter begin, label_iter end, const Label& new_label, label_iter& iter) {
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

    template<class iter_type>
    static void updateLabelSet(const NodeID node, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::LabelVec& spare_labelset, const iter_type start, const iter_type end, typename ParetoQueue::OpVec& updates, LabelInsertionPositionVec& nondominated) {
        typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();

        label_iter unprocessed_range_begin = labelset.begin();
        for (iter_type candidate = start; candidate != end; ++candidate) {
            // short cut dominated check among candidates
            if (candidate->second_weight >= min) { 
                continue; 
            }
            label_iter iter;
            if (isDominated(unprocessed_range_begin, labelset.end(), *candidate, iter)) {
                min = iter->second_weight; 
            } else {
                min = candidate->second_weight;
                nondominated.push_back({(size_t)(iter-labelset.begin()), *candidate});
            }
            unprocessed_range_begin = iter;
        }
        switch (nondominated.size()) {
        case 0:
            break; // nothing to insert
        case 1:
            labelset_simple_insert(node, nondominated.front(), labelset, updates);
            break;
        default:
            // Sentinal: (max position, any label)
            nondominated.push_back({labelset.size(), nondominated.front().label});

            if (is_update_at_the_end(labelset, nondominated.front().pos)) {
                labelset_bulk_update(node, nondominated, labelset, updates);
            } else {
                labelset_rewrite(node, nondominated, labelset, spare_labelset, updates);
            }
        }
        nondominated.clear();
    }

    static inline void labelset_simple_insert(const NodeID node, const LabelInsPos& op, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::OpVec& updates) {
        updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, op.label)});
        const size_t first_nondominated = y_predecessor(labelset, op.pos, op.label);

        if (op.pos == first_nondominated) {
            labelset.insert(labelset.begin() + op.pos, op.label);
        } else {
            for (size_t i = op.pos; i != first_nondominated; ++i) {
                // schedule deletion of dominated labels
                updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, labelset[i])});
            }
            // replace first dominated label and remove the rest
            labelset[op.pos] = op.label;
            labelset.erase(labelset.begin() + op.pos + 1, labelset.begin() + first_nondominated);
        }
    }

    static inline void labelset_bulk_update(const NodeID node, LabelInsertionPositionVec& nondominated, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::OpVec& updates) {
        size_t delta = 0;
        for (size_t i = 0; i < nondominated.size()-1; ++i) {
            LabelInsPos& op = nondominated[i];
            op.pos += delta;

            const size_t first_nondominated = y_predecessor(labelset, op.pos, op.label);
            const size_t range_end = std::min(first_nondominated, nondominated[i+1].pos+delta);
            updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, op.label)});

            if (op.pos == range_end) {
                // No elements dominated directly by us. Just insert here
                labelset.insert(labelset.begin() + op.pos, op.label);
            } else {
                for (size_t i = op.pos; i != range_end; ++i) {
                    // schedule deletion of dominated labels
                    updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, labelset[i])});
                }
                // replace first dominated label and remove the rest
                labelset[op.pos] = op.label;
                labelset.erase(labelset.begin() + op.pos + 1, labelset.begin() + range_end);
            }
            delta += op.pos - range_end + 1;
        }
    }

    static inline void labelset_rewrite(const NodeID node, const LabelInsertionPositionVec& nondominated, typename ParetoQueue::LabelVec& labelset, typename ParetoQueue::LabelVec& spare_labelset, typename ParetoQueue::OpVec& updates) {
        size_t copied_until = 0;
        // power of two to allow efficient caching by the allocator
        spare_labelset.reserve(next_power_of_two(labelset.size() + nondominated.size()));

        for (size_t i = 0; i < nondominated.size()-1; ++i) {
            const LabelInsPos& op = nondominated[i];
            const size_t first_nondominated = y_predecessor(labelset, op.pos, op.label);
            updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, op.label)});

            // Move all non-affected labels
            if (copied_until < op.pos)  {
                spare_labelset.insert(spare_labelset.end(), labelset.begin() + copied_until, labelset.begin() + op.pos);
                copied_until = op.pos;
            }
            // Insert the new label
            spare_labelset.push_back(op.label);

            // Delete all dominated labels by
            size_t range_end = std::min(first_nondominated, nondominated[i+1].pos);
            for (size_t i = op.pos; i != range_end; ++i) {
                updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, labelset[i])});
            }
            copied_until = range_end;
        }
        // copy remaining labels
        spare_labelset.insert(spare_labelset.end(),  labelset.begin() + copied_until, labelset.end());
        labelset.swap(spare_labelset);
        spare_labelset.clear();
    }

    static inline bool is_update_at_the_end(const typename ParetoQueue::LabelVec& labelset, const size_t pos) {
        return labelset.size() > 8 || pos / (double) labelset.size() > 0.75;
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

    */


#endif // _PARETO_LABELSET_H_
