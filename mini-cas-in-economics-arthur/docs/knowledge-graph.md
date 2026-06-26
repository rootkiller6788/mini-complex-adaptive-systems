# Knowledge Graph - CAS in Economics (W. Brian Arthur)

## L1: Definitions (Complete)
- Complex Adaptive System (CAS): Heterogeneous agents interacting locally
- Increasing Returns: Positive feedback, success breeds more success
- Path Dependence: History matters for limiting outcomes
- Lock-in: Irreversible dominance of one technology/standard
- Bounded Rationality: Limited cognitive capacity, satisficing (Simon)
- Inductive Reasoning: Form hypotheses, test, discard (Arthur 1994)
- Emergence: Macro patterns not reducible to micro rules
- Polya Urn: Mathematical model of increasing returns
- Minority Game: N agents choose side; minority wins
- Classifier System: IF-THEN rules with bucket brigade learning

## L2: Core Concepts (Complete)
- Self-Reinforcement: Success increases future success probability
- Multiple Equilibria: Under increasing returns, many outcomes possible
- Network Externalities: Value increases with users
- Adoption Dynamics: Competing technology market share evolution
- Inductive Learning: Agents learn predictive models from experience
- Market Ecology: Co-evolving population of trading strategies
- Combinatorial Evolution: New tech from existing combinations
- Autocatalytic Sets: Self-sustaining technology clusters
- Tipping Points: Threshold for self-sustaining adoption
- Non-Ergodicity: Long-run behavior depends on initial conditions

## L3: Mathematical Structures (Complete)
- Polya Urn stochastic process
- Generalized Polya Urn with arbitrary f(proportion)
- Softmax probabilistic choice function
- NK Fitness Landscape: N components, K interactions
- SDE: dX = drift*dt + diffusion*dW
- Markov transition matrix
- Adoption share simplex
- Network adjacency matrix
- Lorenz Curve + Gini Coefficient
- HHI market concentration index
- Shannon entropy diversity measure
- Hurst exponent long-range dependence

## L4: Fundamental Theorems (Complete)
- Arthur Polya Urn Theorem: X_n -> x* where f(x*)=x*
- Lock-in Theorem: gamma>1 => lock-in with probability 1
- Arthur Adoption Theorem: Increasing returns => monopoly
- Path Dependence Theorem: Non-ergodic => history-dependent limits
- Critical Mass Theorem: p* = n^(-gamma/(gamma+1))
- Shannon Entropy Maximum at uniform distribution
- HHI Bounds: 1/n <= HHI <= 1
- Gini Scale Invariance: G(a*X) = G(X)
- Cascade Termination in finite iterations
- NK Peak Count: E[peaks] = 2^N/(N+1) for K=N-1

## L5: Algorithms/Methods (Complete)
- Polya Urn Monte Carlo simulation
- Arthur urn function f(p)=p^gamma/(p^gamma+(1-p)^gamma)
- Linear urn f(p)=a*p+b
- Sigmoid adoption (Bass) f(p)=1/(1+exp(-k(p-p0)))
- Binary adoption step (Arthur 1989)
- Multi-technology adoption
- Lock-in detection (consecutive threshold)
- Euler-Maruyama SDE scheme
- Drift/diffusion estimation from time series
- Path dependence test (multi-start divergence)
- Schelling utility computation
- Tipping threshold binary search
- Network cascade simulation
- Welford online statistics
- Skewness/kurtosis moments
- Hurst R/S analysis
- Runs test stationarity
- Gini coefficient computation
- Hill estimator for Pareto tail
- Lorenz curve ordinates
- Macro state aggregation
- Crisis detection multi-indicator
- Market concentration HHI dynamics
- El Farol Bar simulation (Arthur 1994)
- Minority game step
- Genetic algorithm (tournament+crossover+mutation)
- Bucket brigade credit assignment
- Classifier system GA
- Technology combination (Arthur 2009)
- Novelty computation in tech ecosystem
- Complementarity detection
- Cluster detection modularity
- NK landscape generation
- Adaptive walk greedy
- Adaptive walk steepest ascent
- Adaptive walk thermal (SA)
- Peak finding exhaustive
- Autocatalytic set detection
- Catalytic closure computation
- ACE full model integrated simulation

## L6: Canonical Problems (Complete)
- El Farol Bar Problem (Arthur 1994)
- Technology Lock-in (Arthur 1989)
- Polya Urn Path Dependence (Arthur et al 1987)
- Santa Fe Artificial Stock Market (Arthur et al 1997)
- NK Adaptive Walk (Kauffman 1993)
- Network Cascade (Watts 2002)

## L7: Applications (Partial+)
- Technology adoption forecasting
- Financial market regime detection
- Platform economics network effects
- Innovation policy for combinatorial evolution
- Inequality analysis for policy

## L8: Advanced Topics (Partial+)
- Non-ergodic economic dynamics
- Heavy-tail distributions (Gabaix 2009)
- Combinatorial evolution networks
- Autocatalytic economic growth
- Agent-Based Computational Economics

## L9: Research Frontiers (Partial)
- AI-driven agent-based economics
- Complexity-based economic policy
- Evolutionary economics with ML
- Climate-economy co-evolution
