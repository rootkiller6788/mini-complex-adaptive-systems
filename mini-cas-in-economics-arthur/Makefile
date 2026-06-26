CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude -Wno-unused-parameter
LDFLAGS = -lm
LIB = build/libarthur_cas.a
SRCS = src/ace_core.c src/ace_dynamics.c src/ace_market.c src/ace_evolution.c
OBJS = build/ace_core.o build/ace_dynamics.o build/ace_market.o build/ace_evolution.o
TEST_SRC = tests/test_arthur_cas.c
TEST_BIN = build/test_arthur_cas

.PHONY: all builddir clean test examples demo

all: builddir $(LIB)

builddir:
	@mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $(LIB) $(OBJS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) $(OBJS) $(LDFLAGS)
	./$(TEST_BIN)

examples: all
	$(CC) $(CFLAGS) -o build/example_el_farol examples/example_el_farol.c $(OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o build/example_polya_urn examples/example_polya_urn.c $(OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o build/example_tech_lockin examples/example_tech_lockin.c $(OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o build/demo_overview demos/demo_overview.c $(OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o build/bench_core benches/bench_core.c $(OBJS) $(LDFLAGS)
	@echo "All examples built."

demo: examples
	@echo "=== Running demo_overview ==="
	./build/demo_overview
	@echo ""
	@echo "=== Running example_el_farol ==="
	./build/example_el_farol
	@echo ""
	@echo "=== Running example_polya_urn ==="
	./build/example_polya_urn
	@echo ""
	@echo "=== Running example_tech_lockin ==="
	./build/example_tech_lockin

clean:
	rm -rf build
