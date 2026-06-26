/**
 * @file langton_lambda.c
 * @brief Langton's Lambda parameter computation and edge-of-chaos analysis
 *
 * Implements:
 *   - Lambda (?) computation: fraction of non-quiescent rule table entries
 *   - Z parameter (Packard 1988): sensitivity measure
 *   - Rule generation conditioned on ? value
 *   - Phase transition sampling and edge-of-chaos detection
 *   - ?-entropy phase diagrams
 *
 * Key result: Complex computation (Wolfram Class IV) emerges at ?_c ? 0.273
 * for k=2, N=5 cellular automata. This is the "edge of chaos" hypothesis.
 *
 * Theorem (Langton 1990): The ? parameter orders CA rule space from
 *   Order (??0) ? Complexity (???_c) ? Chaos (??1).
 *
 * Note on critical value: Subsequent work (Mitchell et al. 1993) showed
 * that ? is correlated with but not sufficient for identifying complex CA.
 * The relationship is statistical, not deterministic.
 *
 * Course alignment: SFI CSS, CMU 15-855, Berkeley CS278
 */

#include "langton_lambda.h"
#include "cellular_automaton.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*??????????????????????????????????????????????????????????????????????
 * L2: Lambda Computation
 *??????????????????????????????????????????????????????????????????????*/

double lambda_compute(const uint8_t *rule_table, size_t table_size,
                      int num_states, int quiescent_state)
{
    if (!rule_table || table_size == 0) return 0.0;

    size_t n_quiescent = 0;
    for (size_t i = 0; i < table_size; i++) {
        if ((int)rule_table[i] == quiescent_state) {
            n_quiescent++;
        }
    }

    /* ? = (k^N - n_q) / k^N = fraction non-quiescent */
    return (double)(table_size - n_quiescent) / (double)table_size;
}

double lambda_z_parameter(const uint8_t *rule_table, size_t table_size,
                          int num_states, int num_inputs)
{
    if (!rule_table || table_size == 0 || num_inputs <= 0) return 0.0;

    /* Z = fraction of rule table entries that are "sensitive":
     * changing one input bit changes the output.
     *
     * For each table entry, test all single-bit flips of the input.
     * If any flip changes output, the entry is "sensitive." */
    size_t n_sensitive = 0;

    for (size_t i = 0; i < table_size; i++) {
        uint8_t base_output = rule_table[i];
        bool sensitive = false;

        /* Try flipping each input bit */
        for (int bit = 0; bit < num_inputs && !sensitive; bit++) {
            size_t flipped_idx = i ^ ((size_t)1 << bit);
            if (flipped_idx < table_size) {
                if (rule_table[flipped_idx] != base_output) {
                    sensitive = true;
                }
            }
        }

        if (sensitive) n_sensitive++;
    }

    return (double)n_sensitive / (double)table_size;
}

double lambda_rule_entropy(const uint8_t *rule_table, size_t table_size, int num_states)
{
    if (!rule_table || table_size == 0 || num_states <= 0) return 0.0;

    /* Count frequency of each output state */
    uint64_t *freqs = (uint64_t *)calloc((size_t)num_states, sizeof(uint64_t));
    if (!freqs) return 0.0;

    for (size_t i = 0; i < table_size; i++) {
        if (rule_table[i] < (uint8_t)num_states) {
            freqs[rule_table[i]]++;
        }
    }

    /* H = -? p_i log_k(p_i) ? entropy in base-k units */
    double entropy = 0.0;
    double logk = log((double)num_states);
    if (logk < 1e-10) { free(freqs); return 0.0; }

    for (int s = 0; s < num_states; s++) {
        if (freqs[s] > 0) {
            double p = (double)freqs[s] / (double)table_size;
            entropy -= p * log(p) / logk;
        }
    }

    free(freqs);
    return entropy;
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Rule Generation via Fisher-Yates on Output Assignments
 *
 * Method: Create a rule table with exactly n_nonzero non-quiescent outputs,
 * randomly permuted using Fisher-Yates shuffle.
 *
 * This generates rules uniformly at random from the set of all rules
 * with a given ? value (the ?-constrained rule ensemble).
 *??????????????????????????????????????????????????????????????????????*/

lambda_rule_t *lambda_rule_generate(const lambda_config_t *config, unsigned int seed)
{
    if (!config || config->table_size == 0) return NULL;

    lambda_rule_t *rule = (lambda_rule_t *)calloc(1, sizeof(lambda_rule_t));
    if (!rule) return NULL;

    rule->config = *config;
    rule->rule_table = (uint8_t *)calloc(config->table_size, sizeof(uint8_t));
    if (!rule->rule_table) {
        free(rule);
        return NULL;
    }

    /* Fill rule table: first n_nonzero entries = random non-quiescent state,
     * remaining entries = quiescent state */
    size_t n_nq = config->non_quiescent_n;
    if (n_nq > config->table_size) n_nq = config->table_size;

    srand(seed);

    for (size_t i = 0; i < config->table_size; i++) {
        if (i < n_nq) {
            /* Random non-quiescent output state in [1, num_states-1] */
            int s = 1 + (rand() % (config->num_states - 1));
            rule->rule_table[i] = (uint8_t)s;
        } else {
            rule->rule_table[i] = (uint8_t)config->quiescent_state;
        }
    }

    /* Fisher-Yates shuffle to randomize positions */
    for (size_t i = config->table_size - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        uint8_t tmp = rule->rule_table[i];
        rule->rule_table[i] = rule->rule_table[j];
        rule->rule_table[j] = tmp;
    }

    /* Compute actual ? and statistics */
    rule->actual_lambda = lambda_compute(rule->rule_table, config->table_size,
                                          config->num_states, config->quiescent_state);
    rule->entropy = lambda_rule_entropy(rule->rule_table, config->table_size,
                                         config->num_states);
    rule->z_parameter = lambda_z_parameter(rule->rule_table, config->table_size,
                                            config->num_states, config->num_inputs);

    return rule;
}

void lambda_rule_destroy(lambda_rule_t *rule)
{
    if (rule) {
        free(rule->rule_table);
        free(rule);
    }
}

lambda_rule_t *lambda_rule_generate_walk(const lambda_config_t *config,
                                          unsigned int seed, int walk_steps)
{
    if (!config) return NULL;

    /* Start from a randomly generated rule */
    lambda_rule_t *rule = lambda_rule_generate(config, seed);
    if (!rule) return NULL;

    /* Perform random walk preserving ? */
    srand(seed + 1);
    for (int step = 0; step < walk_steps; step++) {
        lambda_rule_mutate_preserve_lambda(rule);
    }

    /* Recompute statistics */
    rule->actual_lambda = lambda_compute(rule->rule_table, config->table_size,
                                          config->num_states, config->quiescent_state);
    rule->entropy = lambda_rule_entropy(rule->rule_table, config->table_size,
                                         config->num_states);

    return rule;
}

bool lambda_rule_mutate_preserve_lambda(lambda_rule_t *rule)
{
    if (!rule || !rule->rule_table || rule->config.table_size < 2) return false;

    /* Swap one quiescent entry with one non-quiescent entry.
     * This preserves ? exactly (same count of each type). */
    size_t n = rule->config.table_size;
    int qs = rule->config.quiescent_state;

    /* Find a random quiescent entry */
    size_t qi;
    int tries = 0;
    do {
        qi = (size_t)(rand() % n);
        tries++;
    } while ((int)rule->rule_table[qi] != qs && tries < 1000);
    if ((int)rule->rule_table[qi] != qs) return false;

    /* Find a random non-quiescent entry */
    size_t ni;
    tries = 0;
    do {
        ni = (size_t)(rand() % n);
        tries++;
    } while ((int)rule->rule_table[ni] == qs && tries < 1000);
    if ((int)rule->rule_table[ni] == qs) return false;

    /* Swap */
    uint8_t tmp = rule->rule_table[qi];
    rule->rule_table[qi] = rule->rule_table[ni];
    rule->rule_table[ni] = tmp;

    return true;
}

/*??????????????????????????????????????????????????????????????????????
 * L4: Phase Transition and Edge of Chaos Detection
 *??????????????????????????????????????????????????????????????????????*/

lambda_phase_point_t lambda_sample_phase(const lambda_config_t *config,
                                          size_t n_samples, unsigned int seed)
{
    lambda_phase_point_t result;
    memset(&result, 0, sizeof(result));
    result.lambda = config->lambda;
    result.dominant_class = CA_CLASS_UNKNOWN;

    double sum_entropy = 0.0;
    double sum_mi = 0.0;
    double sum_damage = 0.0;
    double sum_transient = 0.0;
    int class_counts[5] = {0};
    size_t valid_samples = 0;

    for (size_t i = 0; i < n_samples; i++) {
        lambda_rule_t *rule = lambda_rule_generate(config, seed + (unsigned int)(i * 137 + 1));
        if (!rule) continue;

        /* Compute rule entropy as a proxy for spatial entropy */
        double ent = rule->entropy;
        sum_entropy += ent;

        /* Use z-parameter as mutual information proxy */
        sum_mi += rule->z_parameter;

        /* Damage spread proxy: for ? near 0.5, expect maximal spread */
        double damage_proxy = 1.0 - fabs(config->lambda - 0.5) * 2.0;
        sum_damage += damage_proxy;

        /* Transient length estimate: ~ exp(entropy * table_size * constant) */
        double trans = exp(ent * log((double)config->table_size));
        sum_transient += trans;

        /* Classify based on ? and entropy */
        if (config->lambda < 0.15) {
            class_counts[0]++; /* Class I */
        } else if (config->lambda < 0.25 && ent < 0.6) {
            class_counts[1]++; /* Class II */
        } else if (config->lambda > 0.85) {
            class_counts[3]++; /* Class III */
        } else if (ent > 0.4 && ent < 0.8) {
            class_counts[2]++; /* Class IV candidate */
        } else {
            class_counts[3]++; /* Class III */
        }

        valid_samples++;
        lambda_rule_destroy(rule);
    }

    if (valid_samples > 0) {
        result.avg_entropy = sum_entropy / (double)valid_samples;
        result.avg_mutual_info = sum_mi / (double)valid_samples;
        result.avg_damage_spread = sum_damage / (double)valid_samples;
        result.transient_length = sum_transient / (double)valid_samples;

        /* Find dominant class */
        int max_class = 0;
        for (int c = 0; c < 4; c++) {
            if (class_counts[c] > class_counts[max_class]) max_class = c;
        }
        result.dominant_class = (ca_wolfram_class_t)max_class;
    }

    return result;
}

lambda_phase_point_t *lambda_sweep_phase(const lambda_config_t *base_config,
                                          int n_points, size_t n_samples_per_point,
                                          unsigned int seed)
{
    if (!base_config || n_points <= 0) return NULL;

    lambda_phase_point_t *sweep = (lambda_phase_point_t *)calloc(
        (size_t)n_points, sizeof(lambda_phase_point_t));
    if (!sweep) return NULL;

    for (int i = 0; i < n_points; i++) {
        lambda_config_t cfg = *base_config;
        cfg.lambda = (double)i / (double)(n_points - 1); /* sweep [0, 1] */
        cfg.non_quiescent_n = (size_t)(cfg.lambda * (double)cfg.table_size);
        if (cfg.non_quiescent_n > cfg.table_size) cfg.non_quiescent_n = cfg.table_size;

        sweep[i] = lambda_sample_phase(&cfg, n_samples_per_point,
                                        seed + (unsigned int)(i * 997 + 1));
    }

    return sweep;
}

double lambda_detect_edge_of_chaos(const lambda_phase_point_t *sweep, int n_points)
{
    if (!sweep || n_points < 3) return 0.5;

    /* Find ? where d(entropy)/d? is maximized.
     * This is the steepest part of the order-chaos phase transition.
     *
     * Also compute where transient length peaks (indicates critical slowing down). */
    double max_deriv = 0.0;
    double best_lambda = 0.5;

    for (int i = 1; i < n_points - 1; i++) {
        double dlambda = sweep[i+1].lambda - sweep[i-1].lambda;
        if (dlambda < 1e-10) continue;
        double dentropy = sweep[i+1].avg_entropy - sweep[i-1].avg_entropy;
        double deriv = dentropy / dlambda;

        /* Weight by transient length (critical slowing down indicator) */
        double weight = 1.0 + log(1.0 + sweep[i].transient_length);
        double score = deriv * weight;

        if (score > max_deriv) {
            max_deriv = score;
            best_lambda = sweep[i].lambda;
        }
    }

    return best_lambda;
}

double lambda_order_parameter(const lambda_phase_point_t *point)
{
    if (!point) return 0.0;
    /* Order parameter = 1 - normalized entropy.
     * Near 0 = chaotic, near 1 = ordered. */
    return 1.0 - point->avg_entropy;
}

bool lambda_test_edge_hypothesis(const lambda_phase_point_t *sweep, int n_points,
                                  double *peak_lambda, double *peak_probability)
{
    if (!sweep || n_points < 2) return false;

    /* Compute P(Class IV | ?) for each ? value.
     * Edge hypothesis: this probability peaks at intermediate ?. */
    double max_prob = 0.0;
    double best_l = 0.5;

    for (int i = 0; i < n_points; i++) {
        /* Class IV probability proxy: entropy near 0.5, diverse behavior */
        double ent = sweep[i].avg_entropy;
        /* Probability of complexity: peaked when entropy is intermediate */
        double prob = 4.0 * ent * (1.0 - ent); /* parabola peaking at ent=0.5 */
        if (prob > max_prob) {
            max_prob = prob;
            best_l = sweep[i].lambda;
        }
    }

    if (peak_lambda) *peak_lambda = best_l;
    if (peak_probability) *peak_probability = max_prob;

    /* Hypothesis supported if peak probability > 0.5 (entropy near 0.5) */
    return (max_prob > 0.5);
}

/*??????????????????????????????????????????????????????????????????????
 * L8: Advanced Lambda Analysis
 *??????????????????????????????????????????????????????????????????????*/

void lambda_entropy_phase_diagram(const lambda_phase_point_t *sweep, int n_points,
                                   double *lambda_bins, double *entropy_bins,
                                   int n_bins, double **histogram_out)
{
    if (!sweep || n_points <= 0 || n_bins <= 0 || !histogram_out) return;

    /* Allocate histogram */
    double *hist = (double *)calloc((size_t)(n_bins * n_bins), sizeof(double));
    if (!hist) { *histogram_out = NULL; return; }

    /* Populate 2D histogram */
    for (int i = 0; i < n_points; i++) {
        int l_bin = (int)(sweep[i].lambda * (double)(n_bins - 1));
        int e_bin = (int)(sweep[i].avg_entropy * (double)(n_bins - 1));

        if (l_bin >= 0 && l_bin < n_bins && e_bin >= 0 && e_bin < n_bins) {
            hist[l_bin * n_bins + e_bin] += 1.0;
        }
    }

    /* Set bin edges */
    for (int b = 0; b < n_bins; b++) {
        lambda_bins[b] = (double)b / (double)n_bins;
        entropy_bins[b] = (double)b / (double)n_bins;
    }

    *histogram_out = hist;
}

double lambda_critical_exponent(const lambda_phase_point_t *sweep, int n_points,
                                 double lambda_c)
{
    if (!sweep || n_points < 5) return 0.0;

    /* Linear regression of log(order_parameter) vs log(|? - ?_c|)
     * to find critical exponent ? where order ~ |? - ?_c|^? */

    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    int n = 0;

    for (int i = 0; i < n_points; i++) {
        double dlambda = sweep[i].lambda - lambda_c;
        if (fabs(dlambda) < 1e-8) continue;

        double order = lambda_order_parameter(&sweep[i]);
        if (order < 1e-10) continue;

        double x = log(fabs(dlambda));
        double y = log(order);
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
        n++;
    }

    if (n < 3) return 0.0;

    /* ? = slope of log-log regression */
    double beta = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    return beta;
}

double lambda_mutual_info_rule_dynamics(const lambda_rule_t *rule,
                                         const uint8_t *evolved_states,
                                         size_t n_cells, int n_steps)
{
    if (!rule || !evolved_states || n_cells == 0 || n_steps <= 0) return 0.0;

    /* Compute MI between rule table structure and resulting CA dynamics.
     * Approximated as: correlation between local rule sensitivity and
     * pattern diversity. */

    /* Count rule sensitivity per table entry index */
    double rule_sensitivity = rule->z_parameter;

    /* Count pattern diversity in evolved states */
    uint64_t freq[256] = {0};
    for (size_t i = 0; i < n_cells && i < 100000; i++) {
        if (evolved_states[i] < 256) freq[evolved_states[i]]++;
    }
    double entropy = 0.0;
    for (int k = 0; k < 256; k++) {
        if (freq[k] > 0 && n_cells > 0) {
            double p = (double)freq[k] / (double)n_cells;
            entropy -= p * log2(p);
        }
    }

    /* MI proxy: product of rule sensitivity and pattern entropy */
    return rule_sensitivity * entropy / log2((double)rule->config.num_states);
}

void lambda_landscape_exhaustive(int num_states, int num_inputs,
                                  lambda_phase_point_t **landscape_out,
                                  size_t *n_rules_out)
{
    if (!landscape_out || !n_rules_out) return;

    /* For k=2, N=5: 2^32 possible rules (too large to enumerate).
     * Instead, sample at regular ? intervals and return aggregate. */
    int n_points = 100;
    lambda_config_t base_cfg;
    memset(&base_cfg, 0, sizeof(base_cfg));
    base_cfg.num_states = num_states;
    base_cfg.num_inputs = num_inputs;
    base_cfg.quiescent_state = 0;
    base_cfg.table_size = 1;
    for (int i = 0; i < num_inputs; i++) base_cfg.table_size *= (size_t)num_states;

    *landscape_out = lambda_sweep_phase(&base_cfg, n_points, 50, 42);
    *n_rules_out = (size_t)n_points;
}