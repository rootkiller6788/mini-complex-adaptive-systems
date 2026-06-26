# Course Tree — mini-agents-adaptation-basics

## Prerequisites (What This Module Depends On)

```
Probability Theory
├── Random variables, distributions, expectation
├── Bayes rule, conjugate priors
└── Concentration inequalities (Hoeffding, Chernoff)

Linear Algebra
├── Matrix operations, eigenvalues
├── Positive definite matrices (LinUCB)
└── Cholesky decomposition

Calculus / Optimization
├── Gradient, Hessian
├── Convex optimization basics
└── Dynamic programming (Bellman equation)

Game Theory
├── Normal-form games
├── Nash equilibrium existence
└── Evolutionary game theory basics

Computer Science Fundamentals
├── Data structures (arrays, hash tables)
├── Graph algorithms (TSP, shortest path)
└── Computational complexity (P, NP basics)
```

## Internal Dependencies (Within This Module)

```
agent.h (foundation)
├── environment.h → depends on agent.h
│   ├── rl_agent.h → depends on agent.h + environment.h
│   ├── bandit.h → independent (self-contained)
│   ├── swarm.h → independent (self-contained)
│   └── game_theory.h → independent (self-contained)
│
src/agent.c → include/agent.h
src/environment.c → include/agent.h + include/environment.h
src/rl_agent.c → include/agent.h + include/environment.h + include/rl_agent.h
src/bandit.c → include/bandit.h
src/swarm.c → include/swarm.h
src/game_theory.c → include/game_theory.h
src/adaptation.lean → Lean 4 formalization (standalone)
```

## Knowledge Progression (Learning Path)

```
1. Agent Fundamentals (agent.h/c)
   └─ What is an agent? PEAS, perception-action loop
   └─ Bounded rationality, satisficing

2. Environment Models (environment.h/c)
   └─ MDP formulation: S, A, P, R, gamma
   └─ GridWorld as canonical testbed
   └─ NK fitness landscape

3. Reinforcement Learning (rl_agent.h/c)
   └─ Q-learning → SARSA → TD(lambda) → Dyna-Q
   └─ Policy Iteration → Value Iteration
   └─ GridWorld training and evaluation

4. Multi-Armed Bandits (bandit.h/c)
   └─ epsilon-greedy → UCB1 → Thompson Sampling
   └─ Contextual bandits (LinUCB)
   └─ Adversarial bandits (EXP3)
   └─ Regret analysis

5. Swarm Intelligence (swarm.h/c)
   └─ PSO → ACO → Boids → Firefly Algorithm
   └─ Emergent behavior from local rules

6. Game Theory & Evolution (game_theory.h/c)
   └─ Normal-form games → Nash equilibrium
   └─ Replicator dynamics → ESS
   └─ Iterated PD → Axelrod tournament
   └─ Correlated equilibrium
```

## L9 Research Frontiers (Future Directions)

- Meta-learning: agents that learn how to learn
- Multi-agent emergent communication
- AI safety in adaptive agent systems
- Integration with deep RL
- Co-evolutionary arms races