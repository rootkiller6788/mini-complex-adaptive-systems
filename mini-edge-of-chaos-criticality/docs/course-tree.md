# Course Dependency Tree — Edge of Chaos & Criticality

## Prerequisite Knowledge

```
                          [Research Frontiers L9]
                                   |
                    +--------------+--------------+
                    |              |              |
              [Applications L7] [Advanced L8] [Formal Proofs]
                    |              |
          +---------+---------+    +------+------+
          |         |         |    |      |      |
    [Algorithms L5] [Problems L6] [RG] [Multifractals]
          |              |
    +-----+-----+  +-----+-----+
    |     |     |  |     |     |
[Laws L4] [Math L3] [Concepts L2] [Definitions L1]
```

## Dependency Graph (What to Learn First)

### Level 0: Mathematical Prerequisites
- Calculus (derivatives, integrals, ODEs) → needed for L3-L5
- Linear Algebra (eigenvalues, QR decomposition) → needed for L5 (Lyapunov spectrum)
- Probability & Statistics (MLE, KS test, bootstrap) → needed for L5 (power-law fitting)
- Graph Theory (basic) → needed for L7 (network criticality)

### Level 1: Core Definitions (L1)
- `EOCPhaseState` — phase/dynamical regime taxonomy
- `BifurcationType` — bifurcation classification (Strogatz 2018)
- `WolframClass` — CA behavioral classes (Wolfram 1984)
- `UniversalityClass` — universality classes (Stanley 1987)
- `EOCCriticalExponents` — critical exponent notation
- `EOCPowerLawFit` — statistical fitting result
- `EOCSandpileModel` — SOC model structure

### Level 2: Core Concepts (L2)
- Edge of Chaos (Langton 1990) → depends on L1
- Self-Organized Criticality (Bak et al. 1987) → depends on L1
- Universality (Stanley 1987, Cardy 1996) → depends on L1
- Power-Law Distributions (Clauset et al. 2009) → depends on statistics prereq
- Fractal Dimension (Mandelbrot 1982) → depends on measure theory prereq

### Level 3: Mathematical Structures (L3)
- Logistic map and iterated dynamics → depends on calculus
- Lorenz/Rossler/Henon systems → depends on ODEs
- CA rule space topology → depends on L1, combinatorics
- Correlation integral → depends on probability

### Level 4: Fundamental Laws (L4)
- Feigenbaum universality (delta, alpha) → depends on L3
- Lyapunov exponent definition → depends on L3
- Scaling relations (Rushbrooke, Widom, Fisher, Hyperscaling) → depends on L1-L2
- Dhar's Abelian sandpile theorem → depends on L2
- Gutenberg-Richter law → depends on L2
- RG flow to fixed points (Wilson 1971) → depends on L2-L3

### Level 5: Algorithms (L5)
- Sandpile cascade toppling → depends on L2, L3
- MLE power-law fitting → depends on L2, statistics
- Bifurcation diagram construction → depends on L3
- Lyapunov spectrum (Benettin QR) → depends on L4, linear algebra
- Box-counting algorithm → depends on L2
- DFA (Peng et al. 1994) → depends on L2, statistics
- Multifractal spectrum → depends on L3, L4

### Level 6: Canonical Problems (L6)
- BTW avalanche distribution → depends on L2, L5
- Logistic map bifurcation → depends on L3, L4
- Langton lambda phase transition → depends on L2, L5
- Game of Life patterns → depends on L1, L5
- Lorenz attractor → depends on L3
- 1/f noise spectrum → depends on L2, L5

### Level 7: Applications (L7)
- Earthquake modeling (Gutenberg-Richter) → depends on L5, L6
- Forest fire / epidemic spreading → depends on L5, L6
- Signal criticality classification → depends on L5
- Network criticality detection → depends on L5, graph theory

### Level 8: Advanced Topics (L8)
- Manna universality class → depends on L2, L6
- Directed percolation → depends on L1, L4
- Multifractal formalism f(alpha) → depends on L3, L5
- Griffiths phases → depends on L2, L4
- Quenched disorder SOC → depends on L6

### Level 9: Research Frontiers (L9)
- SOC in quantum systems → depends on L2-L8 + quantum mechanics
- Machine learning at criticality → depends on L2-L8 + ML
- COVID as SOC → depends on L7 (epidemic models)
- Climate criticality → depends on L7 (DFA, long-range correlations)
