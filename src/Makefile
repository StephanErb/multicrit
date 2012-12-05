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
CODE=time_labelsetting time_grid_instances1 time_grid_instances2 time_road_instances1 time_road_instances2

TESTS=test_labelset test_labelsetting

#list of all normal / parallel targets
TARGETS = $(TESTS) $(CODE)
PARALLEL =

CXXFLAGS = -O3 -mtune=native -march=native -DNDEBUG -Wall -Wextra
CXXPFLAGS = -O3 -mtune=native -march=native -DNDEBUG -Wall -Wextra -fopenmp
CXXFLAGSDBG = -O3 -g3 -W -Wall -Wunused -Wextra -DPARANOID -fno-inline -fno-default-inline
CXXPFLAGSDBG = -O3 -g3 -W -Wall -Wunused -Wextra -DPARANOID -fopenmp

CXXLIBS= -lrt
CXXPLIBS= -lrt

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

