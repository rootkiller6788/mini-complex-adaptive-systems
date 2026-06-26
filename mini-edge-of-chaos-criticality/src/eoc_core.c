#include "eoc_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Edge of Chaos & Criticality ? Core Implementations
 *
 * L1 Definitions: EOCPhaseState, BifurcationType, WolframClass,
 *   UniversalityClass, EOCCriticalExponents, EOCPowerLawFit,
 *   EOCSandpileModel, EOCTimeSeries
 *
 * L2 Core Concepts: Phase regimes at edge of chaos, universality classes,
 *   critical exponents as measurable quantities
 *
 * L3 Mathematical Structures: Timeseries with full statistical moments,
 *   sandpile lattice with active-site tracking, multifractal angular structure
 * ============================================================================ */

#define PI  3.14159265358979323846
#define E   2.71828182845904523536

/* ============================================================================
 * Sandpile Lifecycle
 * ============================================================================ */

EOCSandpileModel* eoc_sandpile_create(const char* name, int width, int height,
                                       double threshold) {
    EOCSandpileModel* sp = (EOCSandpileModel*)calloc(1, sizeof(EOCSandpileModel));
    if (!sp) return NULL;
    sp->name = strdup(name ? name : "sandpile");
    sp->width = width;
    sp->height = height;
    sp->threshold = threshold;
    int N = width * height;
    sp->heights = (int*)calloc(N, sizeof(int));
    sp->active_sites = (int*)malloc(N * sizeof(int));
    sp->n_active = 0;
    sp->total_avalanches = 0;
    sp->n_avalanches_recorded = 0;
    sp->avalanche_history_capacity = 10000;
    sp->avalanche_sizes = (double*)malloc(sp->avalanche_history_capacity * sizeof(double));
    sp->open_boundaries = true;
    sp->uc = UNIV_BTW;
    sp->state = EOC_SUBCRITICAL;
    return sp;
}

void eoc_sandpile_free(EOCSandpileModel* sp) {
    if (!sp) return;
    free(sp->name);
    free(sp->heights);
    free(sp->active_sites);
    free(sp->avalanche_sizes);
    free(sp);
}

/* ============================================================================
 * Time Series Lifecycle
 * ============================================================================ */

EOCTimeSeries* eoc_timeseries_create(int initial_capacity) {
    EOCTimeSeries* ts = (EOCTimeSeries*)calloc(1, sizeof(EOCTimeSeries));
    if (!ts) return NULL;
    ts->capacity = initial_capacity > 0 ? initial_capacity : 1024;
    ts->values = (double*)malloc(ts->capacity * sizeof(double));
    ts->length = 0;
    ts->mean = 0.0;
    ts->variance = 0.0;
    ts->skewness = 0.0;
    ts->kurtosis = 0.0;
    return ts;
}

void eoc_timeseries_free(EOCTimeSeries* ts) {
    if (!ts) return;
    free(ts->values);
    free(ts);
}

void eoc_timeseries_push(EOCTimeSeries* ts, double value) {
    if (!ts) return;
    if (ts->length >= ts->capacity) {
        ts->capacity *= 2;
        ts->values = (double*)realloc(ts->values, ts->capacity * sizeof(double));
    }
    ts->values[ts->length++] = value;
}

void eoc_timeseries_stats(EOCTimeSeries* ts) {
    if (!ts || ts->length == 0) return;
    int n = ts->length;
    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += ts->values[i];
    ts->mean = sum / n;

    double sum2 = 0.0, sum3 = 0.0, sum4 = 0.0;
    for (int i = 0; i < n; i++) {
        double d = ts->values[i] - ts->mean;
        double d2 = d * d;
        sum2 += d2;
        sum3 += d2 * d;
        sum4 += d2 * d2;
    }
    ts->variance = sum2 / n;
    double sigma = sqrt(ts->variance);
    if (sigma > 1e-15) {
        ts->skewness = sum3 / (n * sigma * sigma * sigma);
        ts->kurtosis = sum4 / (n * sigma * sigma * sigma * sigma) - 3.0;
    } else {
        ts->skewness = 0.0;
        ts->kurtosis = 0.0;
    }
}

double eoc_timeseries_autocorrelation(EOCTimeSeries* ts, int lag) {
    if (!ts || ts->length <= lag) return 0.0;
    int n = ts->length;
    double mx = ts->mean;
    if (mx == 0.0 && ts->variance == 0.0) eoc_timeseries_stats(ts);
    double num = 0.0, den = 0.0;
    for (int i = 0; i < n; i++) {
        double d = ts->values[i] - ts->mean;
        den += d * d;
        if (i + lag < n) num += d * (ts->values[i + lag] - ts->mean);
    }
    return (fabs(den) < 1e-15) ? 0.0 : num / den;
}

/* ============================================================================
 * Bifurcation Point Lifecycle
 * ============================================================================ */

EOCBifurcationPoint* eoc_bifurcation_alloc(int max_attractor) {
    EOCBifurcationPoint* bp = (EOCBifurcationPoint*)calloc(1, sizeof(EOCBifurcationPoint));
    if (!bp) return NULL;
    bp->attractor_values = (double*)calloc(max_attractor, sizeof(double));
    bp->n_attractor = 0;
    bp->period = 0;
    bp->is_chaotic = false;
    bp->lyapunov = 0.0;
    return bp;
}

void eoc_bifurcation_free(EOCBifurcationPoint* bp) {
    if (!bp) return;
    free(bp->attractor_values);
    bp->attractor_values = NULL;
}
void eoc_bifurcation_free_owned(EOCBifurcationPoint* bp) {
    if (!bp) return;
    free(bp->attractor_values);
    free(bp);
}

/* ============================================================================
 * Multifractal Spectrum Lifecycle
 * ============================================================================ */

EOCMultifractalSpectrum* eoc_multifractal_alloc(int n_q) {
    EOCMultifractalSpectrum* ms = (EOCMultifractalSpectrum*)calloc(1, sizeof(EOCMultifractalSpectrum));
    if (!ms) return NULL;
    ms->n_q = n_q;
    ms->q_values = (double*)malloc(n_q * sizeof(double));
    ms->tau_q = (double*)malloc(n_q * sizeof(double));
    ms->alpha_q = (double*)malloc(n_q * sizeof(double));
    ms->f_alpha = (double*)malloc(n_q * sizeof(double));
    ms->D_0 = ms->D_1 = ms->D_2 = 0.0;
    return ms;
}

void eoc_multifractal_free(EOCMultifractalSpectrum* ms) {
    if (!ms) return;
    free(ms->q_values);
    free(ms->tau_q);
    free(ms->alpha_q);
    free(ms->f_alpha);
    free(ms);
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

double eoc_log2(double x) {
    return (x > 0.0) ? log(x) / log(2.0) : 0.0;
}

double eoc_clamp(double x, double lo, double hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

double eoc_urand(void) {
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

double eoc_gauss_rand(void) {
    double u1 = eoc_urand();
    double u2 = eoc_urand();
    return sqrt(-2.0 * log(u1 + 1e-16)) * cos(2.0 * PI * u2);
}

int eoc_pow_int(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i++) result *= base;
    return result;
}

static int cmp_double_asc(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

static int cmp_double_desc(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (db > da) - (db < da);
}

void eoc_sort_double(double* arr, int n, bool ascending) {
    qsort(arr, n, sizeof(double), ascending ? cmp_double_asc : cmp_double_desc);
}

void eoc_histogram(double* data, int n, int n_bins,
                   double* bin_edges, int* counts) {
    if (!data || n <= 0 || n_bins <= 0) return;
    /* Find min and max */
    double dmin = data[0], dmax = data[0];
    for (int i = 1; i < n; i++) {
        if (data[i] < dmin) dmin = data[i];
        if (data[i] > dmax) dmax = data[i];
    }
    double range = dmax - dmin;
    if (range < 1e-15) range = 1.0;
    double eps = range * 0.001;
    for (int i = 0; i <= n_bins; i++) {
        bin_edges[i] = dmin + (range + 2.0 * eps) * i / n_bins - eps;
    }
    for (int i = 0; i < n_bins; i++) counts[i] = 0;
    for (int i = 0; i < n; i++) {
        double v = data[i];
        if (v < bin_edges[0] || v > bin_edges[n_bins]) continue;
        for (int b = 0; b < n_bins; b++) {
            if (v >= bin_edges[b] && v < bin_edges[b + 1]) {
                counts[b]++;
                break;
            }
        }
    }
}

void eoc_matrix_free(EOCMatrix* m) {
    if (!m) return;
    if (m->owns_data && m->data) {
        free(m->data);
    }
    m->data = NULL;
    m->rows = m->cols = 0;
}
