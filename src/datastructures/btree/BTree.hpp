/**
 * Weight-balanced B-Tree with batch updates, partially based on:
 *
 * STX B+ Tree Template Classes v0.8.6
 * Copyright (C) 2008-2011 Timo Bingmann
 *
 */

#ifndef _BTREE_H_
#define _BTREE_H_

#ifdef PARALLEL_BUILD
#include "BTree_parallel.hpp"
#else
#include "BTree_sequential.hpp"
#endif

#endif
