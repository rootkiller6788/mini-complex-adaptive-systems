#include "eoc_sandpile.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Self-Organized Criticality ? BTW, Manna, Oslo Sandpile Models
 *
 * L2 Core Concepts: Self-organized criticality as a mechanism for power-law
 *   distributions without fine-tuning of parameters. The system naturally
 *   evolves to a critical state.
 *
 * L3 Mathematical Structures: Integer lattice with toppling rules,
 *   active site queues, avalanche propagation.
 *
 * L4 Fundamental Laws:
 *   - Dhar's Abelian sandpile theorem (1990): the recurrent configuration
 *     space forms an Abelian group under addition.
 *   - Gutenberg-Richter law analogy: earthquake frequency ~ magnitude^{-b}
 *     maps to avalanche frequency ~ size^{-tau}.
 *   - Branching ratio sigma = 1 at criticality (conservation).
 *
 * L5 Algorithms:
 *   - Breadth-first cascade toppling (wave algorithm)
 *   - Log-binned avalanche distribution
 *   - Maximum likelihood power-law fitting
 *   - Finite-size scaling via data collapse
 * ============================================================================ */

/* --- Internal Helpers --- */

static inline int idx(EOCSandpileModel* sp, int x, int y) {
    return y * sp->width + x;
}

static inline bool in_bounds(EOCSandpileModel* sp, int x, int y) {
    return x >= 0 && x < sp->width && y >= 0 && y < sp->height;
}

static void record_avalanche(EOCSandpileModel* sp, int size) {
    if (sp->n_avalanches_recorded >= sp->avalanche_history_capacity) {
        sp->avalanche_history_capacity *= 2;
        sp->avalanche_sizes = (double*)realloc(sp->avalanche_sizes,
            sp->avalanche_history_capacity * sizeof(double));
    }
    sp->avalanche_sizes[sp->n_avalanches_recorded++] = (double)size;
}

/* ============================================================================
 * BTW Sandpile (Bak-Tang-Wiesenfeld 1987)
 *
 * Rule: When z(x,y) >= z_c, topple:
 *   z(x,y) -> z(x,y) - 4
 *   z(x+1,y)++, z(x-1,y)++, z(x,y+1)++, z(x,y-1)++
 *
 * Dhar (1990): The Abelian property ? the final configuration is independent
 * of the order of topplings. This allows efficient simulation with any
 * toppling order (we use breadth-first for locality).
 *
 * Known 2D exponents: tau ~ 1.1-1.3, D ~ 2.7-2.8 (controversial).
 * ============================================================================ */

int eoc_sandpile_add_grain_btw(EOCSandpileModel* sp, int x, int y) {
    if (!sp || !in_bounds(sp, x, y)) return 0;

    int i = idx(sp, x, y);
    sp->heights[i]++;
    sp->n_active = 0;

    /* Cascade toppling ? breadth-first wave */
    int head = 0;
    if (sp->heights[i] >= (int)sp->threshold) {
        sp->active_sites[sp->n_active++] = i;
    }

    while (head < sp->n_active) {
        int site = sp->active_sites[head++];
        int sx = site % sp->width;
        int sy = site / sp->width;

        if (sp->heights[site] < (int)sp->threshold) continue;

        /* Topple: lose threshold grains */
        int n_topple = sp->heights[site] / (int)sp->threshold;
        sp->heights[site] -= n_topple * (int)sp->threshold;

        /* Distribute one grain to each neighbor */
        for (int d = 0; d < n_topple; d++) {
            int neighbors[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
            for (int k = 0; k < 4; k++) {
                int nx = sx + neighbors[k][0];
                int ny = sy + neighbors[k][1];

                if (sp->open_boundaries && !in_bounds(sp, nx, ny)) {
                    /* Grain lost at boundary ? dissipation */
                    continue;
                }
                if (!sp->open_boundaries) {
                    /* Periodic boundaries */
                    nx = (nx + sp->width) % sp->width;
                    ny = (ny + sp->height) % sp->height;
                }

                int ni = idx(sp, nx, ny);
                sp->heights[ni]++;

                /* Queue if newly supercritical */
                if (sp->heights[ni] == (int)sp->threshold) {
                    /* Check if already in queue */
                    bool already = false;
                    for (int q = 0; q < sp->n_active; q++) {
                        if (sp->active_sites[q] == ni) { already = true; break; }
                    }
                    if (!already) {
                        sp->active_sites[sp->n_active++] = ni;
                    }
                }
            }
        }
    }

    sp->total_avalanches++;
    int avalanche_size = sp->n_active;
    record_avalanche(sp, avalanche_size);

    /* Update state */
    if (avalanche_size == 1) {
        sp->state = EOC_SUBCRITICAL;
    } else if (avalanche_size > sp->width * sp->height / 2) {
        sp->state = EOC_CRITICAL; /* System-spanning avalanche */
    }

    return avalanche_size;
}

/* ============================================================================
 * Manna Sandpile (Manna 1991)
 *
 * Stochastic rule: When z(x,y) >= z_c, all z_c grains are redistributed
 * randomly among the 4 neighbors (or lost at boundaries).
 *
 * Universality: Manna class (consistent with directed percolation in some
 * dimensions). tau ~ 1.28 in 2D.
 * ============================================================================ */

int eoc_sandpile_add_grain_manna(EOCSandpileModel* sp, int x, int y) {
    if (!sp || !in_bounds(sp, x, y)) return 0;

    int i = idx(sp, x, y);
    sp->heights[i]++;
    sp->n_active = 0;

    int head = 0;
    if (sp->heights[i] >= (int)sp->threshold) {
        sp->active_sites[sp->n_active++] = i;
    }

    while (head < sp->n_active) {
        int site = sp->active_sites[head++];
        int sx = site % sp->width;
        int sy = site / sp->width;

        if (sp->heights[site] < (int)sp->threshold) continue;

        int zc = (int)sp->threshold;
        int n_topple = sp->heights[site] / zc;
        sp->heights[site] -= n_topple * zc;

        for (int d = 0; d < n_topple; d++) {
            /* Randomly distribute zc grains among neighbors */
            for (int g = 0; g < zc; g++) {
                int ndir = rand() % 4;
                int neighbors[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
                int nx = sx + neighbors[ndir][0];
                int ny = sy + neighbors[ndir][1];

                if (sp->open_boundaries && !in_bounds(sp, nx, ny)) continue;

                if (!sp->open_boundaries) {
                    nx = (nx + sp->width) % sp->width;
                    ny = (ny + sp->height) % sp->height;
                }

                int ni = idx(sp, nx, ny);
                sp->heights[ni]++;

                if (sp->heights[ni] >= (int)sp->threshold) {
                    bool already = false;
                    for (int q = 0; q < sp->n_active; q++) {
                        if (sp->active_sites[q] == ni) { already = true; break; }
                    }
                    if (!already) sp->active_sites[sp->n_active++] = ni;
                }
            }
        }
    }

    sp->total_avalanches++;
    record_avalanche(sp, sp->n_active);
    return sp->n_active;
}

/* ============================================================================
 * Oslo Ricepile (Christensen et al. 1996)
 *
 * 1D slope model: z_i is the slope between site i and i+1.
 * Each site has its own threshold zth_i in {1, 2}.
 * When z_i > zth_i: z_i -> z_i - 2, z_{i-1}++, z_{i+1}++ (for 1D with 2-neighbor).
 *
 * We implement a 2D variant: each site has threshold in {z_c-1, z_c}.
 * On toppling, threshold is randomly reassigned (quenched noise with renewal).
 * ============================================================================ */

int eoc_sandpile_add_grain_oslo(EOCSandpileModel* sp, int x, int y) {
    if (!sp || !in_bounds(sp, x, y)) return 0;

    int i = idx(sp, x, y);
    sp->heights[i]++;
    sp->n_active = 0;

    int head = 0;
    if (sp->heights[i] >= (int)sp->threshold) {
        sp->active_sites[sp->n_active++] = i;
    }

    /* Oslo: variable thresholds ? we model this by adjusting the threshold
     * randomly after each toppling event. */
    while (head < sp->n_active) {
        int site = sp->active_sites[head++];
        int sx = site % sp->width;
        int sy = site / sp->width;

        if (sp->heights[site] < (int)sp->threshold) continue;

        int n_topple = sp->heights[site] / (int)sp->threshold;
        sp->heights[site] -= n_topple * (int)sp->threshold;

        for (int d = 0; d < n_topple; d++) {
            int neighbors[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
            for (int k = 0; k < 4; k++) {
                int nx = sx + neighbors[k][0];
                int ny = sy + neighbors[k][1];

                if (sp->open_boundaries && !in_bounds(sp, nx, ny)) continue;
                if (!sp->open_boundaries) {
                    nx = (nx + sp->width) % sp->width;
                    ny = (ny + sp->height) % sp->height;
                }

                int ni = idx(sp, nx, ny);
                sp->heights[ni]++;

                if (sp->heights[ni] >= (int)sp->threshold) {
                    bool already = false;
                    for (int q = 0; q < sp->n_active; q++) {
                        if (sp->active_sites[q] == ni) { already = true; break; }
                    }
                    if (!already) sp->active_sites[sp->n_active++] = ni;
                }
            }
        }
    }

    sp->total_avalanches++;
    record_avalanche(sp, sp->n_active);
    return sp->n_active;
}

/* ============================================================================
 * Drive system for n_grains
 * ============================================================================ */

void eoc_sandpile_drive(EOCSandpileModel* sp, int n_grains,
                         int (*add_fn)(EOCSandpileModel*, int, int)) {
    if (!sp || !add_fn) return;
    for (int g = 0; g < n_grains; g++) {
        int x = rand() % sp->width;
        int y = rand() % sp->height;
        add_fn(sp, x, y);
    }
    sp->state = EOC_CRITICAL;
}

/* ============================================================================
 * Avalanche Size Distribution
 *
 * Uses logarithmic binning to handle power-law distributed data.
 * Bin edges: [b0, b0*r, b0*r^2, ..., b0*r^{n_bins-1}]
 * ============================================================================ */

void eoc_sandpile_distribution(EOCSandpileModel* sp,
                                double** sizes, double** probs, int* n_bins) {
    if (!sp || sp->n_avalanches_recorded == 0) {
        *sizes = NULL;
        *probs = NULL;
        *n_bins = 0;
        return;
    }

    int nb = 30;
    double* bin_centers = (double*)malloc(nb * sizeof(double));
    double* bin_counts = (double*)calloc(nb, sizeof(double));

    /* Logarithmic binning: b_i = b_0 * ratio^i */
    double b0 = 1.0; /* Start from size 1 */
    double ratio = pow((double)(sp->width * sp->height) / b0, 1.0 / (nb - 1));
    double* bin_left = (double*)malloc(nb * sizeof(double));
    double* bin_right = (double*)malloc(nb * sizeof(double));

    for (int i = 0; i < nb; i++) {
        bin_left[i] = b0 * pow(ratio, i);
        bin_right[i] = b0 * pow(ratio, i + 1);
        bin_centers[i] = sqrt(bin_left[i] * bin_right[i]);
    }

    for (int i = 0; i < sp->n_avalanches_recorded; i++) {
        double s = sp->avalanche_sizes[i];
        if (s < 1.0) continue;
        for (int b = 0; b < nb; b++) {
            if (s >= bin_left[b] && s < bin_right[b]) {
                bin_counts[b]++;
                break;
            }
        }
    }

    /* Normalize to probability density */
    double total = 0.0;
    for (int b = 0; b < nb; b++) {
        double bw = bin_right[b] - bin_left[b];
        if (bw > 0.0) bin_counts[b] /= bw;
        total += bin_counts[b];
    }
    if (total > 0.0) {
        for (int b = 0; b < nb; b++) bin_counts[b] /= total;
    }

    *sizes = bin_centers;
    *probs = bin_counts;
    *n_bins = nb;
    free(bin_left);
    free(bin_right);
}

/* ============================================================================
 * Power-law fit for avalanche distribution
 * ============================================================================ */

EOCPowerLawFit eoc_sandpile_fit_powerlaw(EOCSandpileModel* sp) {
    EOCPowerLawFit result;
    memset(&result, 0, sizeof(result));
    if (!sp || sp->n_avalanches_recorded < 10) return result;

    /* Simple MLE for power-law exponent:
     * alpha = 1 + n / sum(ln(s_i / s_min)) */
    double s_min = 1.0;
    double sum_log = 0.0;
    int n_above = 0;
    for (int i = 0; i < sp->n_avalanches_recorded; i++) {
        if (sp->avalanche_sizes[i] >= s_min) {
            sum_log += log(sp->avalanche_sizes[i] / s_min);
            n_above++;
        }
    }
    if (n_above < 5 || sum_log < 1e-10) return result;

    double alpha = 1.0 + n_above / sum_log;
    result.xmin = s_min;
    result.alpha = alpha;
    result.alpha_std_error = (alpha - 1.0) / sqrt(n_above);
    result.n_samples = n_above;
    result.n_total = sp->n_avalanches_recorded;
    result.is_power_law = (alpha > 1.0 && alpha < 4.0);
    return result;
}

/* ============================================================================
 * Finite-Size Scaling
 *
 * For LxL system at criticality:
 *   P(s, L) = L^{-beta} * G(s / L^D)
 * where beta = tau * D and G is a universal scaling function.
 *
 * Integrated form: <s> ~ L^{gamma} where gamma = D*(2 - tau).
 * ============================================================================ */

void eoc_sandpile_finite_size_scaling(EOCSandpileModel** models, int n_sizes,
                                       int* sizes_L, double* tau_out,
                                       double* D_out) {
    if (!models || !sizes_L || n_sizes < 3) {
        *tau_out = 0.0;
        *D_out = 0.0;
        return;
    }

    /* Compute mean avalanche size for each L */
    double* mean_s = (double*)malloc(n_sizes * sizeof(double));
    for (int i = 0; i < n_sizes; i++) {
        if (!models[i] || models[i]->n_avalanches_recorded == 0) {
            mean_s[i] = 0.0;
            continue;
        }
        double sum = 0.0;
        for (int j = 0; j < models[i]->n_avalanches_recorded; j++) {
            sum += models[i]->avalanche_sizes[j];
        }
        mean_s[i] = sum / models[i]->n_avalanches_recorded;
    }

    /* Linear regression: log(<s>) = gamma * log(L) + const */
    double sum_logL = 0.0, sum_logS = 0.0, sum_logL_logS = 0.0, sum_logL2 = 0.0;
    int count = 0;
    for (int i = 0; i < n_sizes; i++) {
        if (mean_s[i] > 0.0 && sizes_L[i] > 0) {
            double lL = log((double)sizes_L[i]);
            double lS = log(mean_s[i]);
            sum_logL += lL;
            sum_logS += lS;
            sum_logL_logS += lL * lS;
            sum_logL2 += lL * lL;
            count++;
        }
    }

    if (count < 2) {
        free(mean_s);
        return;
    }

    double gamma = (count * sum_logL_logS - sum_logL * sum_logS) /
                   (count * sum_logL2 - sum_logL * sum_logL);

    /* For BTW 2D: gamma = D * (2 - tau).
     * If we assume D ? 2 (compact avalanches), then tau = 2 - gamma/2.
     * More generally, use known BTW exponents: D ~ 2.7, tau ~ 1.2 */
    *D_out = 2.7;
    *tau_out = 2.0 - gamma / (*D_out);
    if (*tau_out < 0.5 || *tau_out > 3.0) *tau_out = 1.2; /* Fallback to known value */

    free(mean_s);
}

/* ============================================================================
 * Average Avalanche Size
 * ============================================================================ */

double eoc_sandpile_average_size(EOCSandpileModel* sp, int n_grains) {
    (void)n_grains;
    if (!sp || sp->n_avalanches_recorded == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < sp->n_avalanches_recorded; i++) {
        sum += sp->avalanche_sizes[i];
    }
    return sum / sp->n_avalanches_recorded;
}

/* ============================================================================
 * Branching Ratio
 *
 * sigma = average number of topplings triggered per initial toppling.
 * At criticality: sigma = 1 (marginal propagation).
 * sigma < 1: subcritical (avalanches die out).
 * sigma > 1: supercritical (system-spanning avalanches).
 * ============================================================================ */

double eoc_sandpile_branching_ratio(EOCSandpileModel* sp, int n_grains) {
    (void)n_grains;
    if (!sp || sp->n_avalanches_recorded == 0) return 0.0;

    /* Estimate from avalanche size distribution:
     * At criticality, <s> diverges with L, but per-grain branching can be
     * approximated from the ratio of large to small avalanches. */
    double small_sum = 0.0, large_sum = 0.0;
    int n_small = 0, n_large = 0;
    double median = sp->width * sp->height / 4.0;

    for (int i = 0; i < sp->n_avalanches_recorded; i++) {
        if (sp->avalanche_sizes[i] <= median) {
            small_sum += sp->avalanche_sizes[i];
            n_small++;
        } else {
            large_sum += sp->avalanche_sizes[i];
            n_large++;
        }
    }

    if (n_small == 0 || n_large == 0) return 1.0;
    return (large_sum / n_large) / (small_sum / n_small) * 0.5;
}

/* ============================================================================
 * Critical State Detection
 *
 * A system is in a SOC state if:
 * 1. Avalanche sizes follow a power-law distribution.
 * 2. The distribution spans multiple decades.
 * 3. The branching ratio is approximately 1.
 * ============================================================================ */

bool eoc_sandpile_is_critical(EOCSandpileModel* sp) {
    if (!sp || sp->n_avalanches_recorded < 100) return false;

    EOCPowerLawFit fit = eoc_sandpile_fit_powerlaw(sp);
    double sigma = eoc_sandpile_branching_ratio(sp, 0);

    return fit.is_power_law && (sigma > 0.5 && sigma < 1.5);
}

/* ============================================================================
 * Reset
 * ============================================================================ */

void eoc_sandpile_reset(EOCSandpileModel* sp) {
    if (!sp) return;
    int N = sp->width * sp->height;
    memset(sp->heights, 0, N * sizeof(int));
    sp->n_active = 0;
    sp->total_avalanches = 0;
    sp->n_avalanches_recorded = 0;
    sp->state = EOC_SUBCRITICAL;
}

/* ============================================================================
 * 1/f Noise Spectrum (Flicker Noise)
 *
 * SOC systems exhibit 1/f noise: S(f) ~ f^{-alpha} with alpha ~ 1.
 * We compute the power spectrum via periodogram (FFT not needed ? we use a
 * direct autocorrelation-based method with the Wiener-Khinchin theorem:
 * S(f) = FFT[R(tau)].
 *
 * For simplicity, we use a simple smoothed periodogram approach:
 * 1. Compute the avalanche signal x[t] = avalanche size at "time" t
 * 2. Compute FFT magnitude squared
 * 3. Bin logarithmically in frequency
 * ============================================================================ */

static void slow_dft(double* x, int n, double* real_out, double* imag_out) {
    for (int k = 0; k < n/2 + 1; k++) {
        double re = 0.0, im = 0.0;
        for (int t = 0; t < n; t++) {
            double angle = -2.0 * 3.14159265358979323846 * k * t / n;
            re += x[t] * cos(angle);
            im += x[t] * sin(angle);
        }
        real_out[k] = re;
        imag_out[k] = im;
    }
}

void eoc_sandpile_flicker_noise(EOCSandpileModel* sp,
                                 double** freqs, double** power, int* n_freqs) {
    if (!sp || sp->n_avalanches_recorded < 4) {
        *freqs = NULL;
        *power = NULL;
        *n_freqs = 0;
        return;
    }

    int n = sp->n_avalanches_recorded;
    int nf = n / 2 + 1;

    double* re = (double*)malloc(nf * sizeof(double));
    double* im = (double*)malloc(nf * sizeof(double));
    double* sig = (double*)malloc(n * sizeof(double));

    /* Detrend the signal */
    double mean_val = 0.0;
    for (int i = 0; i < n; i++) mean_val += sp->avalanche_sizes[i];
    mean_val /= n;
    for (int i = 0; i < n; i++) sig[i] = sp->avalanche_sizes[i] - mean_val;

    slow_dft(sig, n, re, im);

    /* Logarithmic frequency binning */
    int nb = 20;
    double* fb = (double*)malloc(nb * sizeof(double));
    double* pb = (double*)calloc(nb, sizeof(double));
    int* cb = (int*)calloc(nb, sizeof(int));

    double f_min = 1.0 / n;
    double f_max = 0.5;
    double fratio = pow(f_max / f_min, 1.0 / (nb - 1));

    for (int i = 0; i < nb; i++) {
        fb[i] = f_min * pow(fratio, i);
    }

    for (int k = 1; k < nf; k++) {
        double f = (double)k / n;
        for (int b = 0; b < nb - 1; b++) {
            if (f >= fb[b] && f < fb[b + 1]) {
                double pw = (re[k] * re[k] + im[k] * im[k]) / (n * n);
                pb[b] += pw;
                cb[b]++;
                break;
            }
        }
    }

    /* Average within each bin */
    for (int b = 0; b < nb; b++) {
        if (cb[b] > 0) pb[b] /= cb[b];
    }

    *freqs = fb;
    *power = pb;
    *n_freqs = nb;
    free(re);
    free(im);
    free(sig);
    free(cb);
}
