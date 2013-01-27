#
# Several tests for MCSTL
#
# Copyright (C) 2007 by Andreas Beckmann <beckmann@mpi-inf.mpg.de>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

TOPDIR		?= ..
TESTDIR		?= $(TOPDIR)/tests

CXX		 = g++-4.2
#CXX		 = g++-4.3

# you can override CXX etc. here:
-include $(TESTDIR)/Makefile.local

include $(TESTDIR)/common.mk

###########################################################################

CPPFLAGS	+= $(CPPFLAGS_MCSTL)
LDLIBS		+= $(LDLIBS_MCSTL)

SOURCE_DIRS	 = . compile
SOURCES		:= $(wildcard $(foreach d,$(SOURCE_DIRS),$d/*.cpp))
TARGETS		 = $(SOURCES:.cpp=.bin)


all: all-needed
all: all-optional
all-needed: cmpl-c++-headers
all-needed: targets
all-optional: cmpl-mcstl-headers

targets: $(TARGETS)
$(TARGETS): CC=$(CXX)

cmpl-c++-headers:
	$(MAKE) -C compile-c++-headers

cmpl-mcstl-headers:
	$(MAKE) -C compile-mcstl-headers

clean: clean-compile-c++-headers clean-compile-mcstl-headers
	$(RM) $(foreach d,$(SOURCE_DIRS),$d/*.o $d/*.d)
	$(RM) $(TARGETS)


clean-%:
	$(MAKE) -C $* clean

.PHONY: all all-needed all-optional clean cmpl-c++-headers cmpl-mcstl-headers

-include $(foreach d,$(SOURCE_DIRS),$d/*.d)

.SECONDARY:
