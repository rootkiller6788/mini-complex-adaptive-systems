CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wno-misleading-indentation -Wno-unused-parameter -O2 -Iinclude
LDFLAGS = -lm
AR = ar
LIB = build/libswarm.a

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

TEST_SRC = tests/test_swarm.c
TEST_BIN = build/test_swarm
ifeq ($(OS),Windows_NT)
    TEST_EXE = $(TEST_BIN).exe
    EXE_SUFFIX = .exe
else
    TEST_EXE = $(TEST_BIN)
    EXE_SUFFIX =
endif

EXAMPLE_SRCS = $(wildcard examples/*.c)
EXAMPLE_BINS = $(patsubst examples/%.c,build/%$(EXE_SUFFIX),$(EXAMPLE_SRCS))

.PHONY: all builddir clean test examples demo

all: builddir $(LIB)

builddir:
	mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -lswarm $(LDFLAGS)
	$(TEST_BIN)

examples: all $(EXAMPLE_BINS)
	@echo All examples built.

build/%$(EXE_SUFFIX): examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lswarm $(LDFLAGS)

demo: examples
	@for f in $(EXAMPLE_BINS); do \
		echo "=== $$f ==="; \
		$$f; \
		echo; \
	done

clean:
	rm -rf build
