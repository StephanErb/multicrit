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

CPPFLAGS	+= $(CPPFLAGS_MCSTL)

SUBDIRS		?= c++ c++/meta c++/bits
SKIP_PATTERNS	?=
SKIP_HEADERS	:= $(wildcard $(foreach d,$(SUBDIRS),$(foreach p,$(SKIP_PATTERNS),$(TOPDIR)/$d/$p)))
ALL_HEADERS	:= $(wildcard $(foreach d,$(SUBDIRS),$(TOPDIR)/$d/*.h))
HEADERS		 = $(filter-out $(SKIP_HEADERS), $(ALL_HEADERS))
HEADERS_ESCAPED	+= $(subst /,~,$(HEADERS))
SOURCES		 = $(HEADERS_ESCAPED:%=mcstl-header-%.cpp)


all: sources
#all: preprocess
all: compile

sources: $(SOURCES)
preprocess: $(SOURCES:.cpp=.ii)
compile: $(SOURCES:.cpp=.o)

.PHONY: all sources preprocess compile


%.ii: %.cpp
	$(PREPROCESS.cc) $(OUTPUT_OPTION) $<

mcstl-header-%.cpp::
	echo '#include "$(subst ~,/,$*)"' > $@


clean:
	$(RM) *.ii *.o *.d
	$(RM) mcstl-header-*.cpp


-include *.d

.SECONDARY:
