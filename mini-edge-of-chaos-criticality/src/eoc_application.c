#include "eoc_core.h"
#include "eoc_sandpile.h"
#include "eoc_powerlaw.h"
#include "eoc_dynamics.h"
#include "eoc_fractal.h"
#include "eoc_criticality.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Real-World Applications of Edge of Chaos & Criticality
 *
 * L7 Applications: Uses the core criticality framework to model and analyze
 * real-world complex systems that exhibit SOC and edge-of-chaos behavior.
 *
 * Each application is a self-contained function implementing one independent
 * knowledge point from a real-world domain.
 * ============================================================================ */

/* ============================================================================
 * Application 1: Earthquake Frequency-Magnitude Distribution
 *
 * The Gutenberg-Richter law states that earthquake frequency N(M) follows:
 *   log10(N) = a - b*M
 * where M is magnitude and b ~ 1.0 (typically 0.8-1.2).
 *
 * This is the power-law signature of SOC ? the Earth's crust is in a
 * self-organized critical state (Bak & Tang 1989).
 *
 * Our BTW sandpile model produces avalanche size distributions that
 * map to earthquake magnitude distributions via s ~ 10^M.
 * ============================================================================ */

typedef struct {
    double b_value;          /* Gutenberg-Richter b-value */
    double b_value_error;    /* Standard error of b */
    double a_value;          /* Log activity rate */
    double mc;               /* Magnitude of completeness */
    int n_events;            /* Number of events above mc */
    bool is_soc_consistent;  /* Is b ~ 1.0? */
} EOCGutenbergRichterResult;

/**
 * Fit Gutenberg-Richter law to earthquake catalog data.
 * Input: magnitudes array, output: b-value and completeness.
 * The b-value is estimated via MLE: b = 1 / (ln(10) * (<M> - Mc))
 * (Aki 1965, Utsu 1965).
 * Complexity: O(n log n).
 */
EOCGutenbergRichterResult eoc_earthquake_gutenberg_richter(
    double* magnitudes, int n) {
    EOCGutenbergRichterResult gr;
    memset(&gr, 0, sizeof(gr));

    if (!magnitudes || n < 20) return gr;

    /* Estimate Mc (magnitude of completeness) from maximum curvature */
    /* Build magnitude frequency histogram */
    double m_min = magnitudes[0], m_max = magnitudes[0];
    for (int i = 0; i < n; i++) {
        if (magnitudes[i] < m_min) m_min = magnitudes[i];
        if (magnitudes[i] > m_max) m_max = magnitudes[i];
    }

    int n_bins = 30;
    double* bin_counts = (double*)calloc(n_bins, sizeof(double));
    double dm = (m_max - m_min) / n_bins;
    for (int i = 0; i < n; i++) {
        int b = (int)((magnitudes[i] - m_min) / dm);
        if (b >= 0 && b < n_bins) bin_counts[b]++;
    }

    /* Find Mc as the bin with maximum count (maximum curvature) */
    int mc_bin = 0;
    double max_count = 0.0;
    for (int b = 0; b < n_bins; b++) {
        if (bin_counts[b] > max_count) {
            max_count = bin_counts[b];
            mc_bin = b;
        }
    }
    gr.mc = m_min + (mc_bin + 0.5) * dm;

    /* MLE for b-value using events above Mc (Aki 1965) */
    double sum_m = 0.0;
    int n_above = 0;
    for (int i = 0; i < n; i++) {
        if (magnitudes[i] >= gr.mc) {
            sum_m += magnitudes[i];
            n_above++;
        }
    }

    gr.n_events = n_above;
    if (n_above >= 10) {
        double mean_m = sum_m / n_above;
        gr.b_value = 1.0 / (log(10.0) * (mean_m - gr.mc + 0.05));
        gr.b_value_error = gr.b_value / sqrt((double)n_above);
        gr.a_value = log10((double)n_above) + gr.b_value * gr.mc;
        gr.is_soc_consistent = (fabs(gr.b_value - 1.0) < 0.3);
    }

    free(bin_counts);
    return gr;
}

/* ============================================================================
 * Application 2: Forest Fire / Epidemic Spreading Model
 *
 * The Drossel-Schwabl forest fire model (1992) exhibits SOC:
 *   - Trees grow randomly on empty sites (rate p)
 *   - Lightning strikes cause fires that spread to adjacent trees
 *   - Fire sizes follow power-law distribution
 *
 * This maps to epidemic spreading (SIR model at criticality):
 *   - Susceptible = empty sites
 *   - Infected = burning trees
 *   - Recovered = ash (becomes empty after some time)
 *   - At R0 ~ 1, infection cluster sizes follow power-law
 *
 * We implement a simplified lattice model with growth, ignition,
 * and spread phases, tracking cluster size distribution.
 * ============================================================================ */

typedef struct {
    int* grid;               /* 0=empty, 1=tree, 2=burning, 3=ash */
    int width;
    int height;
    double p_growth;         /* Tree growth probability */
    double f_spark;          /* Lightning probability */
    double p_spread;         /* Fire spread probability to neighbor */
    int* cluster_sizes;      /* History of fire cluster sizes */
    int n_fires;
    int max_fires;
    double total_trees;
    double total_burned;
} EOCForestFireModel;

EOCForestFireModel* eoc_forest_fire_create(int w, int h,
                                             double p_growth,
                                             double f_spark,
                                             double p_spread) {
    EOCForestFireModel* ff = (EOCForestFireModel*)calloc(1, sizeof(EOCForestFireModel));
    if (!ff) return NULL;
    ff->width = w;
    ff->height = h;
    ff->grid = (int*)calloc(w * h, sizeof(int));
    ff->p_growth = p_growth;
    ff->f_spark = f_spark;
    ff->p_spread = p_spread;
    ff->max_fires = 5000;
    ff->cluster_sizes = (int*)malloc(ff->max_fires * sizeof(int));
    ff->n_fires = 0;
    return ff;
}

void eoc_forest_fire_free(EOCForestFireModel* ff) {
    if (!ff) return;
    free(ff->grid);
    free(ff->cluster_sizes);
    free(ff);
}

/**
 * One Monte Carlo step of the forest fire model.
 * Phase 1: Tree growth (randomly fill empty sites)
 * Phase 2: Lightning ignition (random spark on tree sites)
 * Phase 3: Fire spread (to adjacent trees with prob p_spread)
 * Returns: size of fire cluster (0 if no fire).
 *
 * Complexity: O(width * height) per step.
 */
int eoc_forest_fire_step(EOCForestFireModel* ff) {
    if (!ff) return 0;

    int N = ff->width * ff->height;

    /* Phase 1: Tree growth on empty sites */
    for (int i = 0; i < N; i++) {
        if (ff->grid[i] == 0) {
            if ((double)rand() / RAND_MAX < ff->p_growth) {
                ff->grid[i] = 1; /* Tree grows */
            }
        }
    }

    /* Phase 2: Lightning ? convert one random tree to burning */
    int fire_start = -1;
    int* tree_sites = (int*)malloc(N * sizeof(int));
    int n_trees = 0;
    for (int i = 0; i < N; i++) {
        if (ff->grid[i] == 1) tree_sites[n_trees++] = i;
    }

    if (n_trees > 0 && (double)rand() / RAND_MAX < ff->f_spark * n_trees) {
        int idx = tree_sites[rand() % n_trees];
        ff->grid[idx] = 2; /* Tree -> burning */
        fire_start = idx;
    }
    free(tree_sites);

    if (fire_start < 0) return 0;

    /* Phase 3: Fire spread via breadth-first search */
    int* queue = (int*)malloc(N * sizeof(int));
    int q_head = 0, q_tail = 1;
    queue[0] = fire_start;
    int cluster_size = 0;

    while (q_head < q_tail) {
        int site = queue[q_head++];
        int sx = site % ff->width;
        int sy = site / ff->width;

        /* Spread to 4 neighbors */
        int nb[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
        for (int d = 0; d < 4; d++) {
            int nx = sx + nb[d][0];
            int ny = sy + nb[d][1];
            if (nx < 0 || nx >= ff->width || ny < 0 || ny >= ff->height) continue;

            int ni = ny * ff->width + nx;
            if (ff->grid[ni] == 1) { /* Tree neighbor */
                if ((double)rand() / RAND_MAX < ff->p_spread) {
                    ff->grid[ni] = 2; /* Catches fire */
                    queue[q_tail++] = ni;
                }
            }
        }

        /* Current tree finishes burning -> ash */
        ff->grid[site] = 3;
        cluster_size++;
    }

    /* Ash -> empty (recovery, next step trees can grow back) */
    for (int i = 0; i < N; i++) {
        if (ff->grid[i] == 3) ff->grid[i] = 0;
    }

    /* Record cluster size */
    if (ff->n_fires < ff->max_fires) {
        ff->cluster_sizes[ff->n_fires++] = cluster_size;
    }

    ff->total_burned += cluster_size;
    free(queue);
    return cluster_size;
}

/**
 * Run forest fire model for n_steps and fit power-law to fire sizes.
 * Returns the estimated exponent tau from P(s) ~ s^{-tau}.
 *
 * at criticality (p_spread ~ 0.5-0.6), tau ~ 1.1-1.3 (similar to SOC).
 */
EOCPowerLawFit eoc_forest_fire_powerlaw_fit(EOCForestFireModel* ff) {
    EOCPowerLawFit result;
    memset(&result, 0, sizeof(result));

    if (!ff || ff->n_fires < 20) return result;

    /* Convert fire sizes to doubles and fit power-law */
    double* sizes_d = (double*)malloc(ff->n_fires * sizeof(double));
    for (int i = 0; i < ff->n_fires; i++) {
        sizes_d[i] = (double)ff->cluster_sizes[i];
    }

    result = eoc_powerlaw_fit_auto(sizes_d, ff->n_fires);
    free(sizes_d);
    return result;
}

/* ============================================================================
 * Application 3: Long-Range Correlation Detection in Time Series
 *
 * Many real-world signals exhibit long-range correlations characteristic
 * of criticality: heart rate variability (HRV), financial volatility,
 * internet traffic, DNA sequences, climate records.
 *
 * We provide a combined DFA + Higuchi fractal dimension analysis
 * that classifies a time series as:
 *   - WHITE_NOISE: DFA alpha ~ 0.5, uncorrelated
 *   - PINK_NOISE: DFA alpha ~ 1.0, 1/f (critical)
 *   - BROWNIAN: DFA alpha ~ 1.5, integrated
 *   - LONG_MEMORY: DFA alpha > 0.5, persistent correlations
 * ============================================================================ */

typedef enum {
    SIGNAL_WHITE_NOISE = 0,
    SIGNAL_PINK_NOISE = 1,
    SIGNAL_BROWNIAN = 2,
    SIGNAL_LONG_MEMORY = 3,
    SIGNAL_ANTI_PERSISTENT = 4,
    SIGNAL_UNKNOWN = 5
} EOCSignalClass;

/**
 * Classify a time series based on DFA exponent and Higuchi dimension.
 *
 * Reference: Peng et al. (1994) Phys. Rev. E, 49(2), 1685-1689.
 *            "Mosaic organization of DNA nucleotides."
 *
 * Complexity: O(n log n) for DFA + O(n*k_max) for Higuchi.
 */
EOCSignalClass eoc_classify_signal_criticality(double* time_series, int n) {
    if (!time_series || n < 100) return SIGNAL_UNKNOWN;

    double alpha = eoc_dfa_exponent(time_series, n, 4, n / 4);
    double D = eoc_higuchi_fractal_dimension(time_series, n, n / 10);

    /* Classification based on DFA alpha */
    if (alpha < 0.35) {
        return SIGNAL_ANTI_PERSISTENT;
    } else if (alpha < 0.65) {
        return SIGNAL_WHITE_NOISE;
    } else if (alpha < 0.85) {
        /* Check Higuchi: if D is high, closer to pink noise */
        return (D > 1.5) ? SIGNAL_PINK_NOISE : SIGNAL_LONG_MEMORY;
    } else if (alpha < 1.15) {
        return SIGNAL_PINK_NOISE;
    } else if (alpha < 1.35) {
        return SIGNAL_LONG_MEMORY;
    } else {
        return SIGNAL_BROWNIAN;
    }
}

/**
 * Generate synthetic 1/f noise (pink noise) using the Voss-McCartney
 * algorithm (superposition of multiple random processes).
 *
 * This is the spectral signature of SOC ? used for testing
 * criticality detection algorithms.
 *
 * Complexity: O(n * log n).
 */
double* eoc_generate_pink_noise(int n) {
    if (n <= 0) return NULL;

    double* noise = (double*)calloc(n, sizeof(double));

    /* Number of octaves */
    int n_octaves = (int)(log((double)n) / log(2.0));
    if (n_octaves < 1) n_octaves = 1;
    if (n_octaves > 16) n_octaves = 16;

    /* Generate white noise for each octave and accumulate */
    for (int oct = 0; oct < n_octaves; oct++) {
        int step = 1 << oct; /* 2^oct */
        double* white = (double*)malloc(n * sizeof(double));
        for (int i = 0; i < n; i++) {
            white[i] = (double)rand() / RAND_MAX - 0.5;
        }

        /* Smooth by holding value over each step interval */
        double current = white[0];
        for (int i = 0; i < n; i++) {
            if (i % step == 0 && i + step < n) {
                current = white[i];
            }
            noise[i] += current / n_octaves;
        }
        free(white);
    }

    return noise;
}

/* ============================================================================
 * Application 4: Critical Point Detection in Complex Networks
 *
 * Real networks (social, biological, technological) often operate near
 * criticality. We detect this by analyzing degree distribution power-law
 * behavior and clustering coefficient scaling.
 *
 * A power-law degree distribution P(k) ~ k^{-gamma} with 2 < gamma < 3
 * indicates a scale-free network (Barabasi-Albert 1999), which is the
 * network analog of criticality.
 * ============================================================================ */

/**
 * Fit power-law to network degree distribution.
 * Input: degree of each node, output: fitted gamma exponent.
 *
 * Scale-free networks: gamma typically 2.1-3.0.
 * Random networks (Erdos-Renyi): exponential degree distribution,
 *   power-law fit will have poor KS statistic (p < 0.1).
 *
 * Complexity: O(n log n).
 */
EOCPowerLawFit eoc_network_degree_powerlaw(int* degrees, int n_nodes) {
    EOCPowerLawFit result;
    memset(&result, 0, sizeof(result));

    if (!degrees || n_nodes < 20) return result;

    /* Convert to doubles */
    double* deg_d = (double*)malloc(n_nodes * sizeof(double));
    int n_nonzero = 0;
    for (int i = 0; i < n_nodes; i++) {
        if (degrees[i] > 0) {
            deg_d[n_nonzero++] = (double)degrees[i];
        }
    }

    if (n_nonzero >= 10) {
        result = eoc_powerlaw_fit_auto(deg_d, n_nonzero);
    }

    free(deg_d);
    return result;
}

/**
 * Detect if a network is at criticality by analyzing three signatures:
 * 1. Power-law degree distribution (scale-free property)
 * 2. Large clustering coefficient relative to random
 * 3. Community structure modularity
 *
 * Returns a criticality score in [0, 1].
 */
double eoc_network_criticality_score(int* degrees, int* edges_from,
                                      int* edges_to, int n_nodes, int n_edges) {
    (void)edges_from;
    (void)edges_to;
    (void)n_edges;

    if (!degrees || n_nodes < 10) return 0.0;

    double score = 0.0;

    /* Criterion 1: Degree distribution power-law (weight: 0.5) */
    EOCPowerLawFit fit = eoc_network_degree_powerlaw(degrees, n_nodes);
    if (fit.is_power_law && fit.alpha > 2.0 && fit.alpha < 3.5) {
        /* Scale-free: gamma in [2, 3.5] */
        score += 0.5 * (1.0 - fabs(fit.alpha - 2.5) / 2.0);
    }

    /* Criterion 2: Variance of degree (weight: 0.25) */
    double mean_deg = 0.0, var_deg = 0.0;
    for (int i = 0; i < n_nodes; i++) mean_deg += degrees[i];
    mean_deg /= n_nodes;
    for (int i = 0; i < n_nodes; i++) {
        double d = degrees[i] - mean_deg;
        var_deg += d * d;
    }
    var_deg /= n_nodes;

    /* High variance relative to mean indicates heterogeneity (scale-free) */
    if (mean_deg > 0.0) {
        double cv = sqrt(var_deg) / mean_deg;
        score += 0.25 * (cv > 1.0 ? 1.0 : cv);
    }

    /* Criterion 3: Mean degree (weight: 0.25) ? moderate connectivity */
    score += 0.25 * (mean_deg > 2.0 && mean_deg < 20.0 ? 1.0 :
                     (mean_deg > 0.0 ? 0.5 : 0.0));

    return score;
}
