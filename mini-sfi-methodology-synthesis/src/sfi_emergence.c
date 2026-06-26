/**
 * @file    sfi_emergence.c
 * @brief   SFI Methodology: Emergence detection algorithms
 *
 * Implements information-theoretic and statistical emergence
 * detectors: mutual information between scales, transfer entropy,
 * power-law fitting (MLE + KS), Hurst exponent (R/S analysis),
 * Gini coefficient, and multi-scale entropy.
 *
 * Knowledge: L4 (power-law SOC detection), L5 (all algorithms)
 * Course: SFI CSSS ? Emergence module; Cambridge Part III ? Complexity
 */

#include "sfi_emergence.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ---- Internal helpers ---- */

static int compare_doubles(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}

/* Freedman-Diaconis bin width: h = 2 * IQR / n^(1/3) */
static uint32_t fd_bins(const double *data, uint64_t n) {
    if (n < 4) return 2;
    double *sorted = (double *)malloc((size_t)n * sizeof(double));
    if (!sorted) return 10;
    memcpy(sorted, data, (size_t)n * sizeof(double));
    qsort(sorted, (size_t)n, sizeof(double), compare_doubles);
    double q1 = sorted[n / 4];
    double q3 = sorted[3 * n / 4];
    double iqr = q3 - q1;
    free(sorted);
    if (iqr < 1e-12) iqr = 1.0;
    double h = 2.0 * iqr / cbrt((double)n);
    if (h < 1e-12) h = 1.0;
    double range = data[n-1] - data[0]; /* approximate, after sorting */
    /* Recompute range */
    range = 0;
    for (uint64_t i = 0; i < n; i++) {
        if (data[i] > range) range = data[i];
        if (-data[i] > range) range = -data[i];
    }
    /* Quick min/max */
    double vmin = data[0], vmax = data[0];
    for (uint64_t i = 1; i < n; i++) {
        if (data[i] < vmin) vmin = data[i];
        if (data[i] > vmax) vmax = data[i];
    }
    range = vmax - vmin;
    if (range < 1e-12) range = 1.0;
    uint32_t bins = (uint32_t)(range / h) + 1;
    if (bins < 2) bins = 2;
    if (bins > 200) bins = 200;
    return bins;
}

/* Assign value to bin index [0, n_bins-1] */
static uint32_t assign_bin(double val, double vmin, double vmax, uint32_t n_bins) {
    if (vmax - vmin < 1e-15) return 0;
    double frac = (val - vmin) / (vmax - vmin);
    if (frac < 0.0) frac = 0.0;
    if (frac >= 1.0) frac = 1.0 - 1e-15;
    uint32_t b = (uint32_t)(frac * (double)n_bins);
    if (b >= n_bins) b = n_bins - 1;
    return b;
}

/* ================================================================
 * L5: Mutual Information I(Micro; Macro)
 * ================================================================ */

double sfi_mutual_information_micro_macro(
    const double *micro_data, uint64_t micro_dim, uint64_t n_samples,
    const double *macro_data, uint64_t macro_dim) {
    if (!micro_data || !macro_data || n_samples < 2 || micro_dim == 0 || macro_dim == 0)
        return 0.0;

    /* Use first dimensions as proxy for full micro/macro state */
    uint32_t n_bins_micro = fd_bins(micro_data, n_samples);
    uint32_t n_bins_macro = fd_bins(macro_data, n_samples);
    if (n_bins_micro < 2) n_bins_micro = 2;
    if (n_bins_macro < 2) n_bins_macro = 2;

    double vmin_micro = micro_data[0], vmax_micro = micro_data[0];
    double vmin_macro = macro_data[0], vmax_macro = macro_data[0];
    for (uint64_t i = 1; i < n_samples; i++) {
        if (micro_data[i * micro_dim] < vmin_micro) vmin_micro = micro_data[i * micro_dim];
        if (micro_data[i * micro_dim] > vmax_micro) vmax_micro = micro_data[i * micro_dim];
        if (macro_data[i * macro_dim] < vmin_macro) vmin_macro = macro_data[i * macro_dim];
        if (macro_data[i * macro_dim] > vmax_macro) vmax_macro = macro_data[i * macro_dim];
    }

    /* 2-D joint histogram */
    uint32_t hist_size = n_bins_micro * n_bins_macro;
    uint32_t *joint = (uint32_t *)calloc((size_t)hist_size, sizeof(uint32_t));
    uint32_t *marg_micro = (uint32_t *)calloc((size_t)n_bins_micro, sizeof(uint32_t));
    uint32_t *marg_macro = (uint32_t *)calloc((size_t)n_bins_macro, sizeof(uint32_t));
    if (!joint || !marg_micro || !marg_macro) {
        free(joint); free(marg_micro); free(marg_macro);
        return 0.0;
    }

    for (uint64_t i = 0; i < n_samples; i++) {
        uint32_t bm = assign_bin(micro_data[i * micro_dim],
                                 vmin_micro, vmax_micro, n_bins_micro);
        uint32_t bM = assign_bin(macro_data[i * macro_dim],
                                 vmin_macro, vmax_macro, n_bins_macro);
        joint[bm * n_bins_macro + bM]++;
        marg_micro[bm]++;
        marg_macro[bM]++;
    }

    /* MI = sum p(u,v) log2 [ p(u,v) / (p(u) * p(v)) ] */
    double mi = 0.0;
    double inv_n = 1.0 / (double)n_samples;
    for (uint32_t u = 0; u < n_bins_micro; u++) {
        double pu = (double)marg_micro[u] * inv_n;
        if (pu < 1e-15) continue;
        for (uint32_t v = 0; v < n_bins_macro; v++) {
            uint32_t cnt = joint[u * n_bins_macro + v];
            if (cnt == 0) continue;
            double puv = (double)cnt * inv_n;
            double pv = (double)marg_macro[v] * inv_n;
            mi += puv * log2(puv / (pu * pv));
        }
    }

    free(joint);
    free(marg_micro);
    free(marg_macro);
    return (mi < 0.0) ? 0.0 : mi;
}

/* ================================================================
 * L5: Transfer Entropy T_{Y?X}
 * ================================================================ */

double sfi_transfer_entropy(
    const double *src, const double *dst,
    uint64_t n_points, uint64_t embed_dim, uint64_t lag) {
    if (!src || !dst || n_points < embed_dim + lag + 2) return 0.0;
    if (embed_dim == 0 || lag == 0) return 0.0;

    /* TE = I(X_t ; Y_{t-lag}^{t-lag-embed+1} | X_{t-1}^{t-embed})
       Compute using 3-D histogram: X_past, Y_past, X_future */
    uint64_t usable = n_points - embed_dim - lag;
    if (usable < embed_dim) return 0.0;

    /* For simplicity, compute TE using a nearest-neighbour Kraskov estimator */
    /* We approximate with a coarse binning approach */

    /* Extract relevant vectors */
    uint32_t n_bins = 5;  /* Coarse for robust estimation */
    uint32_t hist_size = n_bins * n_bins * n_bins;
    uint32_t *joint3 = (uint32_t *)calloc((size_t)hist_size, sizeof(uint32_t));
    uint32_t *marg_xy = (uint32_t *)calloc((size_t)(n_bins * n_bins), sizeof(uint32_t));
    uint32_t *marg_xx = (uint32_t *)calloc((size_t)(n_bins * n_bins), sizeof(uint32_t));
    uint32_t *marg_x  = (uint32_t *)calloc((size_t)n_bins, sizeof(uint32_t));
    if (!joint3 || !marg_xy || !marg_xx || !marg_x) {
        free(joint3); free(marg_xy); free(marg_xx); free(marg_x);
        return 0.0;
    }

    /* Find range for binning */
    double min_all = src[0], max_all = src[0];
    for (uint64_t i = 0; i < n_points; i++) {
        if (src[i] < min_all) min_all = src[i];
        if (src[i] > max_all) max_all = src[i];
        if (dst[i] < min_all) min_all = dst[i];
        if (dst[i] > max_all) max_all = dst[i];
    }

    uint64_t start = embed_dim + lag - 1;
    uint64_t count = 0;
    for (uint64_t t = start; t < n_points; t++) {
        double x_future = dst[t];                        /* X_t */
        double x_past   = dst[t - lag];                  /* X_{t-lag} */
        double y_past   = src[t - lag];                  /* Y_{t-lag} */
        /* Use block-embedding: average over embed_dim past values */
        double x_past_avg = 0.0;
        for (uint64_t k = 1; k <= embed_dim && k <= (t - lag); k++) {
            x_past_avg += dst[t - lag - k];
        }
        x_past_avg /= (double)embed_dim;
        double y_past_avg = 0.0;
        for (uint64_t k = 1; k <= embed_dim && k <= (t - lag); k++) {
            y_past_avg += src[t - lag - k];
        }
        y_past_avg /= (double)embed_dim;

        uint32_t bx = assign_bin(x_future, min_all, max_all, n_bins);
        uint32_t by = assign_bin(x_past_avg, min_all, max_all, n_bins);
        uint32_t bz = assign_bin(y_past_avg, min_all, max_all, n_bins);

        joint3[bx * n_bins * n_bins + by * n_bins + bz]++;
        marg_xy[bx * n_bins + by]++;
        marg_xx[bx * n_bins + bz]++;
        marg_x[bx]++;
        count++;
    }

    if (count < 10) { free(joint3); free(marg_xy); free(marg_xx); free(marg_x); return 0.0; }

    double inv_n = 1.0 / (double)count;
    /* TE = I(X_t ; Y_past | X_past)
          = H(X_t | X_past) - H(X_t | X_past, Y_past) */
    double h_xx = 0.0, h_xxy = 0.0;

    for (uint32_t bx = 0; bx < n_bins; bx++) {
        double px = (double)marg_x[bx] * inv_n;
        for (uint32_t by = 0; by < n_bins; by++) {
            double pxy = (double)marg_xy[bx * n_bins + by] * inv_n;
            if (pxy > 1e-15 && px > 1e-15)
                h_xx -= pxy * log2(pxy / px);

            for (uint32_t bz = 0; bz < n_bins; bz++) {
                double pxyz = (double)joint3[bx * n_bins * n_bins + by * n_bins + bz] * inv_n;
                if (pxyz > 1e-15 && pxy > 1e-15)
                    h_xxy -= pxyz * log2(pxyz / pxy);
            }
        }
    }

    free(joint3); free(marg_xy); free(marg_xx); free(marg_x);
    return (h_xx - h_xxy > 0.0) ? (h_xx - h_xxy) : 0.0;
}

/* ================================================================
 * L5: Power-Law Fitting (MLE + KS statistic)
 * ================================================================ */

int sfi_fit_power_law(const double *data, uint64_t n,
                      double *alpha_out, double *xmin_out,
                      double *ks_stat_out) {
    if (!data || n < 10 || !alpha_out || !xmin_out || !ks_stat_out) return -1;

    /* MLE for continuous power-law: ? = 1 + n / ? ln(x_i / x_min) */
    double *sorted = (double *)malloc((size_t)n * sizeof(double));
    if (!sorted) return -2;
    memcpy(sorted, data, (size_t)n * sizeof(double));
    qsort(sorted, (size_t)n, sizeof(double), compare_doubles);

    /* Try every unique data point as xmin candidate; pick best KS */
    double best_ks = 1.0, best_alpha = 2.0, best_xmin = sorted[n / 10];
    uint64_t best_n_tail = n;

    for (uint64_t i = 0; i < n - 5; i++) {
        double xm = sorted[i];
        if (xm < 1e-12) continue;
        uint64_t n_tail = n - i;
        if (n_tail < 5) break;

        double sum_log = 0.0;
        for (uint64_t j = i; j < n; j++) {
            sum_log += log(sorted[j] / xm);
        }
        if (sum_log < 1e-12) continue;
        double alpha = 1.0 + (double)n_tail / sum_log;
        if (alpha <= 1.0) continue;

        /* KS statistic: max|empirical_CDF - theoretical_CDF| */
        double ks_max = 0.0;
        for (uint64_t j = i; j < n; j++) {
            double emp_cdf = (double)(j - i + 1) / (double)n_tail;
            double theo_cdf = 1.0 - pow(xm / sorted[j], alpha - 1.0);
            double diff = fabs(emp_cdf - theo_cdf);
            if (diff > ks_max) ks_max = diff;
        }
        if (ks_max < best_ks) {
            best_ks = ks_max;
            best_alpha = alpha;
            best_xmin = xm;
            best_n_tail = n_tail;
        }
    }

    free(sorted);
    *alpha_out = best_alpha;
    *xmin_out = best_xmin;
    *ks_stat_out = best_ks;
    return 0;
}

/* ================================================================
 * L5: Hurst Exponent via R/S Analysis
 * ================================================================ */

double sfi_hurst_exponent_rs(const double *series, uint64_t n) {
    if (!series || n < 8) return 0.5;
    double mean = 0.0;
    for (uint64_t i = 0; i < n; i++) mean += series[i];
    mean /= (double)n;

    /* Compute cumulative deviation Y_t = ?_{i=1}^t (x_i - mean) */
    double *Y = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (!Y) return 0.5;
    Y[0] = 0.0;
    for (uint64_t t = 1; t <= n; t++) {
        Y[t] = Y[t - 1] + (series[t - 1] - mean);
    }

    /* Compute standard deviation */
    double variance = 0.0;
    for (uint64_t i = 0; i < n; i++) {
        double d = series[i] - mean;
        variance += d * d;
    }
    variance /= (double)n;
    double S = sqrt(variance);

    /* R = max(Y) - min(Y) */
    double ymin = Y[0], ymax = Y[0];
    for (uint64_t t = 1; t <= n; t++) {
        if (Y[t] < ymin) ymin = Y[t];
        if (Y[t] > ymax) ymax = Y[t];
    }
    double R = ymax - ymin;
    free(Y);

    /* H = log(R/S) / log(n/2) */
    if (S < 1e-15) return 0.5;
    if (n < 2) return 0.5;
    double RS = R / S;
    double H = log(RS) / log((double)n / 2.0);
    if (H < 0.0) H = 0.0;
    if (H > 1.0) H = 1.0;
    return H;
}

/* ================================================================
 * L5: Gini Coefficient
 * ================================================================ */

double sfi_gini_coefficient(const double *values, uint64_t n) {
    if (!values || n < 2) return 0.0;
    double *sorted = (double *)malloc((size_t)n * sizeof(double));
    if (!sorted) return 0.0;
    memcpy(sorted, values, (size_t)n * sizeof(double));
    qsort(sorted, (size_t)n, sizeof(double), compare_doubles);

    double sum = 0.0;
    for (uint64_t i = 0; i < n; i++) sum += sorted[i];
    if (sum < 1e-15) { free(sorted); return 0.0; }

    double sum_ranked = 0.0;
    for (uint64_t i = 0; i < n; i++) {
        sum_ranked += sorted[i] * (double)(i + 1);
    }

    free(sorted);
    double gini = (2.0 * sum_ranked) / ((double)n * sum) - ((double)(n + 1) / (double)n);
    return (gini < 0.0) ? 0.0 : gini;
}

/* ================================================================
 * L5: Multi-Scale Entropy (Costa 2005)
 * ================================================================ */

/* Sample entropy for a 1-D series with tolerance r */
static double sample_entropy(const double *x, uint64_t n, uint32_t m, double r) {
    if (n < (uint64_t)(m + 2)) return 0.0;
    uint64_t A = 0, B = 0;
    double r2 = r * r;
    for (uint64_t i = 0; i < n - m - 1; i++) {
        for (uint64_t j = i + 1; j < n - m - 1; j++) {
            /* Check if distance < r for first m points */
            int match_m = 1;
            for (uint32_t k = 0; k < m && match_m; k++) {
                double diff = x[i + k] - x[j + k];
                if (diff * diff > r2) match_m = 0;
            }
            if (match_m) {
                B++;
                /* Check m+1 distance */
                double diff = x[i + m] - x[j + m];
                if (diff * diff <= r2) A++;
            }
        }
    }
    if (B == 0) return 0.0;
    return -log((double)A / (double)B);
}

double sfi_multiscale_entropy(const double *series, uint64_t n,
                              uint64_t max_scale, double r_tolerance) {
    if (!series || n < max_scale * 4 || max_scale < 1) return 0.0;
    /* Compute sample entropy at each coarse-graining scale */
    double sum_se = 0.0;
    uint64_t valid_scales = 0;
    for (uint64_t scale = 1; scale <= max_scale && scale <= n / 4; scale++) {
        uint64_t n_coarse = n / scale;
        double *coarse = (double *)malloc((size_t)n_coarse * sizeof(double));
        if (!coarse) continue;
        for (uint64_t i = 0; i < n_coarse; i++) {
            double avg = 0.0;
            for (uint64_t j = 0; j < scale; j++) {
                avg += series[i * scale + j];
            }
            coarse[i] = avg / (double)scale;
        }
        double se = sample_entropy(coarse, n_coarse, 2, r_tolerance);
        sum_se += se;
        valid_scales++;
        free(coarse);
    }
    return (valid_scales > 0) ? sum_se / (double)valid_scales : 0.0;
}

/* ================================================================
 * L5: Integrated Emergence Detection Protocol
 * ================================================================ */

void sfi_detect_emergence(const double *micro_data, uint64_t micro_dim,
                          const double *macro_data, uint64_t macro_dim,
                          uint64_t n_samples,
                          sfi_emergence_detector_t *result) {
    if (!result) return;
    memset(result, 0, sizeof(*result));

    if (!micro_data || !macro_data || n_samples < 10) {
        result->detection = SFI_EMERGE_NOMINAL;
        return;
    }

    /* Mutual information */
    result->mutual_info_micro_macro = sfi_mutual_information_micro_macro(
        micro_data, micro_dim, n_samples, macro_data, macro_dim);

    /* Transfer entropy: Micro ? Macro */
    result->transfer_entropy_m2m = sfi_transfer_entropy(
        macro_data, micro_data, n_samples, 2, 1);

    /* Transfer entropy: Macro ? Micro (downward causation test) */
    result->transfer_entropy_macro_down = sfi_transfer_entropy(
        micro_data, macro_data, n_samples, 2, 1);

    /* Hurst exponent on macro time series */
    result->hurst_exponent = sfi_hurst_exponent_rs(macro_data, n_samples);

    /* Power-law test (macro first dimension) */
    double alpha = 0.0, xmin = 0.0, ks = 0.0;
    double *pos_macro = (double *)malloc((size_t)n_samples * sizeof(double));
    if (pos_macro) {
        for (uint64_t i = 0; i < n_samples; i++) {
            pos_macro[i] = fabs(macro_data[i * macro_dim]) + 1e-12;
        }
        sfi_fit_power_law(pos_macro, n_samples, &alpha, &xmin, &ks);
        result->self_organized_critical = (alpha > 1.0 && alpha < 3.5 && ks < 0.1) ? 1 : 0;
        free(pos_macro);
    }

    /* Gini (on macro first dimension) */
    double *macro_dim0 = (double *)malloc((size_t)n_samples * sizeof(double));
    if (macro_dim0) {
        for (uint64_t i = 0; i < n_samples; i++)
            macro_dim0[i] = macro_data[i * macro_dim];
        result->gini_coefficient = sfi_gini_coefficient(macro_dim0, n_samples);
        free(macro_dim0);
    }

    /* Multi-scale entropy */
    result->excess_kurtosis = sfi_multiscale_entropy(macro_data, n_samples, 5, 0.15);

    /* Decision logic: SFI emergence classification */
    double score = 0.0;
    /* High MI > 0.3 bit = coupling between scales */
    if (result->mutual_info_micro_macro > 0.3) score += 0.2;
    /* Positive TE from micro to macro = bottom-up emergence */
    if (result->transfer_entropy_m2m > 0.1) score += 0.15;
    /* Positive TE from macro to micro = downward causation (strong emergence) */
    if (result->transfer_entropy_macro_down > 0.05) score += 0.25;
    /* Hurst > 0.6 = long-range memory = complex */
    if (result->hurst_exponent > 0.6) score += 0.1;
    /* Power-law signature = SOC */
    if (result->self_organized_critical) score += 0.15;
    /* Gini > 0.3 = significant inequality pattern */
    if (result->gini_coefficient > 0.3) score += 0.15;

    result->emergence_strength = (score > 1.0) ? 1.0 : score;

    if (score < 0.2)
        result->detection = SFI_EMERGE_NOMINAL;
    else if (score < 0.6)
        result->detection = SFI_EMERGE_WEAK;
    else
        result->detection = SFI_EMERGE_STRONG;
}
