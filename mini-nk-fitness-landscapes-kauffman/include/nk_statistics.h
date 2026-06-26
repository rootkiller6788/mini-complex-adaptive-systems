#ifndef NK_STATISTICS_H
#define NK_STATISTICS_H

#include "nk_core.h"

/* ============================================================================
 * Landscape Statistics and Analysis (L4: Laws / L5: Algorithms)
 *
 * Statistical characterization of NK fitness landscapes.
 * Provides tools to verify Kauffman theoretical predictions
 * about complexity catastrophe, ruggedness scaling, and local optima.
 *
 * Key references:
 *   Weinberger (1990): autocorrelation function on NK landscapes
 *   Stadler (1999): fitness landscape statistics and measures
 *   Kauffman (1993): complexity catastrophe, edge of chaos
 *   Malan & Engelbrecht (2013): survey of fitness landscape measures
 * ============================================================================ */

/* === Core Statistics === */

/** Compute comprehensive statistics for an NK landscape by sampling.
 *  Draws sample_size random genotypes and collects fitness distribution,
 *  ruggedness metrics, and adaptive walk properties.
 *  Complexity: O(sample_size * (N*K + walk_cost)). */
NKLandscapeStatistics* nk_statistics_compute(const NKLandscape *land,
                                              int sample_size,
                                              int n_walks,
                                              uint64_t *rng_state);

/** Free landscape statistics. */
void nk_statistics_free(NKLandscapeStatistics *stats);

/** Print a formatted statistics report to stdout. */
void nk_statistics_print(const NKLandscapeStatistics *stats);

/* === Fitness Distribution Analysis === */

/** Compute the fitness distribution histogram from a random sample.
 *  bins: number of histogram bins.
 *  hist[0..bins-1] filled with counts.
 *  Complexity: O(samples * N * K). */
void nk_fitness_histogram(const NKLandscape *land, int samples,
                           int bins, int *hist, uint64_t *rng_state);

/** Compute the mean fitness over a random sample.
 *  Complexity: O(samples * N * K). */
double nk_mean_fitness(const NKLandscape *land, int samples,
                        uint64_t *rng_state);

/** Compute fitness standard deviation over a random sample.
 *  Complexity: O(samples * N * K). */
double nk_std_fitness(const NKLandscape *land, int samples,
                       uint64_t *rng_state);

/* === Ruggedness Metrics === */

/** Compute the autocorrelation function rho(d) of the fitness landscape.
 *  rho(d) = E[F(g) * F(g')] where Hamming(g,g') = d.
 *  Evaluated at a specific distance d by sampling random genotype pairs.
 *
 *  Weinberger (1990): rho(d) approx (1 - d/N)^K for NK landscapes.
 *  This function empirically measures rho to test that prediction.
 *  Complexity: O(n_pairs * N * K). */
double nk_autocorrelation_at_distance(const NKLandscape *land,
                                       int d, int n_pairs,
                                       uint64_t *rng_state);

/** Compute the full autocorrelation function rho(d) for d = 0, 1, ..., N.
 *  Stores results in rho_out[0..N].
 *  Complexity: O(n_pairs * N * K * (N+1)). */
void nk_autocrelation_full(const NKLandscape *land, int n_pairs,
                            double *rho_out, uint64_t *rng_state);

/** Compute the correlation length of the landscape.
 *  Defined as the Hamming distance at which rho(d) drops to 1/e.
 *  Longer correlation length = smoother landscape.
 *  Complexity: O(n_pairs * N^2 * K). */
int nk_correlation_length(const NKLandscape *land, int n_pairs,
                           uint64_t *rng_state);

/** Estimate the number of local optima by exhaustive enumeration
 *  (only feasible for small N <= 20).
 *  For larger N, estimates via sampling.
 *  Complexity: O(2^N * N * K) for exhaustive, O(samples * N^2 * K) for sampling. */
int nk_estimate_local_optima(const NKLandscape *land,
                              int max_samples,
                              uint64_t *rng_state);

/** Complexity catastrophe verification.
 *  For a given N, computes the mean fitness of local optima reached
 *  by adaptive walks for various K values.
 *  Kauffman predicts: mean optimum fitness decreases as K increases.
 *  Stores K values in ks_out and fitness values in fit_out.
 *  Complexity: O(n_walks * N^2 * K_max). */
void nk_complexity_catastrophe(const NKLandscape *land,
                                int n_walks, int max_K,
                                int *ks_out, double *fit_out,
                                uint64_t *rng_state);

/* === Landscape Comparison === */

/** Compute the fitness rank correlation (Spearman rho) between
 *  two landscapes evaluated on the same set of genotypes.
 *  Measures how similar the fitness orderings are.
 *  Near 1: similar; near 0: unrelated; near -1: inversely related.
 *  Complexity: O(n_genotypes * (N*K + log n)). */
double nk_landscape_correlation(const NKLandscape *land_a,
                                 const NKLandscape *land_b,
                                 int n_genotypes,
                                 uint64_t *rng_state);

/** Compute the number of shared local optima between two landscapes
 *  of the same dimension. Measures topological similarity.
 *  Complexity: O(n_starts * N^2 * K). */
int nk_shared_local_optima(const NKLandscape *land_a,
                            const NKLandscape *land_b,
                            int n_starts, uint64_t *rng_state);

#endif /* NK_STATISTICS_H */
