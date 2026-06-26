# Knowledge Graph — mini-genetic-algorithms-holland

## L1: Definitions (Complete)

| # | Definition | C Implementation | Lean Formalization |
|---|-----------|-----------------|-------------------|
| 1 | Gene — atomic unit of heredity | `GeneType` enum in ga_core.h | — |
| 2 | Allele — specific value of a gene | `Allele` union in ga_core.h | `BinAllele` (zero, one, star) |
| 3 | Locus — gene position on chromosome | `Locus` struct in ga_core.h | — |
| 4 | Chromosome — ordered sequence of genes | `Chromosome` struct in ga_core.h | `Chromosome` structure |
| 5 | Population — collection of chromosomes | `Population` struct in ga_core.h | `Population` structure |
| 6 | Fitness — objective function value | `fitness` field in Chromosome | `Fitness` type alias |
| 7 | Schema — similarity template over {0,1,*} | `Schema` struct in ga_theory.h | `Schema` structure |
| 8 | Schema Order o(H) — number of fixed positions | `ga_schema_order()` in ga_theory.c | `schema_order` def + theorems |
| 9 | Schema Defining Length δ(H) — outermost fixed distance | `ga_schema_defining_length()` | `schema_defining_length` def + theorems |
| 10 | Building Block — short, low-order, fit schema | `is_building_block()` in ga_theory.h | `is_building_block` def |
| 11 | Encoding — genotype-to-phenotype mapping | `EncodingType` enum in ga_encoding.h | — |
| 12 | Pareto Dominance — multi-objective comparison | `ga_pareto_dominates()` in ga_fitness.h | `pareto_dominates` def |

## L2: Core Concepts (Complete)

| # | Concept | Implementation |
|---|---------|---------------|
| 1 | Generation — one iteration of the GA loop | `ga_generation()` in ga_core.c |
| 2 | Selection Pressure — intensity of fitness-based selection | `ga_selection_pressure()` in ga_core.c |
| 3 | Exploration vs Exploitation — diversity vs convergence tradeoff | mutation rate + crossover rate tuning |
| 4 | Convergence — population reaching a steady state | `ga_has_converged()` in ga_core.c |
| 5 | Premature Convergence — early loss of diversity | stall detection in `ga_run()` |
| 6 | Effective Population Size | `ga_effective_population_size()` in ga_core.c |
| 7 | Hamming Distance — binary chromosome similarity | `ga_hamming_distance()` in ga_core.c |
| 8 | Euclidean Distance — real chromosome similarity | `ga_euclidean_distance()` in ga_core.c |
| 9 | Diversity — population genetic variance | `ga_population_diversity()` in ga_core.c |
| 10 | Gene Pool Entropy | `ga_gene_entropy()` in ga_core.c |
| 11 | Niching — subpopulation formation | `ga_fitness_sharing()` in ga_population.c |
| 12 | Crowding — local competition replacement | `ga_replace_crowding()`, `ga_deterministic_crowding()` |
| 13 | Co-evolution — interacting populations | `ga_coevolution_fitness()` in ga_population.c |
| 14 | Speciation — clustering into species | `ga_speciate()` in ga_population.c |

## L3: Mathematical Structures (Complete)

| # | Structure | Implementation |
|---|-----------|---------------|
| 1 | Binary string search space {0,1}^L | `GENE_BINARY` type + binary chromosome operations |
| 2 | Real vector search space R^n | `GENE_REAL` type + real chromosome operations |
| 3 | Permutation space S_n | `GENE_PERMUTATION` type + OX/PMX crossover |
| 4 | Schema lattice (2^L partially ordered by specificity) | `Schema` + `ga_schema_from_two()` |
| 5 | Fitness landscape F: {0,1}^L → R | `ga_fitness_*()` functions |
| 6 | Walsh coefficients / Fourier basis | `ga_epistasis_measure()` in ga_fitness.c |
| 7 | Fitness-distance correlation | `ga_fdc()` in ga_fitness.c |
| 8 | Autocorrelation function | `ga_fitness_autocorrelation()` in ga_fitness.c |
| 9 | Information content metric | `ga_information_content()` in ga_fitness.c |
| 10 | Pareto front (multi-objective) | `ga_pareto_dominates()`, `ga_crowding_distance()` |
| 11 | Markov chain transition matrix (Vose model) | `ga_vose_mixing_entry()`, `ga_vose_heuristic_step()` |
| 12 | Shannon entropy on gene distributions | `ga_population_fitness_entropy()` in ga_population.c |

## L4: Fundamental Laws (Complete)

| # | Theorem | C Implementation | Lean Formalization |
|---|---------|-----------------|-------------------|
| 1 | **Schema Theorem** (Holland 1975): `E[m(H,t+1)] ≥ m(H,t)·(f(H)/f̄)·[1-pc·δ(H)/(L-1)]·(1-pm)^o(H)` | `ga_schema_theorem_lower_bound()` in ga_theory.c | `schema_crossover_survival_nonneg`, `schema_order_le_length` |
| 2 | **Building Block Hypothesis** (Goldberg 1989) | `ga_identify_building_blocks()` in ga_theory.c | `is_building_block` def |
| 3 | **Implicit Parallelism**: O(N³) schemata processed | `ga_implicit_parallelism()` in ga_theory.c | — |
| 4 | **Price's Equation** (Price 1970): `Δz̄ = Cov(w,z)/w̄ + E(w·Δz)/w̄` | `ga_price_equation()` in ga_theory.c | — |
| 5 | **Fisher's Fundamental Theorem**: `Δf̄ ≈ Var_g(f)/f̄` | `ga_fisher_fundamental_theorem()` in ga_theory.c | — |
| 6 | **No Free Lunch Theorem** (Wolpert & Macready 1997) | `ga_nfl_bound()` in ga_theory.c | `nfl_ratio_le_one`, `nfl_expected_success_upper` |
| 7 | **Vose's Dynamical System** (infinite population) | `ga_vose_heuristic_step()` in ga_theory.c | — |
| 8 | Schema Crossover Survival: `P(survive|cross) ≥ 1-δ/(L-1)` | `ga_crossover_disruption_rate()` in ga_operators.c | `schema_defining_length_le_length` |

## L5: Algorithms / Methods (Complete)

| # | Algorithm | Implementation |
|---|-----------|---------------|
| 1 | Roulette Wheel Selection | `ga_select_roulette()` in ga_operators.c |
| 2 | Tournament Selection | `ga_select_tournament()` in ga_operators.c |
| 3 | Rank Selection | `ga_select_rank()` in ga_operators.c |
| 4 | Stochastic Universal Sampling (SUS) | `ga_select_sus()` in ga_operators.c |
| 5 | Truncation Selection | `ga_select_truncation()` in ga_operators.c |
| 6 | Single-Point Crossover | `ga_crossover_one_point()` in ga_operators.c |
| 7 | Two-Point Crossover | `ga_crossover_two_point()` in ga_operators.c |
| 8 | Uniform Crossover | `ga_crossover_uniform()` in ga_operators.c |
| 9 | Arithmetic (BLX-α) Crossover | `ga_crossover_arithmetic()` in ga_operators.c |
| 10 | Simulated Binary Crossover (SBX) | `ga_crossover_sbx()` in ga_operators.c |
| 11 | Order Crossover (OX) | `ga_crossover_ox()` in ga_operators.c |
| 12 | Partially Mapped Crossover (PMX) | `ga_crossover_pmx()` in ga_operators.c |
| 13 | Bit-Flip Mutation | `ga_mutate_bit_flip()` in ga_operators.c |
| 14 | Gaussian Mutation | `ga_mutate_gaussian()` in ga_operators.c |
| 15 | Uniform Mutation | `ga_mutate_uniform()` in ga_operators.c |
| 16 | Polynomial Mutation | `ga_mutate_polynomial()` in ga_operators.c |
| 17 | Swap Mutation | `ga_mutate_swap()` in ga_operators.c |
| 18 | Inversion Mutation | `ga_mutate_inversion()` in ga_operators.c |
| 19 | Scramble Mutation | `ga_mutate_scramble()` in ga_operators.c |
| 20 | Elitist Replacement | `ga_replace_elitist()` in ga_operators.c |
| 21 | Steady-State Replacement | `ga_replace_steady_state()` in ga_operators.c |
| 22 | (μ+λ) Replacement | `ga_replace_mu_plus_lambda()` in ga_operators.c |
| 23 | (μ,λ) Replacement | `ga_replace_mu_comma_lambda()` in ga_operators.c |
| 24 | Crowding Replacement | `ga_replace_crowding()` in ga_operators.c |
| 25 | Binary Encoding / Decoding | `ga_encode_binary_from_real()` / `ga_decode_binary_to_real()` |
| 26 | Gray Coding | `ga_binary_to_gray()` / `ga_gray_to_binary()` |
| 27 | Random Keys Encoding | `ga_encode_random_keys_init()` / `ga_decode_random_keys()` |
| 28 | Population Restart | `ga_restart()` in ga_core.c |

## L6: Canonical Problems (Complete)

| # | Problem | Implementation |
|---|---------|---------------|
| 1 | **OneMax** — count ones | `ga_fitness_onemax()` in ga_fitness.c |
| 2 | **Needle-in-a-Haystack** — exact match | `ga_fitness_needle()` in ga_fitness.c |
| 3 | **Deceptive Trap** — misleading local optima | `ga_fitness_deceptive_trap()` in ga_fitness.c |
| 4 | **Royal Road** — building block hierarchy | `ga_fitness_royal_road()` in ga_fitness.c |
| 5 | **NK Landscape** — tunable epistasis | `ga_fitness_nk_landscape()` in ga_fitness.c |
| 6 | **Sphere** function optimization | `ga_fitness_sphere()` in ga_fitness.c |
| 7 | **Rosenbrock** (banana) function | `ga_fitness_rosenbrock()` in ga_fitness.c |
| 8 | **Rastrigin** — highly multimodal | `ga_fitness_rastrigin()` in ga_fitness.c |
| 9 | **Ackley** function | `ga_fitness_ackley()` in ga_fitness.c |
| 10 | **Schwefel** function | `ga_fitness_schwefel()` in ga_fitness.c |
| 11 | **Griewank** function | `ga_fitness_griewank()` in ga_fitness.c |
| 12 | **Traveling Salesman Problem** (TSP) | `ga_fitness_tsp()` in ga_fitness.c |
| 13 | **0/1 Knapsack** | `ga_fitness_knapsack()` in ga_fitness.c |
| 14 | **Graph Coloring** | `ga_fitness_graph_coloring()` in ga_fitness.c |
| 15 | **Job Shop Scheduling** (JSSP) | `ga_fitness_jobshop()` in ga_fitness.c |

## L7: Applications (Complete)

| # | Application | Implementation |
|---|-------------|---------------|
| 1 | **Island Model GA** — parallel subpopulations with migration | `ga_island_model_*()` in ga_population.c |
| 2 | **Spatial/Cellular GA** — 2D grid with local neighborhoods | `ga_spatial_*()` in ga_population.c |
| 3 | **Multi-Objective Optimization** — Pareto front + NSGA-II nondominated sort | `ga_nondominated_sort()`, `ga_pareto_dominates()` |
| 4 | **External Archive** — Pareto-optimal set maintenance | `ga_archive_*()` in ga_population.c |
| 5 | **Dynamic Restart** — adaptive diversity injection | `ga_restart()` in ga_core.c |
| 6 | **Fitness Sharing** — niching for multimodal optimization | `ga_fitness_sharing()` in ga_population.c |

## L8: Advanced Topics (Complete)

| # | Topic | Implementation |
|---|-------|---------------|
| 1 | **Vose Infinite Population Model** — Markov chain GA | `ga_vose_mixing_entry()`, `ga_vose_heuristic_step()` |
| 2 | **Messy GA** — variable-length encoding | `ga_messy_init()`, `ga_messy_resolve()`, `ga_messy_cut_splice()` |
| 3 | **Co-evolutionary Dynamics** — competitive/cooperative | `ga_competitive_fitness()`, `ga_cooperative_fitness()` |
| 4 | **Speciation** — clustering into species | `ga_speciate()` in ga_population.c |
| 5 | **Non-dominated Sorting** — NSGA-II front ranking | `ga_nondominated_sort()` in ga_population.c |
| 6 | **Fitness Landscape Analysis** — FDC, epistasis, autocorrelation | `ga_fdc()`, `ga_epistasis_measure()`, `ga_fitness_autocorrelation()` |

## L9: Research Frontiers (Partial)

| # | Topic | Status |
|---|-------|--------|
| 1 | Quality Diversity (MAP-Elites) | Documented, not implemented |
| 2 | Hyper-heuristics for operator selection | Documented |
| 3 | Linkage Learning / EDAs | Partially via messy GA |
| 4 | Theory of GA Dynamics (Vose) | Implemented (heuristic) |
| 5 | Runtime Analysis of GA | Documented |
