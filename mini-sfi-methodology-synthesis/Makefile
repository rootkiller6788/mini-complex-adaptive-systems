# Makefile for mini-sfi-methodology-synthesis
# SFI Methodology Synthesis Module

CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11 -O2 -g
INCLUDES := -Iinclude
LDFLAGS  := -lm

SRCDIR   := src
TESTDIR  := tests
EXAMPLEDIR := examples

# Source files
SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(SRC:.c=.o)

# Test files
TESTS := $(wildcard $(TESTDIR)/*.c)
TESTBINS := $(TESTS:.c=.exe)

# Example files
EXAMPLES := $(wildcard $(EXAMPLEDIR)/*.c)
EXAMPLEBINS := $(EXAMPLES:.c=.exe)

.PHONY: all clean test examples check

all: test examples

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TESTDIR)/%.exe: $(TESTDIR)/%.c $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(OBJ) $(LDFLAGS) -o $@

test: $(TESTBINS)
	@echo "=== Running SFI Methodology Synthesis Tests ==="
	@for t in $(TESTBINS); do echo "--- $$(basename $$t) ---"; ./$$t || exit 1; echo; done
	@echo "=== All Tests Passed ==="

$(EXAMPLEDIR)/%.exe: $(EXAMPLEDIR)/%.c $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(OBJ) $(LDFLAGS) -o $@

examples: $(EXAMPLEBINS)

clean:
	rm -f src/*.o tests/*.exe examples/*.exe

check: $(OBJ)
	@echo "All source files compile successfully."

wc:
	@echo "include/ lines:" && wc -l include/*.h
	@echo "src/ lines:" && wc -l src/*.c
	@echo "Total include+src:" && cat include/*.h src/*.c | wc -l
