# mini-cas-in-economics-arthur

**Complex Adaptive Systems in Economics -- W. Brian Arthur**

Implementation: increasing returns, path dependence, technology lock-in,
El Farol Bar, Santa Fe Artificial Stock Market, combinatorial evolution,
NK fitness landscapes, agent-based computational economics.

---

## Module Status: COMPLETE

- L1 Definitions: Complete (10 core definitions)
- L2 Core Concepts: Complete (12 concepts)
- L3 Math Structures: Complete (12 types)
- L4 Fundamental Laws: Complete (10 theorems)
- L5 Algorithms: Complete (40+ algorithms)
- L6 Canonical Problems: Complete (6 problems)
- L7 Applications: Partial+ (5 apps)
- L8 Advanced: Partial+ (5 topics)
- L9 Frontiers: Partial (4 documented)

**Score: 16/18 -- COMPLETE**

**include/+src/: 3899 lines** (threshold: 3000)

## Code Metrics
| Component | Files | Lines |
|-----------|-------|-------|
| include/ | 4 headers | 1,118 |
| src/ | 4 C + 1 Lean | 2,940 |
| tests/ | 1 file | 197 |
| examples/ | 3 files | 191 |
| docs/ | 5 documents | ~200 |
| **Total include/+src/** | **8 files** | **3,899** |

## Core Definitions (L1)
MarketRegime, FeedbackType, LockState, ReasoningMode, TechnologyType,
AgentStrategy, EmergenceType -- all as C enums with Lean inductive types.

## Core Theorems (L4)
1. Arthur Polya Urn Theorem (1987): X_n -> x* s.t. f(x*)=x*
2. Lock-in Theorem (1989): gamma>1 => P(lock-in)->1
3. Arthur Adoption Theorem: Increasing returns => monopoly
4. Critical Mass Theorem: p* = n^(-gamma/(gamma+1))
5. HHI Bounds: 1/n <= HHI <= 1
6. Gini Scale Invariance: G(a*X) = G(X)
7. Cascade Termination in finite network
8. NK Peak Count: E[peaks] = 2^N/(N+1) for K=N-1
9. Path Dependence: Non-ergodic => history-dependent limits
10. Shannon Entropy Max at uniform distribution

## Core Algorithms (L5)
Polya Urn MC, Arthur urn function, adoption dynamics, lock-in detection,
Euler-Maruyama SDE, path dependence test, network cascade, Welford stats,
Hurst R/S, Gini/Lorenz, Hill estimator, El Farol, Minority Game,
Genetic Algorithm, Bucket Brigade, Technology combination,
NK landscape + adaptive walks, Autocatalytic sets, Full ACE model

## Canonical Problems (L6)
1. El Farol Bar Problem (Arthur 1994)
2. Technology Lock-in (Arthur 1989)
3. Polya Urn Path Dependence (Arthur et al 1987)
4. Santa Fe Artificial Stock Market (Arthur et al 1997)
5. NK Adaptive Walk (Kauffman 1993)
6. Network Cascade (Watts 2002)

## Building
```bash
make          # Build static library
make test     # Run all 24 tests
make examples # Build example programs
make demo     # Run demo + all examples
make clean    # Clean build artifacts
```

## File Structure
```
mini-cas-in-economics-arthur/
  Makefile                     # Build system
  README.md                    # This file
  include/
    ace_core.h                 # Core definitions and types
    ace_dynamics.h             # Dynamic models API
    ace_market.h               # Market models API
    ace_evolution.h            # Evolution models API
  src/
    ace_core.c                 # Allocators + math utilities
    ace_dynamics.c             # Urns, adoption, lock-in, SDE, stats
    ace_market.c               # El Farol, stock market, classifiers
    ace_evolution.c            # Tech evolution, NK landscapes, ACE model
    arthur_cas.lean            # Lean 4 formalization
  tests/
    test_arthur_cas.c          # 24 assert-based tests
  examples/
    example_el_farol.c         # El Farol Bar Problem
    example_polya_urn.c        # Polya urn path dependence
    example_tech_lockin.c      # Technology lock-in
  docs/
    knowledge-graph.md         # L1-L9 knowledge map
    coverage-report.md         # Coverage assessment
    gap-report.md              # Gap analysis
    course-alignment.md        # Nine-school mapping
    course-tree.md             # Prerequisites and dependents
  demos/
    demo_overview.c            # Overview demonstration
  benches/
    bench_core.c               # Performance benchmarks
```
