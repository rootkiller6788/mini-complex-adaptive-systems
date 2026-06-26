/**
 * @file alife_metrics.h
 * @brief Quantitative metrics for artificial life systems
 *
 * Provides computational tools to measure:
 *   - Emergence: how much system-level behavior exceeds agent-level prediction
 *   - Self-organization: increase in internal order without external direction
 *   - Complexity: balance between order and disorder (effective complexity)
 *   - Open-endedness: novelty production rate over evolutionary time
 *
 * These metrics operationalize the qualitative concepts from ALife research
 * into computable quantities suitable for simulation analysis.
 *
 * References:
 *   - Shalizi, C.R. (2001) "Causal Architecture, Complexity and Self-Organization"
 *   - Crutchfield, J.P. & Young, K. (1989) "Inferring Statistical Complexity"
 *   - Bedau, M.A. et al. (2000) "Open Problems in Artificial Life"
 *   - Gell-Mann, M. & Lloyd, S. (1996) "Information measures, effective
 *     complexity, and total information"
 *
 * Course alignment: Santa Fe Institute CSS, MIT 6.841, Princeton COS 551
 */

#ifndef ALIFE_METRICS_H
#define ALIFE_METRICS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* L1: Core Definitions */

/** Emergence metric result */
typedef struct {
    double emergence_score;       /**< 0 (none) to 1 (maximum emergence) */
    double self_org_score;        /**< self-organization score */
    double complexity_score;      /**< effective complexity */
    double novelty_rate;          /**< rate of novel pattern production */
    double predictability_gain;   /**< how much system-level info helps prediction */
} alife_emergence_t;

/** Entropy analysis for a single time slice */
typedef struct {
    double shannon_entropy;       /**< H(X) = -? p(x) log p(x) */
    double conditional_entropy;   /**< H(X|N) ? entropy given neighbor states */
    double mutual_info_space;     /**< I(X; N) ? mutual info with spatial neighbors */
    double mutual_info_time;      /**< I(X_t; X_{t-1}) ? temporal mutual information */
    double excess_entropy;        /**< E = lim_{L??} [H(L) - L?h_?] */
    double entropy_rate;          /**< h_? ? entropy rate per symbol */
    size_t n_unique_patterns;     /**< number of distinct local patterns observed */
} alife_entropy_t;

/** Statistical complexity (Crutchfield's ?-machine complexity) */
typedef struct {
    double statistical_complexity; /**< C_? = H[S] ? entropy of causal states */
    double entropy_rate_estimate;  /**< estimated h_? */
    size_t n_causal_states;        /**< number of inferred causal states */
    double predictive_information; /**< I(X_past; X_future) */
} alife_stat_complexity_t;

/** Configuration for metrics computation */
typedef struct {
    int history_length;          /**< how many past states to use */
    int pattern_radius;          /**< radius for local pattern extraction */
    int n_bootstrap_samples;     /**< bootstrap samples for confidence intervals */
    unsigned int seed;           /**< random seed for sampling */
    double significance_level;   /**< ? for statistical tests */
} alife_metrics_config_t;

/* L2: Core Concepts ? Information-Theoretic Metrics */

/** Compute Shannon entropy H = -? p_i log_2(p_i) from frequency array.
 *  @param freqs  frequency counts for each symbol
 *  @param n_symbols  number of distinct symbols
 *  @param total  total count ? freqs[i]
 *  @return entropy in bits */
double alife_shannon_entropy(const uint64_t *freqs, size_t n_symbols, uint64_t total);

/** Compute conditional entropy H(Y|X) from joint and marginal frequencies */
double alife_conditional_entropy(const uint64_t *joint_freqs,
                                  const uint64_t *marginal_freqs_x,
                                  size_t n_x, size_t n_y, uint64_t total);

/** Compute mutual information I(X;Y) from joint and marginal distributions */
double alife_mutual_information(const uint64_t *joint_freqs,
                                 const uint64_t *marg_x, const uint64_t *marg_y,
                                 size_t n_x, size_t n_y, uint64_t total);

/** Compute KL divergence D_KL(P||Q) */
double alife_kl_divergence(const double *p, const double *q, size_t n);

/** Compute Jensen-Shannon divergence JSD(P||Q) = (D_KL(P||M) + D_KL(Q||M))/2 */
double alife_js_divergence(const double *p, const double *q, size_t n);

/* L5: Emergence and Self-Organization Metrics */

/**
 * Compute emergence score from agent-level and system-level models.
 * Emergence = I(System; Future | Agent_predictions) ?
 * the information the system state provides about the future beyond
 * what individual agent states can predict.
 *
 * @param agent_preds  predictability using agent-level model (R^2 or similar)
 * @param system_preds predictability using system-level model
 * @return emergence score (0 = reducible to agents, 1 = strongly emergent)
 */
double alife_emergence_measure(double agent_preds, double system_preds);

/**
 * Compute full emergence analysis on CA state history.
 * Analyzes multiple aspects: entropy evolution, mutual information,
 * excess entropy, and novelty production rate.
 */
alife_emergence_t alife_analyze_emergence(const uint8_t **state_history,
                                           size_t n_states,
                                           int64_t width, int64_t height,
                                           const alife_metrics_config_t *cfg);

/** Compute self-organization score: increase in mutual information
 *  between system components over time, normalized by entropy. */
double alife_self_organization(const uint8_t **state_history, size_t n_states,
                                int64_t width, int64_t height);

/** Compute Tononi's ? (integrated information) approximation for a grid.
 *  ? ? effective information across the minimum information partition.
 *  Lower bound using pairwise mutual information integration. */
double alife_phi_integrated_info(const uint8_t *state,
                                  int64_t width, int64_t height,
                                  int num_states);

/* L4: Complexity Measures ? Theoretical Guarantees */

/**
 * Compute effective complexity (Gell-Mann & Lloyd 1996).
 * EC = total information - (random + redundant information).
 * Approximated as mutual information between past and future
 * (predictive information / excess entropy).
 */
double alife_effective_complexity(const uint8_t **state_history, size_t n_states,
                                   int64_t width, int64_t height);

/** Compute Lempel-Ziv complexity (normalized).
 *  Approximates Kolmogorov complexity from below using LZ78 parsing. */
double alife_lempel_ziv_complexity(const uint8_t *data, size_t length);

/** Compute thermodynamic depth: minimal computation needed to produce state.
 *  Approximated as number of non-redundant computation steps. */
double alife_thermodynamic_depth(const uint8_t **state_history, size_t n_states,
                                  int64_t width, int64_t height);

/** Compute Crutchfield's statistical complexity from CA data.
 *  Reconstructs ?-machine from time series via causal state splitting. */
alife_stat_complexity_t alife_statistical_complexity_calc(
    const uint8_t **state_history, size_t n_states,
    int64_t width, int64_t height, int num_states,
    const alife_metrics_config_t *cfg);

/* L8: Advanced ALife Metrics */

/** Compute novelty rate: fraction of patterns at time t not seen before time t.
 *  Uses approximate pattern hashing with Bloom filter for efficiency. */
double alife_novelty_rate(const uint8_t **state_history, size_t n_states,
                           int64_t width, int64_t height, int pattern_radius);

/** Detect "open-ended evolution" signature:
 *  novelty rate bounded away from zero over long time scales. */
bool alife_detect_open_ended(const uint8_t **state_history, size_t n_states,
                              int64_t width, int64_t height,
                              double *novelty_trend);

/** Compute Bar-Yam's multi-scale complexity: sum of mutual information
 *  across all spatial scales, normalized. */
double alife_multiscale_complexity(const uint8_t *state,
                                    int64_t width, int64_t height,
                                    int max_scale);

/** Compute the "ALife test" metric (Bedau et al.): does the system
 *  exhibit (1) emergence, (2) self-organization, (3) adaptation,
 *  (4) open-endedness, and (5) life-like behavior? */
void alife_comprehensive_test(const uint8_t **state_history, size_t n_states,
                               int64_t width, int64_t height,
                               const alife_metrics_config_t *cfg,
                               double scores_out[5]);

#endif /* ALIFE_METRICS_H */