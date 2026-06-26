# Makefile for mini-genetic-algorithms-holland
# Genetic Algorithms — John Holland's Theory and Practice
#
# Covers: Schema Theorem, Building Block Hypothesis, Selection/Crossover/Mutation,
#         Fitness Landscapes, Island Models, Spatial GA, Co-evolution, Multi-Objective

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2 -g
LDFLAGS := -lm

SRCDIR  := src
INCDIR  := include
TESTDIR := tests
EXAMDIR := examples
BUILDDIR := build

SRCS    := $(wildcard $(SRCDIR)/*.c)
TESTS   := $(wildcard $(TESTDIR)/*.c)
EXAMPLES := $(wildcard $(EXAMDIR)/*.c)

OBJS    := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
TESTBINS := $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/test_%,$(TESTS))
EXAMPLEBINS := $(patsubst $(EXAMDIR)/%.c,$(BUILDDIR)/example_%,$(EXAMPLES))

INCFLAGS := -I$(INCDIR)

.PHONY: all clean test examples check

all: $(BUILDDIR)/libga.a $(TESTBINS) $(EXAMPLEBINS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCFLAGS) -c $< -o $@

$(BUILDDIR)/libga.a: $(OBJS)
	ar rcs $@ $^

$(BUILDDIR)/test_%: $(TESTDIR)/%.c $(BUILDDIR)/libga.a | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCFLAGS) $< -L$(BUILDDIR) -lga $(LDFLAGS) -o $@

$(BUILDDIR)/example_%: $(EXAMDIR)/%.c $(BUILDDIR)/libga.a | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCFLAGS) $< -L$(BUILDDIR) -lga $(LDFLAGS) -o $@

test: $(TESTBINS)
	@echo "=== Running Tests ==="
	@for t in $(TESTBINS); do \
		echo "--- $$(basename $$t) ---"; \
		./$$t && echo "PASS" || echo "FAIL"; \
	done
	@echo "=== Tests Complete ==="

examples: $(EXAMPLEBINS)
	@echo "=== Running Examples ==="
	@for e in $(EXAMPLEBINS); do \
		echo "--- $$(basename $$e) ---"; \
		./$$e; \
	done

check:
	@echo "=== Line Count Check ==="
	@lines=$$(cat $(SRCDIR)/*.c $(INCDIR)/*.h 2>/dev/null | wc -l); \
	echo "include/ + src/ = $$lines lines"; \
	if [ $$lines -ge 3000 ]; then echo "PASS: >=3000 lines"; else echo "FAIL: <3000 lines"; fi

clean:
	rm -rf $(BUILDDIR)
	rm -f *.o
