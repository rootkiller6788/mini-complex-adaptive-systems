#ifndef EOC_CORE_H
#define EOC_CORE_H

#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Edge of Chaos & Criticality ? Core Definitions and Types
 *
 * Foundational references:
 *   Langton, C.G. (1990). "Computation at the edge of chaos: Phase transitions
 *     and emergent computation." Physica D, 42(1-3), 12-37.
 *   Bak, P., Tang, C., & Wiesenfeld, K. (1987). "Self-organized criticality:
 *     An explanation of the 1/f noise." Physical Review Letters, 59(4), 381-384.
 *   Wolfram, S. (1984). "Universality and complexity in cellular automata."
 *     Physica D, 10(1-2), 1-35.
 *   Kauffman, S.A. (1993). The Origins of Order. Oxford University Press.
 *   Stanley, H.E. (1987). Introduction to Phase Transitions and Critical
 *     Phenomena. Oxford University Press.
 *   Jensen, H.J. (1998). Self-Organized Criticality. Cambridge University Press.
 * ============================================================================ */

/* --- Phase / Dynamical Regime Classification --- */

typedef enum {
    EOC_FROZEN = 0,         /* Fixed point, static order (Wolfram Class I) */
    EOC_PERIODIC = 1,       /* Limit cycles, periodic (Wolfram Class II) */
    EOC_CHAOTIC = 2,        /* Deterministic chaos (Wolfram Class III) */
    EOC_COMPLEX = 3,        /* Edge of chaos / structured (Wolfram Class IV) */
    EOC_CRITICAL = 4,       /* Self-organized critical state */
    EOC_SUBCRITICAL = 5,    /* Below critical threshold */
    EOC_SUPERCRITICAL = 6,  /* Above critical threshold */
    EOC_TRANSCRITICAL = 7   /* Transitional regime */
} EOCPhaseState;

/* --- Bifurcation Types --- */

typedef enum {
    BIFUR_SADDLE_NODE = 0,      /* Fold bifurcation: dx/dt = r + x^2 */
    BIFUR_TRANSCRITICAL = 1,    /* dx/dt = r*x - x^2 */
    BIFUR_PITCHFORK_SUPER = 2,  /* Supercritical: dx/dt = r*x - x^3 */
    BIFUR_PITCHFORK_SUB = 3,    /* Subcritical: dx/dt = r*x + x^3 - x^5 */
    BIFUR_HOPF_SUPER = 4,       /* Supercritical Hopf: limit cycle birth */
    BIFUR_HOPF_SUB = 5,         /* Subcritical Hopf */
    BIFUR_PERIOD_DOUBLING = 6,  /* Flip: period-doubling cascade */
    BIFUR_NEIMARK_SACKER = 7,   /* Torus bifurcation */
    BIFUR_CUSP = 8,             /* Cusp catastrophe */
    BIFUR_BOGDANOV_TAKENS = 9   /* Double-zero eigenvalue bifurcation */
} BifurcationType;

/* --- Wolfram Cellular Automaton Classes --- */

typedef enum {
    WOLFRAM_CLASS_I = 0,    /* Evolves to homogeneous fixed state */
    WOLFRAM_CLASS_II = 1,   /* Evolves to simple separated periodic structures */
    WOLFRAM_CLASS_III = 2,  /* Evolves to chaotic aperiodic patterns */
    WOLFRAM_CLASS_IV = 3    /* Evolves to complex localized structures (edge of chaos) */
} WolframClass;

/* --- Universality Classes --- */

typedef enum {
    UNIV_DIRECTED_PERCOLATION = 0,    /* DP class (Reggeon field theory) */
    UNIV_ISING_2D = 1,               /* 2D Ising universality */
    UNIV_ISING_3D = 2,               /* 3D Ising universality */
    UNIV_MEAN_FIELD = 3,             /* Infinite dimension / all-to-all coupling */
    UNIV_XY = 4,                     /* Kosterlitz-Thouless transition */
    UNIV_POTTS_Q3 = 5,               /* q=3 Potts model */
    UNIV_MANNA = 6,                  /* Manna sandpile universality */
    UNIV_BTW = 7,                    /* Bak-Tang-Wiesenfeld universality */
    UNIV_OSLO = 8,                   /* Oslo ricepile universality */
    UNIV_PERCOLATION_2D = 9,         /* 2D random percolation */
    UNIV_BRANCHING_PROCESS = 10      /* Mean-field branching */
} UniversalityClass;

/* --- Numeric Types for Dynamics --- */

typedef struct {
    double* data;
    int rows;
    int cols;
    bool owns_data;
} EOCMatrix;

typedef struct {
    double* components;
    int dimension;
} EOCVector;

typedef struct {
    double* values;
    int length;
    int capacity;
    double mean;
    double variance;
    double skewness;
    double kurtosis;
} EOCTimeSeries;

/* --- Phase Space State --- */

typedef struct {
    double* position;
    double* velocity;
    int n_dims;
    double time;
    EOCPhaseState regime;
} EOCPhasePoint;

/* --- Bifurcation Diagram Point --- */

typedef struct {
    double parameter;
    double* attractor_values;
    int n_attractor;
    int period;
    bool is_chaotic;
    double lyapunov;
} EOCBifurcationPoint;

/* --- Critical Exponents (standard notation) --- */

typedef struct {
    double alpha;
    double beta_b;
    double gamma;
    double delta;
    double nu;
    double eta;
    double z;
    double tau_avalanche;
    double D_avalanche;
    double alpha_flicker;
    int    spatial_dimension;
} EOCCriticalExponents;

/* --- Power-Law Distribution Fitting Result --- */

typedef struct {
    double xmin;
    double alpha;
    double alpha_std_error;
    double ks_statistic;
    double p_value;
    int n_samples;
    int n_total;
    bool is_power_law;
} EOCPowerLawFit;

/* --- Self-Organized Criticality (SOC) Model Descriptor --- */

typedef struct {
    char* name;
    int width;
    int height;
    double threshold;
    int* heights;
    int* active_sites;
    int n_active;
    int total_avalanches;
    double* avalanche_sizes;
    int n_avalanches_recorded;
    int avalanche_history_capacity;
    bool open_boundaries;
    UniversalityClass uc;
    EOCPhaseState state;
} EOCSandpileModel;

/* --- Correlation Integral (for fractal dimension) --- */

typedef struct {
    double* radii;
    double* c_epsilon;
    int n_radii;
    double dimension;
    double r_squared;
} EOCCorrelationIntegral;

/* --- Multifractal Spectrum --- */

typedef struct {
    double* q_values;
    double* tau_q;
    double* alpha_q;
    double* f_alpha;
    int n_q;
    double D_0;
    double D_1;
    double D_2;
} EOCMultifractalSpectrum;

/* --- Core Initialization and Lifecycle --- */

EOCSandpileModel* eoc_sandpile_create(const char* name, int width, int height,
                                       double threshold);
void eoc_sandpile_free(EOCSandpileModel* sp);

EOCTimeSeries* eoc_timeseries_create(int initial_capacity);
void eoc_timeseries_free(EOCTimeSeries* ts);
void eoc_timeseries_push(EOCTimeSeries* ts, double value);
void eoc_timeseries_stats(EOCTimeSeries* ts);
double eoc_timeseries_autocorrelation(EOCTimeSeries* ts, int lag);

EOCBifurcationPoint* eoc_bifurcation_alloc(int max_attractor);
/** Free only internal memory (attractor_values). Does not free bp itself. */
void eoc_bifurcation_free(EOCBifurcationPoint* bp);
/** Free internal memory AND the struct itself (for individually-allocated). */
void eoc_bifurcation_free_owned(EOCBifurcationPoint* bp);

EOCMultifractalSpectrum* eoc_multifractal_alloc(int n_q);
void eoc_multifractal_free(EOCMultifractalSpectrum* ms);

/* --- Utility --- */

double eoc_log2(double x);
double eoc_clamp(double x, double lo, double hi);
double eoc_urand(void);
double eoc_gauss_rand(void);
int eoc_pow_int(int base, int exp);
void eoc_sort_double(double* arr, int n, bool ascending);
void eoc_histogram(double* data, int n, int n_bins,
                   double* bin_edges, int* counts);
void eoc_matrix_free(EOCMatrix* m);

#endif /* EOC_CORE_H */
