#
# Commmon rules etc. for the MCSTL testing framework
#
# Copyright (C) 2007 by Andreas Beckmann <beckmann@mpi-inf.mpg.de>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

OPTIMIZE		?= -O3
WARNINGS		?= -W -Wall
UNAME_M			:= $(shell uname -m)
CPPFLAGS_i686		?= -march=i686
CPPFLAGS		+= $(OPTIMIZE) $(WARNINGS)
CPPFLAGS		+= $(CPPFLAGS_$(UNAME_M))
CPPFLAGS		+= -MD
MCSTL			?= $(TOPDIR)
CPPFLAGS_MCSTL		+= -fopenmp $(CPPFLAGS_MCSTL_EXTRA) -I$(MCSTL)/c++ -D__MCSTL__
LDLIBS_MCSTL		+= -fopenmp
CPPFLAGS_MCSTL_EXTRA	+= $(if $(findstring 4.3,$(CXX)),-I$(MCSTL)/c++/mod_stl/gcc-4.3)


PREPROCESS.cc	 = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -E

%.bin: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

