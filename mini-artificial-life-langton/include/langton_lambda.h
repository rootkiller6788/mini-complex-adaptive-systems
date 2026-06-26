/**
 * @file langton_lambda.h
 * @brief Langton's Lambda Parameter ? quantifying the edge of chaos
 *
 * Chris Langton (1990) introduced the ? parameter as a scalar measure
 * of cellular automaton rule space. ? is defined as the fraction of
 * rule table entries that map to a non-quiescent (non-zero) state.
 *
 * Key result: For a broad class of CA, complex computation (Wolfram
 * Class IV) emerges near a critical ? value, the "edge of chaos."
 *
 *   ?_c ? 0.273 for k=2, N=5 (2-state, 5-neighbor CA)
 *
 * This file provides tools for:
 *   - Computing ? for arbitrary CA rules
 *   - Sampling the rule space at fixed ?
 *   - Analyzing the order-chaos phase transition
 *
 * References:
 *   - Langton, C.G. (1990) "Computation at the edge of chaos"
 *   - Mitchell, M. et al. (1993) "Revisiting the edge of chaos"
 *   - Packard, N.H. (1988) "Adaptation toward the edge of chaos"
 *
 * Course alignment: Santa Fe Institute CSS, Berkeley CS278, CMU 15-855
 */

#ifndef LANGTON_LAMBDA_H
#define LANGTON_LAMBDA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cellular_automaton.h"

/* L1: Core Definitions */

/** Lambda configuration for rule space exploration */
typedef struct {
    int num_states;           /**< k ? alphabet size (typically 2) */
    int num_inputs;           /**< N ? neighborhood size (number of input cells) */
    int quiescent_state;      /**< s_q ? the "rest" state (typically 0) */
    double lambda;            /**< target ? value ? [0, 1] */
    size_t table_size;        /**< k^N ? total entries in rule table */
    size_t non_quiescent_n;   /**< number of non-quiescent outputs (? * table_size) */
} lambda_config_t;

/** A single CA rule as a rule table with ? annotation */
typedef struct {
    lambda_config_t config;   /**< ? configuration */
    uint8_t *rule_table;      /**< rule output for each input neighborhood */
    double actual_lambda;     /**< actual ? computed from table */
    double entropy;           /**< Shannon entropy of rule outputs */
    double z_parameter;       /**< Z = fraction of "sensitive" entries (Packard) */
} lambda_rule_t;

/** Phase transition analysis result */
typedef struct {
    double lambda;            /**< ? value */
    double avg_entropy;       /**< average spatial entropy over sampled rules */
    double std_entropy;       /**< std deviation of entropy */
    double avg_mutual_info;   /**< average mutual information */
    double avg_damage_spread; /**< average damage spread (Lyapunov proxy) */
    double transient_length;  /**< average transient length before stabilization */
    ca_wolfram_class_t dominant_class; /**< most common Wolfram class at this ? */
} lambda_phase_point_t;

/* L2: Core Concepts ? Lambda Computation */

/** Compute ? for a given rule table.
 *  ? = (k^N - n_q) / k^N where n_q = count of quiescent output entries.
 *  For rules with multiple quiescent states: ? = fraction of non-quiescent outputs. */
double lambda_compute(const uint8_t *rule_table, size_t table_size,
                      int num_states, int quiescent_state);

/** Compute Z parameter (Packard 1988): fraction of rule table entries
 *  that are "sensitive" ? where flipping one input bit changes the output. */
double lambda_z_parameter(const uint8_t *rule_table, size_t table_size,
                          int num_states, int num_inputs);

/** Compute rule table entropy H = -? p_i log_k(p_i) where p_i = frequency of output i */
double lambda_rule_entropy(const uint8_t *rule_table, size_t table_size, int num_states);

/* L5: Rule Generation Algorithms */

/** Generate a random rule table conditioned on target ?.
 *  Uses Fisher-Yates shuffle on output assignments.
 *  O(table_size). */
lambda_rule_t *lambda_rule_generate(const lambda_config_t *config, unsigned int seed);

/** Free lambda_rule_t */
void lambda_rule_destroy(lambda_rule_t *rule);

/** Generate n independent rules at fixed ? and compute statistics */
lambda_phase_point_t lambda_sample_phase(const lambda_config_t *config,
                                          size_t n_samples, unsigned int seed);

/** Generate rules at ? values sweeping from 0 to 1 in n_points steps.
 *  Returns phase transition curve. Used to locate the edge of chaos. */
lambda_phase_point_t *lambda_sweep_phase(const lambda_config_t *base_config,
                                          int n_points, size_t n_samples_per_point,
                                          unsigned int seed);

/** Generate rule tables at specific ? by constrained random walk in rule space */
lambda_rule_t *lambda_rule_generate_walk(const lambda_config_t *config,
                                          unsigned int seed, int walk_steps);

/** Mutate a lambda rule, preserving ? exactly.
 *  Swaps two output entries: one quiescent, one non-quiescent.
 *  O(1) if indices are tracked, O(table_size) otherwise. */
bool lambda_rule_mutate_preserve_lambda(lambda_rule_t *rule);

/* L4: Edge of Chaos Detection */

/** Detect edge of chaos from phase transition data.
 *  The edge of chaos ?_c is identified as the ? where:
 *    d(entropy)/d? is maximized (steepest slope of phase transition)
 *    AND transient length peaks.
 *  O(n_points) on the sweep data. */
double lambda_detect_edge_of_chaos(const lambda_phase_point_t *sweep, int n_points);

/** Compute the order parameter for a rule: normalized entropy.
 *  Near 0 = ordered (Class I/II), near 1 = chaotic (Class III),
 *  intermediate ? 0.5 = complex (Class IV). */
double lambda_order_parameter(const lambda_phase_point_t *point);

/** Test the "edge of chaos" hypothesis:
 *  H0: Complex behavior (Class IV) is equally likely at all ?.
 *  This function computes the conditional probability P(Class IV | ?)
 *  and tests if it peaks at intermediate ?. */
bool lambda_test_edge_hypothesis(const lambda_phase_point_t *sweep, int n_points,
                                  double *peak_lambda, double *peak_probability);

/* L8: Advanced Lambda Analysis */

/** Compute ?-entropy phase diagram: 2D histogram of (?, entropy) pairs */
void lambda_entropy_phase_diagram(const lambda_phase_point_t *sweep, int n_points,
                                   double *lambda_bins, double *entropy_bins,
                                   int n_bins, double **histogram_out);

/** Find the critical exponent ? of the order parameter near ?_c:
 *    order_parameter(?) ~ |? - ?_c|^?
 *  Uses linear regression on log-log scale. */
double lambda_critical_exponent(const lambda_phase_point_t *sweep, int n_points,
                                 double lambda_c);

/** Compute mutual information between rule table entries and CA dynamics.
 *  Tests whether ? alone captures all order-chaos information. */
double lambda_mutual_info_rule_dynamics(const lambda_rule_t *rule,
                                         const uint8_t *evolved_states,
                                         size_t n_cells, int n_steps);

/** Generate the "? landscape": for each possible rule at k=2, N=5,
 *  compute ? and classify. Validates Langton's original experiment. */
void lambda_landscape_exhaustive(int num_states, int num_inputs,
                                  lambda_phase_point_t **landscape_out,
                                  size_t *n_rules_out);

#endif /* LANGTON_LAMBDA_H */