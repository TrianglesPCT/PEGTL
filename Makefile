# Copyright (c) 2014-2015 Dr. Colin Hirsch and Daniel Frey
# Please see LICENSE for license or visit https://github.com/ColinH/PEGTL

CXX ?= g++

CPPFLAGS := -I. -std=c++11 -pedantic
CXXFLAGS := -Wall -Wextra -Werror -O3

.PHONY: all tgz test clean

SOURCES := $(wildcard */*.cc)
DEPENDS := $(SOURCES:%.cc=build/%.d)
BINARIES := $(SOURCES:%.cc=build/%)

UNIT_TESTS := $(filter build/unit_tests/%,$(BINARIES))

all: run

run: $(BINARIES)
	@set -e; for T in $(UNIT_TESTS); do $$T; done
	@echo "All $(words $(UNIT_TESTS)) unit tests passed."

clean:
	rm -rf build/*

.SECONDARY:

build/%.d: %.cc Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -MM -MQ $@ $< -o $@

build/%: %.cc build/%.d
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

ifeq ($(findstring $(MAKECMDGOALS),clean),)
-include $(DEPENDS)
endif