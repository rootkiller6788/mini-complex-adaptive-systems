/**
 * @file    sfi_emergence.h
 * @brief   SFI Methodology: Emergence detection and quantification
 *
 * Implements information-theoretic and statistical approaches to
 * detecting and quantifying emergent phenomena in complex adaptive
 * systems.  Emergence is the SFI signature concept: macro-level
 * patterns that are not reducible to micro-level rules alone.
 *
 * References: Bedau (1997) "Weak Emergence"; Crutchfield (1994)
 * "The Calculi of Emergence"; Shalizi (2001) "Causal Architecture,
 * Complexity, and Self-Organization"
 *
 * Knowledge Levels: L1 (Definitions), L2 (Core Concepts),
 *                   L3 (Math), L4 (Laws), L5 (Algorithms)
 */

#ifndef SFI_EMERGENCE_H_
#define SFI_EMERGENCE_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * L1: Definitions ? Types of Emergence
 * ================================================================ */

/**
 * L1: sfi_emergence_type_t ? Bedau's taxonomy of emergence
 *
 * NOMINAL:   Two distinct levels exist but no claim of novelty.
 * WEAK:      Macro patterns are derivable ONLY through simulation
 *            of micro rules ? epistemological emergence.
 * STRONG:    Macro patterns have downward causal power on micro
 *            constituents ? ontological emergence.
 */
typedef enum {
    SFI_EMERGE_NOMINAL = 0,  /**< Trivial aggregation               */
    SFI_EMERGE_WEAK    = 1,  /**< Epistemological ? Bedau 1997      */
    SFI_EMERGE_STRONG  = 2   /**< Ontological ? downward causation   */
} sfi_emergence_type_t;

/**
 * L1: sfi_microstate_encoding_t ? How microstates are encoded
 */
typedef enum {
    SFI_ENC_RAW_POSITIONS    = 0,  /**< Raw (x,y,energy) vector       */
    SFI_ENC_FOURIER_SPECTRUM = 1,  /**< Spatial Fourier modes         */
    SFI_ENC_AGENT_DENSITY    = 2,  /**< Binned density histogram      */
    SFI_ENC_STRATEGY_VECTOR  = 3,  /**< Strategy/behaviour encoding   */
    SFI_ENC_SYMBOLIC_CA      = 4   /**< Cellular automaton state      */
} sfi_microstate_encoding_t;

/**
 * L1: sfi_emergence_detector_t ? Multi-method emergence detector
 *
 * SFI methodology: no single metric captures emergence.
 * We combine information-theoretic, statistical, and
 * dynamical measures.
 */
typedef struct {
    /* Information-theoretic measures ? L3 */
    double   mutual_info_micro_macro;    /**< I(Micro; Macro) */
    double   multi_info_3body;           /**< 3-body mutual information */
    double   transfer_entropy_m2m;       /**< TE: Micro(t-1) ? Macro(t) */
    double   transfer_entropy_macro_down;/**< TE: Macro(t-1) ? Micro(t) */
    /* Statistical measures */
    double   variance_explained_linear;  /**< Fraction of macro variance explained by micro */
    double   excess_kurtosis;            /**< Non-gaussianity of macro fluctuations */
    double   hurst_exponent;             /**< Long-range memory in macro time series */
    double   gini_coefficient;           /**< Wealth/outcome inequality */
    /* Dynamical measures */
    double   sensitivity_to_initial;     /**< Lyapunov-like divergence rate */
    double   critical_slowing_down;      /**< Recovery rate indicator */
    int32_t  self_organized_critical;    /**< 1 if power-law event sizes detected */
    /* Decision */
    sfi_emergence_type_t detection;      /**< Nominal / Weak / Strong */
    double   emergence_strength;         /**< Aggregate emergence score [0,1] */
} sfi_emergence_detector_t;

/* ================================================================
 * L1: Order Parameter
 * ================================================================ */

/**
 * L1: sfi_order_parameter_t ? A macroscopic observable
 *
 * In SFI language, order parameters are macroscopic variables
 * that characterize the emergent phase of a CAS.  Examples:
 * magnetisation in Ising, segregation index in Schelling,
 * flock polarisation in Boids (Vicsek).
 */
typedef struct {
    char     name[64];               /**< Descriptive name */
    double   current_value;          /**< Latest measurement */
    double   mean_value;             /**< Running average */
    double   variance;               /**< Running variance */
    double   min_value;              /**< Historical minimum */
    double   max_value;              /**< Historical maximum */
    uint64_t measurement_count;      /**< Number of samples */
    double   threshold_transition;   /**< Known phase-transition threshold */
} sfi_order_parameter_t;

/* ================================================================
 * L5: Emergence Detection Algorithms
 * ================================================================ */

/**
 * L5: Compute mutual information between micro and macro levels.
 *
 * Algorithm: histogram-based MI estimator with Freedman-Diaconis
 * bin-width selection.  Discretises microstate and macrostate
 * vectors, constructs joint histogram, computes
 *   I(M; F) = Sigma p(m,f) log2[p(m,f) / (p(m) p(f))].
 * Complexity: O(N * B) where B = number of bins.
 */
double sfi_mutual_information_micro_macro(
    const double *micro_data, uint64_t micro_dim, uint64_t n_samples,
    const double *macro_data, uint64_t macro_dim);

/**
 * L5: Compute transfer entropy T_{Y->X}.
 *
 * SFI key metric: tests Granger-causal influence between
 * variables in a model-free, non-parametric way.
 *   T_{Y->X} = I(X_t ; Y_past | X_past)
 * Complexity: O(N * k * B) where k = embedding dimension.
 */
double sfi_transfer_entropy(
    const double *src, const double *dst,
    uint64_t n_points, uint64_t embed_dim, uint64_t lag);

/**
 * L5: Fit power-law exponent (MLE) and test with KS statistic.
 *
 * SFI foundational observation: many CAS macro observables
 * (avalanche sizes, extinction events, city sizes) follow
 * power-laws.  This function fits alpha for p(x) ~ x^{-alpha}.
 * Complexity: O(N log N) ? sorting required.
 */
int sfi_fit_power_law(const double *data, uint64_t n,
                      double *alpha_out, double *xmin_out,
                      double *ks_stat_out);

/**
 * L5: Compute Hurst exponent via R/S analysis.
 *
 * Long-range memory is an SFI signature of self-organized
 * criticality.  H > 0.5 = persistent; H < 0.5 = anti-persistent.
 * Complexity: O(N^2) worst-case, O(N log N) with correction.
 */
double sfi_hurst_exponent_rs(const double *series, uint64_t n);

/**
 * L5: Compute Gini coefficient of an array.
 *
 * Measures inequality ? an emergent macro pattern that
 * arises from local agent interactions in Sugarscape
 * and El Farol.
 * Complexity: O(N log N).
 */
double sfi_gini_coefficient(const double *values, uint64_t n);

/**
 * L5: Multi-scale entropy (sample entropy) for complexity measurement.
 *
 * Costa et al. (2005) multiscale entropy ? SFI approach to
 * distinguishing noise from complex signals.
 * Complexity: O(N * scales * pattern_matching).
 */
double sfi_multiscale_entropy(const double *series, uint64_t n,
                              uint64_t max_scale, double r_tolerance);

/**
 * L5: Perform emergence detection integrating all metrics.
 *
 * Runs the full SFI emergence detection protocol.
 * Complexity: sum of sub-algorithm costs.
 */
void sfi_detect_emergence(const double *micro_data, uint64_t micro_dim,
                          const double *macro_data, uint64_t macro_dim,
                          uint64_t n_samples,
                          sfi_emergence_detector_t *result);

#ifdef __cplusplus
}
#endif

#endif /* SFI_EMERGENCE_H_ */
