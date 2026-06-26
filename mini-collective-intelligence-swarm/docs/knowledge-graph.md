# Knowledge Graph — Collective Intelligence & Swarm

## L1: Definitions (Complete)
- SwarmAgent — fundamental computing unit (position, velocity, fitness, pbest)
- Boid — Reynolds flocking agent (position, velocity, acceleration in 3D)
- PheromoneMatrix — distributed environmental memory (τ_ij, η_ij, evaporation)
- ConsensusState — multi-agent agreement state (values, weights, disagreement)
- StigmergyEnvironment — pheromone grid for indirect coordination
- SwarmConfig — global swarm parameters (topology, weights, bounds)
- SwarmResult — optimization output (best position, history, convergence)
- FitnessLandscape — landscape characterization (rugosity, deceptiveness)
- AntTour / ACOProblem — TSP solution representation
- Sandpile — self-organized criticality model (BTW 1987)
- FlockConfig / PSOConfig / ACOConfig — algorithm-specific parameters

## L2: Core Concepts (Complete)
- Collective intelligence vs. individual intelligence
- Emergent behavior: macro patterns from micro rules
- Stigmergy: indirect coordination via environment
- Decentralized control: no central coordinator
- Self-organization: spontaneous order without external direction
- Positive feedback: pheromone amplification, recruitment
- Negative feedback: evaporation, crowding, saturation
- Exploration vs. exploitation tradeoff
- Local information → global intelligence
- Swarm topology: global, ring, von Neumann, random, star, dynamic
- Phase transition: disordered → ordered collective motion
- Self-organized criticality: power-law distributions
- Consensus: distributed agreement without central coordination

## L3: Mathematical Structures (Complete)
- Vector space operations over ℝ^D (position, velocity, gradients)
- Adjacency matrices, graph Laplacian L = D - A
- Row-stochastic weight matrices for consensus
- Pheromone transition probability: p_ij ∝ τ_ij^α · η_ij^β
- Reynolds steering forces: separation, alignment, cohesion
- PSO velocity update: v = ωv + c₁r₁(p-x) + c₂r₂(l-x)
- Constriction factor: χ = 2κ/|2-φ-√(φ²-4φ)|
- Vicsek order parameter: φ = |Σv_i|/(N·v₀)
- Cucker-Smale communication kernel: a(r) = K/(1+r²)^β
- Deneubourg response: P(τ) = τ²/(τ²+K²)
- Diffusion equation: ∂τ/∂t = D·∇²τ - λτ + S
- Fitness landscape metrics: rugosity, correlation length
- Information theory: transfer entropy, active information storage

## L4: Fundamental Laws/Theorems (Complete)
- **Reynolds' Three Rules (1987)**: Separation, Alignment, Cohesion → flocking
- **Clerc-Kennedy Constriction (2002)**: PSO converges iff φ > 4, χ ∈ (0,1]
- **Olfati-Saber Consensus Theorem (2004)**: Connected graph → average consensus
- **Cucker-Smale Theorem (2007)**: β < 1/2 → unconditional flocking convergence
- **Deneubourg Symmetry Breaking (1990)**: Pheromone amplification → bifurcation
- **Vicsek Phase Transition (1995)**: ∃ η_c where disorder → order
- **Bak-Tang-Wiesenfeld SOC (1987)**: Sandpile → power-law avalanches
- **Tsitsiklis Consensus Result (1984)**: Distributed agreement in asynchronous systems
- **Kalman Return Difference**: |I+K(jwI-A)⁻¹B| ≥ 1 (connected to flock stability)

## L5: Algorithms/Methods (Complete)
- Particle Swarm Optimization (Kennedy-Eberhart 1995)
- PSO with inertia weight (Shi-Eberhart 1998)
- PSO with constriction factor (Clerc-Kennedy 2002)
- Fully Informed Particle Swarm (Mendes et al. 2004)
- Quantum-behaved PSO (Sun et al. 2004)
- Ant System (Dorigo et al. 1991)
- Ant Colony System (Dorigo & Gambardella 1997)
- MAX-MIN Ant System (Stützle & Hoos 2000)
- Elitist AS / Rank-based AS
- Reynolds flocking with spatial hashing
- Vicsek self-driven particle model
- Cucker-Smale flocking model
- Consensus iteration (linear and gossip-based)
- Pairwise gossip (Boyd et al. 2006)
- Broadcast gossip / Geographic gossip
- Metropolis-Hastings consensus weights
- Majority vote / Voter model / Threshold model
- 2-opt / 3-opt local search for TSP
- Deneubourg threshold trail following
- Digital pheromone for UAV coordination
- Auction-based task allocation
- Market-based economic dispatch

## L6: Canonical Problems (Complete)
- Function optimization via PSO (Sphere, Rosenbrock, Rastrigin, Ackley, Griewank, Schwefel)
- Traveling Salesman Problem via ACO (30-city instances)
- Flocking simulation (50 boids, order parameter convergence)
- Distributed average consensus (ring topology)
- Binary bridge symmetry breaking experiment
- Sandpile self-organized criticality
- Vicsek phase transition (order parameter vs noise)
- Collective decision making (majority vote, quorum sensing)
- Vehicle routing problem (capacitated VRP)
- Multiple knapsack via ACO

## L7: Applications (Complete - 4 applications)
- **Swarm Robotics**: task allocation (threshold + auction), formation control
- **Supply Chain**: inventory management with ant-inspired pheromone routing
- **Smart Grid**: distributed economic dispatch, demand response coordination
- **UAV Swarm**: search & surveillance with digital pheromone maps
- **Communication Networks**: AntHocNet MANET routing, sensor clustering
- **Data Mining**: PSO feature selection, ant-based clustering
- **Finance**: Minority game (Challet & Zhang 1997)

## L8: Advanced Topics (Complete - 6 topics)
- **Phase transition analysis**: Binder cumulant, susceptibility, critical noise
- **Emergence metrics**: Gestalt emergence, statistical complexity, excess entropy
- **Information-theoretic measures**: Transfer entropy, active information storage
- **Computational mechanics**: CA classification (Wolfram 1-4), Langton λ
- **Critical slowing down**: Early warning signals (Kendall τ test)
- **Multi-swarm coevolution**: Cooperative PSO, niching and speciation
- **Fitness landscape analysis**: Rugosity, correlation length, deceptiveness
- **Agent-Based Model framework**: Generic ABM with callback-based agents

## L9: Research Frontiers (Partial — documented, not implemented)
- Meta-heuristic theory: why/when swarm algorithms work
- Quantum swarm optimization
- Human swarm interaction
- Swarm learning and federated optimization
- Formal verification of swarm protocols
