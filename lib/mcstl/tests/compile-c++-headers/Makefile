#
# Several compile tests for MCSTL
#
# Copyright (C) 2007 by Andreas Beckmann <beckmann@mpi-inf.mpg.de>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

TOPDIR		?= ../..
TESTDIR		?= $(TOPDIR)/tests

CXX		 = g++-4.2
#CXX		 = g++-4.3

# you can override CXX etc. here:
-include $(TESTDIR)/Makefile.local

include $(TESTDIR)/common.mk

###########################################################################

MCSTL_HEADERS	 = mcstl.h
CXX_HEADERS	 = algorithm bitset complex deque exception fstream functional iomanip ios iosfwd iostream istream iterator limits list locale map memory new numeric ostream queue set sstream stack stdexcept streambuf string typeinfo utility valarray vector

MCSTL_SOURCES	 = $(MCSTL_HEADERS:%=c++-%.cpp)
CXX_SOURCES	 = $(CXX_HEADERS:%=c++-%.cpp)


all: sources
#all: preprocess
all: compile

sources: $(MCSTL_SOURCES) $(CXX_SOURCES)
preprocess: $(CXX_SOURCES:.cpp=.diff)
compile: $(CXX_SOURCES:.cpp=.nomcstl.o)
compile: $(CXX_SOURCES:.cpp=.mcstl.o)
compile: $(MCSTL_SOURCES:.cpp=.mcstl.o)

.PHONY: all sources preprocess compile


%.diff: %.nomcstl.ii %.mcstl.ii
	diff -u $^ > $@ || true

%.nomcstl.ii: %.cpp
	$(PREPROCESS.cc) $(CPPFLAGS_NOMCSTL) $(OUTPUT_OPTION) $<

%.mcstl.ii: %.cpp
	$(PREPROCESS.cc) $(CPPFLAGS_MCSTL) $(OUTPUT_OPTION) $<

%.nomcstl.o: %.cpp
	$(COMPILE.cc) $(CPPFLAGS_NOMCSTL) $(OUTPUT_OPTION) $<

%.mcstl.o: %.cpp
	$(COMPILE.cc) $(CPPFLAGS_MCSTL) $(OUTPUT_OPTION) $<

c++-%.cpp::
	echo '#include <$*>' > $@


clean:
	$(RM) *.ii *.diff *.o *.d
	$(RM) c++-*.cpp


-include *.d

.SECONDARY:
