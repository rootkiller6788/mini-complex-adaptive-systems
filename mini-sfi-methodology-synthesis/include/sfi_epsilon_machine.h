/**
 * @file    sfi_epsilon_machine.h
 * @brief   SFI Methodology: Computational mechanics and epsilon-machines
 *
 * Implements Crutchfield's computational mechanics framework
 * for discovering patterns and causal architecture in complex
 * systems.  The epsilon-machine is a minimal, optimal predictor
 * of a stochastic process; its states (causal states) group
 * histories that yield identical future distributions.
 *
 * References: Crutchfield (1994) "The Calculi of Emergence";
 * Shalizi & Crutchfield (2001) "Computational Mechanics: Pattern
 * and Prediction"; Crutchfield (2012) "Between Order and Chaos"
 *
 * Knowledge Levels: L1 (Definitions), L3 (Math), L4 (Laws),
 *                   L5 (Algorithms), L8 (Advanced)
 */

#ifndef SFI_EPSILON_MACHINE_H_
#define SFI_EPSILON_MACHINE_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * L1: Definitions ? Causal States and epsilon-Machines
 * ================================================================ */

/**
 * L1: sfi_causal_state_t ? A causal state in the epsilon-machine
 *
 * Groups all histories that have the same conditional
 * distribution over futures.  This is the fundamental
 * equivalence relation of computational mechanics:
 *   h1 ~_epsilon h2  iff  P(Future | h1) = P(Future | h2)
 *
 * The set of causal states S forms a Markov chain ? the
 * epsilon-machine is the minimal sufficient statistic for
 * predicting the process.
 */
typedef struct {
    uint32_t  id;                      /**< Unique state identifier */
    uint32_t  n_histories;             /**< Number of histories in this state */
    double   *future_distribution;     /**< P(future | state) ? length = max_future_len */
    uint32_t  future_len;              /**< Length of future distribution vector */
    double    stationary_prob;         /**< pi(state) in stationary distribution */
    uint32_t *transition_targets;      /**< Deterministic transition targets, size=alphabet */
    double   *transition_probs;        /**< Transition probabilities, size=alphabet */
    uint32_t  alphabet_size;           /**< Size of symbol alphabet */
    double    statistical_complexity_contrib; /**< -pi*log(pi) contribution to C_mu */
} sfi_causal_state_t;

/**
 * L1: sfi_epsilon_machine_t ? Complete epsilon-machine
 *
 * The epsilon-machine is the minimal, deterministic Hidden
 * Markov Model that perfectly predicts the process.
 *
 * Key properties (L4):
 *   - Unifilarity: given state s and symbol a, next state is unique
 *   - Minimality:  any other predictor with same predictive power
 *                  has ? number of states
 *   - C_mu = H[S]  (statistical complexity = entropy of states)
 */
typedef struct {
    sfi_causal_state_t *states;        /**< Array of causal states */
    uint32_t             n_states;     /**< Number of distinct causal states */
    uint32_t             alphabet_size;/**< Symbols are {0..alphabet_size-1} */
    uint32_t            *start_state;  /**< Index of start state (empty history) */
    double               entropy_rate; /**< h_mu = conditioned entropy rate */
    double               statistical_complexity; /**< C_mu = H[S] */
    double               excess_entropy;         /**< E = I[Past; Future] */
    /* L4: Machine properties */
    int32_t              is_minimal;    /**< 1 if no state merging possible */
    int32_t              is_unifilar;   /**< 1 if deterministic transitions */
    uint32_t             recurrent_states; /**< Number of recurrent states */
    uint32_t             transient_states; /**< Number of transient states */
} sfi_epsilon_machine_t;

/**
 * L1: sfi_cssr_config_t ? CSSR algorithm configuration
 *
 * CSSR (Causal-State Splitting Reconstruction) is Shalizi's
 * algorithm for inferring epsilon-machines from data.
 */
typedef struct {
    uint32_t  max_history_len;         /**< L: maximum history length to consider */
    uint32_t  max_future_len;          /**< M: maximum future length */
    double    significance_level;      /**< Alpha for chi-squared test (0.01 default) */
    uint32_t  min_samples_per_state;   /**< Minimum histories before splitting */
    int32_t   use_chi_square;          /**< 1 = chi-square, 0 = Kolmogorov-Smirnov */
    int32_t   verbose;                 /**< 1 = print reconstruction progress */
} sfi_cssr_config_t;

/* ================================================================
 * L3: Mathematical Structures ? Causal Architecture
 * ================================================================ */

/**
 * L3: sfi_causal_architecture_t ? Higher-level causal structure
 *
 * Beyond individual causal states, this captures the
 * architecture of the epsilon-machine: crypticity,
 * oracular information, gauge information, and the
 * complexity-entropy diagram position.
 */
typedef struct {
    double   statistical_complexity;   /**< C_mu = H[S] */
    double   entropy_rate;             /**< h_mu */
    double   crypticity;              /**< Chi = I[S_{past}; S_{future} | S_{present}] */
    double   oracular_information;     /**< Information about future beyond next symbol */
    double   gauge_information;        /**< Information in state that is not predictive */
    double   bound_information;       /**< I[S_{past}; S_{present}] */
    /* Diagram position ? L4 */
    double   complexity_entropy_ratio; /**< C_mu / h_mu ? position in C-H diagram */
    int32_t  phase;                    /**< 0=periodic, 1=chaotic, 2=complex, 3=random */
} sfi_causal_architecture_t;

/* ================================================================
 * L5: CSSR Algorithm API
 * ================================================================ */

/**
 * L5: CSSR ? Causal-State Splitting Reconstruction.
 *
 * The core SFI computational mechanics algorithm.  Given a
 * symbolic time series, reconstructs the minimal epsilon-machine.
 *
 * Algorithm (Shalizi & Crutchfield 2001):
 *   1. Initialise all histories of length L in one state.
 *   2. For each state, test if future distributions differ
 *      for histories after appending each symbol.
 *   3. If significantly different (chi-squared test), split.
 *   4. Determinise transitions (unifilarity property).
 *   5. Recur until no more splits.
 *
 * Complexity: O(N * A^L * M * |S|) where A=alphabet, L=max_hist,
 *             M=max_future, |S|=final states.
 */
int sfi_cssr_reconstruct(const uint8_t *data, uint64_t n,
                         uint32_t alphabet_size,
                         const sfi_cssr_config_t *config,
                         sfi_epsilon_machine_t *machine);

/**
 * L5: Free all resources of an epsilon-machine.
 */
void sfi_epsilon_machine_destroy(sfi_epsilon_machine_t *machine);

/**
 * L5: Compute stationary distribution of the epsilon-machine.
 *
 * Solves pi * T = pi, sum(pi)=1 for the transition matrix T.
 * Complexity: O(|S|^3) via eigendecomposition.
 */
int sfi_epsilon_machine_stationary(sfi_epsilon_machine_t *machine);

/**
 * L5: Compute causal architecture from epsilon-machine.
 *
 * Derives crypticity, oracular information, gauge information.
 * Complexity: O(|S| * A).
 */
int sfi_causal_architecture_compute(
    const sfi_epsilon_machine_t *machine,
    sfi_causal_architecture_t *arch);

/**
 * L5: Simulate the epsilon-machine forward for n_steps.
 *
 * Generates a synthetic sequence from the machine.
 * Complexity: O(n_steps).
 */
int sfi_epsilon_machine_simulate(const sfi_epsilon_machine_t *machine,
                                 uint64_t n_steps, uint64_t seed,
                                 uint8_t *output);

/**
 * L5: Merge two epsilon-machines (if they describe correlated processes).
 *
 * Returns a product machine that jointly describes both
 * processes.  Used for discovering inter-process interactions.
 * Complexity: O(|S1| * |S2| * A).
 */
int sfi_epsilon_machine_merge(const sfi_epsilon_machine_t *m1,
                              const sfi_epsilon_machine_t *m2,
                              sfi_epsilon_machine_t *merged);

/**
 * L4: Test unifilarity property.
 *
 * An epsilon-machine is unifilar if, for each state s and
 * symbol a, there is at most one next state.
 * Complexity: O(|S| * A).
 */
int sfi_epsilon_machine_is_unifilar(const sfi_epsilon_machine_t *machine);

/**
 * L4: Minimise epsilon-machine by merging bisimilar states.
 *
 * Two states are bisimilar if they have identical future
 * distributions.  Complexity: O(|S|? * M).
 */
int sfi_epsilon_machine_minimise(sfi_epsilon_machine_t *machine);

#ifdef __cplusplus
}
#endif

#endif /* SFI_EPSILON_MACHINE_H_ */
