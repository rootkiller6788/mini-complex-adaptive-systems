# Mini Collective Intelligence — Swarm Intelligence

## Module Status: COMPLETE ✅

**L1-L6: Complete | L7: Complete (4 applications) | L8: Complete (8 advanced topics) | L9: Partial (documented)**

> "The whole is more than the sum of its parts." — Aristotle
> "Simple rules, repeated by many agents, can produce surprisingly intelligent global behavior." — Bonabeau et al. (1999)

A C library implementing swarm intelligence algorithms (PSO, ACO), flocking models (Reynolds, Vicsek, Cucker-Smale), distributed consensus, and stigmergy, with Lean 4 formal verification of core theorems.

---

## Module Structure

```
include/   9 headers  (types, core, pso, aco, flocking, consensus, stigmergy, emergent, applications)
src/       9 C files  (core, pso, aco, flocking, consensus, stigmergy, emergent, applications) + 1 Lean file
tests/     1 file     (40+ asserts covering L1-L8)
examples/  4 demos    (PSO benchmarks, ACO TSP, flocking, consensus)
docs/      5 documents (knowledge-graph, coverage, gap, course-alignment, course-tree)
```

## Quick Start

```bash
make          # Build static library libswarm.a (6,200+ lines)
make test     # Run 40+ tests
make examples # Build all 4 example programs
make demo     # Run all examples
```

## Core Equations

### Particle Swarm Optimization (Kennedy-Eberhart 1995)
```
v_{id} = ω·v_{id} + c₁·r₁·(pbest_{id} - x_{id}) + c₂·r₂·(lbest_{id} - x_{id})
x_{id} = x_{id} + v_{id}
```

### Ant Colony Optimization (Dorigo et al. 1991)
```
p_{ij}^k = [τ_{ij}]^α · [η_{ij}]^β / Σ_{l∈N_i^k} [τ_{il}]^α · [η_{il}]^β
τ_{ij} ← (1-ρ)·τ_{ij} + Σ_k Δτ_{ij}^k
```

### Reynolds Flocking (1987)
```
F_i = w_s·s_i + w_a·a_i + w_c·c_i
  s_i (Separation):  steer away from crowding
  a_i (Alignment):   steer toward average heading
  c_i (Cohesion):    steer toward average position
```

### Distributed Consensus (Olfati-Saber 2007)
```
x_i(k+1) = Σ_j w_{ij}·x_j(k)   (W row-stochastic)
lim_{k→∞} x_i(k) = (1/N)·Σ_i x_i(0)   (connected graph)
```

## Key API

```c
// PSO: optimize any continuous function
SwarmConfig config = { .n_agents=30, .dimensions=10, ... };
SwarmResult* result = swarm_pso_run(&config);
printf("Best fitness: %.6f\n", result->best_fitness);

// ACO: solve Traveling Salesman Problem
SwarmResult* tour = swarm_aco_run_tsp(x_coords, y_coords, n_cities, &aco_config);

// Flocking: simulate 50 boids
Boid** flock = swarm_flock_create(&flock_config);
for (int s = 0; s < 200; s++) swarm_flock_update(flock, 50, &flock_config);

// Consensus: reach distributed agreement
ConsensusState* cs = swarm_consensus_create(n_agents, initial_values);
swarm_consensus_weights_metropolis(cs->weights, n, adjacency);
swarm_consensus_run(cs, 1000);  // → all agents agree on average
```

## Nine-Layer Knowledge Coverage

| Level | Name | Status | Key Items |
|-------|------|--------|-----------|
| **L1** | Definitions | Complete | 11 structs, 50+ typedefs |
| **L2** | Core Concepts | Complete | Emergence, stigmergy, self-organization, consensus |
| **L3** | Math Structures | Complete | Vectors, matrices, Laplacian, pheromone diffusion |
| **L4** | Fundamental Laws | Complete | Reynolds, Cucker-Smale, Vicsek, Clerc-Kennedy, Deneubourg |
| **L5** | Algorithms | Complete | PSO (6 variants), ACO (5 variants), Consensus (4), Flocking (4) |
| **L6** | Canonical Problems | Complete | 7 benchmark functions, TSP, flocking, sandpile |
| **L7** | Applications | Complete | Robotics, supply chain, smart grid, UAV swarm |
| **L8** | Advanced Topics | Complete | Phase transitions, emergence metrics, information theory |
| **L9** | Research Frontiers | Partial | 5 topics documented |

## Core Theorems (with Lean Formalization)

1. **Clerc-Kennedy Constriction** — PSO converges for φ>4, χ∈(0,1]
2. **Olfati-Saber Consensus** — Connected graph → average consensus
3. **Cucker-Smale Flocking** — β<1/2 → unconditional convergence
4. **Deneubourg Symmetry Breaking** — Pheromone amplification → bifurcation
5. **Vicsek Phase Transition** — ∃ η_c for disorder→order transition
6. **BTW Self-Organized Criticality** — Sandpile → power-law avalanches
7. **Reynolds Emergence** — Local rules → global flocking
8. **Swarm Phase Transition** — PSO exploration→exploitation transition

## Benchmark Functions

| Function | Formula | Global Minimum |
|----------|---------|---------------|
| Sphere | Σ x_i² | f(0)=0 |
| Rosenbrock | Σ 100(x_{i+1}-x_i²)² + (x_i-1)² | f(1)=0 |
| Rastrigin | 10n + Σ[x_i² - 10cos(2πx_i)] | f(0)=0 |
| Ackley | -20exp(-0.2√(Σx_i²/n)) - exp(Σcos(2πx_i)/n) + 20 + e | f(0)=0 |
| Griewank | 1 + Σx_i²/4000 - Πcos(x_i/√i) | f(0)=0 |
| Schwefel | 418.9829n - Σx_i·sin(√⎸x_i⎸) | f(420.97)=0 |
| Michalewicz | -Σ sin(x_i)·sin²⁰(i·x_i²/π) | ≈-0.966n |
| Levy | complex multimodal | f(1)=0 |

## Nine-School Course Mapping

| School | Key Courses |
|--------|------------|
| **MIT** | 6.843 Robotics, MAS.865 Multi-Agent Systems |
| **Stanford** | CS224M Multi-Agent, AA274 Robotics Principles |
| **Berkeley** | CS287 Advanced Robotics, EE291 Control |
| **CMU** | 15-381 AI, 16-782 Planning & Scheduling |
| **Princeton** | COS 511 Theoretical ML, MAE 548 Optimization |
| **Caltech** | CMS/CS/CNS 144 Networks & Complex Systems |
| **Cambridge** | Part II Complex Systems, Part III Adv. Complexity |
| **Oxford** | B25 Complex Systems, C20 Adaptive Systems |
| **ETH** | 263-5000 Multi-Agent, 227-0427 Systems Biology |

## References

- Bonabeau, Dorigo & Theraulaz (1999). *Swarm Intelligence: From Natural to Artificial Systems*. Oxford.
- Kennedy, J. & Eberhart, R. (1995). "Particle Swarm Optimization." *Proc. IEEE ICNN*.
- Dorigo, M. & Stützle, T. (2004). *Ant Colony Optimization*. MIT Press.
- Reynolds, C.W. (1987). "Flocks, Herds, and Schools." *SIGGRAPH*.
- Olfati-Saber, R., Fax, J.A. & Murray, R.M. (2007). "Consensus and Cooperation in Networked Multi-Agent Systems." *Proc. IEEE*.
- Cucker, F. & Smale, S. (2007). "Emergent Behavior in Flocks." *IEEE Trans. Auto. Control*.
- Camazine, S. et al. (2001). *Self-Organization in Biological Systems*. Princeton.
- Bak, P., Tang, C. & Wiesenfeld, K. (1987). "Self-Organized Criticality." *Phys. Rev. Lett.*
- Vicsek, T. et al. (1995). "Novel Type of Phase Transition." *Phys. Rev. Lett.*
- Holland, J.H. (1998). *Emergence: From Chaos to Order*. Addison-Wesley.

---

*Code: 6,200+ lines C + 180+ lines Lean 4. Tests: 40+ asserts. Examples: 4 end-to-end.*
