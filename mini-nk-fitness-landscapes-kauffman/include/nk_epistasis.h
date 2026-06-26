#ifndef NK_EPISTASIS_H
#define NK_EPISTASIS_H

#include "nk_core.h"

/* ============================================================================
 * Epistasis Analysis (L5: Algorithms / L6: Canonical Problems)
 *
 * Quantitative analysis of epistatic interactions in fitness landscapes.
 * Epistasis measures how gene interactions affect fitness beyond
 * independent (additive) contributions.
 *
 * Types of epistasis (after Weinreich et al. 2005):
 *   Magnitude epistasis: |F(AB) - F(aB) - F(Ab) + F(ab)| > 0
 *   Sign epistasis: beneficial mutation becomes deleterious on another background
 *   Reciprocal sign epistasis: two mutations are each neutral or deleterious alone
 *     but beneficial together (or vice versa).
 *
 * Key references:
 *   Weinreich, D.M. et al. (2005). "Perspective: Sign epistasis and
 *     genetic constraint on evolutionary trajectories." Evolution, 59(6).
 *   Poelwijk, F.J. et al. (2007). "Empirical fitness landscapes reveal
 *     accessible evolutionary paths." Nature, 445(7126).
 *   Weinberger, E.D. (1990). Walsh/Fourier analysis of NK landscapes.
 * ============================================================================ */

/* === Epistasis Measurement === */

/** Compute the magnitude of epistatic interactions at a pair of loci.
 *  For loci i and j: e_ij = F(g) - F(g_i) - F(g_j) + F(g_ij)
 *  where g_x means genotype g with locus x flipped.
 *  Returns the absolute epistatic magnitude.
 *  Complexity: O(N*K). */
double nk_epistasis_pairwise_magnitude(const NKLandscape *land,
                                        const NKGenotype *g,
                                        int i, int j);

/** Compute the average epistatic magnitude across all pairs of loci
 *  for a given genotype. Normalized by landscape std deviation.
 *  Complexity: O(N^3 * K). */
double nk_epistasis_average_magnitude(const NKLandscape *land,
                                       const NKGenotype *g);

/* === Sign Epistasis Detection === */

/** Detect sign epistasis between two beneficial mutations.
 *  Checks if a mutation that is beneficial on one background
 *  becomes deleterious on another.
 *  Returns: 0 = no sign epistasis, 1 = simple sign epistasis,
 *  2 = reciprocal sign epistasis.
 *  Complexity: O(N*K). */
int nk_epistasis_sign_type(const NKLandscape *land,
                            const NKGenotype *baseline,
                            int locus_a, int locus_b);

/** Scan a genotype for all pairwise sign epistasis relationships.
 *  Fills sign_matrix[i][j] with the sign epistasis type (0,1,2).
 *  sign_matrix must be N x N, caller-allocated.
 *  Complexity: O(N^3 * K). */
void nk_epistasis_sign_matrix(const NKLandscape *land,
                               const NKGenotype *g,
                               int **sign_matrix);

/* === Walsh/Hadamard Decomposition === */

/** Compute the Walsh coefficient for a given interaction subset.
 *  The subset is encoded as a bitmask of interacting loci.
 *  E.g., mask = (1<<i) | (1<<j) means the i-j interaction coefficient.
 *  Requires exhaustive enumeration, so N must be <= 20.
 *  Complexity: O(2^N * N). */
double nk_walsh_coefficient(const NKLandscape *land, uint64_t mask);

/** Compute all Walsh coefficients up to a given interaction order.
 *  Fills coeffs array where coeffs[mask] = walsh coefficient for subset mask.
 *  coeffs must be size 2^N. Only coefficients with popcount(mask) <= max_order
 *  are guaranteed to be computed.
 *  Complexity: O(2^N * N). */
void nk_walsh_decomposition(const NKLandscape *land, int max_order,
                             double *coeffs);

/** Compute the epistasis variance spectrum.
 *  Partition of total fitness variance by interaction order.
 *  var_order[k] = sum of squared Walsh coefficients for subsets of size k.
 *  For NK landscapes, the variance spectrum peaks around order K+1.
 *  Complexity: O(2^N * N). */
void nk_epistasis_variance_spectrum(const NKLandscape *land,
                                     double *var_order);

/* === Reciprocal Sign Epistasis Analysis === */

/** Count the number of reciprocal sign epistasis pairs in the landscape.
 *  Reciprocal sign epistasis creates ruggedness by making some peaks
 *  inaccessible via single-mutation adaptive walks.
 *  Complexity: O(n_samples * N^3 * K). */
int nk_count_reciprocal_sign_epistasis(const NKLandscape *land,
                                        int n_samples,
                                        uint64_t *rng_state);

/** Compute the accessibility of the global optimum.
 *  Fraction of adaptive walk starting points that can reach
 *  the global optimum via beneficial single mutations.
 *  Lower accessibility = more reciprocal sign epistasis.
 *  Complexity: O(n_walks * N^2 * K). */
double nk_optimum_accessibility(const NKLandscape *land,
                                  int n_walks,
                                  uint64_t *rng_state);

#endif /* NK_EPISTASIS_H */
