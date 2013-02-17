#!/bin/bash
#
# Copyright 2005-2013 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks.
#
# Threading Building Blocks is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# Threading Building Blocks is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Threading Building Blocks; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As a special exception, you may use this file as part of a free software
# library without restriction.  Specifically, if other files instantiate
# templates or use macros or inline functions from this file, or you compile
# this file and link it with other files to produce an executable, this
# file does not by itself cause the resulting executable to be covered by
# the GNU General Public License.  This exception does not however
# invalidate any other reasons why the executable file might be covered by
# the GNU General Public License.


get_ia32(){ 
	echo SUBSTITUTE_IA32_ARCH_HERE
}

get_ia64(){
	echo SUBSTITUTE_IA64_ARCH_HERE
}

get_intel64(){
	echo cc4.1.0_libc2.4_kernel2.6.16.21
}

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export TBBROOT=$DIR/../lib/tbb
if [[ "$1" != "ia32" && "$1" != "intel64" && "$1" != "ia64" ]]; then
   echo "ERROR: Unknown switch '$1'. Accepted values: ia32, intel64, ia64"
   return 1;
fi

TBB_ARCH=$(get_$1)

if [ -z "${LD_LIBRARY_PATH}" ]
then
   LD_LIBRARY_PATH="$TBBROOT/lib/$1/$TBB_ARCH"; export LD_LIBRARY_PATH
else
   LD_LIBRARY_PATH="$TBBROOT/lib/$1/$TBB_ARCH:${LD_LIBRARY_PATH}"; export LD_LIBRARY_PATH
fi
if [ -z "${LIBRARY_PATH}" ]
then
   LIBRARY_PATH="$TBBROOT/lib/$1/$TBB_ARCH"; export LIBRARY_PATH
else
   LIBRARY_PATH="$TBBROOT/lib/$1/$TBB_ARCH:${LIBRARY_PATH}"; export LIBRARY_PATH
fi
if [ -z "${CPATH}" ]; then
    CPATH="${TBBROOT}/include"; export CPATH
else
    CPATH="${TBBROOT}/include:$CPATH"; export CPATH
fi

