# Course Tree — Collective Intelligence & Swarm

## Prerequisite Dependency Tree

```
mini-collective-intelligence-swarm
├── mini-agents-adaptation-basics        [L1-L2: agent definition, adaptation]
├── mini-sfi-methodology-synthesis       [L2: complex systems thinking]
├── mini-cas-in-economics-arthur         [L2: economic agent models]
├── mini-edge-of-chaos-criticality       [L2: phase transitions, SOC]
├── mini-genetic-algorithms-holland      [L5: evolutionary search]
├── mini-nk-fitness-landscapes-kauffman  [L3: fitness landscape math]
├── mini-network-science (27)            [L3: graph theory, Laplacian]
├── mini-multi-agent-coordination (23)   [L5: multi-agent theory]
└── mini-decentralized-large-scale (22)  [L2: decentralized control]
```

## Topic Dependency Graph

```
Probability & Statistics
  └─ Random number generation (xoshiro256**)
      └─ Stochastic agent behavior

Linear Algebra
  ├─ Vector operations (R^D)
  ├─ Matrix operations (pheromone, adjacency)
  └─ Graph Laplacian & spectral theory
      └─ Consensus convergence analysis

Differential Equations
  ├─ Diffusion equation (pheromone)
  ├─ Flocking dynamics (Cucker-Smale ODE)
  └─ Sandpile toppling rules (discrete PDE)

Optimization Theory
  ├─ Continuous optimization (gradient-free)
  │   └─ PSO (stochastic search)
  └─ Combinatorial optimization
      └─ ACO (TSP, VRP, knapsack)

Control Theory
  ├─ Distributed consensus protocol
  ├─ Formation control (virtual forces)
  └─ Flocking (decentralized feedback)

Information Theory
  ├─ Transfer entropy
  ├─ Active information storage
  └─ Mutual information in swarms

Statistical Physics
  ├─ Phase transitions (Vicsek)
  ├─ Self-organized criticality (sandpile)
  └─ Order parameters

Biology / Ethology
  ├─ Ant foraging behavior
  ├─ Bird flocking (starlings)
  ├─ Slime mold aggregation
  └─ Bee nest site selection

Computer Science
  ├─ Agent-based modeling
  ├─ Cellular automata (Wolfram classification)
  └─ Distributed algorithms (gossip)
```

## Build Order (within this module)
1. swarm_types.h — define all data structures
2. swarm_core.h + swarm_core.c — math utilities
3. swarm_pso.h + .c — PSO (depends on core)
4. swarm_aco.h + .c — ACO (depends on core)
5. swarm_flocking.h + .c — Flocking (depends on core)
6. swarm_consensus.h + .c — Consensus (depends on core)
7. swarm_stigmergy.h + .c — Stigmergy (depends on core)
8. swarm_emergent.h + .c — Emergent behavior (depends on all above)
9. swarm_applications.h + .c — Applications (depends on all above)
10. swarm_formal.lean — Formal verification (standalone)
