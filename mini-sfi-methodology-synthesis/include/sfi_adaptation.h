/**
 * @file    sfi_adaptation.h
 * @brief   SFI Methodology: Adaptation, co-evolution, and fitness landscapes
 *
 * Implements the core SFI concept of adaptation on rugged fitness
 * landscapes (Kauffman's NK model), replicator dynamics from
 * evolutionary game theory, and classifier-system-style rule
 * adaptation (Holland).
 *
 * References: Kauffman "Origins of Order" (1993);
 * Holland "Adaptation in Natural and Artificial Systems" (1975);
 * Nowak "Evolutionary Dynamics" (2006)
 *
 * Knowledge Levels: L1 (Definitions), L2 (Concepts), L3 (Math),
 *                   L4 (Laws: NK complexity catastrophe), L5 (Algorithms)
 */

#ifndef SFI_ADAPTATION_H_
#define SFI_ADAPTATION_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * L1: Definitions ? Fitness Landscapes (Kauffman NK model)
 * ================================================================ */

/**
 * L1: sfi_nk_landscape_t ? NK rugged fitness landscape
 *
 * An N-locus genotype with K epistatic interactions per locus.
 * K = 0  ? Fujiyama landscape (smooth, single peak)
 * K = N-1 ? fully random landscape (uncorrelated, many peaks)
 * The "complexity catastrophe" at K ? N-1 is Kauffman's
 * central result (L4).
 */
typedef struct {
    uint32_t  N;                   /**< Number of loci (genotype length) */
    uint32_t  K;                   /**< Epistatic interactions per locus */
    double   *fitness_table;       /**< Lookup table: size 2^N * (K+1) */
    uint32_t *interaction_matrix;  /**< Which loci affect which: N * (K+1) */
    /* Pre-computed statistics */
    double    mean_fitness;        /**< Mean over all genotypes */
    double    std_fitness;         /**< Std dev over all genotypes */
    double    autocorr_1step;      /**< 1-step fitness autocorrelation */
    uint32_t  local_optima_count;  /**< Number of local fitness peaks */
} sfi_nk_landscape_t;

/**
 * L1: sfi_replicator_state_t ? Replicator dynamics state
 *
 * Tracks population fractions of n_strategies types,
 * evolving under payoff matrix A (n?n).
 *   dx_i/dt = x_i [ (Ax)_i - x^T A x ]
 */
typedef struct {
    uint32_t  n_strategies;
    double   *frequencies;         /**< Current population fractions */
    double   *payoffs;            /**< Current per-strategy payoff */
    double   *payoff_matrix;      /**< n?n game matrix (row vs col) */
    double    mean_population_payoff;
} sfi_replicator_state_t;

/**
 * L1: sfi_classifier_t ? Holland-style classifier system rule
 *
 * A condition-action rule: IF condition matches percept
 * THEN post action.  Rules compete via "bucket brigade"
 * or Q-learning-like credit assignment.
 */
typedef struct {
    uint32_t  id;
    char      condition[64];       /**< Bit-string condition (# = don't care) */
    char      action[64];          /**< Action bit-string */
    double    strength;            /**< Rule strength / fitness */
    double    specificity;         /**< Fraction of non-# bits in condition */
    uint64_t  fire_count;          /**< Times this rule fired */
    uint64_t  match_count;         /**< Times this rule matched */
    uint32_t  generation_born;     /**< Evolutionary time of creation */
} sfi_classifier_t;

/* ================================================================
 * L2: Adaptive Landscape Dynamics
 * ================================================================ */

/**
 * L2: sfi_landscape_dynamics_t ? Co-evolving fitness landscapes
 *
 * SFI key insight (Kauffman, Holland): in CAS, the fitness
 * landscape is NOT static.  Each agent's adaptation changes
 * the landscape for other agents (co-evolution).  This is
 * the "dancing fitness landscape" problem.
 */
typedef struct {
    sfi_nk_landscape_t **landscapes;  /**< One landscape per agent */
    uint32_t             n_agents;
    double              *correlation_matrix; /**< Agent-agent landscape correlation */
    uint32_t             update_count;      /**< Landscape deformation counter */
    double               average_ruggedness; /**< Mean K/(N-1) ratio */
} sfi_landscape_dynamics_t;

/* ================================================================
 * L5: API ? NK Landscape
 * ================================================================ */

/**
 * L5: Generate an NK fitness landscape.
 *
 * Random interaction matrix (each locus interacts with K others,
 * plus itself) and random fitness contributions.
 * Complexity: O(2^N * K).
 */
int sfi_nk_landscape_generate(sfi_nk_landscape_t *land,
                              uint32_t N, uint32_t K, uint64_t seed);

/**
 * L5: Free NK landscape resources.
 */
void sfi_nk_landscape_destroy(sfi_nk_landscape_t *land);

/**
 * L5: Evaluate fitness of a given genotype on the landscape.
 *
 * Genotype encoded as N-bit integer (0 to 2^N - 1).
 * Complexity: O(K) per evaluation.
 */
double sfi_nk_evaluate_fitness(const sfi_nk_landscape_t *land,
                               uint64_t genotype);

/**
 * L5: Adaptive walk on NK landscape ? greedy local search.
 *
 * Starting from genotype, flips one bit at a time, accepts
 * if fitness increases.  Stops at local peak.
 * Complexity: O(N * K * steps_to_peak).
 */
uint64_t sfi_nk_greedy_walk(const sfi_nk_landscape_t *land,
                            uint64_t start_genotype,
                            double *final_fitness,
                            uint32_t *steps_taken);

/**
 * L5: Count local optima on NK landscape.
 *
 * A genotype is a local optimum if all N single-bit mutants
 * have lower fitness.  Exhaustive for small N, sampled for N>20.
 * Complexity: O(2^N * N * K) exhaustive.
 */
uint32_t sfi_nk_count_local_optima(const sfi_nk_landscape_t *land,
                                   uint32_t sample_limit);

/**
 * L5: Compute fitness landscape autocorrelation.
 *
 * rho(d=1): correlation between 1-mutant neighbours.
 * High rho ? smooth landscape; low rho ? rugged.
 * Complexity: O(2^N).
 */
double sfi_nk_autocorrelation(const sfi_nk_landscape_t *land);

/* ---- Replicator Dynamics ? L5 ---- */

/**
 * L5: Initialise replicator dynamics with n strategies.
 *
 * Uniform initial frequencies.  Payoff matrix set externally.
 * Complexity: O(n).
 */
int sfi_replicator_init(sfi_replicator_state_t *rep, uint32_t n);

/**
 * L5: Free replicator state.
 */
void sfi_replicator_destroy(sfi_replicator_state_t *rep);

/**
 * L5: Single Euler step of the replicator equation.
 *
 * dx_i/dt = x_i [ (A x)_i - x^T A x ]
 * dt is the integration time step.
 * Complexity: O(n^2) ? matrix-vector multiply.
 */
void sfi_replicator_step(sfi_replicator_state_t *rep, double dt);

/**
 * L5: Check if replicator system is at a fixed point.
 *
 * Returns 1 if max |dx_i/dt| < tolerance.
 */
int sfi_replicator_is_fixed_point(const sfi_replicator_state_t *rep,
                                  double tolerance);

/**
 * L4: Holland's Schema Theorem ? compute schema fitness estimate.
 *
 * Given population of genotypes with fitness values,
 * estimates expected growth of schema H under selection,
 * crossover, and mutation.  Returns expected copies in
 * next generation.
 * Complexity: O(P * L) where P = population, L = genotype length.
 */
double sfi_schema_theorem_expected_copies(
    const uint64_t *population, const double *fitness,
    uint64_t pop_size, uint32_t genotype_len,
    uint32_t schema_mask, uint32_t schema_value,
    double crossover_rate, double mutation_rate);

/* ---- Classifier System ? L5 ---- */

/**
 * L5: Create a new classifier rule.
 *
 * Condition and action are bit-strings (0/1/#).
 * Complexity: O(len).
 */
sfi_classifier_t sfi_classifier_create(uint32_t id,
    const char *condition, const char *action);

/**
 * L5: Test if a classifier's condition matches a given percept.
 *
 * '#' is wildcard (matches 0 or 1).
 * Returns 1 if match, 0 otherwise.
 */
int sfi_classifier_matches(const sfi_classifier_t *cl,
                           const char *percept);

/**
 * L5: Update classifier strength via bucket-brigade algorithm.
 *
 * Core Holland classifier system credit assignment.
 * Winning classifier pays bid to previous winner.
 * Complexity: O(1).
 */
void sfi_classifier_bucket_brigade(sfi_classifier_t *winner,
                                   sfi_classifier_t *prev_winner,
                                   double bid_fraction, double reward);

#ifdef __cplusplus
}
#endif

#endif /* SFI_ADAPTATION_H_ */
