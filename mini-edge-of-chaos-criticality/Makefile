CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm
EXE_EXT = .exe
LIB = build/libedge_of_chaos_criticality.a
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TEST_SRC = tests/test_eoc.c
TEST_BIN = build/test_eoc$(EXE_EXT)
EXAMPLE_SRCS = $(wildcard examples/*.c)
EXAMPLE_BINS = $(patsubst examples/%.c,build/%$(EXE_EXT),$(EXAMPLE_SRCS))

.PHONY: all builddir clean test examples demo

all: builddir $(LIB)

builddir:
	@mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -ledge_of_chaos_criticality $(LDFLAGS)
	$(TEST_BIN)

examples: all $(EXAMPLE_BINS)
	@echo "All examples built."

build/%$(EXE_EXT): examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -ledge_of_chaos_criticality $(LDFLAGS)

demo: examples
	@echo "=== Running sandpile_demo ==="
	build/example_sandpile$(EXE_EXT)
	@echo ""
	@echo "=== Running logistic_map_demo ==="
	build/example_logistic_map$(EXE_EXT)
	@echo ""
	@echo "=== Running cellular_automata_demo ==="
	build/example_cellular_automata$(EXE_EXT)

clean:
	rm -rf build
