# Makefile for mini-artificial-life-langton
# Langton's Ant and Artificial Life — Cellular Automaton Theory

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g -Iinclude
LDFLAGS := -lm
AR := ar
ARFLAGS := rcs

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)
LIB := libalife.a

TEST_SRCS := $(wildcard tests/*.c)
TEST_BINS := $(TEST_SRCS:.c=.test)

EXAMPLE_SRCS := $(wildcard examples/*.c)
EXAMPLE_BINS := $(EXAMPLE_SRCS:.c=.ex)

.PHONY: all clean test examples lib

all: lib test examples

lib: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/%.test: tests/%.c $(LIB)
	$(CC) $(CFLAGS) $< -L. -lalife $(LDFLAGS) -o $@

examples/%.ex: examples/%.c $(LIB)
	$(CC) $(CFLAGS) $< -L. -lalife $(LDFLAGS) -o $@

test: $(TEST_BINS)
	@echo "=== Running Tests ==="
	@passed=0; failed=0; \
	for t in $(TEST_BINS); do \
		if ./$$t; then \
			echo "PASS: $$t"; passed=$$((passed+1)); \
		else \
			echo "FAIL: $$t"; failed=$$((failed+1)); \
		fi; \
	done; \
	echo "=== $$passed passed, $$failed failed ==="; \
	if [ $$failed -gt 0 ]; then exit 1; fi

examples: $(EXAMPLE_BINS)

clean:
	rm -f src/*.o $(LIB) tests/*.test examples/*.ex
