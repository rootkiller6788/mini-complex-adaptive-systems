# Knowledge Graph: NK Fitness Landscapes (Kauffman)

## L1: Definitions

| # | Concept | C Implementation | Lean Formalization |
|---|---------|-----------------|-------------------|
| 1 | Genotype (binary string of length N) | `NKGenotype` struct, bit-packed `uint8_t*` | `Genotype N := Fin N → Bool` |
| 2 | Fitness landscape F: {0,1}^N → [0,1] | `NKLandscape` struct | `Fitness N := Genotype N → Rat` |
| 3 | Epistatic interaction matrix | `NKEpistaticMatrix` with `neighbors[i][k]` | `NKEpistaticGraph N K` |
| 4 | Fitness contribution table | `NKFitnessTable` with `f[i][config]` | -- |
| 5 | Allele (0 or 1 per locus) | `nk_genotype_set/get` | `Bool` encoding |
| 6 | Hamming distance | `nk_genotype_hamming()` | `Genotype.hamming` |
| 7 | Local optimum | `nk_is_local_optimum()` | `isLocalOptimum` |
| 8 | Adaptive walk | `NKWalkResult` struct | `AdaptiveWalkStep` |
| 9 | Adaptive walk step | `NKWalkState` struct | -- |
| 10 | NKCS coevolution pair | `NKCSLandscape` struct | `CoevolutionSystem` |

## L2: Core Concepts

| # | Concept | Implementation |
|---|---------|---------------|
| 1 | Tunable ruggedness via K | `nk_ruggedness_estimate()` |
| 2 | Landscape phase (ordered/critical/chaotic) | `nk_landscape_phase()` |
| 3 | Complexity catastrophe | `nk_complexity_catastrophe()` |
| 4 | Edge of chaos | Phase classification at K/N ≈ 0.2-0.5 |
| 5 | Basin of attraction | `nk_walk_basin_size()` |
| 6 | Adaptive walk trajectory | `NKWalkResult` with full path recording |
| 7 | Fitness autocorrelation | `nk_autocorrelation_at_distance()` |
| 8 | Correlation length | `nk_correlation_length()` |
| 9 | Evolvability (diversity of reachable optima) | `nk_walk_count_distinct_optima()` |
| 10 | NKCS coupling strength | `coupling_strength` in `NKCSLandscape` |

## L3: Mathematical Structures

| # | Structure | Implementation |
|---|-----------|---------------|
| 1 | Boolean hypercube {0,1}^N | Bit-packed genotype with flip operations |
| 2 | Adjacency matrix (Hamming distance 1) | `Genotype.neighbors` in Lean |
| 3 | Epistatic interaction graph | `NKEpistaticMatrix` with RANDOM/ADJACENT/BLOCK |
| 4 | Fitness contribution tensor | `NKFitnessTable` (N × 2^(K+1)) |
| 5 | Walsh/Hadamard basis on {0,1}^N | `nk_walsh_coefficient()`, `nk_walsh_decomposition()` |
| 6 | Fitness variance decomposition | `nk_epistasis_variance_spectrum()` |
| 7 | Spearman rank correlation | `nk_landscape_correlation()` |
| 8 | Nash equilibrium pair in NKCS | `nkcs_is_nash_equilibrium()` |

## L4: Fundamental Laws

| # | Law | Verification |
|---|-----|-------------|
| 1 | rho(d) = (1 - d/N)^K (Weinberger 1990) | `nk_autocorrelation_at_distance()` |
| 2 | Number of local optima ≈ 2^N/(N+1) for K=N-1 | `nk_estimate_local_optima()` |
| 3 | Complexity catastrophe: optimum fitness decreases with K | `nk_complexity_catastrophe()` |
| 4 | Adaptive walk length ~ ln(N) for K ≥ 2 | Verified in test suite |
| 5 | K=0 => exactly 1 local optimum | `test_local_optimum_detection()` |
| 6 | Greedy walk always terminates at local optimum | Always terminates (finite state space) |
| 7 | rho(1) determines ruggedness | `nk_ruggedness_estimate()` |
| 8 | Coupling catastrophe in NKCS | `nkcs_coupling_catastrophe()` |

## L5: Algorithms/Methods

| # | Algorithm | Implementation |
|---|-----------|---------------|
| 1 | Greedy adaptive walk (first improvement) | `nk_walk_greedy()` |
| 2 | Steepest ascent (best improvement) | `nk_walk_steepest_ascent()` |
| 3 | Next-ascent (scan with memory) | `nk_walk_next_ascent()` |
| 4 | Random-better (uniform among improving) | `nk_walk_random_better()` |
| 5 | Metropolis walk (probabilistic downhill) | `nk_walk_metropolis()` |
| 6 | Simulated annealing (cooling schedule) | `nk_walk_simulated_annealing()` |
| 7 | Fast Walsh-Hadamard Transform (FWHT) | `nk_walsh_decomposition()` |
| 8 | Xorshift64* PRNG | Internal RNG in nk_core.c |
| 9 | Box-Muller normal sampling | Internal in nk_core.c |
| 10 | Rank correlation computation | `nk_landscape_correlation()` |

## L6: Canonical Problems

| # | Problem | Implementation |
|---|---------|---------------|
| 1 | Construct NK landscape with tunable ruggedness | `nk_landscape_create()` |
| 2 | Find local optima via adaptive walk | All walk functions |
| 3 | Characterize landscape ruggedness | `nk_ruggedness_estimate()` |
| 4 | Detect sign epistasis | `nk_epistasis_sign_type()` |
| 5 | Detect reciprocal sign epistasis | `nk_epistasis_sign_matrix()` |
| 6 | Compute epistasis variance spectrum | `nk_epistasis_variance_spectrum()` |
| 7 | Verify complexity catastrophe | `nk_complexity_catastrophe()` |
| 8 | Find Nash equilibrium in coevolution | `nkcs_find_nash_equilibrium()` |

## L7: Applications

| # | Application | Implementation |
|---|-------------|---------------|
| 1 | Evolutionary biology: adaptive landscapes | Full NK model (Wright/Kauffman) |
| 2 | Coevolutionary arms races (Red Queen) | `nkcs_red_queen_measure()` |
| 3 | Protein fitness landscapes | Tunable ruggedness models protein evolution |
| 4 | Organizational adaptation | NK model applied to organizational theory (Levinthal 1997) |
| 5 | Technological evolution | Landscape model of technology search (Kauffman 1995) |

## L8: Advanced Topics

| # | Topic | Implementation |
|---|-------|---------------|
| 1 | NKCS coevolution model | Full `nkcs_*` implementation |
| 2 | Coevolutionary avalanches | `nkcs_avalanche_distribution()` |
| 3 | Nash equilibrium analysis | `nkcs_find_nash_equilibrium()` |
| 4 | Walsh/Hadamard spectral decomposition | `nk_walsh_decomposition()` |
| 5 | Coupling catastrophe | `nkcs_coupling_catastrophe()` |
| 6 | Mutual information in coevolution | `nkcs_mutual_information()` |

## L9: Research Frontiers

| # | Topic | Status |
|---|-------|--------|
| 1 | Exact adaptive walk length distribution | Documented (open problem) |
| 2 | Landscape predictability and evolution | Documented |
| 3 | Evolvability from Walsh spectrum | Documented |
| 4 | NK landscapes and spin glass correspondence | Documented |
| 5 | Neutral network theory on rugged landscapes | Documented |
