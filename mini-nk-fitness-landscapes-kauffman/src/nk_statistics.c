#include "nk_core.h"
#include "nk_statistics.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * NK Landscape Statistics (L4: Fundamental Laws / L5: Algorithms)
 *
 * Statistical characterization of NK fitness landscapes.
 * Implements Weinberger (1990) autocorrelation analysis,
 * Kauffman (1993) complexity catastrophe measurement,
 * and contemporary fitness landscape metrics.
 * ============================================================================ */

/* ============================================================================
 * Comprehensive Statistics (L4: Fundamental Law)
 *
 * Computes a full statistical profile of a landscape, combining
 * fitness distribution analysis, ruggedness metrics, and adaptive
 * walk properties into a single summary.
 * ============================================================================ */

NKLandscapeStatistics* nk_statistics_compute(const NKLandscape *land,
                                              int sample_size,
                                              int n_walks,
                                              uint64_t *rng) {
    if (!land || sample_size <= 0 || !rng) return NULL;

    NKLandscapeStatistics *stats = (NKLandscapeStatistics*)calloc(1, sizeof(NKLandscapeStatistics));
    if (!stats) return NULL;
    stats->sample_size = sample_size;

    double *fitnesses = (double*)malloc(sample_size * sizeof(double));
    if (!fitnesses) { free(stats); return NULL; }

    /* --- Fitness distribution --- */
    double sum = 0.0, sum2 = 0.0;
    for (int s = 0; s < sample_size; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, rng);
        double f = nk_fitness(land, g);
        fitnesses[s] = f;
        sum += f;
        sum2 += f * f;
        nk_genotype_free(g);
    }

    stats->mean_fitness = sum / sample_size;
    double variance = (sum2 / sample_size) - (stats->mean_fitness * stats->mean_fitness);
    stats->std_fitness = (variance > 0.0) ? sqrt(variance) : 0.0;

    /* --- Autocorrelation at distance 1 --- */
    stats->autocorrelation = 1.0 - nk_ruggedness_estimate(land, sample_size);
    stats->ruggedness = 1.0 - stats->autocorrelation;

    /* --- Adaptive walks --- */
    int n_opt = 0;
    double sum_walk_len = 0.0;
    double sum_opt_fitness = 0.0;

    for (int w = 0; w < n_walks; w++) {
        NKGenotype *start = nk_genotype_create(land->N);
        nk_genotype_randomize(start, rng);
        NKWalkResult *wr = nk_walk_greedy(land, start, 10 * land->N);
        if (wr && wr->reached_optimum) {
            n_opt++;
            sum_walk_len += wr->steps;
            sum_opt_fitness += wr->path[wr->steps]->fitness;
        }
        nk_walk_result_free(wr);
        nk_genotype_free(start);
    }

    if (n_opt > 0) {
        stats->mean_walk_length = sum_walk_len / n_opt;
        stats->fitness_at_optimum = sum_opt_fitness / n_opt;
    }

    /* --- Local optima estimation --- */
    stats->estimated_local_optima = nk_estimate_local_optima(land, sample_size, rng);

    /* --- Epistasis measure (fraction of non-additive fitness variance) --- */
    double epistasis_sum = 0.0;
    int n_epi_samples = (sample_size < 100) ? sample_size : 100;
    for (int s = 0; s < n_epi_samples; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, rng);
        double base_f = nk_fitness(land, g);
        /* Compute fitness under additive assumption: sum of single-locus effects */
        double additive_f = 0.0;
        for (int i = 0; i < land->N; i++) {
            /* Contribution of locus i in isolation (K=0 equivalent) */
            additive_f += nk_locus_contribution(land, i, g);
        }
        additive_f /= land->N;
        epistasis_sum += fabs(base_f - additive_f);
        nk_genotype_free(g);
    }
    stats->epistasis_measure = epistasis_sum / n_epi_samples;

    free(fitnesses);
    return stats;
}

void nk_statistics_free(NKLandscapeStatistics *stats) {
    free(stats);
}

void nk_statistics_print(const NKLandscapeStatistics *stats) {
    if (!stats) { printf("(null statistics)\n"); return; }
    printf("============================================================\n");
    printf("  NK Fitness Landscape Statistics\n");
    printf("============================================================\n");
    printf("  Samples drawn:          %d\n", stats->sample_size);
    printf("  Mean fitness:           %.6f\n", stats->mean_fitness);
    printf("  Std dev fitness:        %.6f\n", stats->std_fitness);
    printf("  Autocorrelation rho(1): %.6f\n", stats->autocorrelation);
    printf("  Ruggedness:             %.6f\n", stats->ruggedness);
    printf("  Est. local optima:      %d\n", stats->estimated_local_optima);
    printf("  Mean walk length:       %.2f\n", stats->mean_walk_length);
    printf("  Mean optimum fitness:   %.6f\n", stats->fitness_at_optimum);
    printf("  Epistasis measure:      %.6f\n", stats->epistasis_measure);
    printf("============================================================\n");
}

/* ============================================================================
 * Fitness Distribution Analysis (L5: Algorithm)
 * ============================================================================ */

void nk_fitness_histogram(const NKLandscape *land, int samples,
                           int bins, int *hist, uint64_t *rng) {
    if (!land || samples <= 0 || bins <= 0 || !hist || !rng) return;
    for (int b = 0; b < bins; b++) hist[b] = 0;

    for (int s = 0; s < samples; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, rng);
        double f = nk_fitness(land, g);
        int bin = (int)(f * bins);
        if (bin < 0) bin = 0;
        if (bin >= bins) bin = bins - 1;
        hist[bin]++;
        nk_genotype_free(g);
    }
}

double nk_mean_fitness(const NKLandscape *land, int samples,
                        uint64_t *rng) {
    if (!land || samples <= 0 || !rng) return 0.0;
    double sum = 0.0;
    for (int s = 0; s < samples; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, rng);
        sum += nk_fitness(land, g);
        nk_genotype_free(g);
    }
    return sum / samples;
}

double nk_std_fitness(const NKLandscape *land, int samples,
                       uint64_t *rng) {
    if (!land || samples <= 1 || !rng) return 0.0;
    double mean = nk_mean_fitness(land, samples, rng);
    double sum_sq = 0.0;
    for (int s = 0; s < samples; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, rng);
        double f = nk_fitness(land, g);
        double diff = f - mean;
        sum_sq += diff * diff;
        nk_genotype_free(g);
    }
    return sqrt(sum_sq / (samples - 1));
}

/* ============================================================================
 * Autocorrelation Analysis (L4: Fundamental Law)
 *
 * Weinberger (1990) showed that for NK landscapes with random
 * epistatic interactions:
 *   rho(d) = (1 - d/N)^K
 *
 * This analytical result is a FUNDAMENTAL LAW of NK landscapes.
 * It connects epistasis K to the decay of fitness correlations
 * across Hamming distance, providing the mathematical basis for
 * tunable ruggedness.
 * ============================================================================ */

double nk_autocorrelation_at_distance(const NKLandscape *land,
                                       int d, int n_pairs,
                                       uint64_t *rng) {
    if (!land || d < 0 || d > land->N || n_pairs <= 0 || !rng)
        return 0.0;
    if (d == 0) return 1.0;

    double sum_f1 = 0.0, sum_f2 = 0.0;
    double sum_f1f2 = 0.0, sum_f1sq = 0.0, sum_f2sq = 0.0;

    for (int p = 0; p < n_pairs; p++) {
        /* Generate random genotype, then flip d random loci */
        NKGenotype *g1 = nk_genotype_create(land->N);
        nk_genotype_randomize(g1, rng);
        NKGenotype *g2 = nk_genotype_clone(g1);

        /* Flip d distinct random loci */
        for (int flip = 0; flip < d; flip++) {
            int i = (int)(nk_random_uniform(rng) * land->N);
            if (i >= land->N) i = land->N - 1;
            nk_genotype_flip(g2, i);
        }

        double f1 = nk_fitness(land, g1);
        double f2 = nk_fitness(land, g2);

        sum_f1 += f1;
        sum_f2 += f2;
        sum_f1f2 += f1 * f2;
        sum_f1sq += f1 * f1;
        sum_f2sq += f2 * f2;

        nk_genotype_free(g1);
        nk_genotype_free(g2);
    }

    double mean1 = sum_f1 / n_pairs;
    double mean2 = sum_f2 / n_pairs;
    double var1 = (sum_f1sq / n_pairs) - (mean1 * mean1);
    double var2 = (sum_f2sq / n_pairs) - (mean2 * mean2);

    if (var1 <= 0.0 || var2 <= 0.0) return 0.0;

    double cov = (sum_f1f2 / n_pairs) - (mean1 * mean2);
    return cov / sqrt(var1 * var2);
}

void nk_autocrelation_full(const NKLandscape *land, int n_pairs,
                            double *rho_out, uint64_t *rng) {
    if (!land || !rho_out || !rng) return;
    for (int d = 0; d <= land->N; d++) {
        rho_out[d] = nk_autocorrelation_at_distance(land, d, n_pairs, rng);
    }
}

int nk_correlation_length(const NKLandscape *land, int n_pairs,
                           uint64_t *rng) {
    if (!land || !rng) return -1;
    double threshold = 1.0 / 2.718281828459045;  /* 1/e */
    for (int d = 1; d <= land->N; d++) {
        double rho = nk_autocorrelation_at_distance(land, d, n_pairs, rng);
        if (rho < threshold) return d;
    }
    return land->N;
}

/* ============================================================================
 * Local Optima Estimation (L4: Fundamental Law)
 *
 * For small N (<= 20), exhaustive enumeration is feasible.
 * For larger N, we estimate via random sampling and local optimum checks.
 *
 * Theorem (Kauffman 1993): For K = N-1 (fully random landscape),
 *   E[# local optima] = 2^N / (N + 1)
 * This is because for each genotype, the probability it is a local
 * optimum is 1/(N+1) in the fully uncorrelated case.
 * ============================================================================ */

int nk_estimate_local_optima(const NKLandscape *land,
                              int max_samples,
                              uint64_t *rng) {
    if (!land || !rng) return -1;

    /* For small N, exhaustive enumeration */
    if (land->N <= 20 && max_samples >= (1 << land->N)) {
        int count = 0;
        int total = 1 << land->N;
        NKGenotype *g = nk_genotype_create(land->N);
        for (int gi = 0; gi < total; gi++) {
            for (int i = 0; i < land->N; i++) {
                nk_genotype_set(g, i, (gi >> i) & 1);
            }
            if (nk_is_local_optimum(land, g)) count++;
        }
        nk_genotype_free(g);
        return count;
    }

    /* For larger N, estimate via sampling */
    int found = 0;
    int checked = 0;
    for (int s = 0; s < max_samples; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, rng);
        if (nk_is_local_optimum(land, g)) found++;
        checked++;
        nk_genotype_free(g);

        /* Early stopping if we have a reasonable estimate */
        if (checked >= 1000 && found > 0) {
            double ci_half = 1.96 * sqrt((double)found/checked *
                              (1.0 - (double)found/checked) / checked);
            double frac = (double)found / checked;
            if (ci_half < 0.1 * frac) break;  /* 10% relative precision */
        }
    }

    if (checked == 0) return 0;
    /* Extrapolate: fraction * total genotypes */
    double frac = (double)found / checked;
    /* For large N we cannot compute exact 2^N, return per-million estimate */
    return (int)(frac * 1000000.0);
}

/* ============================================================================
 * Complexity Catastrophe (L4: Fundamental Law)
 *
 * Kauffman's complexity catastrophe: as epistasis K increases,
 * the mean fitness of reachable local optima DECREASES.
 * This is because high K creates conflicting constraints:
 * improving one locus tends to disrupt epistatic contributions
 * from other loci.
 *
 * This measures the empirical verification of the catastrophe
 * by creating landscapes with different K values and measuring
 * the mean optimum fitness reached by adaptive walks.
 * ============================================================================ */

void nk_complexity_catastrophe(const NKLandscape *land,
                                int n_walks, int max_K,
                                int *ks_out, double *fit_out,
                                uint64_t *rng) {
    if (!land || !ks_out || !fit_out || !rng || max_K >= land->N) return;

    uint64_t base_seed = land->seed;

    for (int k = 0; k <= max_K; k++) {
        ks_out[k] = k;

        /* Create landscape with this K (same N, random topology) */
        NKLandscape *tmp = nk_landscape_create(land->N, k,
            NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM,
            base_seed + k * 10000);
        if (!tmp) { fit_out[k] = 0.0; continue; }

        double sum_fit = 0.0;
        int n_opt = 0;
        for (int w = 0; w < n_walks; w++) {
            NKGenotype *start = nk_genotype_create(tmp->N);
            nk_genotype_randomize(start, rng);
            NKWalkResult *wr = nk_walk_greedy(tmp, start, 10 * tmp->N);
            if (wr && wr->reached_optimum) {
                sum_fit += wr->path[wr->steps]->fitness;
                n_opt++;
            }
            nk_walk_result_free(wr);
            nk_genotype_free(start);
        }

        fit_out[k] = (n_opt > 0) ? (sum_fit / n_opt) : 0.0;
        nk_landscape_free(tmp);
    }
}

/* ============================================================================
 * Landscape Comparison (L5: Algorithm)
 *
 * Spearman rank correlation between two landscapes.
 * Measures the similarity of fitness orderings on the same genotypes.
 * This is the standard measure for comparing fitness landscape
 * topology (Stadler 1999).
 * ============================================================================ */

double nk_landscape_correlation(const NKLandscape *land_a,
                                 const NKLandscape *land_b,
                                 int n_genotypes,
                                 uint64_t *rng) {
    if (!land_a || !land_b || n_genotypes < 2 || !rng) return 0.0;
    if (land_a->N != land_b->N) return 0.0;

    double *fa = (double*)malloc(n_genotypes * sizeof(double));
    double *fb = (double*)malloc(n_genotypes * sizeof(double));
    double *ranka = (double*)malloc(n_genotypes * sizeof(double));
    double *rankb = (double*)malloc(n_genotypes * sizeof(double));
    if (!fa || !fb || !ranka || !rankb) {
        free(fa); free(fb); free(ranka); free(rankb);
        return 0.0;
    }

    /* Generate random genotypes and evaluate both landscapes */
    for (int i = 0; i < n_genotypes; i++) {
        NKGenotype *g = nk_genotype_create(land_a->N);
        nk_genotype_randomize(g, rng);
        fa[i] = nk_fitness(land_a, g);
        fb[i] = nk_fitness(land_b, g);
        nk_genotype_free(g);
    }

    /* Rank assignment for landscape A */
    {
        /* Create index array, sort by fitness */
        int *idx = (int*)malloc(n_genotypes * sizeof(int));
        for (int i = 0; i < n_genotypes; i++) idx[i] = i;

        /* Simple insertion sort for ranking (n small, typically < 1000) */
        for (int i = 1; i < n_genotypes; i++) {
            int key = idx[i];
            int j = i - 1;
            while (j >= 0 && fa[idx[j]] > fa[key]) {
                idx[j+1] = idx[j];
                j--;
            }
            idx[j+1] = key;
        }
        for (int i = 0; i < n_genotypes; i++) ranka[idx[i]] = (double)(i + 1);
        free(idx);
    }

    /* Rank assignment for landscape B */
    {
        int *idx = (int*)malloc(n_genotypes * sizeof(int));
        for (int i = 0; i < n_genotypes; i++) idx[i] = i;
        for (int i = 1; i < n_genotypes; i++) {
            int key = idx[i];
            int j = i - 1;
            while (j >= 0 && fb[idx[j]] > fb[key]) {
                idx[j+1] = idx[j];
                j--;
            }
            idx[j+1] = key;
        }
        for (int i = 0; i < n_genotypes; i++) rankb[idx[i]] = (double)(i + 1);
        free(idx);
    }

    /* Spearman correlation: rho = 1 - 6*sum(d^2)/(n*(n^2-1)) */
    double sum_d2 = 0.0;
    for (int i = 0; i < n_genotypes; i++) {
        double d = ranka[i] - rankb[i];
        sum_d2 += d * d;
    }

    double rho = 1.0 - (6.0 * sum_d2) / (n_genotypes * (n_genotypes * n_genotypes - 1.0));

    free(fa); free(fb); free(ranka); free(rankb);
    return rho;
}

int nk_shared_local_optima(const NKLandscape *land_a,
                            const NKLandscape *land_b,
                            int n_starts, uint64_t *rng) {
    if (!land_a || !land_b || n_starts <= 0 || !rng) return 0;
    if (land_a->N != land_b->N) return 0;

    int shared = 0;
    for (int s = 0; s < n_starts; s++) {
        NKGenotype *start = nk_genotype_create(land_a->N);
        nk_genotype_randomize(start, rng);

        NKWalkResult *wa = nk_walk_greedy(land_a, start, 10 * land_a->N);
        NKWalkResult *wb = nk_walk_greedy(land_b, start, 10 * land_b->N);

        if (wa && wb && wa->reached_optimum && wb->reached_optimum) {
            NKGenotype *opt_a = wa->path[wa->steps]->genotype;
            NKGenotype *opt_b = wb->path[wb->steps]->genotype;
            if (nk_genotype_equal(opt_a, opt_b)) shared++;
        }

        nk_walk_result_free(wa);
        nk_walk_result_free(wb);
        nk_genotype_free(start);
    }

    return shared;
}
