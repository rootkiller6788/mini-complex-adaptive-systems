# Course Tree — mini-genetic-algorithms-holland

## Prerequisite Dependency Tree

```
mini-genetic-algorithms-holland
├── Mathematical Foundations (external)
│   ├── Probability Theory (random selection, mutation rates)
│   ├── Linear Algebra (real-valued encoding, distance metrics)
│   ├── Discrete Mathematics (binary strings, combinatorics)
│   ├── Optimization Theory (fitness landscapes, local/global optima)
│   └── Statistics (variance, correlation, entropy)
│
├── mini-complexity-foundations (sibling module)
│   ├── P vs NP (TSP is NP-hard → motivates GAs)
│   └── NP-Completeness (approximation via metaheuristics)
│
├── mini-circuit-complexity (sibling module)
│   └── Boolean functions (binary GA operates on {0,1}^L)
│
├── mini-algorithmic-complexity (sibling module)
│   └── Approximation algorithms (GA as constant-factor approximator)
│
└── Internal Dependencies
    ├── ga_core.h → ga_core.c [L1, L2, L3]
    ├── ga_operators.h → ga_operators.c [L5] (depends on ga_core.h)
    ├── ga_fitness.h → ga_fitness.c [L1, L3, L6] (depends on ga_core.h)
    ├── ga_theory.h → ga_theory.c [L1, L3, L4, L8] (depends on ga_core.h, ga_operators.h)
    ├── ga_encoding.h → ga_encoding.c [L2, L3, L5, L8] (depends on ga_core.h)
    └── ga_population.h → ga_population.c [L2, L3, L7, L8] (depends on ga_core.h, ga_fitness.h)
```

## Knowledge Dependency Flow

```
L1 Definitions
  └─→ L2 Core Concepts
       └─→ L3 Mathematical Structures
            ├─→ L4 Fundamental Laws (Schema Theorem, NFL, Price's Eq)
            ├─→ L5 Algorithms (Selection, Crossover, Mutation, Replacement)
            └─→ L6 Canonical Problems (OneMax, TSP, Rastrigin, ...)
                 ├─→ L7 Applications (Island Model, Spatial GA, Multi-Objective)
                 └─→ L8 Advanced Topics (Vose Model, Messy GA, Co-evolution)
                      └─→ L9 Research Frontiers (Quality Diversity, Hyper-heuristics)
```

## Concept Dependency Map

```
Chromosome → Population → Fitness Function → Selection
                                                   ↓
Schema ← Crossover ← Mutation ← Evaluation ← Replacement
   ↓
Schema Theorem → Building Block Hypothesis → Implicit Parallelism
   ↓
Price's Equation → Fisher's Theorem → Vose Model → No Free Lunch
```

## Implementation Order

1. `ga_core.h` / `ga_core.c` — data structures + GA loop
2. `ga_operators.h` / `ga_operators.c` — variation operators
3. `ga_fitness.h` / `ga_fitness.c` — fitness functions + landscape
4. `ga_theory.h` / `ga_theory.c` — schema theory + formal theorems
5. `ga_encoding.h` / `ga_encoding.c` — representations
6. `ga_population.h` / `ga_population.c` — advanced population management
7. `ga_lean.lean` — formal verification of core properties
