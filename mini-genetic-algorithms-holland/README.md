# mini-genetic-algorithms-holland

**Genetic Algorithms — John H. Holland's Evolutionary Computation Framework**

Complete implementation of John Holland's genetic algorithms including Schema Theory,
Building Block Hypothesis, comprehensive operator library, fitness landscape analysis,
and advanced multi-population models. Covers L1-L6 Complete, L7-L8 Complete, L9 Partial.

---

## Module Status: COMPLETE ✅

- **L1**: Complete — 12 core definitions (Gene, Allele, Locus, Chromosome, Population, Schema, etc.)
- **L2**: Complete — 14 core concepts (selection pressure, convergence, diversity, niching, etc.)
- **L3**: Complete — 12 mathematical structures (binary/real/permutation spaces, schema lattice, etc.)
- **L4**: Complete — 8 fundamental laws (Schema Theorem, Building Block Hypothesis, Price's Eq, NFL, etc.)
- **L5**: Complete — 28 algorithms (5 selection, 7 crossover, 7 mutation, 6 replacement, encoding schemes)
- **L6**: Complete — 15 canonical problems (OneMax, TSP, Rastrigin, Rosenbrock, NK, Royal Road, etc.)
- **L7**: Complete — 6 applications (Island Model, Spatial GA, Multi-Objective, External Archive, etc.)
- **L8**: Complete — 6 advanced topics (Vose Model, Messy GA, Co-evolution, Speciation, NSGA-II, etc.)
- **L9**: Partial — 5 research topics documented, some partially implemented

**Score: 17/18 — COMPLETE**

---

## Core Definitions (L1)

| Term | Definition | Struct |
|------|-----------|--------|
| **Gene** | Atomic unit of heredity at a locus | `GeneType` enum |
| **Allele** | Specific value of a gene (0, 1, real, integer, symbol) | `Allele` union |
| **Locus** | Position of a gene on a chromosome with type & bounds | `Locus` struct |
| **Chromosome** | Ordered sequence of genes (genotype) | `Chromosome` struct |
| **Population** | Collection of chromosomes in a generation | `Population` struct |
| **Fitness** | Objective function value → quality of solution | `fitness` field |
| **Schema** | Similarity template over {0, 1, \*} | `Schema` struct |
| **Schema Order o(H)** | Number of fixed (non-\*) positions | `ga_schema_order()` |
| **Defining Length δ(H)** | Distance between first and last fixed positions | `ga_schema_defining_length()` |
| **Building Block** | Short, low-order, above-average fitness schema | `is_building_block()` |

---

## Core Theorems (L4)

### Schema Theorem (Holland, 1975)

$$E[m(H, t+1)] \geq m(H, t) \cdot \frac{f(H)}{\bar{f}} \cdot \left[1 - p_c \cdot \frac{\delta(H)}{L-1}\right] \cdot (1 - p_m)^{o(H)}$$

- **C implementation**: `ga_schema_theorem_lower_bound()` in `ga_theory.c`
- **Lean formalization**: `schema_crossover_survival_nonneg`, `schema_order_le_length`

### Building Block Hypothesis (Goldberg, 1989)

Short, low-order, highly fit schemata (building blocks) are sampled, recombined,
and resampled to form strings of potentially higher fitness.

- **C**: `ga_identify_building_blocks()`, `ga_building_block_value()`

### Implicit Parallelism

A GA processing N chromosomes implicitly evaluates approximately N³ schemata per generation.

- **C**: `ga_implicit_parallelism()`

### Price's Equation (Price, 1970)

$$\Delta\bar{z} = \frac{\text{Cov}(w_i, z_i)}{\bar{w}} + \frac{\mathbb{E}(w_i \cdot \Delta z_i)}{\bar{w}}$$

- **C**: `ga_price_equation()` — decomposes evolutionary change into selection + transmission

### Fisher's Fundamental Theorem

$$\Delta\bar{f} \approx \frac{\text{Var}_g(f)}{\bar{f}}$$

- **C**: `ga_fisher_fundamental_theorem()`

### No Free Lunch (Wolpert & Macready, 1997)

Averaged over all possible fitness functions, all black-box algorithms perform equally.

- **C**: `ga_nfl_bound()`
- **Lean**: `nfl_ratio_le_one`, `nfl_expected_success_upper`

---

## Core Algorithms (L5)

### Selection Operators
- Roulette Wheel (fitness-proportionate)
- Tournament (k-tournament)
- Rank-based (linear ranking)
- Stochastic Universal Sampling (SUS)
- Truncation

### Crossover Operators
- Single-Point, Two-Point, Uniform (binary)
- Arithmetic BLX-α (real-valued)
- Simulated Binary Crossover SBX (real-valued)
- Order Crossover OX (permutation)
- Partially Mapped Crossover PMX (permutation)

### Mutation Operators
- Bit-Flip (binary)
- Gaussian (real-valued)
- Uniform (reset)
- Polynomial (real-valued, controlled)
- Swap (permutation)
- Inversion (permutation)
- Scramble (permutation)

### Replacement Strategies
- Generational
- Steady-State
- Elitist
- (μ+λ) and (μ,λ) Evolution Strategies
- Crowding (diversity-preserving)

---

## Canonical Problems (L6)

### Binary Problems
- **OneMax** — `ga_fitness_onemax()`
- **Deceptive Trap** — `ga_fitness_deceptive_trap()`
- **Royal Road** — `ga_fitness_royal_road()`
- **NK Landscape** — `ga_fitness_nk_landscape()`

### Real-valued Optimization
- **Sphere** — `ga_fitness_sphere()`
- **Rosenbrock** (banana) — `ga_fitness_rosenbrock()`
- **Rastrigin** (multimodal) — `ga_fitness_rastrigin()`
- **Ackley** — `ga_fitness_ackley()`
- **Schwefel** — `ga_fitness_schwefel()`
- **Griewank** — `ga_fitness_griewank()`

### Combinatorial
- **TSP** — `ga_fitness_tsp()`
- **Knapsack** — `ga_fitness_knapsack()`
- **Graph Coloring** — `ga_fitness_graph_coloring()`
- **Job Shop Scheduling** — `ga_fitness_jobshop()`

---

## Nine-School Course Mapping

| School | Course | Key Topics Mapped |
|--------|--------|-------------------|
| **MIT** | 6.034 AI, 6.841 Adv Complexity | GA loop, Schema Theorem |
| **Stanford** | CS 229 ML, CS 261 Optimization | Real-coded GA, Multi-objective |
| **Berkeley** | CS 188 AI, CS 289A | Fitness landscapes, NFL |
| **CMU** | 15-381 AI, 15-780 | NK landscapes, Co-evolution |
| **Princeton** | COS 402 AI, COS 511 | Deception, combinatorial optimization |
| **Caltech** | CS 156 ML | Spatial GA, criticality |
| **Cambridge** | Part II/III AI | Holland's theory, Vose model |
| **Oxford** | Computational Intelligence | Multi-objective, Island models |
| **ETH** | 263-5210 ML, 252-0538 Optimization | Formal theory, representations |

---

## Directory Structure

```
mini-genetic-algorithms-holland/
├── Makefile              # make test → run all tests
├── README.md             # This file
├── include/
│   ├── ga_core.h         # Core GA data structures
│   ├── ga_operators.h    # Selection, crossover, mutation, replacement
│   ├── ga_fitness.h      # Fitness functions + landscape analysis
│   ├── ga_theory.h       # Schema theory, NFL, Price's equation
│   ├── ga_encoding.h     # Binary, real, permutation, messy encoding
│   └── ga_population.h   # Population management, island models, spatial GA
├── src/
│   ├── ga_core.c         # Core GA loop + chromosome operations
│   ├── ga_operators.c    # All operator implementations
│   ├── ga_fitness.c      # Fitness functions + landscape analysis
│   ├── ga_theory.c       # Schema theory + formal foundations
│   ├── ga_encoding.c     # Encoding scheme implementations
│   ├── ga_population.c   # Population + island + spatial GA
│   └── ga_lean.lean      # Lean 4 formalization
├── tests/
│   ├── test_core.c       # Core GA unit tests
│   ├── test_operators.c  # Operator unit tests
│   └── test_theory.c     # Schema theory unit tests
├── examples/
│   ├── example_ga_onemax.c          # OneMax end-to-end
│   ├── example_ga_tsp.c             # TSP solution
│   ├── example_ga_function_opt.c    # Rastrigin optimization
│   └── example_schema_analysis.c    # Schema theorem validation
├── docs/
│   ├── knowledge-graph.md    # L1-L9 knowledge mapping
│   ├── coverage-report.md    # Coverage assessment
│   ├── gap-report.md         # Missing items + priorities
│   ├── course-alignment.md   # Nine-school curriculum mapping
│   └── course-tree.md        # Dependency tree
├── demos/
└── benches/
```

---

## Build & Test

```bash
# Build everything
make

# Run all tests
make test

# Run examples
make examples

# Check line count (must be ≥ 3000)
make check

# Clean build artifacts
make clean
```

---

## Reference

| Work | Author | Year |
|------|--------|------|
| *Adaptation in Natural and Artificial Systems* | Holland | 1975, 1992 |
| *Genetic Algorithms in Search, Optimization & Machine Learning* | Goldberg | 1989 |
| *An Introduction to Genetic Algorithms* | Mitchell | 1998 |
| *Introduction to Evolutionary Computing* | Eiben & Smith | 2015 |
| *Evolutionary Algorithms in Theory and Practice* | Bäck | 1996 |
| *Multi-Objective Optimization using EAs* | Deb | 2001 |
| *The Simple Genetic Algorithm* | Vose | 1999 |
| *Representations for Genetic and Evolutionary Algorithms* | Rothlauf | 2006 |
| *No Free Lunch Theorems for Optimization* (IEEE TEC) | Wolpert & Macready | 1997 |
