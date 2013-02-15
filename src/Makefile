#####################################################
#
# Configure your specific makefile here
# 
# I) In the first section put down all your different
#	 targets you may want to build
#
# II) Setup flags for compilation below the target section
#
# III) If you know how to, setup special targets in the lower
#     section of the makefile
#
# IV) If you need to include external paths, set DEPENDENCYDIRS
#     to include external dependencies also
#
# IMPORTANT: you need to call make configure first
#            to generate VPATH information. Call this
#            also after adding more source direcotries.
#
# call with DEBUG=yes for debug mode (e.g. make all DEBUG=yes)
#
#####################################################
CODE=time_grid_instances1 time_grid_instances2 time_road_instances1 time_road_instances2 time_pq_set time_pq_btree time_labelsetting tbb_inner_loop_parallelization
TESTS=test_labelset test_labelsetting test_pq_btree test_pq_btree_paretomin

#list of all normal / parallel targets
TARGETS = $(TESTS) $(CODE)
PARALLEL = $(TESTS) $(CODE)

DEFAULT_OPS = -Wall -Wextra -DDCACHE_LINESIZE=$(shell getconf LEVEL1_DCACHE_LINESIZE) $(CPPFLAGS)

# Sequential
CXXFLAGS = -O3 -mtune=native -march=native -DNDEBUG $(DEFAULT_OPS) -std=c++11
CXXFLAGSDBG = -O3 -fno-inline -fno-default-inline -g3 -W $(DEFAULT_OPS) -std=c++11
CXXLIBS= -lrt -ltbb

# Parallel
CXXPFLAGS = -O3 -mtune=native -march=native -DNDEBUG $(DEFAULT_OPS) -std=c++11 -DPARALLEL_BUILD
CXXPFLAGSDBG = -O3 -fno-inline -fno-default-inline -g3 -W $(DEFAULT_OPS) -std=c++11 -DPARALLEL_BUILD -DTBB_USE_DEBUG=1 -DTBB_USE_ASSERT=1 -DTBB_USE_THREADING_TOOLS=1
CXXPLIBS= -lrt -ltbb -ltbbmalloc -lpthread

#set directorys here the project depends upon (e.g. otherwise linked directories)
DEPENDENCYDIRS=.

LOG_BUILDERROR=build.error.log

####################################################
# DO NOT EDIT BELOW THIS LINE!
# (unless of course you know what you are doing)
####################################################
#place to store the dependency files in
DEPDIR = ./dep/
#place to store binary files in
BINDIR = ./bin/
#place to store object files in
OBJDIR = ./bin/obj/
#place to store vpath information in
VPDIR = ./.vpath/

#set to yes for debug build, use parameter to do it
ifndef DEBUG
DEBUG=no
endif

#necessary for clean. to be a phony target
CLEAN_TARGETS = $(addprefix clean.,$(TARGETS))\
				$(addprefix clean.,$(addsuffix .par,$(PARALLEL)))\
				$(addprefix clean.,$(addsuffix .dbg,$(TARGETS)))\
				$(addprefix clean.,$(addsuffix .dbg,$(addsuffix .par,$(PARALLEL))))

.PHONY: all clean $(CLEAN_TARGETS)

PARALLEL_TARGETS = $(addsuffix .par,$(PARALLEL))

ifeq ($(MAKECMDGOALS),clean)
BUILD_TARGETS :=
else ifeq ($(MAKECMDGOALS),all)
ifeq ($(DEBUG),yes)
BUILD_TARGETS := $(addsuffix .dbg,$(TARGETS)) $(addsuffix .dbg,$(PARALLEL_TARGETS))
else
BUILD_TARGETS := $(TARGETS) $(PARALLEL_TARGETS)
endif
else
ifeq ($(DEBUG),yes)
BUILD_TARGETS := $(addsuffix .dbg,$(MAKECMDGOALS))
else
BUILD_TARGETS := $(MAKECMDGOALS)
endif
endif

SUFFIX =
ifeq ($(DEBUG),yes)
SUFFIX := .dbg
endif

SHELL = /bin/bash

-include settings.mk

all: $(TARGETS) $(PARALLEL_TARGETS)

test: all
	cd $(BINDIR); $(foreach test,$(TESTS),./$(test);)
	cd $(BINDIR); $(foreach test,$(PARA_TESTS),./$(test);)

time_pq_mcstl_set: time_pq_set
	/software/gcc-4.2.4-64/bin/g++-4.2.4 -O3 -mtune=native -march=native -DNDEBUG $(DEFAULT_OPS) -DENABLE_MCSTL -fopenmp -I../lib/mcstl/c++ -o ./bin/time_pq_mcstl_set time_pq_set.cpp

