/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl.h
 *  @brief End-user include file. Provides advanced settings and tuning options for the MCSTL. */

#ifndef _MCSTL_H
#define _MCSTL_H 1

#ifndef __MCSTL__
#define __MCSTL__
#endif

/** 
 * @namespace mcstl
 * @brief Contains all entities related to the parallel implementation of the STL.
 * It is called the Multi-Core Standard Template Library.
 */

#include <bits/mcstl_features.h>
#include <bits/mcstl_compiletime_settings.h>
#include <bits/mcstl_types.h>
#include <bits/mcstl_tags.h>
#include <bits/mcstl_settings.h>

#endif /* _MCSTL_H */
