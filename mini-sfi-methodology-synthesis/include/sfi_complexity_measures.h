/**
 * @file    sfi_complexity_measures.h
 * @brief   SFI Methodology: Complexity measures and quantification
 *
 * Implements the SFI canon of complexity measures: statistical
 * complexity (Crutchfield), Lempel-Ziv complexity, effective
 * complexity (Gell-Mann), logical depth (Bennett), and the
 * Langton lambda parameter for cellular automata.
 *
 * References: Crutchfield & Young (1989) "Inferring Statistical
 * Complexity"; Gell-Mann & Lloyd (1996) "Information Measures,
 * Effective Complexity, and Total Information"; Bennett (1988)
 * "Logical Depth and Physical Complexity"
 *
 * Knowledge Levels: L1 (Definitions), L3 (Math Structures),
 *                   L5 (Algorithms)
 */

#ifndef SFI_COMPLEXITY_MEASURES_H_
#define SFI_COMPLEXITY_MEASURES_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * L1: Complexity Measure Definitions
 * ================================================================ */

/**
 * L1: sfi_complexity_profile_t ? Multi-metric complexity profile
 *
 * SFI methodology: no single number captures "complexity."
 * We compute a vector of complementary measures.
 */
typedef struct {
    double   statistical_complexity;   /**< Crutchfield C_mu = H[S] */
    double   entropy_rate;             /**< h_mu ? Shannon entropy rate */
    double   excess_entropy;           /**< E = I[Past; Future] */
    double   lempel_ziv;               /**< Normalised LZ complexity */
    double   effective_complexity;     /**< Gell-Mann EC = total_info - R - N */
    double   logical_depth;            /**< Bennett ? time to compute from shortest program */
    double   kolmogorov_approx;        /**< Approximate KC via compression ratio */
    double   thermodynamic_depth;      /**< Lloyd-Pagels depth */
    uint32_t causal_states_found;      /**< Number of causal states (epsilon-machine) */
    double   fractal_dimension;        /**< Box-counting dimension of attractor */
    double   lyapunov_spectrum_max;    /**< Largest Lyapunov exponent */
    double   integration_tononi;       /**< Tononi's Phi (integrated information) */
} sfi_complexity_profile_t;

/**
 * L1: sfi_lambda_profile_t ? Langton ? parameter and derivatives
 *
 * Langton (1990) discovered that CA rule-space has a phase
 * transition at a critical ? value, analogous to the
 * edge of chaos.  ? is the fraction of rule-table entries
 * that map to non-quiescent states.
 */
typedef struct {
    double   lambda;                   /**< ? = fraction non-quiescent outputs */
    double   lambda_entropy;           /**< Shannon entropy of rule outputs */
    double   mutual_info_neighbours;   /**< Spatial MI between neighbour pairs */
    int32_t  ca_class;                 /**< Wolfram class (I-IV) estimate */
    double   transient_length;         /**< Mean transient before periodic */
    double   attractor_length;         /**< Mean attractor cycle length */
} sfi_lambda_profile_t;

/* ================================================================
 * L5: Complexity Computation Algorithms
 * ================================================================ */

/**
 * L5: Compute Lempel-Ziv complexity (LZ76).
 *
 * Parses the binary string into production components.
 * c(n) = number of distinct phrases.
 * Normalised: C_LZ = c(n) * log2(n) / n.
 * Complexity: O(n) using suffix-tree-like parsing.
 */
double sfi_lempel_ziv_complexity(const uint8_t *data, uint64_t n);

/**
 * L5: Compute approximate Kolmogorov complexity via gzip compression.
 *
 * K(x) ? |compress(x)|.  Practical approximation to the
 * uncomputable Kolmogorov complexity.
 * Complexity: O(n log n) dominated by deflate.
 */
double sfi_kolmogorov_approx_gzip(const uint8_t *data, uint64_t n);

/**
 * L5: Compute entropy rate of a symbolic sequence.
 *
 * h_mu = lim_{L??} H(L)/L using block entropies.
 * Complexity: O(n * max_block_size).
 */
double sfi_entropy_rate(const uint8_t *symbols, uint64_t n,
                        uint32_t alphabet_size, uint32_t max_block);

/**
 * L5: Compute statistical complexity (Crutchfield's C_mu).
 *
 * C_mu = H[S] where S is the causal state distribution.
 * Requires epsilon-machine reconstruction first.
 * Complexity: O(number_of_causal_states).
 */
double sfi_statistical_complexity(const double *causal_state_probs,
                                  uint32_t n_states);

/**
 * L5: Compute excess entropy (predictive information).
 *
 * E = I[Past; Future] = mutual information between
 * semi-infinite past and future.
 * Complexity: O(n * L?) for block-entropy method.
 */
double sfi_excess_entropy(const uint8_t *symbols, uint64_t n,
                          uint32_t alphabet_size, uint32_t max_block);

/**
 * L5: Compute Langton lambda for a 1-D CA rule.
 *
 * Given rule table (k^2r+1 entries), lambda = fraction of
 * entries that produce state != 0 (quiescent).
 * Complexity: O(k^{2r+1}).
 */
sfi_lambda_profile_t sfi_compute_lambda(const uint32_t *rule_table,
    uint32_t k, uint32_t r, uint32_t quiescent_state);

/**
 * L5: Estimate Wolfram CA class from spatio-temporal pattern.
 *
 * Class I:  ? homogeneous fixed point
 * Class II: ? simple periodic
 * Class III: ? chaotic aperiodic
 * Class IV: ? complex (long transients, persistent structures)
 * Complexity: O(W * T).
 */
int sfi_wolfram_class_estimate(const uint8_t *spacetime,
    uint32_t width, uint32_t time_steps);

/**
 * L5: Compute fractal dimension via box-counting.
 *
 * Applied to system attractor in embedded state space.
 * Complexity: O(N * log N).
 */
double sfi_box_counting_dimension(const double *points,
    uint64_t n_points, uint32_t embed_dim, uint32_t n_scales);

/**
 * L5: Compute largest Lyapunov exponent (Rosenstein algorithm).
 *
 * Complexity: O(N?) neighbours search + O(N) divergence tracking.
 */
double sfi_largest_lyapunov(const double *time_series, uint64_t n,
                            uint32_t embed_dim, uint32_t lag);

/**
 * L5: Compute full complexity profile.
 *
 * Integrates all complexity measures into a single profile.
 */
void sfi_complexity_profile_compute(
    const uint8_t *symbolic_data, uint64_t sym_len,
    const double *continuous_data, uint64_t cont_len,
    sfi_complexity_profile_t *profile);

#ifdef __cplusplus
}
#endif

#endif /* SFI_COMPLEXITY_MEASURES_H_ */
