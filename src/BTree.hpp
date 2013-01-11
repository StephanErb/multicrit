// $Id$ -*- fill-column: 79 -*-
/** \file btree.h
 * Contains the main B+ tree implementation template class btree.
 */

/*
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

#ifndef _STX_BTREE_H_
#define _STX_BTREE_H_

template<typename data_type>
struct Operation {
    enum OpType {INSERT, DELETE};
    OpType type;
    data_type data;
    //Operation(const OpType& x, const data_type& y) : type(x), data(y) {}
};

// *** Required Headers from the STL
#include <algorithm>
#include <functional>
#include <istream>
#include <ostream>
#include <iostream>
#include <memory>
#include <cstddef>
#include <assert.h>
#include <array>

// *** Debugging Macros
#ifdef BTREE_DEBUG

#include <iostream>

/// Print out debug information to std::cout if BTREE_DEBUG is defined.
#define BTREE_PRINT(x)          do { if (debug) (std::cout << x); } while(0)

/// Assertion only if BTREE_DEBUG is defined. This is not used in verify().
#define BTREE_ASSERT(x)         do { assert(x); } while(0)

#else

/// Print out debug information to std::cout if BTREE_DEBUG is defined.
#define BTREE_PRINT(x)          do { } while(0)

/// Assertion only if BTREE_DEBUG is defined. This is not used in verify().
#define BTREE_ASSERT(x)         do { } while(0)

#endif

/// The maximum of a and b. Used in some compile-time formulas.
#define BTREE_MAX(a,b)          ((a) < (b) ? (b) : (a))


/// STX - Some Template Extensions namespace
namespace stx {

/** Generates default traits for a B+ tree used as a set. It estimates leaf and
 * inner node sizes by assuming a cache line size of 256 bytes. */
template <typename _Key>
struct btree_default_set_traits
{
    /// If true, the tree will self verify it's invariants after each insert()
    /// or erase(). The header must have been compiled with BTREE_DEBUG defined.
    static const bool   selfverify = false;

    /// If true, the tree will print out debug information and a tree dump
    /// during insert() or erase() operation. The header must have been
    /// compiled with BTREE_DEBUG defined and key_type must be std::ostream
    /// printable.
    static const bool   debug = false;

    /// Number of slots in each leaf of the tree. Estimated so that each node
    /// has a size of about 256 bytes.
    static const int    leafslots = BTREE_MAX( 8, 256 / (sizeof(_Key)) );

    /// Number of slots in each inner node of the tree. Estimated so that each node
    /// has a size of about 256 bytes.
    static const int    innerslots = BTREE_MAX( 8, 256 / (sizeof(_Key) + sizeof(void*)) );
};


/** @brief Basic class implementing a base B+ tree data structure in memory.
 *
 * The base implementation of a memory B+ tree. It is based on the
 * implementation in Cormen's Introduction into Algorithms, Jan Jannink's paper
 * and other algorithm resources. Almost all STL-required function calls are
 * implemented. The asymptotic time requirements of the STL are not always
 * fulfilled in theory, however in practice this B+ tree performs better than a
 * red-black tree by using more memory. The insertion function splits the nodes
 * on the recursion unroll. Erase is largely based on Jannink's ideas.
 *
 * This class is specialized into btree_set, btree_multiset, btree_map and
 * btree_multimap using default template parameters and facade functions.
 */
template <typename _Key,
          typename _Compare = std::less<_Key>,
          typename _Traits = btree_default_set_traits<_Key>,
          typename _Alloc = std::allocator<_Key> >
class btree
{
public:
    /// First template parameter: The key type of the B+ tree. This is stored
    /// in inner nodes and leaves
    typedef _Key                        key_type;

    /// Second template parameter: Key comparison function object
    typedef _Compare                    key_compare;

    /// Third template parameter: Traits object used to define more parameters
    /// of the B+ tree
    typedef _Traits                     traits;

    /// Fourth template parameter: STL allocator for tree nodes
    typedef _Alloc                      allocator_type;


    /// Typedef of our own type
    typedef btree<key_type, key_compare, traits, allocator_type> btree_self;

    /// Size type used to count keys
    typedef size_t                              size_type;

    // FIXME: Workaround!
    typedef struct { } iterator;

public:
    // *** Static Constant Options and Values of the B+ Tree

    /// Base B+ tree parameter: The number of key/data slots in each leaf
    static const unsigned short         leafslotmax =  traits::leafslots;

    /// Base B+ tree parameter: The number of key slots in each inner node,
    /// this can differ from slots in each leaf.
    static const unsigned short         innerslotmax =  traits::innerslots;

    /// Computed B+ tree parameter: The minimum number of key/data slots used
    /// in a leaf. If fewer slots are used, the leaf will be merged or slots
    /// shifted from it's siblings.
    static const unsigned short minleafslots = (leafslotmax / 2);

    /// Computed B+ tree parameter: The minimum number of key slots used
    /// in an inner node. If fewer slots are used, the inner node will be
    /// merged or slots shifted from it's siblings.
    static const unsigned short mininnerslots = (innerslotmax / 2);

    /// Debug parameter: Enables expensive and thorough checking of the B+ tree
    /// invariants after each insert/erase operation.
    static const bool                   selfverify = traits::selfverify;

    /// Debug parameter: Prints out lots of debug information about how the
    /// algorithms change the tree. Requires the header file to be compiled
    /// with BTREE_DEBUG and the key type must be std::ostream printable.
    static const bool                   debug = traits::debug;

private:
    // *** Node Classes for In-Memory Nodes

    /// The header structure of each node in-memory. This structure is extended
    /// by inner_node or leaf_node.
    struct node
    {
        /// Level in the b-tree, if level == 0 -> leaf node
        unsigned short  level;

        /// Number of key slotuse use, so number of valid children or data
        /// pointers
        unsigned short  slotuse;

        /// Delayed initialisation of constructed node
        inline void initialize(const unsigned short l)
        {
            level = l;
            slotuse = 0;
        }

        /// True if this is a leaf node
        inline bool isleafnode() const
        {
            return (level == 0);
        }
    };

    /// Extended structure of a inner node in-memory. Contains only keys and no
    /// data items.
    struct inner_node : public node
    {
        /// Define an related allocator for the inner_node structs.
        typedef typename _Alloc::template rebind<inner_node>::other alloc_type;

        /// Keys of children or data pointers
        key_type        slotkey[innerslotmax];

        /// Pointers to children
        node*           childid[innerslotmax+1];

        /// Set variables to initial values
        inline void initialize(const unsigned short l)
        {
            node::initialize(l);
        }

        /// True if the node's slots are full
        inline bool isfull() const
        {
            return (node::slotuse == innerslotmax);
        }

        /// True if few used entries, less than half full
        inline bool isfew() const
        {
            return (node::slotuse <= mininnerslots);
        }

        /// True if node has too few entries
        inline bool isunderflow() const
        {
            return (node::slotuse < mininnerslots);
        }
    };

    /// Extended structure of a leaf node in memory. Contains pairs of keys and
    /// data items. Key and data slots are kept in separate arrays, because the
    /// key array is traversed very often compared to accessing the data items.
    struct leaf_node : public node
    {
        /// Define an related allocator for the leaf_node structs.
        typedef typename _Alloc::template rebind<leaf_node>::other alloc_type;

        /// Keys of children or data pointers
        key_type        slotkey[leafslotmax];

        /// Set variables to initial values
        inline void initialize()
        {
            node::initialize(0);
        }

        /// True if the node's slots are full
        inline bool isfull() const
        {
            return (node::slotuse == leafslotmax);
        }

        /// True if few used entries, less than half full
        inline bool isfew() const
        {
            return (node::slotuse <= minleafslots);
        }

        /// True if node has too few entries
        inline bool isunderflow() const
        {
            return (node::slotuse < minleafslots);
        }
    };

public:
    // *** Small Statistics Structure

    /** A small struct containing basic statistics about the B+ tree. It can be
     * fetched using get_stats(). */
    struct tree_stats
    {
        /// Number of items in the B+ tree
        size_type       itemcount;

        /// Number of leaves in the B+ tree
        size_type       leaves;

        /// Number of inner nodes in the B+ tree
        size_type       innernodes;

        /// Base B+ tree parameter: The number of key/data slots in each leaf
        static const unsigned short     leafslots = btree_self::leafslotmax;

        /// Base B+ tree parameter: The number of key slots in each inner node.
        static const unsigned short     innerslots = btree_self::innerslotmax;

        /// Zero initialized
        inline tree_stats()
            : itemcount(0),
              leaves(0), innernodes(0)
        {
        }

        /// Return the total number of nodes
        inline size_type nodes() const
        {
            return innernodes + leaves;
        }

        /// Return the average fill of leaves
        inline double avgfill_leaves() const
        {
            return static_cast<double>(itemcount) / (leaves * leafslots);
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

public:
    // *** Constructors and Destructor

    /// Default constructor initializing an empty B+ tree with the standard key
    /// comparison function
    explicit inline btree(const allocator_type &alloc = allocator_type())
        : root(NULL), allocator(alloc)
    {
    }

    /// Constructor initializing an empty B+ tree with a special key
    /// comparison object
    explicit inline btree(const key_compare &kcf,
                          const allocator_type &alloc = allocator_type())
        : root(NULL), key_less(kcf), allocator(alloc)
    {
    }

    /// Frees up all used B+ tree memory pages
    inline ~btree()
    {
        clear();
    }

private:
    // *** Convenient Key Comparison Functions Generated From key_less

    /// True if a <= b ? constructed from key_less()
    inline bool key_lessequal(const key_type &a, const key_type b) const
    {
        return !key_less(b, a);
    }

    /// True if a > b ? constructed from key_less()
    inline bool key_greater(const key_type &a, const key_type &b) const
    {
        return key_less(b, a);
    }

    /// True if a >= b ? constructed from key_less()
    inline bool key_greaterequal(const key_type &a, const key_type b) const
    {
        return !key_less(a, b);
    }

    /// True if a == b ? constructed from key_less(). This requires the <
    /// relation to be a total order, otherwise the B+ tree cannot be sorted.
    inline bool key_equal(const key_type &a, const key_type &b) const
    {
        return !key_less(a, b) && !key_less(b, a);
    }


private:
    // *** Node Object Allocation and Deallocation Functions

    /// Return an allocator for leaf_node objects
    typename leaf_node::alloc_type leaf_node_allocator()
    {
        return typename leaf_node::alloc_type(allocator);
    }

    /// Return an allocator for inner_node objects
    typename inner_node::alloc_type inner_node_allocator()
    {
        return typename inner_node::alloc_type(allocator);
    }

    /// Allocate and initialize a leaf node
    inline leaf_node* allocate_leaf()
    {
        leaf_node *n = new (leaf_node_allocator().allocate(1)) leaf_node();
        n->initialize();
        stats.leaves++;
        return n;
    }

    /// Allocate and initialize an inner node
    inline inner_node* allocate_inner(unsigned short level)
    {
        inner_node *n = new (inner_node_allocator().allocate(1)) inner_node();
        n->initialize(level);
        stats.innernodes++;
        return n;
    }

    /// Correctly free either inner or leaf node, destructs all contained key
    /// and value objects
    inline void free_node(node *n)
    {
        if (n->isleafnode()) {
            leaf_node *ln = static_cast<leaf_node*>(n);
            typename leaf_node::alloc_type a(leaf_node_allocator());
            a.destroy(ln);
            a.deallocate(ln, 1);
            stats.leaves--;
        }
        else {
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
    void clear()
    {
        if (root)
        {
            clear_recursive(root);
            free_node(root);

            root = NULL;

            stats = tree_stats();
        }

        BTREE_ASSERT(stats.itemcount == 0);
    }

private:
    /// Recursively free up nodes
    void clear_recursive(node *n)
    {
        if (n->isleafnode())
        {
            leaf_node *leafnode = static_cast<leaf_node*>(n);

            for (unsigned int slot = 0; slot < leafnode->slotuse; ++slot)
            {
                // data objects are deleted by leaf_node's destructor
            }
        }
        else
        {
            inner_node *innernode = static_cast<inner_node*>(n);

            for (unsigned short slot = 0; slot < innernode->slotuse + 1; ++slot)
            {
                clear_recursive(innernode->childid[slot]);
                free_node(innernode->childid[slot]);
            }
        }
    }


public:
    // *** B+ Tree Node Binary Search Functions

    /// Searches for the first key in the node n less or equal to key. Uses
    /// binary search with an optional linear self-verification. This is a
    /// template function, because the slotkey array is located at different
    /// places in leaf_node and inner_node.
    template <typename node_type>
    inline int find_lower(const node_type *n, const key_type& key) const
    {
        if (n->slotuse == 0) return 0;

        int lo = 0,
            hi = n->slotuse - 1;

        while(lo < hi)
        {
            int mid = (lo + hi) >> 1;

            if (key_lessequal(key, n->slotkey[mid])) {
                hi = mid - 1;
            }
            else {
                lo = mid + 1;
            }
        }

        if (hi < 0 || key_less(n->slotkey[hi], key))
            hi++;

        BTREE_PRINT("btree::find_lower: on " << n << " key " << key << " -> (" << lo << ") " << hi << ", ");

        // verify result using simple linear search
        if (selfverify)
        {
            int i = n->slotuse - 1;
            while(i >= 0 && key_lessequal(key, n->slotkey[i]))
                i--;
            i++;

            BTREE_PRINT("testfind: " << i << std::endl);
            BTREE_ASSERT(i == hi);
        }
        else {
            BTREE_PRINT(std::endl);
        }

        return hi;
    }

    /// Searches for the first key in the node n greater than key. Uses binary
    /// search with an optional linear self-verification. This is a template
    /// function, because the slotkey array is located at different places in
    /// leaf_node and inner_node.
    template <typename node_type>
    inline int find_upper(const node_type *n, const key_type& key) const
    {
        if (n->slotuse == 0) return 0;

        int lo = 0,
            hi = n->slotuse - 1;

        while(lo < hi)
        {
            int mid = (lo + hi) >> 1;

            if (key_less(key, n->slotkey[mid])) {
                hi = mid - 1;
            }
            else {
                lo = mid + 1;
            }
        }

        if (hi < 0 || key_lessequal(n->slotkey[hi], key))
            hi++;

        BTREE_PRINT("btree::find_upper: on " << n << " key " << key << " -> (" << lo << ") " << hi << ", ");

        // verify result using simple linear search
        if (selfverify)
        {
            int i = n->slotuse - 1;
            while(i >= 0 && key_less(key, n->slotkey[i]))
                i--;
            i++;

            BTREE_PRINT("btree::find_upper testfind: " << i << std::endl);
            BTREE_ASSERT(i == hi);
        }
        else {
            BTREE_PRINT(std::endl);
        }

        return hi;
    }

public:
    // *** Access Functions to the Item Count

    /// Return the number of key/data pairs in the B+ tree
    inline size_type size() const
    {
        return stats.itemcount;
    }

    /// Returns true if there is at least one key/data pair in the B+ tree
    inline bool empty() const
    {
        return (size() == size_type(0));
    }

    /// Return a const reference to the current statistics.
    inline const struct tree_stats& get_stats() const
    {
        return stats;
    }





public:
    typedef std::vector<Operation<key_type> > update_list;

    void applyUpdates(const update_list& updates) {
        if (root == NULL) {
            root = allocate_leaf();
            spare_leaf = allocate_leaf();
            stats.leaves -= 1;
        }
        // Compute exclusive prefix sum, so that weightdelta[end]-weightdelta[begin] 
        // computes the weight delta realized by the updates in range [begin, end)
        std::vector<long> weightdelta;
        weightdelta.reserve(updates.size()+1);
        partial_sum(updates.begin(), updates.end(), weightdelta.begin());

        if (root->isleafnode()) {
            mergeIntoLeaf(static_cast<leaf_node*>(root), static_cast<leaf_node*>(spare_leaf), updates, 0, updates.size());
            std::swap(root, spare_leaf);
        }

        if (root->slotuse == 0) {
            free_node(root);
            // FIXME: free spare leaf!
            root = NULL;
        }

        if (selfverify) {
            verify();
        }
    }

private:

    /* Exclusive prefix sum */
    static void partial_sum(typename update_list::const_iterator first, typename update_list::const_iterator last, std::vector<long>::iterator result) {
        long val = 0;
        *result++ = val;
        while (first != last) {
            *result++ = val = val + ((first++)->type == Operation<key_type>::INSERT ? 1 : -1);
        }
    }

    void mergeIntoLeaf(leaf_node* in_leaf, leaf_node* out_leaf, const update_list& updates, const size_t begin, const size_t end) {
        int in = 0; // existing key to read
        int out = 0; // position where to write

        print_node(std::cout, in_leaf);

        for (size_t i = begin; i != end; ++i) {
            switch (updates[i].type) {
            case Operation<key_type>::DELETE:
                // We know the element is in here, so no bounds checks
                while (!key_equal(in_leaf->slotkey[in], updates[i].data)) {
                    out_leaf->slotkey[out++] = in_leaf->slotkey[in++];
                }
                ++in; // delete the element by jumping over it
                break;
            case Operation<key_type>::INSERT:
                while(in < in_leaf->slotuse && key_less(in_leaf->slotkey[in], updates[i].data)) {
                    out_leaf->slotkey[out++] = in_leaf->slotkey[in++];
                }
                out_leaf->slotkey[out++] = updates[i].data;
                break;
            }
        } 
        while (in < in_leaf->slotuse) {
            out_leaf->slotkey[out++] = in_leaf->slotkey[in++];
        }
        out_leaf->slotuse = out;
        stats.itemcount += out - in;  
        print_node(std::cout, out_leaf);
    }


#ifdef BTREE_DEBUG
public:
    // *** Debug Printing

    /// Print out the B+ tree structure with keys onto the given ostream. This
    /// function requires that the header is compiled with BTREE_DEBUG and that
    /// key_type is printable via std::ostream.
    void print(std::ostream &os) const
    {
        if (root) {
            print_node(os, root, 0, true);
        }
    }

    /// Print out only the leaves via the double linked list.
    void print_leaves(std::ostream &os) const
    {
        os << "leaves:" << std::endl;


    }

private:

    /// Recursively descend down the tree and print out nodes.
    static void print_node(std::ostream &os, const node* node, unsigned int depth=0, bool recursive=false)
    {
        for(unsigned int i = 0; i < depth; i++) os << "  ";

        os << "node " << node << " level " << node->level << " slotuse " << node->slotuse << std::endl;

        if (node->isleafnode())
        {
            const leaf_node *leafnode = static_cast<const leaf_node*>(node);

            for(unsigned int i = 0; i < depth; i++) os << "  ";

            for (unsigned int slot = 0; slot < leafnode->slotuse; ++slot)
            {
                os << leafnode->slotkey[slot] << "  "; // << "(data: " << leafnode->slotdata[slot] << ") ";
            }
            os << std::endl;
        }
        else
        {
            const inner_node *innernode = static_cast<const inner_node*>(node);

            for(unsigned int i = 0; i < depth; i++) os << "  ";

            for (unsigned short slot = 0; slot < innernode->slotuse; ++slot)
            {
                os << "(" << innernode->childid[slot] << ") " << innernode->slotkey[slot] << " ";
            }
            os << "(" << innernode->childid[innernode->slotuse] << ")" << std::endl;

            if (recursive)
            {
                for (unsigned short slot = 0; slot < innernode->slotuse + 1; ++slot)
                {
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
    void verify() const
    {
        key_type minkey, maxkey;
        tree_stats vstats;

        if (root)
        {
            verify_node(root, &minkey, &maxkey, vstats);

            assert( vstats.itemcount == stats.itemcount );
            assert( vstats.leaves == stats.leaves );
            assert( vstats.innernodes == stats.innernodes );
        }
    }

private:

    /// Recursively descend down the tree and verify each node
    void verify_node(const node* n, key_type* minkey, key_type* maxkey, tree_stats &vstats) const
    {
        BTREE_PRINT("verifynode " << n << std::endl);

        if (n->isleafnode())
        {
            const leaf_node *leaf = static_cast<const leaf_node*>(n);

            assert( leaf == root || !leaf->isunderflow() );
            assert( leaf->slotuse > 0 );

            for(unsigned short slot = 0; slot < leaf->slotuse - 1; ++slot)
            {
                assert(key_lessequal(leaf->slotkey[slot], leaf->slotkey[slot + 1]));
            }

            *minkey = leaf->slotkey[0];
            *maxkey = leaf->slotkey[leaf->slotuse - 1];

            vstats.leaves++;
            vstats.itemcount += leaf->slotuse;
        }
        else // !n->isleafnode()
        {
            const inner_node *inner = static_cast<const inner_node*>(n);
            vstats.innernodes++;

            assert( inner == root || !inner->isunderflow() );
            assert( inner->slotuse > 0 );

            for(unsigned short slot = 0; slot < inner->slotuse - 1; ++slot)
            {
                assert(key_lessequal(inner->slotkey[slot], inner->slotkey[slot + 1]));
            }

            for(unsigned short slot = 0; slot <= inner->slotuse; ++slot)
            {
                const node *subnode = inner->childid[slot];
                key_type subminkey = key_type();
                key_type submaxkey = key_type();

                assert(subnode->level + 1 == inner->level);
                verify_node(subnode, &subminkey, &submaxkey, vstats);

                BTREE_PRINT("verify subnode " << subnode << ": " << subminkey << " - " << submaxkey << std::endl);

                if (slot == 0)
                    *minkey = subminkey;
                else
                    assert(key_greaterequal(subminkey, inner->slotkey[slot-1]));

                if (slot == inner->slotuse)
                    *maxkey = submaxkey;
                else
                    assert(key_equal(inner->slotkey[slot], submaxkey));
            }
        }
    }
};

} // namespace stx

#endif // _STX_BTREE_H_
