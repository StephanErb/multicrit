#!/bin/csh
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


setenv TBBROOT "SUBSTITUTE_INSTALL_DIR_HERE" 
set TBB_ARCH_ia32=SUBSTITUTE_IA32_ARCH_HERE
set TBB_ARCH_ia64=SUBSTITUTE_IA64_ARCH_HERE
set TBB_ARCH_intel64=SUBSTITUTE_INTEL64_ARCH_HERE

switch ("$1")
  case "ia32":
      set TBB_ARCH=$TBB_ARCH_ia32
      breaksw;
  case "intel64":
      set TBB_ARCH=$TBB_ARCH_intel64
      breaksw;
  case "ia64":
      set TBB_ARCH=$TBB_ARCH_ia64
      breaksw;
  default:
      echo "ERROR: Unknown switch '$1'. Accepted values: ia32, intel64, ia64"
      breaksw;
endsw

if (! $?LIBRARY_PATH) then 
    setenv LIBRARY_PATH "${TBBROOT}/lib/${1}/${TBB_ARCH}" 
else 
    setenv LIBRARY_PATH "${TBBROOT}/lib/${1}/${TBB_ARCH}:$LIBRARY_PATH" 
endif 
if (! $?LD_LIBRARY_PATH) then 
    setenv LD_LIBRARY_PATH "${TBBROOT}/lib/${1}/${TBB_ARCH}" 
else 
    setenv LD_LIBRARY_PATH "${TBBROOT}/lib/${1}/${TBB_ARCH}:$LD_LIBRARY_PATH" 
endif 
if (! $?CPATH) then 
    setenv CPATH "${TBBROOT}/include" 
else 
    setenv CPATH "${TBBROOT}/include:$CPATH" 
endif 
