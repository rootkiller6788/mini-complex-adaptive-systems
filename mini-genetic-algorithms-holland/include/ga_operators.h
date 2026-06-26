/**
 * ga_operators.h — Selection, Crossover, Mutation, and Replacement Operators
 *
 * These are the fundamental variation operators that drive genetic search.
 * Each operator implements a distinct mechanism for exploring and exploiting
 * the search space, analogous to natural evolutionary forces.
 *
 * Reference: Goldberg "Genetic Algorithms in Search, Optimization & ML" (1989) Ch3-4
 *            Bäck "Evolutionary Algorithms in Theory and Practice" (1996)
 *            Eiben & Smith "Introduction to Evolutionary Computing" (2015)
 *
 * Knowledge: L1 Definitions, L2 Core Concepts, L5 Algorithms/Methods
 */

#ifndef GA_OPERATORS_H
#define GA_OPERATORS_H

#include "ga_core.h"

/* ====================================================================
 * L5: Selection Operators
 *
 * Selection is the driver of evolutionary pressure. It determines which
 * individuals contribute genetic material to the next generation based
 * on their fitness. Each method embodies different selection pressure
 * and diversity preservation trade-offs.
 * ==================================================================== */

/**
 * Roulette Wheel (Fitness Proportionate) Selection
 * P(i) = f_i / Σ f_j
 *
 * Issues premature convergence when fitness variance is low.
 * Complexity: O(N) per selection with prefix sum precomputation
 *
 * Reference: Holland (1975), Goldberg (1989) Ch2
 */
int         ga_select_roulette(const Population *pop);

/**
 * Tournament Selection: pick k random individuals, return the best.
 * Selection pressure controlled by tournament size k.
 * k=2 is default; larger k increases pressure.
 *
 * Complexity: O(k) per selection
 * Reference: Goldberg & Deb (1991)
 */
int         ga_select_tournament(const Population *pop, int tournament_size);

/**
 * Rank Selection: selection probability based on rank (not raw fitness).
 * Linear ranking: P(rank_r) = (2-sp)/N + 2r(sp-1)/(N(N-1))
 * where sp ∈ [1.0, 2.0] is selection pressure parameter.
 *
 * Reference: Baker (1985), Whitley (1989)
 */
int         ga_select_rank(const Population *pop, double selection_pressure);

/**
 * Stochastic Universal Sampling (SUS): unbiased sampling with one spin.
 * Single random value + N equally spaced pointers.
 * Guarantees zero bias: E[offspring_count_i] = N * P(i)
 *
 * Reference: Baker (1987)
 */
void        ga_select_sus(const Population *pop, int *selected_indices, int n_select);

/**
 * Truncation Selection: select the best τ fraction of the population,
 * then uniformly randomly pick from them.
 *
 * Reference: Mühlenbein & Schlierkamp-Voosen (1993)
 */
int         ga_select_truncation(const Population *pop, double truncation_ratio);

/**
 * Compute selection probabilities for the population using the current method.
 * Fills probs[] array of length pop->size.
 */
void        ga_selection_probabilities(const Population *pop,
                                        SelectionMethod method,
                                        double *probs,
                                        double param);

/* ====================================================================
 * L5: Crossover (Recombination) Operators
 *
 * Crossover combines genetic material from two parents to produce offspring.
 * It exploits existing building blocks by recombining them.
 * ==================================================================== */

/**
 * Single-Point Crossover: cut at position k, swap segments.
 * parent1: AAA|AAA  →  child1: AAA|BBB
 * parent2: BBB|BBB  →  child2: BBB|AAA
 *
 * Reference: Holland (1975)
 */
void        ga_crossover_one_point(const Chromosome *p1, const Chromosome *p2,
                                    Chromosome *c1, Chromosome *c2);

/**
 * Two-Point Crossover: cut at positions k1, k2, swap middle segment.
 * parent1: AA|AAA|AA  →  child1: AA|BBB|AA
 * parent2: BB|BBB|BB  →  child2: BB|AAA|BB
 *
 * Reference: De Jong (1975)
 */
void        ga_crossover_two_point(const Chromosome *p1, const Chromosome *p2,
                                    Chromosome *c1, Chromosome *c2);

/**
 * Uniform Crossover: each gene independently chosen from either parent
 * with probability 0.5 (or bias parameter).
 *
 * Reference: Syswerda (1989)
 */
void        ga_crossover_uniform(const Chromosome *p1, const Chromosome *p2,
                                  Chromosome *c1, Chromosome *c2,
                                  double bias);

/**
 * Arithmetic Crossover (BLX-α): blend crossover for real-valued genes.
 * child = α*p1 + (1-α)*p2  where α ∈ [-α_ext, 1+α_ext]
 *
 * Reference: Eshelman & Schaffer (1993)
 */
void        ga_crossover_arithmetic(const Chromosome *p1, const Chromosome *p2,
                                     Chromosome *c1, Chromosome *c2,
                                     double alpha_ext);

/**
 * Simulated Binary Crossover (SBX): mimics single-point crossover on reals.
 * Uses spread factor β such that c1,c2 = 0.5[(p1+p2) ± β|p1-p2|]
 *
 * Reference: Deb & Agrawal (1995)
 */
void        ga_crossover_sbx(const Chromosome *p1, const Chromosome *p2,
                              Chromosome *c1, Chromosome *c2,
                              double eta);

/**
 * Order Crossover (OX): for permutation chromosomes.
 * Preserves relative ordering of genes.
 *
 * Reference: Davis (1985)
 */
void        ga_crossover_ox(const Chromosome *p1, const Chromosome *p2,
                             Chromosome *c1, Chromosome *c2);

/**
 * Partially Mapped Crossover (PMX): for permutation encoding.
 * Maps positions from parent segments, preserving absolute positions.
 *
 * Reference: Goldberg & Lingle (1985)
 */
void        ga_crossover_pmx(const Chromosome *p1, const Chromosome *p2,
                              Chromosome *c1, Chromosome *c2);

/* ====================================================================
 * L5: Mutation Operators
 *
 * Mutation introduces random perturbations to maintain genetic diversity
 * and prevent premature convergence to local optima. It provides the
 * "insurance policy" against irreversible loss of genetic material.
 * ==================================================================== */

/**
 * Bit-Flip Mutation: flip each bit with probability pm.
 * For binary chromosomes: 0↔1.
 *
 * Reference: Holland (1975)
 */
void        ga_mutate_bit_flip(Chromosome *c, double mutation_rate);

/**
 * Gaussian Mutation: add N(0, σ) noise to real-valued genes.
 * σ adapts based on gene range: σ = mutation_rate * (upper-lower).
 *
 * Reference: Bäck & Schwefel (1993)
 */
void        ga_mutate_gaussian(Chromosome *c, double mutation_rate,
                                double step_size);

/**
 * Uniform Mutation: reset a gene to uniform random value within bounds.
 *
 * Reference: Michalewicz (1996)
 */
void        ga_mutate_uniform(Chromosome *c, double mutation_rate);

/**
 * Polynomial Mutation: for real-valued genes with distribution index η.
 * More controlled than Gaussian; widely used with SBX crossover.
 *
 * Reference: Deb (2001)
 */
void        ga_mutate_polynomial(Chromosome *c, double mutation_rate,
                                  double eta);

/**
 * Swap Mutation: swap two randomly selected genes.
 * Used for permutation chromosomes.
 */
void        ga_mutate_swap(Chromosome *c);

/**
 * Inversion Mutation: reverse a random subsequence of genes.
 * Used for permutation chromosomes.
 */
void        ga_mutate_inversion(Chromosome *c);

/**
 * Scramble Mutation: reorder a random subsequence of genes randomly.
 * More disruptive than swap or inversion.
 */
void        ga_mutate_scramble(Chromosome *c);

/* ====================================================================
 * L5: Replacement Strategies
 *
 * Replacement determines which individuals survive to the next generation,
 * controlling the balance between exploration (new individuals) and
 * exploitation (keeping fit individuals).
 * ==================================================================== */

/**
 * Generational Replacement: entire parent population replaced by offspring.
 */
void        ga_replace_generational(Population *parent, Population *offspring);

/**
 * Steady-State Replacement: replace worst N_worst individuals with
 * best N_worst from offspring. N_worst = |offspring|.
 */
void        ga_replace_steady_state(Population *parent,
                                     const Population *offspring);

/**
 * Elitist Replacement: keep best n_elite parents, fill rest with offspring.
 * Guarantees monotonic improvement of best fitness.
 */
void        ga_replace_elitist(Population *parent, Population *offspring,
                                int n_elite);

/**
 * (μ+λ) Replacement: select best μ from combined pool of μ parents + λ offspring.
 */
void        ga_replace_mu_plus_lambda(Population *parent,
                                       Population *offspring);

/**
 * (μ,λ) Replacement: select best μ from λ offspring only (parents discarded).
 * Requires λ > μ. Better for noisy fitness functions.
 */
void        ga_replace_mu_comma_lambda(Population *parent,
                                        Population *offspring);

/**
 * Crowding Replacement: each offspring replaces the most similar parent.
 * Preserves diversity by local competition.
 *
 * Reference: De Jong (1975), Mahfoud (1995)
 */
void        ga_replace_crowding(Population *parent, Population *offspring);

/* ====================================================================
 * L3: Operator Analysis Functions
 * ==================================================================== */

/** Compute the expected number of schema disrupted by a given crossover */
double      ga_crossover_disruption_rate(int chromosome_length,
                                          CrossoverMethod method);

/** Compute the probability that a schema of defining length δ survives mutation */
double      ga_schema_survival_probability(int defining_length, int order,
                                            double mutation_rate, int chrom_len);

/** Compute positional bias: tendency to disrupt linkage between adjacent genes */
double      ga_positional_bias(CrossoverMethod method, int chrom_len);

#endif /* GA_OPERATORS_H */
