# Mini Complex Adaptive Systems

A collection of **from-scratch, zero-dependency C implementations** of the foundational models and computational frameworks for complex adaptive systems (CAS). Each sub-module maps to MIT, Stanford, and Santa Fe Institute courses, covering agent-based modeling, artificial life, complexity economics, swarm intelligence, edge-of-chaos dynamics, genetic algorithms, NK fitness landscapes, and the SFI synthesis methodology — all built for the complex systems scientist.

## Sub-Modules

| Sub-Module | Topics | Key Courses |
|------------|--------|-------------|
| [mini-agents-adaptation-basics](mini-agents-adaptation-basics/) | Intelligent agents, multi-armed bandits, environment models, game theory, Q-learning/SARSA/TD(λ), swarm primitives | MIT 6.7900, Stanford CS229 |
| [mini-artificial-life-langton](mini-artificial-life-langton/) | Cellular automata (1D/2D), Langton's Ant, Langton's λ parameter, pattern detection (still lifes, oscillators, gliders), ALife metrics | MIT 6.045, Stanford CS358, SFI CSSS |
| [mini-cas-in-economics-arthur](mini-cas-in-economics-arthur/) | Agent-based computational economics, increasing returns & path dependence, Polya urn processes, El Farol Bar Problem, Santa Fe Artificial Stock Market, combinatorial technology evolution | MIT 14.13, Stanford MS&E 448 |
| [mini-collective-intelligence-swarm](mini-collective-intelligence-swarm/) | Particle Swarm Optimization (PSO), Ant Colony Optimization (ACO), flocking/boids, distributed consensus, stigmergy & self-organization, emergent collective dynamics | MIT 6.821, Stanford AA 174 |
| [mini-edge-of-chaos-criticality](mini-edge-of-chaos-criticality/) | Wolfram CA classes, Langton λ phase transition, self-organized criticality (BTW sandpile), power-law detection & fitting, nonlinear dynamics (bifurcation, Lyapunov), fractal geometry | MIT 6.045, Stanford CS358, Caltech CS154 |
| [mini-genetic-algorithms-holland](mini-genetic-algorithms-holland/) | GA core (selection, crossover, mutation), encoding schemes, fitness landscape analysis, population management, schema theory & building block hypothesis | MIT 6.7201, Stanford EE 364a |
| [mini-nk-fitness-landscapes-kauffman](mini-nk-fitness-landscapes-kauffman/) | NK landscape generation, epistasis analysis (magnitude, sign, reciprocal sign), adaptive walks (greedy, steepest-ascent, Metropolis), landscape statistics, NKCS coevolution | MIT 6.7201, Stanford MS&E 448 |
| [mini-sfi-methodology-synthesis](mini-sfi-methodology-synthesis/) | Macro-level CAS types, adaptation & replicator dynamics, complexity measures (statistical complexity, Lempel-Ziv, effective complexity), emergence detection, ε-machines, network science methods | SFI CSSS, Stanford CS358 |

## Design Philosophy

- **Zero external dependencies** — pure C99/C11, only standard library headers
- **Self-contained sub-modules** — each has its own `include/`, `src/`, `CMakeLists.txt`, and smoke tests
- **Theory-to-code mapping** — every module includes inline references to foundational texts (Holland, Kauffman, Arthur, Langton, Wolfram, Mitchell) and lecture notes
- **CAS canon coverage** — all implementations align with the Santa Fe Institute Complex Systems Summer School (CSSS) core curriculum

## Building

Each sub-module is standalone. Build with CMake:

```bash
cd mini-agents-adaptation-basics
mkdir build && cd build
cmake ..
make
./smoke_test
```

Requires a **C99-compliant compiler** and **CMake ≥ 3.14**.

## Project Structure

```
28. mini-complex-adaptive-systems/
├── mini-agents-adaptation-basics/      # Agents, bandits, game theory, reinforcement learning
├── mini-artificial-life-langton/       # Cellular automata, Langton's Ant, λ parameter, ALife metrics
├── mini-cas-in-economics-arthur/       # ACE, increasing returns, El Farol, Santa Fe Stock Market
├── mini-collective-intelligence-swarm/ # PSO, ACO, flocking, consensus, stigmergy, emergence
├── mini-edge-of-chaos-criticality/     # Wolfram classes, SOC sandpile, power laws, fractals
├── mini-genetic-algorithms-holland/    # Selection, crossover, mutation, schema theory
├── mini-nk-fitness-landscapes-kauffman/ # NK landscapes, epistasis, adaptive walks, coevolution
├── mini-sfi-methodology-synthesis/     # SFI canon: emergence, ε-machines, complexity measures
├── .gitignore
├── README.md
└── README-CN.md
```

## License

MIT
