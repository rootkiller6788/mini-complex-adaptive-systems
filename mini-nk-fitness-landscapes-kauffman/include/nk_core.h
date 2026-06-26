#ifndef NK_CORE_H
#define NK_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * NK Fitness Landscapes (Kauffman, 1993) -- Core Definitions
 *
 * Foundational references:
 *   Kauffman, S.A. (1993). The Origins of Order: Self-Organization and
 *     Selection in Evolution. Oxford University Press.
 *   Kauffman, S.A. & Levin, S. (1987). "Towards a general theory of adaptive
 *     walks on rugged landscapes." J. Theor. Biol., 128(1), 11-45.
 *   Kauffman, S.A. & Weinberger, E.D. (1989). "The NK model of rugged
 *     fitness landscapes and its application to maturation of the immune
 *     response." J. Theor. Biol., 141(2), 211-245.
 *   Weinberger, E.D. (1990). "Correlated and uncorrelated fitness landscapes
 *     and how to tell the difference." Biol. Cybern., 63(5), 325-336.
 *   Stadler, P.F. (1999). "Fitness landscapes." In: Biological Evolution
 *     and Statistical Physics. Springer.
 * ============================================================================ */

/* --- Epistatic Interaction Types (L1: Definitions) --- */

/** How gene contributions depend on other genes.
 *  Kauffman original NK: random neighbor adjacency.
 *  Variants: Altenberg (1995) generalized, Stadler (1999) topological. */
typedef enum {
    NK_INTERACTION_RANDOM = 0,
    NK_INTERACTION_ADJACENT = 1,
    NK_INTERACTION_BLOCK = 2,
    NK_INTERACTION_SCALE_FREE = 3,
    NK_INTERACTION_SMALL_WORLD = 4
} NKInteractionType;

/* --- Walk Strategy Types (L1: Definitions) --- */

/** Adaptive walk strategies on the fitness landscape.
 *  Greedy: Kauffman & Levin (1987) first improvement
 *  NextAscent: systematic scan for improvement
 *  SteepestAscent: evaluate all neighbors, take best
 *  RandomBetter: neutral theory (Kimura, 1983) among improving moves
 *  Metropolis: accept worse with probability exp(-delta/T)
 *  SimulatedAnneal: temperature schedule annealing */
typedef enum {
    NK_WALK_GREEDY = 0,
    NK_WALK_NEXT_ASCENT = 1,
    NK_WALK_STEEPEST_ASCENT = 2,
    NK_WALK_RANDOM_BETTER = 3,
    NK_WALK_METROPOLIS = 4,
    NK_WALK_SIMULATED_ANNEAL = 5
} NKWalkStrategy;

/* --- Landscape Structure Types (L1: Definitions) --- */

/** Fitness contribution distribution shape. */
typedef enum {
    NK_CONTRIB_UNIFORM = 0,
    NK_CONTRIB_NORMAL = 1,
    NK_CONTRIB_EXPONENTIAL = 2,
    NK_CONTRIB_BIMODAL = 3,
    NK_CONTRIB_LOGNORMAL = 4
} NKContribDistribution;

/* --- Genotype (L1: Core Definition) --- */

/** A genotype is a binary string of length N.
 *  Locus i in {0, ..., N-1} takes allele 0 or 1.
 *  Fitness landscape maps {0,1}^N to [0,1].
 *  Storage: bit-packed, one byte holds 8 alleles. */
typedef struct {
    int N;
    uint8_t *alleles;   /* bit-packed, size = (N+7)/8 */
} NKGenotype;

/* --- Epistatic Interaction Matrix (L3: Mathematical Structure) --- */

/** Encodes which K loci interact with each locus i.
 *  neighbors[i][0..K-1] are the loci interacting with i.
 *  K=0: additive model, each locus independent.
 *  K=N-1: fully epistatic, all loci interact.
 *
 *  Theorem (Kauffman 1993): ruggedness increases monotonically
 *  with K, from smooth (K=0) to maximally rugged (K=N-1). */
typedef struct {
    int N;
    int K;
    int **neighbors;    /* neighbors[i][k] = j */
    NKInteractionType itype;
} NKEpistaticMatrix;

/* --- Fitness Contribution Table (L3: Mathematical Structure) --- */

/** For locus i and each of its 2^(K+1) possible allele configurations,
 *  stores a random fitness contribution in [0,1].
 *
 *  Total fitness: F(g) = (1/N) * sum_i f_i(config_i)
 *  Configuration encoding: bit 0 = allele of locus i,
 *  bits 1..K = alleles of its K neighbors.
 *
 *  Physical meaning: each f_i represents the catalytic contribution
 *  of gene i given the regulatory state of its epistatic partners. */
typedef struct {
    int N;
    int K;
    double **f;         /* f[i][config] in [0,1], N x 2^(K+1) */
    NKContribDistribution dist_type;
} NKFitnessTable;

/* --- NK Landscape Configuration (L2: Core Concept) --- */

/** Complete NK fitness landscape instance.
 *  Combines genotype space, epistatic structure, and fitness values.
 *  Reference: Kauffman (1993), Chapter 3. */
typedef struct {
    int N;
    int K;
    NKEpistaticMatrix *epistasis;
    NKFitnessTable *fitness;
    uint64_t seed;
    char *name;
} NKLandscape;

/* --- Walk State (L2: Core Concept) --- */

/** One step in an adaptive walk trajectory. */
typedef struct {
    NKGenotype *genotype;
    double fitness;
    int step;
} NKWalkState;

/* --- Walk Result (L2: Core Concept) --- */

/** Complete adaptive walk record.
 *  Theorem (Kauffman & Levin 1987): expected walk length ~ ln(N)
 *  for K >= 2 on random NK landscapes. */
typedef struct {
    NKWalkState **path;     /* path[0..steps], path[0] = start */
    int steps;
    bool reached_optimum;
    NKWalkStrategy strategy;
} NKWalkResult;

/* --- Landscape Statistics Bundle (L2: Core Concept) --- */

/** Aggregate statistics describing an NK landscape. */
typedef struct {
    double mean_fitness;
    double std_fitness;
    double autocorrelation;
    double ruggedness;
    int estimated_local_optima;
    double mean_walk_length;
    double fitness_at_optimum;
    double epistasis_measure;
    int sample_size;
} NKLandscapeStatistics;

/* --- NKCS Coevolution Pair (L7/L8) --- */

/** Two species coevolve on coupled fitness landscapes.
 *  Each species fitness depends on its own genotype and
 *  C loci of the other species.
 *
 *  Reference: Kauffman & Johnsen (1991). "Coevolution to the edge
 *    of chaos." J. Theor. Biol., 149(4), 467-505. */
typedef struct {
    NKLandscape *species_a;
    NKLandscape *species_b;
    int C;
    int **coupling_a_to_b;
    int **coupling_b_to_a;
    NKFitnessTable *fitness_a;
    NKFitnessTable *fitness_b;
    double coupling_strength;
} NKCSLandscape;

/* --- NK Model Phase Classification (L2: Core Concept) --- */

/** Landscape phase based on K/N ratio.
 *  Ordered: K/N < 0.2, smooth, few local optima
 *  Critical: 0.2 <= K/N <= 0.5, edge of chaos
 *  Chaotic: K/N > 0.5, rugged, many local optima */
typedef enum {
    NK_PHASE_ORDERED = 0,
    NK_PHASE_CRITICAL = 1,
    NK_PHASE_CHAOTIC = 2
} NKPhaseClassification;

/* === Shared Random Number Generators === */

/** Xorshift64* uniform random in [0, 1).
 *  Vigna (2016). ACM Trans. Math. Softw. */
double nk_random_uniform(uint64_t *rng);

/** Box-Muller normal random N(0, 1).
 *  Box & Muller (1958). Ann. Math. Stat. */
double nk_random_normal(uint64_t *rng);

#endif /* NK_CORE_H */
