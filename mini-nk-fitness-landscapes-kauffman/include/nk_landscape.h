#ifndef NK_LANDSCAPE_H
#define NK_LANDSCAPE_H

#include "nk_core.h"

/* ============================================================================
 * NK Landscape Generation and Fitness Evaluation (L3-L5)
 *
 * Implements the core NK model operations:
 *   - Landscape construction with various epistatic topologies
 *   - Fitness evaluation of arbitrary genotypes
 *   - Neighbor enumeration and local optimum detection
 *   - Landscape phase classification
 *
 * Key theorems implemented:
 *   Thm 1 (Kauffman 1993): Fitness landscape ruggedness is tunable via K.
 *   Thm 2: For K=0, landscape has a single global optimum.
 *   Thm 3: For K=N-1, number of local optima ~ 2^N / (N+1).
 *   Thm 4: Mean walk length to optimum scales as O(log N) for K >= 2.
 * ============================================================================ */

/* === Genotype Lifecycle === */

/** Allocate a genotype of length N with all alleles set to 0.
 *  Complexity: O(N). Returns NULL on allocation failure. */
NKGenotype* nk_genotype_create(int N);

/** Deep-copy a genotype. Complexity: O(N). */
NKGenotype* nk_genotype_clone(const NKGenotype *g);

/** Free genotype memory. Safe to call with NULL. */
void nk_genotype_free(NKGenotype *g);

/** Set allele at locus i to value a (0 or 1).
 *  Complexity: O(1). Asserts 0 <= i < N. */
void nk_genotype_set(NKGenotype *g, int i, int a);

/** Get allele at locus i. Returns 0 or 1.
 *  Complexity: O(1). Asserts 0 <= i < N. */
int nk_genotype_get(const NKGenotype *g, int i);

/** Flip the allele at locus i. Complexity: O(1). */
void nk_genotype_flip(NKGenotype *g, int i);

/** Compare two genotypes for equality. Complexity: O(N). */
bool nk_genotype_equal(const NKGenotype *a, const NKGenotype *b);

/** Compute Hamming distance between two genotypes.
 *  Complexity: O(N). Returns number of loci where alleles differ. */
int nk_genotype_hamming(const NKGenotype *a, const NKGenotype *b);

/** Generate a random genotype (uniform over {0,1}^N).
 *  Uses the provided random state for reproducibility.
 *  Complexity: O(N). */
void nk_genotype_randomize(NKGenotype *g, uint64_t *rng_state);

/** Format genotype as a binary string. Caller must free.
 *  Complexity: O(N). */
char* nk_genotype_to_string(const NKGenotype *g);

/** Parse genotype from binary string (e.g., "010110").
 *  Returns NULL if string contains non-binary characters.
 *  Complexity: O(N). */
NKGenotype* nk_genotype_from_string(const char *s);

/* === Epistatic Matrix Construction === */

/** Allocate an N x K epistatic matrix with a given interaction type.
 *  For RANDOM: each locus i has K distinct randomly-chosen neighbors.
 *  For ADJACENT: neighbors are i+1, ..., i+K (mod N) on a ring.
 *  For BLOCK: partitioned into blocks of size K+1.
 *  Complexity: O(N*K). */
NKEpistaticMatrix* nk_epistasis_create(int N, int K, NKInteractionType itype,
                                        uint64_t *rng_state);

/** Free epistatic matrix. */
void nk_epistasis_free(NKEpistaticMatrix *em);

/** Get the configuration index for locus i given a genotype.
 *  The configuration encodes the allele of i (bit 0) and the
 *  alleles of its K neighbors (bits 1..K).
 *  Complexity: O(K). */
int nk_epistasis_config(const NKEpistaticMatrix *em, int i,
                         const NKGenotype *g);

/* === Fitness Table Construction === */

/** Allocate and populate a fitness table with random contributions.
 *  Each f[i][config] is drawn from the specified distribution.
 *  For UNIFORM: U(0,1). For NORMAL: N(0.5, 0.15) clamped to [0,1].
 *  Complexity: O(N * 2^(K+1)). Warning: exponential in K. */
NKFitnessTable* nk_fitness_table_create(int N, int K,
                                         NKContribDistribution dist,
                                         uint64_t *rng_state);

/** Free fitness table. */
void nk_fitness_table_free(NKFitnessTable *ft);

/** Get raw fitness contribution for locus i in configuration cfg.
 *  Complexity: O(1). */
double nk_fitness_table_get(const NKFitnessTable *ft, int i, int cfg);

/* === Landscape Lifecycle === */

/** Create an NK landscape with specified parameters.
 *  N: number of loci (1 <= N <= 64 for practical computation).
 *  K: epistatic degree (0 <= K < N).
 *  Complexity: O(N * 2^(K+1)) -- exponential in K, feasible for K <= 15. */
NKLandscape* nk_landscape_create(int N, int K,
                                  NKInteractionType itype,
                                  NKContribDistribution dist,
                                  uint64_t seed);

/** Free an NK landscape and all its substructures. */
void nk_landscape_free(NKLandscape *land);

/* === Fitness Evaluation === */

/** Evaluate the fitness of a genotype on the landscape.
 *  F(g) = (1/N) * sum_{i=0}^{N-1} f_i(config_i(g)).
 *  Complexity: O(N * K). Returns value in [0, 1]. */
double nk_fitness(const NKLandscape *land, const NKGenotype *g);

/** Compute fitness contribution of a single locus i.
 *  Useful for analyzing epistatic effects and performing
 *  sensitivity analysis on individual gene contributions.
 *  Complexity: O(K). */
double nk_locus_contribution(const NKLandscape *land, int i,
                              const NKGenotype *g);

/** Compute fitness of all N one-bit-flip neighbors of a genotype.
 *  Stores results in fitness_out[0..N-1] where fitness_out[i]
 *  is the fitness of genotype with locus i flipped.
 *  Complexity: O(N^2 * K). */
void nk_neighbor_fitnesses(const NKLandscape *land, const NKGenotype *g,
                            double *fitness_out);

/* === Local Optimum Detection === */

/** Check if a genotype is a local optimum (no 1-bit flip improves fitness).
 *  Complexity: O(N^2 * K). */
bool nk_is_local_optimum(const NKLandscape *land, const NKGenotype *g);

/** Find the best one-bit-flip neighbor of a genotype.
 *  Stores the index of the best flip in best_i (or -1 if none improves).
 *  Complexity: O(N^2 * K). */
void nk_best_neighbor(const NKLandscape *land, const NKGenotype *g,
                       int *best_i, double *best_fitness);

/** Check if the first improving neighbor (systematic scan from locus 0).
 *  Stores the first improving flip in first_i, or -1 if local optimum.
 *  Complexity: O(N * K) average case. */
void nk_first_improving_neighbor(const NKLandscape *land,
                                  const NKGenotype *g,
                                  int *first_i, double *new_fitness);

/* === Landscape Phase Classification === */

/** Classify the landscape into ordered/critical/chaotic phases
 *  based on the K/N ratio.
 *  Reference: Kauffman (1993) Chapter 4. */
NKPhaseClassification nk_landscape_phase(const NKLandscape *land);

/** Estimate the ruggedness of the landscape by computing the
 *  fitness autocorrelation at Hamming distance 1.
 *  rho(1) = Corr[F(g), F(g')] where Hamming(g,g') = 1.
 *  Values near 1: smooth; near 0: rugged.
 *  Complexity: O(N^2 * K * samples). */
double nk_ruggedness_estimate(const NKLandscape *land, int samples);

#endif /* NK_LANDSCAPE_H */
