#include "nk_core.h"
#include "nk_epistasis.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Epistasis Analysis on NK Fitness Landscapes (L5/L6)
 *
 * Quantitative analysis of epistatic interactions.
 * Epistasis measures how gene interactions affect fitness beyond
 * independent (additive) contributions.
 *
 * Three types (Weinreich et al. 2005):
 *   Magnitude epistasis: non-additive fitness effect
 *   Sign epistasis: sign of mutational effect depends on background
 *   Reciprocal sign epistasis: two mutations are deleterious alone
 *     but beneficial together (or vice versa)
 *
 * Walsh/Hadamard analysis (Weinberger 1990):
 *   Decomposes fitness function into orthogonal basis functions
 *   on the Boolean hypercube {0,1}^N.
 * ============================================================================ */

/* ============================================================================
 * Pairwise Epistasis Magnitude (L5: Algorithm)
 *
 * For loci i and j:
 *   e_ij = F(g) - F(g_i) - F(g_j) + F(g_ij)
 * where g_x is g with locus x flipped.
 *
 * This is the standard "genetic interaction" measure from
 * quantitative genetics (Fisher 1918, Phillips 2008).
 *
 * Independent knowledge point: 2-locus epistasis as the
 * deviation from additive fitness expectation.
 * ============================================================================ */

double nk_epistasis_pairwise_magnitude(const NKLandscape *land,
                                        const NKGenotype *g,
                                        int i, int j) {
    if (!land || !g || i < 0 || j < 0 || i >= land->N || j >= land->N)
        return 0.0;
    if (i == j) return 0.0;

    double f_00 = nk_fitness(land, g);

    NKGenotype *gi = nk_genotype_clone(g);
    nk_genotype_flip(gi, i);
    double f_10 = nk_fitness(land, gi);

    NKGenotype *gj = nk_genotype_clone(g);
    nk_genotype_flip(gj, j);
    double f_01 = nk_fitness(land, gj);

    NKGenotype *gij = nk_genotype_clone(g);
    nk_genotype_flip(gij, i);
    nk_genotype_flip(gij, j);
    double f_11 = nk_fitness(land, gij);

    nk_genotype_free(gi);
    nk_genotype_free(gj);
    nk_genotype_free(gij);

    /* Epistasis = deviation from additive expectation */
    double e = f_00 - f_10 - f_01 + f_11;
    return fabs(e);
}

double nk_epistasis_average_magnitude(const NKLandscape *land,
                                       const NKGenotype *g) {
    if (!land || !g) return 0.0;
    double sum = 0.0;
    int n_pairs = 0;
    for (int i = 0; i < land->N; i++) {
        for (int j = i + 1; j < land->N; j++) {
            sum += nk_epistasis_pairwise_magnitude(land, g, i, j);
            n_pairs++;
        }
    }
    if (n_pairs == 0) return 0.0;
    return sum / n_pairs;
}

/* ============================================================================
 * Sign Epistasis Detection (L6: Canonical Problem)
 *
 * Sign epistasis: a mutation that is beneficial on one genetic background
 * becomes deleterious on another.
 *
 * Reciprocal sign epistasis: two mutations are each deleterious alone
 * but jointly beneficial (or vice versa). This creates MULTI-PEAKED
 * landscapes where some peaks are inaccessible by single-step walks.
 *
 * Detection algorithm (Poelwijk et al. 2007, Nature):
 *   Define delta_i(bg) = F(flip(i)) - F(bg) for background bg.
 *   Simple sign epistasis: delta_a(00) and delta_a(01) have opposite signs.
 *   Reciprocal sign epistasis: delta_a(00)*delta_a(11) < 0 AND
 *                               delta_b(00)*delta_b(11) < 0.
 * ============================================================================ */

int nk_epistasis_sign_type(const NKLandscape *land,
                            const NKGenotype *baseline,
                            int locus_a, int locus_b) {
    if (!land || !baseline || locus_a < 0 || locus_b < 0
        || locus_a >= land->N || locus_b >= land->N)
        return 0;
    if (locus_a == locus_b) return 0;

    /* Background 00 (baseline) */
    double f00 = nk_fitness(land, baseline);

    /* Background 01 (b flipped) */
    NKGenotype *g01 = nk_genotype_clone(baseline);
    nk_genotype_flip(g01, locus_b);
    double f01 = nk_fitness(land, g01);

    /* Background 10 (a flipped) */
    NKGenotype *g10 = nk_genotype_clone(baseline);
    nk_genotype_flip(g10, locus_a);
    double f10 = nk_fitness(land, g10);

    /* Background 11 (both flipped) */
    NKGenotype *g11 = nk_genotype_clone(baseline);
    nk_genotype_flip(g11, locus_a);
    nk_genotype_flip(g11, locus_b);
    double f11 = nk_fitness(land, g11);

    nk_genotype_free(g01);
    nk_genotype_free(g10);
    nk_genotype_free(g11);

    /* Effect of a on background 00 */
    double da_00 = f10 - f00;
    /* Effect of a on background 01 (b=1) */
    double da_01 = f11 - f01;
    /* Effect of b on background 00 */
    double db_00 = f01 - f00;
    /* Effect of b on background 10 (a=1) */
    double db_10 = f11 - f10;

    /* Tolerance for zero effects (near-neutral mutations) */
    double eps = 1e-9;

    int sign_a_00 = (da_00 > eps) ? 1 : ((da_00 < -eps) ? -1 : 0);
    int sign_a_01 = (da_01 > eps) ? 1 : ((da_01 < -eps) ? -1 : 0);
    int sign_b_00 = (db_00 > eps) ? 1 : ((db_00 < -eps) ? -1 : 0);
    int sign_b_10 = (db_10 > eps) ? 1 : ((db_10 < -eps) ? -1 : 0);

    /* Simple sign epistasis: a's effect changes sign across b's backgrounds */
    int simple = (sign_a_00 != 0 && sign_a_01 != 0 && sign_a_00 != sign_a_01) ? 1 : 0;
    if (simple) {
        /* Check for reciprocal sign epistasis */
        int reciprocal = (sign_a_00 != 0 && sign_a_01 != 0 &&
                          sign_a_00 != sign_a_01 &&
                          sign_b_00 != 0 && sign_b_10 != 0 &&
                          sign_b_00 != sign_b_10) ? 1 : 0;
        return reciprocal ? 2 : 1;
    }
    return 0;
}

void nk_epistasis_sign_matrix(const NKLandscape *land,
                               const NKGenotype *g,
                               int **sign_matrix) {
    if (!land || !g || !sign_matrix) return;
    for (int i = 0; i < land->N; i++) {
        for (int j = 0; j < land->N; j++) {
            if (i == j) {
                sign_matrix[i][j] = 0;
            } else {
                sign_matrix[i][j] = nk_epistasis_sign_type(land, g, i, j);
            }
        }
    }
}

/* ============================================================================
 * Walsh/Hadamard Decomposition (L5: Algorithm / L6: Canonical Problem)
 *
 * Decomposes the fitness function F: {0,1}^N -> [0,1] into the
 * Walsh/Hadamard basis. This is the Fourier transform on the
 * Boolean hypercube.
 *
 * F(g) = sum_{S subseteq [N]} w_S * chi_S(g)
 * where chi_S(g) = (-1)^{sum_{i in S} g_i} is the parity function.
 *
 * w_S is the Walsh coefficient for interaction subset S.
 * |w_S| measures the contribution of interaction S to fitness.
 * The squared coefficients partition the total fitness variance
 * by interaction order (Weinberger 1990).
 *
 * Independent knowledge point: spectral decomposition of
 * pseudo-Boolean functions, connecting fitness landscapes
 * to discrete Fourier analysis.
 * ============================================================================ */

/* Compute popcount (number of 1 bits) in a 64-bit integer.
 * Independent knowledge point: efficient bit-counting algorithm
 * (parallel prefix sum / SWAR technique). */
static int popcount_u64(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (int)((x * 0x0101010101010101ULL) >> 56);
}

double nk_walsh_coefficient(const NKLandscape *land, uint64_t mask) {
    if (!land) return 0.0;
    if (land->N > 20) return 0.0;  /* Too large for exhaustive enumeration */

    int total = 1 << land->N;
    double sum = 0.0;
    NKGenotype *g = nk_genotype_create(land->N);

    for (int gi = 0; gi < total; gi++) {
        /* Set genotype bits */
        for (int i = 0; i < land->N; i++) {
            nk_genotype_set(g, i, (gi >> i) & 1);
        }

        double f = nk_fitness(land, g);

        /* chi_S(g) = (-1)^{sum_{i in S} g_i}
         * = (-1)^{popcount(gi & mask)}
         * = 1 if popcount even, -1 if odd */
        int parity = popcount_u64((uint64_t)gi & mask) & 1;
        double chi = parity ? -1.0 : 1.0;

        sum += f * chi;
    }

    nk_genotype_free(g);

    /* Normalize: w_S = (1/2^N) * sum_g F(g) * chi_S(g) */
    return sum / (double)total;
}

void nk_walsh_decomposition(const NKLandscape *land, int max_order,
                             double *coeffs) {
    if (!land || !coeffs || land->N > 20) return;

    int total = 1 << land->N;
    /* Zero out all coefficients */
    for (int m = 0; m < total; m++) coeffs[m] = 0.0;

    NKGenotype *g = nk_genotype_create(land->N);

    /* Compute FFT over {0,1}^N using Fast Walsh-Hadamard Transform (FWHT).
     * This is O(N * 2^N) instead of O(4^N) for the naive approach.
     * Independent knowledge point: Fast Walsh-Hadamard Transform,
     * the Boolean analogue of the FFT. */
    double *F = (double*)malloc(total * sizeof(double));
    if (!F) { nk_genotype_free(g); return; }

    /* Populate F[g] with fitness values */
    for (int gi = 0; gi < total; gi++) {
        for (int i = 0; i < land->N; i++) {
            nk_genotype_set(g, i, (gi >> i) & 1);
        }
        F[gi] = nk_fitness(land, g);
    }

    /* In-place FWHT */
    for (int len = 1; len < total; len <<= 1) {
        for (int start = 0; start < total; start += 2 * len) {
            for (int k = 0; k < len; k++) {
                double u = F[start + k];
                double v = F[start + len + k];
                F[start + k] = u + v;
                F[start + len + k] = u - v;
            }
        }
    }

    /* Normalize and filter by order */
    for (int m = 0; m < total; m++) {
        int order = popcount_u64((uint64_t)m);
        if (order <= max_order) {
            coeffs[m] = F[m] / (double)total;
        }
    }

    free(F);
    nk_genotype_free(g);
}

void nk_epistasis_variance_spectrum(const NKLandscape *land,
                                     double *var_order) {
    if (!land || !var_order || land->N > 20) return;

    int total = 1 << land->N;
    double *coeffs = (double*)calloc(total, sizeof(double));
    if (!coeffs) return;

    nk_walsh_decomposition(land, land->N, coeffs);

    /* Zero out variance array */
    for (int k = 0; k <= land->N; k++) var_order[k] = 0.0;

    /* Sum squared coefficients by order */
    for (int m = 0; m < total; m++) {
        int order = popcount_u64((uint64_t)m);
        if (order <= land->N) {
            var_order[order] += coeffs[m] * coeffs[m];
        }
    }

    free(coeffs);
}

/* ============================================================================
 * Reciprocal Sign Epistasis Analysis (L6: Canonical Problem)
 *
 * Reciprocal sign epistasis is the primary cause of multi-peaked
 * fitness landscapes. It creates "forbidden" evolutionary paths
 * where two individually deleterious mutations must occur together
 * to cross a fitness valley.
 *
 * This analysis counts how many reciprocal sign epistasis pairs
 * exist in the landscape, which directly relates to landscape
 * ruggedness and the number of inaccessible peaks.
 * ============================================================================ */

int nk_count_reciprocal_sign_epistasis(const NKLandscape *land,
                                        int n_samples,
                                        uint64_t *rng) {
    if (!land || n_samples <= 0 || !rng) return 0;

    int total_rse = 0;
    int total_checked = 0;

    for (int s = 0; s < n_samples; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, rng);

        for (int i = 0; i < land->N; i++) {
            for (int j = i + 1; j < land->N; j++) {
                if (nk_epistasis_sign_type(land, g, i, j) == 2) {
                    total_rse++;
                }
                total_checked++;
            }
        }

        nk_genotype_free(g);

        /* Limit total checked pairs for performance */
        if (total_checked > 10000) break;
    }

    return total_rse;
}

double nk_optimum_accessibility(const NKLandscape *land,
                                  int n_walks,
                                  uint64_t *rng) {
    if (!land || n_walks <= 0 || !rng) return 0.0;

    /* First find the global optimum by exhaustive or sampling search */
    NKGenotype *global_opt = NULL;
    double best_fitness = -1.0;

    /* Search via multiple adaptive walks with random starts */
    for (int w = 0; w < (n_walks > 200 ? 200 : n_walks); w++) {
        NKGenotype *start = nk_genotype_create(land->N);
        nk_genotype_randomize(start, rng);

        /* Use steepest ascent for better optimum discovery */
        NKWalkResult *wr = nk_walk_steepest_ascent(land, start, 10 * land->N);
        if (wr && wr->reached_optimum) {
            double f = wr->path[wr->steps]->fitness;
            if (f > best_fitness) {
                best_fitness = f;
                if (global_opt) nk_genotype_free(global_opt);
                global_opt = nk_genotype_clone(wr->path[wr->steps]->genotype);
            }
        }
        nk_walk_result_free(wr);
        nk_genotype_free(start);
    }

    if (!global_opt) return 0.0;

    /* Now start adaptive walks from random genotypes and check
     * if they can reach the candidate global optimum */
    int reached = 0;
    for (int w = 0; w < n_walks; w++) {
        NKGenotype *start = nk_genotype_create(land->N);
        nk_genotype_randomize(start, rng);

        NKWalkResult *wr = nk_walk_greedy(land, start, 10 * land->N);
        if (wr && wr->reached_optimum) {
            if (nk_genotype_equal(wr->path[wr->steps]->genotype, global_opt)) {
                reached++;
            }
        }
        nk_walk_result_free(wr);
        nk_genotype_free(start);
    }

    nk_genotype_free(global_opt);
    return (double)reached / (double)n_walks;
}
