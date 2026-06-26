/**
 * ga_theory.h — Schema Theory and Theoretical Foundations of GA
 *
 * Schema theory, developed by John Holland, provides the mathematical
 * foundation for understanding why genetic algorithms work. It explains
 * how GAs process building blocks through implicit parallelism.
 *
 * Reference: Holland "Adaptation in Natural and Artificial Systems" (1975,1992)
 *            Goldberg "Genetic Algorithms" (1989) Ch2 — Schema Theorem
 *            Vose "The Simple Genetic Algorithm" (1999) — Markov model
 *            Stephens & Waelbroeck "Schemas, Building Blocks, and GP" (1999)
 *
 * Knowledge: L1 Definitions, L3 Math Structures, L4 Fundamental Laws
 */

#ifndef GA_THEORY_H
#define GA_THEORY_H

#include "ga_core.h"
#include "ga_operators.h"

/* ====================================================================
 * L1: Schema Definition
 *
 * A schema H is a similarity template describing a subset of chromosomes
 * with similarities at certain positions. In the alphabet {0,1,*},
 * where * is a "don't care" symbol (wildcard).
 * ==================================================================== */

#define GA_SCHEMA_MAX_LENGTH  128
#define GA_SCHEMA_ALPHABET_SZ 3    /* {0, 1, *} */

typedef enum {
    SCHEMA_ALLELE_0 = 0,
    SCHEMA_ALLELE_1 = 1,
    SCHEMA_ALLELE_STAR = 2   /* don't care */
} SchemaAllele;

typedef struct {
    int           length;                         /* chromosome length this schema matches */
    SchemaAllele  alleles[GA_SCHEMA_MAX_LENGTH]; /* fixed alleles at each position */
    int           order;                          /* o(H): number of fixed positions */
    int           defining_length;                /* δ(H): distance between first & last fixed */
    double        observed_fitness;               /* average fitness of chromosomes matching H */
    int           count;                          /* number of chromosomes matching H in population */
    double        upper_bound_fitness;            /* CLO bound (Wright 2007) */
    int           generation_discovered;
} Schema;

/* ====================================================================
 * L1: Schema Operations
 * ==================================================================== */

/** Create a schema from a template string (e.g., "01*10**1") */
Schema  ga_schema_create(const char *template_str);

/** Check if a chromosome is an instance of (matches) the schema */
bool    ga_schema_matches(const Schema *schema, const Chromosome *c);

/** Compute the order o(H) — number of fixed positions */
int     ga_schema_order(const Schema *schema);

/** Compute the defining length δ(H) — distance between outermost fixed positions */
int     ga_schema_defining_length(const Schema *schema);

/**
 * Generate all schemata of order k from a chromosome.
 * For a chromosome of length L, there are C(L,k) * 2^k such schemata.
 */
int     ga_chromosome_schemata(const Chromosome *c, int order,
                                Schema *result, int max_results);

/**
 * Generalization: make a schema from two chromosomes by keeping common
 * alleles and wildcarding differences.
 */
Schema  ga_schema_from_two(const Chromosome *a, const Chromosome *b);

/** Print schema in human-readable form (e.g., "01*10**1") */
void    ga_schema_print(const Schema *schema, char *buf, int buf_len);

/* ====================================================================
 * L4: Schema Theorem (Holland, 1975)
 *
 * E[m(H, t+1)] ≥ m(H, t) * [f(H)/f̄] * [1 - p_c * δ(H)/(L-1) * P_survive(H)]
 *    * [1 - p_m]^o(H)
 *
 * where:
 *   m(H, t)   = number of instances of schema H at generation t
 *   f(H)      = average fitness of chromosomes matching H
 *   f̄         = average fitness of entire population
 *   p_c       = crossover probability
 *   δ(H)      = defining length of schema H
 *   L         = chromosome length
 *   p_m       = mutation probability (per bit)
 *   o(H)      = order of schema H
 *   P_survive = probability schema survives crossover
 * ==================================================================== */

/**
 * Compute the Schema Theorem's lower bound on expected copies in next
 * generation.
 *
 * Returns E[m(H, t+1)], the expected number of instances.
 */
double  ga_schema_theorem_lower_bound(const Schema *schema,
                                       const Population *pop,
                                       double crossover_rate,
                                       double mutation_rate);

/**
 * Compute Schema Theorem upper bound (more recent refinements).
 * Uses exact survival probability with two-point crossover.
 *
 * Reference: Stephens & Waelbroeck (1999)
 */
double  ga_schema_theorem_upper_bound(const Schema *schema,
                                       const Population *pop,
                                       double crossover_rate,
                                       double mutation_rate);

/**
 * Schema fitness estimation: average fitness of all chromosomes
 * in the population that match the given schema.
 */
double  ga_schema_avg_fitness(const Schema *schema, const Population *pop);

/**
 * Count the number of chromosomes in the population that match
 * the schema (instances of H at generation t).
 */
int     ga_schema_count(const Schema *schema, const Population *pop);

/* ====================================================================
 * L4: Building Block Hypothesis (Goldberg, 1989)
 *
 * "Short, low-order, and highly fit schemata are sampled, recombined,
 *  and resampled to form strings of potentially higher fitness."
 *
 * GA works by combining building blocks (short, low-order, above-average
 * fitness schemata) into better solutions through crossover.
 * ==================================================================== */

/**
 * Identify building blocks: schemata of order ≤ max_order and
 * defining_length ≤ max_delta with above-average fitness.
 * Returns number of building blocks found.
 */
int     ga_identify_building_blocks(const Population *pop,
                                     int max_order,
                                     int max_delta,
                                     Schema *blocks,
                                     int max_blocks);

/**
 * Compute the "building block value" of a schema:
 * BBV(H) = f(H) / (o(H) * δ(H)) — higher is better building block
 */
double  ga_building_block_value(const Schema *schema, const Population *pop);

/**
 * Test if a schema is a building block:
 * (1) low order: o(H) ≤ max_order
 * (2) short defining length: δ(H) ≤ max_delta
 * (3) above-average fitness: f(H) > f̄
 */
bool    ga_is_building_block(const Schema *schema, const Population *pop,
                              int max_order, int max_delta);

/* ====================================================================
 * L4: Implicit Parallelism
 *
 * Holland showed that a GA processing N chromosomes implicitly evaluates
 * approximately N³ schemata per generation. This is the "implicit
 * parallelism" or "intrinsic parallelism" of GA.
 * ==================================================================== */

/**
 * Estimate the number of schemata implicitly processed:
 * O(N³) — each of N chromosomes belongs to 2^L schemata,
 * but the GA effectively processes ~N³ of them.
 *
 * Reference: Holland (1975), Goldberg (1989) p.40-42
 */
double  ga_implicit_parallelism(int population_size, int chromosome_length);

/* ====================================================================
 * L4: Deception Analysis
 *
 * A problem is deceptive when low-order schemata guide search away
 * from the global optimum (they have higher fitness than the schemata
 * containing the global optimum).
 * ==================================================================== */

/**
 * Check if a fitness function is deceptive at a given schema level.
 * Returns true if the schema containing the optimum has lower
 * average fitness than its competitors.
 */
bool    ga_is_deceptive(const Schema *opt_schema, const Schema *comp_schema,
                         const Population *pop);

/** Compute deception level: max fitness gap between misleading and true schemata */
double  ga_deception_level(const Schema *true_schema,
                            const Schema **misleading_schemata,
                            int n_misleading, const Population *pop);

/* ====================================================================
 * L4: Price's Equation and Fisher's Fundamental Theorem
 * ==================================================================== */

/**
 * Price's Equation for evolutionary change:
 * Δz̄ = Cov(w_i, z_i) / w̄ + E(w_i * Δz_i) / w̄
 *
 * where z is the trait value and w is fitness.
 * Codifies the dual roles of selection (covariance term) and
 * transmission (expectation term).
 *
 * Reference: Price (1970)
 */
void    ga_price_equation(const double *trait_values,
                           const double *fitness_values,
                           const double *trait_changes,
                           int n,
                           double *selection_effect,
                           double *transmission_effect);

/**
 * Fisher's Fundamental Theorem of Natural Selection:
 * The rate of increase in fitness of any organism at any time
 * is equal to its genetic variance in fitness at that time.
 *
 * Δf̄ ≈ Var_g(f) / f̄
 */
double  ga_fisher_fundamental_theorem(const double *fitness_values,
                                       int n);

/* ====================================================================
 * L8: Vose's Infinite Population Model (Markov Chain GA)
 *
 * Models the GA as a Markov chain over all possible populations.
 * In the infinite population limit, the GA trajectory is governed
 * by a deterministic quadratic dynamical system:
 *   x_i' = (1/σ̄) Σ_j Σ_k x_j x_k M_{i,{j,k}}
 *
 * Reference: Vose "The Simple Genetic Algorithm" (1999)
 * ==================================================================== */

/**
 * Compute the mixing matrix entry M[i, {j,k}]:
 * Probability that crossover of j and k followed by mutation produces i.
 */
double  ga_vose_mixing_entry(int i, int j, int k, int string_length);

/**
 * Compute the heuristic mixing matrix for a population of size N.
 * Approximates the Vose dynamical system trajectory.
 */
void    ga_vose_heuristic_step(double *pop_distribution,
                                int num_strings,
                                double mutation_rate,
                                const double *fitness_values);

/* ====================================================================
 * L4: No Free Lunch Theorem (Wolpert & Macready, 1997)
 * ==================================================================== */

/**
 * No Free Lunch for optimization: averaged over all possible fitness
 * functions, all black-box optimization algorithms perform equally.
 *
 * This function computes the NFL implied performance bound:
 * For any algorithm A, E[performance] = constant.
 *
 * Reference: Wolpert & Macready (1997)
 */
double  ga_nfl_bound(int search_space_size, int sample_size);

#endif /* GA_THEORY_H */
