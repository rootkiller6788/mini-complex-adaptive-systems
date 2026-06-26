# Course Tree — mini-artificial-life-langton

## Prerequisite Knowledge Dependencies

```
                          ┌──────────────────────────┐
                          │  Theory of Computation    │
                          │  (Turing Machines,        │
                          │   Decidability,           │
                          │   Church-Turing Thesis)   │
                          └────────────┬─────────────┘
                                       │
                    ┌──────────────────┼──────────────────┐
                    │                  │                   │
         ┌──────────▼──────────┐ ┌────▼────────┐ ┌───────▼──────────┐
         │ Cellular Automata   │ │ Information  │ │ Discrete Math    │
         │ (von Neumann, Ulam, │ │ Theory       │ │ (Graph Theory,   │
         │  Wolfram, Conway)   │ │ (Shannon,    │ │  Combinatorics,  │
         │                     │ │  Kolmogorov) │ │  Group Theory)   │
         └──────────┬──────────┘ └────┬─────────┘ └───────┬──────────┘
                    │                 │                    │
         ┌──────────▼─────────────────▼────────────────────▼──────────┐
         │                                                             │
         │          mini-artificial-life-langton (this module)          │
         │                                                             │
         │  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
         │  │ Langton Ant  │  │ CA Framework │  │ Lambda Parameter  │  │
         │  │ Simulation   │  │ (1D & 2D)    │  │ (Edge of Chaos)   │  │
         │  └──────┬───────┘  └──────┬───────┘  └────────┬──────────┘  │
         │         │                 │                    │             │
         │         └─────────────────┼────────────────────┘             │
         │                           │                                  │
         │         ┌─────────────────▼────────────────────┐             │
         │         │         ALife Metrics                 │             │
         │         │  (Emergence, Complexity,              │             │
         │         │   Self-Organization, Novelty)          │             │
         │         └─────────────────┬────────────────────┘             │
         │                           │                                  │
         │         ┌─────────────────┼────────────────────┐             │
         │         │                 │                     │             │
         │  ┌──────▼──────┐  ┌───────▼────────┐  ┌───────▼──────────┐  │
         │  │ Pattern      │  │ Evolutionary   │  │ Lean 4           │  │
         │  │ Detection    │  │ Dynamics        │  │ Formalization    │  │
         │  └──────────────┘  └────────────────┘  └──────────────────┘  │
         │                                                             │
         └─────────────────────────┬───────────────────────────────────┘
                                   │
         ┌─────────────────────────┼───────────────────────────────────┐
         │                         │                                    │
         │  ┌──────────────────────▼──────────────────────────────┐     │
         │  │  L9: Research Frontiers                              │     │
         │  │  - Space-filling ant conjecture                      │     │
         │  │  - Optimal lambda_c for computation                  │     │
         │  │  - Open-ended evolution at edge of chaos              │     │
         │  │  - Quantum CA                                         │     │
         │  │  - Meta-complexity of rule spaces                    │     │
         │  └──────────────────────────────────────────────────────┘     │
         └──────────────────────────────────────────────────────────────┘
```

## Internal Dependency Graph
```
langton_ant.h ──────► langton_ant.c
                          │
cellular_automaton.h ─► cellular_automaton.c ──┐
                          │                      │
langton_lambda.h ────────┼──► langton_lambda.c   │
                          │       │               │
alife_metrics.h ────────►│──► alife_metrics.c    │
                          │       │               │
alife_patterns.h ───────►│──► alife_patterns.c   │
                          │                      │
                          └──────────────────────┤
                                                 │
alife_evolution.c (standalone, uses internal types)
alife_lean.lean (standalone, formalizes concepts)
```
