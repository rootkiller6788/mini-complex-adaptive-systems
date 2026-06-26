# mini-nk-fitness-landscapes-kauffman

## Module Status: COMPLETE ✅

- **L1 Definitions**: Complete (10/10)
- **L2 Core Concepts**: Complete (10/10)
- **L3 Mathematical Structures**: Complete (8/8)
- **L4 Fundamental Laws**: Complete (8/8)
- **L5 Algorithms/Methods**: Complete (10/10)
- **L6 Canonical Problems**: Complete (8/8)
- **L7 Applications**: Complete (5 applications)
- **L8 Advanced Topics**: Partial+ (6/8 implemented, 2 documented)
- **L9 Research Frontiers**: Partial (documented, not implemented)

**Total Score: 17/18**

## Overview

Implementation of Stuart Kauffman's NK fitness landscape model (1993, "The Origins of Order"). The NK model is a mathematical framework for studying adaptive evolution on rugged fitness landscapes with tunable epistasis (K). This module provides complete C implementations with a Lean 4 formalization.

## Core Definitions

| Concept | Definition |
|---------|-----------|
| NK Landscape | F: {0,1}^N → [0,1], N genes each interacting with K others |
| Genotype | Binary string of length N (bit-packed representation) |
| Fitness | F(g) = (1/N) Σᵢ fᵢ(configᵢ(g)), average of N epistatic contributions |
| Epistasis (K) | Number of genes regulating each gene; tunes landscape ruggedness |
| Local Optimum | Genotype where no single-locus mutation improves fitness |
| Adaptive Walk | Sequence of fitness-improving single-locus mutations |
| NKCS Model | Two-species coevolution on coupled NK landscapes (Kauffman & Johnsen 1991) |

## Core Theorems (with formulas)

| Theorem | Formula | Reference |
|---------|---------|-----------|
| Weinberger Autocorrelation | ρ(d) = (1 - d/N)^K | Weinberger (1990) |
| Local Optima Count | E[#optima] = 2^N / (N+1) for K=N-1 | Kauffman & Levin (1987) |
| Walk Length Scaling | E[walk steps] ~ ln(N) for K ≥ 2 | Kauffman (1993) |
| Complexity Catastrophe | E[optimum fitness] ↓ as K ↑ | Kauffman (1993), Ch. 4 |
| Correlation Length | L_c ≈ N/K | Weinberger (1990) |
| Coupling Catastrophe | E[Nash fitness] ↓ as C ↑ | Kauffman & Johnsen (1991) |

## Core Algorithms

| Algorithm | Complexity | Description |
|-----------|-----------|-------------|
| Greedy Walk | O(N·K) per step | First-improvement hill-climbing |
| Steepest Ascent | O(N²·K) per step | Best-improvement (Wright 1932) |
| Next-Ascent | O(N·K) per step | Scan with memory, no positional bias |
| Random-Better | O(N²·K) per step | Uniform among improvements (Kimura 1983) |
| Metropolis Walk | O(N·K) per step | Probabilistic downhill moves |
| Simulated Annealing | O(N·K) per step | Cooling schedule (Kirkpatrick et al. 1983) |
| Fast Walsh-Hadamard Transform | O(N·2^N) | Spectral decomposition of fitness |
| Nash Equilibrium Search | O(N²·(K+C)) per iter | Best-response dynamics in NKCS |

## Canonical Problems

1. Construct NK landscape with tunable ruggedness
2. Find local optima via adaptive walks
3. Characterize landscape ruggedness from fitness correlations
4. Detect sign epistasis and reciprocal sign epistasis
5. Compute epistasis variance spectrum via Walsh decomposition
6. Verify complexity catastrophe empirically
7. Find Nash equilibrium in NKCS coevolution
8. Measure Red Queen dynamics in coevolving systems

## Nine-School Curriculum Mapping

| School | Course | Connection |
|--------|--------|-----------|
| MIT | 6.045/6.841 | Complexity landscapes, phase transitions |
| Stanford | CS254 | Random CSPs and fitness landscape structure |
| Berkeley | CS172/CS278 | NP-hardness of global optimum (K≥2) |
| CMU | 15-455/15-855 | Adaptive walks as approximation algorithms |
| Princeton | COS 522 | NK landscapes as candidate one-way functions |
| Caltech | CS 151 | Spin glass correspondence, statistical physics |
| Cambridge | Part II/III | Reductions for fitness landscape hardness |
| Oxford | Comp Complexity | Quantum walks on fitness landscapes |
| ETH | 263-4650 | Formal verification of landscape properties |

## Building and Testing

```bash
make          # Build static library
make test     # Build and run test suite (19 tests)
make examples # Build all example programs
make demo     # Build and run comprehensive demo
make clean    # Remove build artifacts
```

## File Structure

```
mini-nk-fitness-landscapes-kauffman/
├── Makefile
├── README.md                  ← This file
├── include/
│   ├── nk_core.h              (193 lines) Core types and definitions
│   ├── nk_landscape.h         (171 lines) Landscape generation and fitness
│   ├── nk_walk.h              (131 lines) Adaptive walk algorithms
│   ├── nk_statistics.h        (120 lines) Landscape statistics
│   ├── nk_epistasis.h         (108 lines) Epistasis analysis
│   └── nk_coevolution.h       (149 lines) NKCS coevolution
├── src/
│   ├── nk_core.c              (569 lines) Core implementation
│   ├── nk_walk.c              (576 lines) Walk algorithms
│   ├── nk_statistics.c        (489 lines) Statistics
│   ├── nk_epistasis.c         (408 lines) Epistasis analysis
│   ├── nk_coevolution.c       (664 lines) Coevolution
│   └── nk_landscape.lean      (266 lines) Lean 4 formalization
├── tests/
│   └── test_nk.c              (470 lines) 19 comprehensive tests
├── examples/
│   ├── example_landscape.c    (75 lines) Basic NK exploration
│   ├── example_adaptive_walk.c(103 lines) Walk strategy comparison
│   └── example_statistics.c   (93 lines) Statistical analysis
├── demos/
│   └── demo_nk.c              (172 lines) Full workflow demo
├── benches/
│   └── bench_core.c           Performance benchmarks
└── docs/
    ├── knowledge-graph.md     L1-L9 knowledge map
    ├── coverage-report.md     Completeness assessment
    ├── gap-report.md          Remaining gaps
    ├── course-alignment.md    Nine-school mapping
    └── course-tree.md         Prerequisite tree
```

## References

- Kauffman, S.A. (1993). *The Origins of Order: Self-Organization and Selection in Evolution*. Oxford University Press.
- Kauffman, S.A. & Levin, S. (1987). Towards a general theory of adaptive walks on rugged landscapes. *J. Theor. Biol.*, 128(1), 11-45.
- Kauffman, S.A. & Johnsen, S. (1991). Coevolution to the edge of chaos. *J. Theor. Biol.*, 149(4), 467-505.
- Weinberger, E.D. (1990). Correlated and uncorrelated fitness landscapes and how to tell the difference. *Biol. Cybern.*, 63(5), 325-336.
- Weinreich, D.M. et al. (2005). Perspective: Sign epistasis and genetic constraint on evolutionary trajectories. *Evolution*, 59(6).
- Stadler, P.F. (1999). Fitness landscapes. In: *Biological Evolution and Statistical Physics*. Springer.
- Kauffman, S.A. (1995). *At Home in the Universe: The Search for the Laws of Self-Organization and Complexity*. Oxford University Press.
