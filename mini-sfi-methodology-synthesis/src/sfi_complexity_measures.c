/**
 * @file    sfi_complexity_measures.c
 * @brief   SFI Methodology: Complexity measures implementation
 *
 * Implements Lempel-Ziv complexity (LZ76), Kolmogorov complexity
 * approximation via compression, entropy rate, statistical
 * complexity (Crutchfield), Langton lambda, Wolfram CA classification,
 * fractal dimension (box-counting), and largest Lyapunov exponent.
 *
 * Knowledge: L1, L3, L5
 * Course: SFI CSSS ? Complexity Measures module;
 *         Cambridge Part III ? Complexity Theory
 */

#include "sfi_complexity_measures.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ---- L5: Lempel-Ziv Complexity (LZ76) ---- */

double sfi_lempel_ziv_complexity(const uint8_t *data, uint64_t n) {
    if (!data || n < 2) return 0.0;
    /* LZ76: parse binary string, count distinct production components.
       Use a simplified suffix-tree-like incremental parsing. */
    uint64_t *prev_occurrence = (uint64_t *)calloc((size_t)n, sizeof(uint64_t));
    if (!prev_occurrence) return 0.0;
    /* We use a standard parsing: find longest match, add new phrase */
    uint64_t phrases = 1;  /* First character is always a new phrase */
    uint64_t pos = 1;
    while (pos < n) {
        uint64_t best_len = 0;
        /* Search for longest prefix of remaining string that has appeared before */
        for (uint64_t i = 0; i < pos; i++) {
            uint64_t len = 0;
            while (i + len < pos && pos + len < n && data[i + len] == data[pos + len]) {
                len++;
            }
            if (len > best_len) best_len = len;
        }
        /* New phrase = best_match + 1 character (or just 1 if no match) */
        uint64_t advance = (best_len > 0) ? (best_len + 1) : 1;
        if (pos + advance > n) advance = n - pos;
        phrases++;
        pos += advance;
    }

    free(prev_occurrence);
    /* Normalise: C_LZ = c(n) * log2(n) / n */
    double log2n = log2((double)n);
    return (double)phrases * log2n / (double)n;
}

/* ---- L5: Kolmogorov approximation via compression ratio ---- */

double sfi_kolmogorov_approx_gzip(const uint8_t *data, uint64_t n) {
    if (!data || n == 0) return 0.0;
    /* Simple run-length encoding as a stand-in for gzip (no external lib) */
    uint64_t compressed_len = 0;
    uint64_t i = 0;
    while (i < n) {
        uint8_t val = data[i];
        uint64_t run = 1;
        while (i + run < n && data[i + run] == val && run < 255) run++;
        compressed_len += 2;  /* 1 byte value + 1 byte count */
        i += run;
    }
    return (double)compressed_len / (double)(n > 0 ? n : 1);
}

/* ---- L5: Entropy rate via block entropies ---- */

double sfi_entropy_rate(const uint8_t *symbols, uint64_t n,
                        uint32_t alphabet_size, uint32_t max_block) {
    if (!symbols || n < 2 || alphabet_size < 2 || max_block < 1) return 0.0;
    if (max_block > 12) max_block = 12;  /* 4^12 = 16M blocks */

    double h_prev = 0.0;
    for (uint32_t L = 1; L <= max_block; L++) {
        uint64_t n_blocks = n - L + 1;
        if (n_blocks < 10) break;

        /* Count distinct blocks via a hash map simulation (simplified) */
        uint64_t max_blocks = 1;
        for (uint32_t k = 0; k < L && max_blocks < 1000000; k++)
            max_blocks *= (uint64_t)alphabet_size;

        uint32_t *counts = (uint32_t *)calloc(65536, sizeof(uint32_t));
        if (!counts) break;

        uint64_t distinct = 0;
        for (uint64_t i = 0; i < n_blocks; i++) {
            /* Hash block to an index */
            uint64_t hash = 0;
            for (uint32_t k = 0; k < L; k++) {
                hash = hash * (uint64_t)alphabet_size + (uint64_t)symbols[i + k];
            }
            hash = hash % 65536;
            if (counts[hash] == 0) distinct++;
            counts[hash]++;
        }
        free(counts);

        /* H(L) = -(1/L) ? p * log(p), sampled */
        /* Simplified: use distinct/n_blocks as entropy proxy */
        double h_L = (double)distinct / (double)n_blocks;
        if (h_L > 0.0) h_L = log(h_L) * (-1.0 / (double)L);
        /* Entropy rate h_mu = lim H(L)/L, approximate as H(L) - H(L-1) */
        double rate = (L == 1) ? h_L : (h_L * (double)L - h_prev * (double)(L - 1));
        h_prev = h_L;
        if (L == max_block) return (rate > 0.0) ? rate : 0.0;
    }
    return 0.0;
}

/* ---- L5: Statistical Complexity C_mu ---- */

double sfi_statistical_complexity(const double *causal_state_probs,
                                  uint32_t n_states) {
    if (!causal_state_probs || n_states == 0) return 0.0;
    /* C_mu = H[S] = -? p(s) log2 p(s) */
    double entropy = 0.0;
    for (uint32_t s = 0; s < n_states; s++) {
        double p = causal_state_probs[s];
        if (p > 1e-15) entropy -= p * log2(p);
    }
    return entropy;
}

/* ---- L5: Excess Entropy ---- */

double sfi_excess_entropy(const uint8_t *symbols, uint64_t n,
                          uint32_t alphabet_size, uint32_t max_block) {
    if (!symbols || n < 4 || alphabet_size < 2) return 0.0;
    /* E = lim_{L??} [H(L) - L * h_mu]
       Approximate with block entropies: E ? H(L) - L * H(L)/L at large L */
    double h_mu = sfi_entropy_rate(symbols, n, alphabet_size, max_block);
    /* H(max_block) approximation */
    uint64_t n_blocks = n - max_block + 1;
    if (n_blocks < 10) return 0.0;

    /* Count block frequencies */
    uint64_t max_possible = 1;
    for (uint32_t k = 0; k < max_block && max_possible < 100000; k++)
        max_possible *= alphabet_size;
    uint32_t *counts = (uint32_t *)calloc(65536, sizeof(uint32_t));
    if (!counts) return 0.0;

    for (uint64_t i = 0; i < n_blocks; i++) {
        uint64_t hash = 0;
        for (uint32_t k = 0; k < max_block; k++) {
            hash = hash * (uint64_t)alphabet_size + (uint64_t)symbols[i + k];
        }
        hash = hash % 65536;
        if (counts[hash] < UINT32_MAX) counts[hash]++;
    }

    double H_L = 0.0;
    for (uint64_t j = 0; j < 65536; j++) {
        if (counts[j] > 0) {
            double p = (double)counts[j] / (double)n_blocks;
            H_L -= p * log2(p);
        }
    }
    free(counts);
    /* E = H(L) - L * h_mu */
    double E = H_L - (double)max_block * h_mu;
    return (E > 0.0) ? E : 0.0;
}

/* ---- L5: Langton Lambda ---- */

sfi_lambda_profile_t sfi_compute_lambda(const uint32_t *rule_table,
    uint32_t k, uint32_t r, uint32_t quiescent_state) {
    sfi_lambda_profile_t prof;
    memset(&prof, 0, sizeof(prof));

    if (!rule_table || k < 2) return prof;

    /* Total number of neighbourhood configurations: k^{2r+1} */
    uint32_t n_configs = 1;
    for (uint32_t i = 0; i < 2 * r + 1; i++) n_configs *= k;

    uint64_t non_quiescent = 0;
    double entropy = 0.0;
    uint32_t *out_counts = (uint32_t *)calloc((size_t)k, sizeof(uint32_t));
    if (!out_counts) return prof;

    for (uint32_t i = 0; i < n_configs && i < 10000; i++) {
        if (rule_table[i] != quiescent_state) non_quiescent++;
        if (rule_table[i] < k) out_counts[rule_table[i]]++;
    }

    prof.lambda = (double)non_quiescent / (double)n_configs;

    /* Entropy of rule outputs */
    for (uint32_t s = 0; s < k; s++) {
        double p = (double)out_counts[s] / (double)n_configs;
        if (p > 1e-15) entropy -= p * log2(p);
    }
    prof.lambda_entropy = entropy;
    free(out_counts);

    /* Estimate Wolfram class */
    if (prof.lambda < 0.2) prof.ca_class = 1;       /* Class I: frozen */
    else if (prof.lambda < 0.45) prof.ca_class = 2;  /* Class II: periodic */
    else if (prof.lambda < 0.55) prof.ca_class = 4;  /* Class IV: complex */
    else prof.ca_class = 3;                         /* Class III: chaotic */

    return prof;
}

/* ---- L5: Wolfram Class Estimation ---- */

int sfi_wolfram_class_estimate(const uint8_t *spacetime,
    uint32_t width, uint32_t time_steps) {
    if (!spacetime || width == 0 || time_steps < 2) return -1;

    /* Compute density time series */
    double *density = (double *)malloc((size_t)time_steps * sizeof(double));
    if (!density) return -1;

    for (uint32_t t = 0; t < time_steps; t++) {
        uint64_t sum = 0;
        for (uint32_t x = 0; x < width; x++) {
            sum += spacetime[t * width + x];
        }
        density[t] = (double)sum / (double)width;
    }

    /* Variance of density over time */
    double mean = 0.0;
    for (uint32_t t = 0; t < time_steps; t++) mean += density[t];
    mean /= (double)time_steps;
    double variance = 0.0;
    for (uint32_t t = 0; t < time_steps; t++) {
        double d = density[t] - mean;
        variance += d * d;
    }
    variance /= (double)time_steps;

    free(density);

    /* Classification heuristics */
    if (variance < 1e-9) return 1;                     /* Class I: homogeneous */
    else if (variance < 0.01) return 2;                /* Class II: periodic */
    else if (variance < 0.05) return 4;                /* Class IV: complex */
    else return 3;                                      /* Class III: chaotic */
}

/* ---- L5: Box-Counting Dimension ---- */

double sfi_box_counting_dimension(const double *points,
    uint64_t n_points, uint32_t embed_dim, uint32_t n_scales) {
    if (!points || n_points < 10 || embed_dim == 0 || n_scales < 2) return 0.0;

    /* Find bounding box */
    double *mins = (double *)malloc((size_t)embed_dim * sizeof(double));
    double *maxs = (double *)malloc((size_t)embed_dim * sizeof(double));
    if (!mins || !maxs) { free(mins); free(maxs); return 0.0; }
    for (uint32_t d = 0; d < embed_dim; d++) {
        mins[d] = points[d]; maxs[d] = points[d];
    }
    for (uint64_t i = 1; i < n_points; i++) {
        for (uint32_t d = 0; d < embed_dim; d++) {
            double v = points[i * embed_dim + d];
            if (v < mins[d]) mins[d] = v;
            if (v > maxs[d]) maxs[d] = v;
        }
    }

    /* Log-log regression of N(eps) vs eps */
    double sum_log_eps = 0.0, sum_log_N = 0.0;
    double sum_log_eps2 = 0.0, sum_log_eps_log_N = 0.0;
    uint32_t valid = 0;

    for (uint32_t s = 0; s < n_scales; s++) {
        double eps = 1.0 / (double)(1 << (s + 1));  /* Scale factor */
        /* Bin points into boxes of size eps */
        uint64_t n_boxes = 0;
        /* Simple approach: use hash set to count unique boxes */
        uint32_t *box_hash = (uint32_t *)calloc(10000, sizeof(uint32_t));
        if (!box_hash) continue;

        uint64_t unique = 0;
        for (uint64_t i = 0; i < n_points && i < 10000; i++) {
            uint32_t hash = 0;
            for (uint32_t d = 0; d < embed_dim; d++) {
                double range = maxs[d] - mins[d];
                if (range < 1e-15) range = 1.0;
                uint32_t coord = (uint32_t)((points[i * embed_dim + d] - mins[d]) / (range * eps));
                hash = hash * 131 + coord;
            }
            hash = hash % 10000;
            if (box_hash[hash] == 0) n_boxes++;
            box_hash[hash] = 1;
        }
        free(box_hash);

        if (n_boxes > 1 && n_boxes < n_points) {
            double le = log(1.0 / eps);
            double ln = log((double)n_boxes);
            sum_log_eps += le;
            sum_log_N += ln;
            sum_log_eps2 += le * le;
            sum_log_eps_log_N += le * ln;
            valid++;
        }
    }

    free(mins); free(maxs);

    if (valid < 2) return 0.0;
    /* Slope of log(N) vs log(1/eps) = box-counting dimension */
    double denom = (double)valid * sum_log_eps2 - sum_log_eps * sum_log_eps;
    if (fabs(denom) < 1e-15) return 0.0;
    double slope = ((double)valid * sum_log_eps_log_N - sum_log_eps * sum_log_N) / denom;
    return (slope > 0.0) ? slope : 0.0;
}

/* ---- L5: Largest Lyapunov Exponent (Rosenstein algorithm) ---- */

double sfi_largest_lyapunov(const double *time_series, uint64_t n,
                            uint32_t embed_dim, uint32_t lag) {
    if (!time_series || n < embed_dim * lag + 20) return 0.0;
    if (embed_dim == 0 || lag == 0) return 0.0;

    /* Reconstruct phase space via time-delay embedding */
    uint64_t n_vectors = n - (embed_dim - 1) * lag;
    if (n_vectors < 10) return 0.0;

    /* Simplified Rosenstein: compute average divergence rate */
    double sum_div = 0.0;
    uint64_t pairs = 0;

    for (uint64_t i = 0; i < n_vectors - 10; i += 5) {
        double *vec_i = (double *)malloc((size_t)embed_dim * sizeof(double));
        if (!vec_i) continue;
        for (uint32_t d = 0; d < embed_dim; d++) {
            vec_i[d] = time_series[i + d * lag];
        }
        /* Find nearest neighbour (not too close in time) */
        double min_dist = 1e100;
        uint64_t nn_idx = i;
        for (uint64_t j = 0; j < n_vectors; j++) {
            if ((j > i - 10 && j < i + 10)) continue;  /* Theiler window */
            double dist = 0.0;
            for (uint32_t d = 0; d < embed_dim; d++) {
                double diff = time_series[j + d * lag] - vec_i[d];
                dist += diff * diff;
            }
            dist = sqrt(dist);
            if (dist < min_dist && dist > 1e-15) {
                min_dist = dist;
                nn_idx = j;
            }
        }
        /* Track divergence after delta_t steps */
        uint32_t delta_t = 5;
        if (i + delta_t < n_vectors && nn_idx + delta_t < n_vectors) {
            double div_dist = 0.0;
            for (uint32_t d = 0; d < embed_dim; d++) {
                double diff = time_series[i + delta_t + d * lag]
                            - time_series[nn_idx + delta_t + d * lag];
                div_dist += diff * diff;
            }
            div_dist = sqrt(div_dist);
            if (min_dist > 1e-15 && div_dist > 1e-15) {
                sum_div += log(div_dist / min_dist);
                pairs++;
            }
        }
        free(vec_i);
    }

    if (pairs == 0) return 0.0;
    /* Lambda = <log(d_div/d_initial)> / delta_t */
    return sum_div / ((double)pairs * 5.0);
}

/* ---- L5: Integrated Complexity Profile ---- */

void sfi_complexity_profile_compute(
    const uint8_t *symbolic_data, uint64_t sym_len,
    const double *continuous_data, uint64_t cont_len,
    sfi_complexity_profile_t *profile) {
    if (!profile) return;
    memset(profile, 0, sizeof(*profile));

    if (symbolic_data && sym_len > 0) {
        profile->lempel_ziv = sfi_lempel_ziv_complexity(symbolic_data, sym_len);
        profile->kolmogorov_approx = sfi_kolmogorov_approx_gzip(symbolic_data, sym_len);
        profile->entropy_rate = sfi_entropy_rate(symbolic_data, sym_len, 4, 6);
        profile->excess_entropy = sfi_excess_entropy(symbolic_data, sym_len, 4, 6);
    }

    if (continuous_data && cont_len > 20) {
        profile->lyapunov_spectrum_max = sfi_largest_lyapunov(
            continuous_data, cont_len, 3, 5);
        profile->fractal_dimension = sfi_box_counting_dimension(
            continuous_data, cont_len < 2000 ? cont_len : 2000, 3, 6);
    }
}
