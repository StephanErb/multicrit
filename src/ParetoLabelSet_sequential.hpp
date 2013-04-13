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

typedef std::vector< Label > LabelVec;
typedef typename LabelVec::iterator label_iter;
typedef typename LabelVec::const_iterator const_label_iter;
typedef typename std::vector<NodeLabel>::iterator nodelabel_iter;


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
          typename _Compare = std::less<_Key>,
          typename _Traits = labelset_default_traits,
          typename _Alloc = tbb::cache_aligned_allocator<_Key>>
class BtreeParetoLabelSet : public btree_base_copy<_Key, _Compare, _Traits, _Alloc> {

protected:
    typedef btree_base_copy<_Key, _Compare, _Traits, _Alloc> base;

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
            if (i.data.first_weight == j.data.first_weight) {
                return i.data.second_weight < j.data.second_weight;
            }
            return i.data.first_weight < j.data.first_weight;
        }
    } groupByWeight;

    template<class NodeID, class candidates_iter_type, class Stats>
    void updateLabelSet(const NodeID node, const candidates_iter_type start, const candidates_iter_type end, std::vector<Operation<NodeLabel>>& pq_updates, Stats&) {
        tls_data->local_updates.clear();

        inner_node_data fake_slot;
        fake_slot.childid = root;
        batch_type = INSERTS_ONLY;
        generateUpdates(fake_slot, start, end, std::numeric_limits<typename Label::weight_type>::max());

        if (tls_data->local_updates.size() > 0) {
            assert(std::is_sorted(tls_data->local_updates.begin(), tls_data->local_updates.end(), groupByWeight));
            apply_updates(tls_data->local_updates, batch_type);

            for (const auto& op : tls_data->local_updates) {
                const auto type = op.type == Operation<Label>::INSERT ? Operation<NodeLabel>::INSERT : Operation<NodeLabel>::DELETE;
                pq_updates.push_back({type, NodeLabel(node, op.data)});
            }
        }
    }

    inline void setup(ThreadLocalLSData& _data) {
        tls_data = &_data;
    }

private:

    template<class candidates_iter_type>
    Label::weight_type generateUpdates(const inner_node_data& slot, const candidates_iter_type start, const candidates_iter_type end, Label::weight_type min) {

        if (slot.childid->isleafnode()) {
            return generateLeafUpdates(slot, start, end, min);
        } else {
            const inner_node* const inner = static_cast<inner_node*>(slot.childid);
            const width_type slotuse = inner->slotuse;

            bool continue_check = false;
            auto sub_start = start;
            for (width_type i = 0; i < slotuse; ++i) {
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
                    min = generateUpdates(inner->slot[i], sub_start, sub_end, min);
                    // if the last element (router) is dominated, also check for dominance in the next leaf
                    continue_check = min <= inner->slot[i].slotkey.second_weight;
                }
                sub_start = sub_end;
                min = std::min(min, inner->slot[i].slotkey.second_weight);
            } 
            if (sub_start != end) {
                for (candidates_iter_type candidate = sub_start; candidate != end; ++candidate) {
                    const Label& new_label = *candidate;
                    if (new_label.second_weight >= min) {
                        continue; 
                    }
                    min = new_label.second_weight;
                    tls_data->local_updates.push_back({Operation<Label>::INSERT, new_label});
                }
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

    template<class candidates_iter_type>
    inline Label::weight_type generateLeafUpdates(const inner_node_data& slot, const candidates_iter_type start, const candidates_iter_type end, Label::weight_type min) {
        const leaf_node* const leaf = static_cast<leaf_node*>(slot.childid);
        const width_type slotuse = leaf->slotuse;
        width_type i = 0;

        // Delete elements by a new labels inserted into a previous leaf
        while (i < slotuse && leaf->slotkey[i].second_weight >= min) {
            batch_type = INSERTS_AND_DELETES;
            tls_data->local_updates.push_back({Operation<Label>::DELETE, leaf->slotkey[i++]});
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
                auto iter = --tls_data->local_updates.end();
                while (iter != --tls_data->local_updates.begin() && iter->data.first_weight >= new_label.first_weight) {
                    --iter;
                }
                tls_data->local_updates.insert(++iter, {Operation<Label>::INSERT, new_label});

                while (i < slotuse && leaf->slotkey[i].second_weight >= min) {
                    batch_type = INSERTS_AND_DELETES;
                    tls_data->local_updates.push_back({Operation<Label>::DELETE, leaf->slotkey[i++]});
                }
            }
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

        UpdateDescriptor upd = {rebuild_needed, new_size, 0, _updates.size()};
        inner_node_data fake_slot;
        fake_slot.childid = root;

        if (rebuild_needed) {
            BTREE_PRINT("Root-level rewrite session started for new level " << level << std::endl);
            auto& leaves = get_resized_leaves_array(new_size);
            rewrite(fake_slot.childid, 0, upd, leaves);
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
    inline size_type setOperationsAndComputeWeightDelta(const T& _updates, const OperationBatchType _batch_type) {
        updates = _updates.data();
        batch_type = _batch_type;
        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)
        if (_batch_type == INSERTS_AND_DELETES) {
            long val = 0; // exclusive prefix sum
            assert(tls_data->weightdelta.capacity() > _updates.size());
            tls_data->weightdelta[0] = val;
            for (size_type i = 0; i < _updates.size();) {
                val = val + updates[i].type;
                tls_data->weightdelta[++i] = val;
            }
            return size() + tls_data->weightdelta[_updates.size()];
        } else {
            // We can use the shortcut based on the 
            return size() + _updates.size() * batch_type;
        }
    }
};  




class VectorParetoLabelSet {
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

        labels.insert(labels.begin(), Label(min, max));
        labels.insert(labels.end(), Label(max, min));
    }

    template<class NodeID, class candidates_iter_type, class Stats>
    void updateLabelSet(const NodeID node, const candidates_iter_type start, const candidates_iter_type end, std::vector<Operation<NodeLabel>>& updates, Stats& stats) {
        typename Label::weight_type min = std::numeric_limits<typename Label::weight_type>::max();
        int modifications = 0;

        label_iter labelset_iter = labels.begin();
        for (candidates_iter_type candidate = start; candidate != end; ++candidate) {
            Label new_label = *candidate;
            // short cut dominated check among candidates
            if (new_label.second_weight >= min) { 
                stats.report(LABEL_DOMINATED);
                stats.report(DOMINATION_SHORTCUT);
                continue; 
            }
            label_iter iter;
            if (isDominated(labelset_iter, labels.end(), new_label, iter)) {
                stats.report(LABEL_DOMINATED);
                min = iter->second_weight; 
                #ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
                    const double size = labels.size()-2; // without both sentinals
                    const double position = iter - (labels.begin() + 1); // without leading sentinal 
                    if (size > 0) set_dominations[(int) (100.0 * position / size)]++;
                #endif  
                continue;
            }
            min = new_label.second_weight;
            stats.report(LABEL_NONDOMINATED);
            updates.push_back({Operation<NodeLabel>::INSERT, NodeLabel(node, new_label)});

            label_iter first_nondominated = y_predecessor(iter, new_label);

            #ifdef GATHER_DATASTRUCTURE_MODIFICATION_LOG
                const double size = labels.size()-2; // without both sentinals
                const double position = iter - (labels.begin() + 1); // without leading sentinal 
                if (size > 0) set_insertions[(int) (100.0 * position / size)]++;
            #endif  

            if (iter == first_nondominated) {
                // delete range is empty, so just insert
                labelset_iter = labels.insert(first_nondominated, new_label);
            } else {
                ++modifications; // only log non-trivial
                // schedule deletion of dominated labels
                for (label_iter i = iter; i != first_nondominated; ++i) {
                    updates.push_back({Operation<NodeLabel>::DELETE, NodeLabel(node, *i)});
                }
                // replace first dominated label and remove the rest
                *iter = new_label;
                labelset_iter = labels.erase(++iter, first_nondominated);
            }
        }

        stats.report(CANDIDATE_LABELS_PER_NODE, end - start);
        if (modifications > 0) stats.report(LS_MODIFICATIONS_PER_NODE, modifications);
    }

    // Accessors, corrected for internal sentinals
    size_t size() const { return labels.size() - 2; };
    label_iter begin() { return ++labels.begin(); }
    const_label_iter begin() const { return ++labels.begin(); }
    label_iter end() { return --labels.end(); }
    const_label_iter end() const { return --labels.end(); }


private:
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

    /** First label where the y-coord is truly smaller */
    static inline label_iter y_predecessor(const label_iter& begin, const Label& new_label) {
        label_iter i = begin;
        while (secondWeightGreaterOrEquals(*i, new_label)) {
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

};


#endif // _PARETO_LABELSET_H_
