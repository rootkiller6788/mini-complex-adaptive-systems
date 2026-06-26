CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm
EXE_EXT = .exe
LIB = build/libnk_fitness_landscapes.a
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TEST_SRC = tests/test_nk.c
TEST_BIN = build/test_nk$(EXE_EXT)
EXAMPLE_SRCS = $(wildcard examples/*.c)
EXAMPLE_BINS = $(patsubst examples/%.c,build/%$(EXE_EXT),$(EXAMPLE_SRCS))
DEMO_SRCS = $(wildcard demos/*.c)
DEMO_BINS = $(patsubst demos/%.c,build/%$(EXE_EXT),$(DEMO_SRCS))

.PHONY: all builddir clean test examples demo

all: builddir $(LIB)

builddir:
	@mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -lnk_fitness_landscapes $(LDFLAGS)
	$(TEST_BIN)

examples: all $(EXAMPLE_BINS)
	@echo "All NK landscape examples built."

build/%$(EXE_EXT): examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lnk_fitness_landscapes $(LDFLAGS)

demo: examples $(DEMO_BINS)
	@echo "=== Running NK Fitness Landscape Demo ==="
	build/demo_nk$(EXE_EXT)

build/demo_nk$(EXE_EXT): demos/demo_nk.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lnk_fitness_landscapes $(LDFLAGS)

clean:
	rm -rf build
