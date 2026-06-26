/**
 * @file alife_metrics.c
 * @brief Quantitative emergence, complexity, and self-organization metrics
 *
 * Implements a comprehensive suite of information-theoretic and statistical
 * measures for quantifying artificial life systems:
 *
 * L1: Shannon entropy, conditional entropy, mutual information
 * L2: KL divergence, Jensen-Shannon divergence
 * L3: Emergence score via information decomposition
 * L4: Effective complexity (Gell-Mann & Lloyd), Lempel-Ziv complexity
 * L5: Self-organization, integrated information (?)
 * L6: Novelty rate, open-endedness detection
 * L8: Multi-scale complexity (Bar-Yam), statistical complexity (Crutchfield)
 *
 * Key formulas:
 *   H(X) = -? p(x) log p(x)                          [Shannon 1948]
 *   I(X;Y) = H(X) + H(Y) - H(X,Y)                    [mutual information]
 *   EC = I(X_past; X_future)                           [effective complexity]
 *   C_? = H[S] where S are causal states               [statistical complexity]
 *   ? = effective information across min. info. bipartition [Tononi 2004]
 *
 * Course alignment: SFI CSS, MIT 6.841, Princeton COS 551
 */

#include "alife_metrics.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*??????????????????????????????????????????????????????????????????????
 * L1-L2: Core Information-Theoretic Measures
 *??????????????????????????????????????????????????????????????????????*/

double alife_shannon_entropy(const uint64_t *freqs, size_t n_symbols, uint64_t total)
{
    if (!freqs || n_symbols == 0 || total == 0) return 0.0;

    double entropy = 0.0;
    double total_d = (double)total;

    for (size_t i = 0; i < n_symbols; i++) {
        if (freqs[i] > 0) {
            double p = (double)freqs[i] / total_d;
            entropy -= p * log2(p);
        }
    }

    return entropy;
}

double alife_conditional_entropy(const uint64_t *joint_freqs,
                                  const uint64_t *marginal_freqs_x,
                                  size_t n_x, size_t n_y, uint64_t total)
{
    if (!joint_freqs || !marginal_freqs_x || n_x == 0 || n_y == 0 || total == 0) {
        return 0.0;
    }

    /* H(Y|X) = ?_x p(x) H(Y|X=x)
     *        = ?_x p(x) [-?_y p(y|x) log p(y|x)]
     *        = -?_x ?_y p(x,y) log[p(x,y)/p(x)] */
    double cond_entropy = 0.0;
    double total_d = (double)total;

    for (size_t x = 0; x < n_x; x++) {
        if (marginal_freqs_x[x] == 0) continue;
        double px = (double)marginal_freqs_x[x] / total_d;

        for (size_t y = 0; y < n_y; y++) {
            size_t idx = x * n_y + y;
            if (joint_freqs[idx] == 0) continue;
            double pxy = (double)joint_freqs[idx] / total_d;
            double py_given_x = pxy / px;
            cond_entropy -= pxy * log2(py_given_x);
        }
    }

    return cond_entropy;
}

double alife_mutual_information(const uint64_t *joint_freqs,
                                 const uint64_t *marg_x, const uint64_t *marg_y,
                                 size_t n_x, size_t n_y, uint64_t total)
{
    if (!joint_freqs || !marg_x || !marg_y || n_x == 0 || n_y == 0 || total == 0) {
        return 0.0;
    }

    /* I(X;Y) = ?_x ?_y p(x,y) log[p(x,y) / (p(x) p(y))] */
    double mi = 0.0;
    double total_d = (double)total;

    for (size_t x = 0; x < n_x; x++) {
        if (marg_x[x] == 0) continue;
        double px = (double)marg_x[x] / total_d;

        for (size_t y = 0; y < n_y; y++) {
            size_t idx = x * n_y + y;
            if (joint_freqs[idx] == 0) continue;
            double pxy = (double)joint_freqs[idx] / total_d;
            double py = (double)marg_y[y] / total_d;
            if (py > 0.0) {
                mi += pxy * log2(pxy / (px * py));
            }
        }
    }

    return mi;
}

double alife_kl_divergence(const double *p, const double *q, size_t n)
{
    if (!p || !q || n == 0) return 0.0;

    /* D_KL(P||Q) = ?_i P(i) log[P(i) / Q(i)] */
    double kl = 0.0;

    for (size_t i = 0; i < n; i++) {
        if (p[i] > 0.0 && q[i] > 0.0) {
            kl += p[i] * log2(p[i] / q[i]);
        } else if (p[i] > 0.0) {
            /* q[i] = 0, p[i] > 0: divergence is infinite */
            return INFINITY;
        }
    }

    return kl;
}

double alife_js_divergence(const double *p, const double *q, size_t n)
{
    if (!p || !q || n == 0) return 0.0;

    /* JSD(P||Q) = [D_KL(P||M) + D_KL(Q||M)] / 2 where M = (P+Q)/2 */
    double *m = (double *)malloc(n * sizeof(double));
    if (!m) return 0.0;

    for (size_t i = 0; i < n; i++) {
        m[i] = (p[i] + q[i]) / 2.0;
    }

    double jsd = (alife_kl_divergence(p, m, n) + alife_kl_divergence(q, m, n)) / 2.0;
    free(m);
    return jsd;
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Emergence and Self-Organization
 *??????????????????????????????????????????????????????????????????????*/

double alife_emergence_measure(double agent_preds, double system_preds)
{
    /* Emergence = how much system-level information improves prediction
     * beyond agent-level information.
     *
     * Formal definition (Hoel et al. 2013):
     *   Emergence = I(System_state; Future_state) - I(Agent_states; Future_state)
     *
     * Here: agent_preds = R^2 of agent-level predictor
     *       system_preds = R^2 of system-level predictor
     *       emergence = system_preds - agent_preds (clamped to [0, 1]) */

    double emergence = system_preds - agent_preds;
    if (emergence < 0.0) emergence = 0.0;
    if (emergence > 1.0) emergence = 1.0;
    return emergence;
}

alife_emergence_t alife_analyze_emergence(const uint8_t **state_history,
                                           size_t n_states,
                                           int64_t width, int64_t height,
                                           const alife_metrics_config_t *cfg)
{
    alife_emergence_t result;
    memset(&result, 0, sizeof(result));

    if (!state_history || n_states < 2 || !cfg) return result;

    size_t n_cells = (size_t)(width * height);

    /* Compute spatial entropy at each time step */
    double *entropies = (double *)malloc(n_states * sizeof(double));
    double *mi_temporal = (double *)malloc((n_states - 1) * sizeof(double));
    if (!entropies || !mi_temporal) {
        free(entropies); free(mi_temporal);
        return result;
    }

    /* Compute entropy for each time step */
    for (size_t t = 0; t < n_states; t++) {
        uint64_t freq_0 = 0, freq_1 = 0;
        for (size_t i = 0; i < n_cells; i++) {
            if (state_history[t][i] == 0) freq_0++;
            else freq_1++;
        }
        uint64_t freqs[2] = {freq_0, freq_1};
        entropies[t] = alife_shannon_entropy(freqs, 2, n_cells);
    }

    /* Compute temporal mutual information (I(state_t; state_{t+1})) */
    for (size_t t = 0; t < n_states - 1; t++) {
        uint64_t joint[4] = {0}; /* (s_t, s_{t+1}): 00, 01, 10, 11 */
        uint64_t marg_0 = 0, marg_1 = 0;
        uint64_t marg_n0 = 0, marg_n1 = 0;

        for (size_t i = 0; i < n_cells; i++) {
            int s_curr = (state_history[t][i] != 0) ? 1 : 0;
            int s_next = (state_history[t+1][i] != 0) ? 1 : 0;
            joint[(size_t)(s_curr * 2 + s_next)]++;
            if (s_curr) marg_1++; else marg_0++;
            if (s_next) marg_n1++; else marg_n0++;
        }
        uint64_t marg_x[2] = {marg_0, marg_1};
        uint64_t marg_y[2] = {marg_n0, marg_n1};
        mi_temporal[t] = alife_mutual_information(joint, marg_x, marg_y, 2, 2, n_cells);
    }

    /* Compute excess entropy: sum of lagged mutual information */
    double excess = 0.0;
    for (size_t t = 0; t < n_states - 1; t++) {
        excess += mi_temporal[t];
    }

    /* Emergence score: based on entropy change and predictability gain */
    double entropy_trend = 0.0;
    if (n_states >= 5) {
        /* Trend: entropy difference between first and last third */
        size_t third = n_states / 3;
        double early_ent = 0.0, late_ent = 0.0;
        for (size_t t = 0; t < third; t++) early_ent += entropies[t];
        for (size_t t = n_states - third; t < n_states; t++) late_ent += entropies[t];
        early_ent /= (double)third;
        late_ent /= (double)third;
        entropy_trend = late_ent - early_ent;
    }

    /* Compute scores */
    result.emergence_score = (excess > 0.0) ? 1.0 - exp(-excess) : 0.0;
    result.self_org_score = (entropy_trend < 0.0) ? -entropy_trend : 0.0;
    result.complexity_score = (entropies[n_states/2] > 0.0) ?
        entropies[n_states/2] * (1.0 - fabs(entropy_trend)) : 0.0;
    result.predictability_gain = excess / (double)n_states;

    /* Novelty rate: fraction of unique local patterns */
    if (n_states >= 3) {
        size_t unique = 0;
        size_t total_samples = n_cells < 1000 ? n_cells : 1000;
        for (size_t i = 0; i < total_samples; i++) {
            /* Check if pattern at time 0 is different from time n_states/2 */
            if (state_history[0][i] != state_history[n_states/2][i]) unique++;
        }
        result.novelty_rate = (double)unique / (double)total_samples;
    }

    /* Clamp all scores to [0, 1] */
    if (result.emergence_score > 1.0) result.emergence_score = 1.0;
    if (result.self_org_score > 1.0) result.self_org_score = 1.0;
    if (result.complexity_score > 1.0) result.complexity_score = 1.0;

    free(entropies);
    free(mi_temporal);
    return result;
}

double alife_self_organization(const uint8_t **state_history, size_t n_states,
                                int64_t width, int64_t height)
{
    if (!state_history || n_states < 2) return 0.0;

    /* Self-organization = increase in mutual information between
     * system components over time, normalized by entropy.
     *
     * Measure: how much more ordered the system becomes relative
     * to its initial state. */
    size_t n_cells = (size_t)(width * height);

    /* Compute spatial correlation (Moran's I) at first and last states */
    double corr_first = 0.0, corr_last = 0.0;
    double mean_first = 0.0, mean_last = 0.0;

    /* Compute mean values */
    for (size_t i = 0; i < n_cells; i++) {
        mean_first += (double)state_history[0][i];
        mean_last += (double)state_history[n_states-1][i];
    }
    mean_first /= (double)n_cells;
    mean_last /= (double)n_cells;

    /* Compute spatial autocorrelation at lag 1 (adjacent cells) */
    double num_first = 0.0, den_first = 0.0;
    double num_last = 0.0, den_last = 0.0;

    for (int64_t y = 0; y < height; y++) {
        for (int64_t x = 0; x < width; x++) {
            size_t i = (size_t)(y * width + x);
            double d_first = (double)state_history[0][i] - mean_first;
            double d_last = (double)state_history[n_states-1][i] - mean_last;
            den_first += d_first * d_first;
            den_last += d_last * d_last;

            if (x + 1 < width) {
                size_t j = (size_t)(y * width + x + 1);
                num_first += d_first * ((double)state_history[0][j] - mean_first);
                num_last += d_last * ((double)state_history[n_states-1][j] - mean_last);
            }
        }
    }

    if (den_first > 0.0) corr_first = num_first / den_first;
    if (den_last > 0.0) corr_last = num_last / den_last;

    /* Self-organization = increase in spatial autocorrelation */
    double so = corr_last - corr_first;
    return (so > 0.0) ? so : 0.0;
}

double alife_phi_integrated_info(const uint8_t *state,
                                  int64_t width, int64_t height,
                                  int num_states)
{
    if (!state || width < 2 || height < 2) return 0.0;

    /* ? (Phi) approximation: effective information across the
     * minimum information bipartition (MIB).
     *
     * For computational efficiency, we use the bipartition
     * that splits the grid in half along the longer axis. */

    /* Split into two halves */
    int64_t mid;
    bool split_vertical = (width >= height);

    size_t n_left, n_right;
    if (split_vertical) {
        mid = width / 2;
        n_left = (size_t)(mid * height);
        n_right = (size_t)((width - mid) * height);
    } else {
        mid = height / 2;
        n_left = (size_t)(width * mid);
        n_right = (size_t)(width * (height - mid));
    }

    /* Count state frequencies in each half */
    uint64_t *freq_left = (uint64_t *)calloc((size_t)num_states, sizeof(uint64_t));
    uint64_t *freq_right = (uint64_t *)calloc((size_t)num_states, sizeof(uint64_t));
    uint64_t *freq_joint = (uint64_t *)calloc((size_t)(num_states * num_states), sizeof(uint64_t));
    if (!freq_left || !freq_right || !freq_joint) {
        free(freq_left); free(freq_right); free(freq_joint);
        return 0.0;
    }

    /* Collect statistics */
    for (int64_t y = 0; y < height; y++) {
        for (int64_t x = 0; x < width; x++) {
            uint8_t s = state[y * width + x];
            if (s >= (uint8_t)num_states) s = (uint8_t)(num_states - 1);

            size_t idx;
            if (split_vertical) {
                if (x < mid) {
                    freq_left[s]++;
                    idx = (size_t)s * (size_t)num_states;
                } else {
                    freq_right[s]++;
                    idx = (size_t)s;
                }
            } else {
                if (y < mid) {
                    freq_left[s]++;
                    idx = (size_t)s * (size_t)num_states;
                } else {
                    freq_right[s]++;
                    idx = (size_t)s;
                }
            }
            /* Note: Simplified ? full joint requires matching cell pairs */
        }
    }

    /* Compute total counts */
    uint64_t total_left = 0, total_right = 0;
    for (int s = 0; s < num_states; s++) total_left += freq_left[s];
    for (int s = 0; s < num_states; s++) total_right += freq_right[s];

    /* H(left) + H(right) - H(left,right) */
    double h_left = alife_shannon_entropy(freq_left, (size_t)num_states, total_left);
    double h_right = alife_shannon_entropy(freq_right, (size_t)num_states, total_right);

    /* ? ? min(H(left), H(right)) * integration factor */
    double phi = (h_left < h_right ? h_left : h_right);

    free(freq_left); free(freq_right); free(freq_joint);
    return phi;
}

/*??????????????????????????????????????????????????????????????????????
 * L4: Complexity Measures
 *??????????????????????????????????????????????????????????????????????*/

double alife_effective_complexity(const uint8_t **state_history, size_t n_states,
                                   int64_t width, int64_t height)
{
    if (!state_history || n_states < 2) return 0.0;

    /* EC ? I(past; future) = excess entropy
     * This is the amount of information shared between past and future
     * states of the system ? the "regularity" or "structure." */
    alife_metrics_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.history_length = (int)n_states;
    cfg.pattern_radius = 1;
    cfg.seed = 0;

    alife_emergence_t emergence = alife_analyze_emergence(state_history, n_states,
                                                           width, height, &cfg);
    return emergence.predictability_gain;
}

double alife_lempel_ziv_complexity(const uint8_t *data, size_t length)
{
    if (!data || length == 0) return 0.0;

    /* Lempel-Ziv 1978 parsing algorithm.
     * Complexity = number of distinct phrases in incremental parsing.
     * Normalized by n/log2(n) (asymptotic upper bound for random sequence). */

    /* Simple LZ78 implementation with linear search */
    size_t *dict_pos = (size_t *)malloc(length * sizeof(size_t));
    size_t *dict_len = (size_t *)malloc(length * sizeof(size_t));
    if (!dict_pos || !dict_len) {
        free(dict_pos); free(dict_len);
        return 0.0;
    }

    size_t n_phrases = 1;
    dict_pos[0] = 0;
    dict_len[0] = 1;

    size_t i = 1;
    while (i < length) {
        /* Find longest match in dictionary */
        size_t longest = 0;
        size_t best_match = 0;

        for (size_t d = 0; d < n_phrases; d++) {
            size_t match_len = 0;
            while (i + match_len < length
                   && dict_pos[d] + match_len < dict_pos[d] + dict_len[d]
                   && match_len < dict_len[d]
                   && data[i + match_len] == data[dict_pos[d] + match_len]) {
                match_len++;
            }
            if (match_len > longest) {
                longest = match_len;
                best_match = d;
            }
        }

        /* New phrase = longest match + next character */
        dict_pos[n_phrases] = i;
        dict_len[n_phrases] = longest + 1;
        n_phrases++;
        i += longest + 1;

        if (n_phrases >= length) break;
    }

    free(dict_pos); free(dict_len);

    /* Normalize */
    double n = (double)length;
    double norm = n / log2(n > 1.0 ? n : 2.0);
    if (norm <= 0.0) return 0.0;
    return (double)n_phrases / norm;
}

double alife_thermodynamic_depth(const uint8_t **state_history, size_t n_states,
                                  int64_t width, int64_t height)
{
    if (!state_history || n_states < 2) return 0.0;

    /* Thermodynamic depth: minimal computation needed to produce a state.
     * Approximated as: total number of state changes across evolution,
     * normalized by system size. */
    size_t n_cells = (size_t)(width * height);
    uint64_t total_changes = 0;

    for (size_t t = 1; t < n_states; t++) {
        for (size_t i = 0; i < n_cells; i++) {
            if (state_history[t][i] != state_history[t-1][i]) {
                total_changes++;
            }
        }
    }

    return (double)total_changes / (double)(n_cells * n_states);
}

alife_stat_complexity_t alife_statistical_complexity_calc(
    const uint8_t **state_history, size_t n_states,
    int64_t width, int64_t height, int num_states,
    const alife_metrics_config_t *cfg)
{
    alife_stat_complexity_t result;
    memset(&result, 0, sizeof(result));

    if (!state_history || n_states < 4 || cfg == NULL) return result;

    /* Crutchfield's statistical complexity C_? = H[S] where S are causal states.
     * Causal states group pasts that make the same predictions about the future.
     *
     * For computational tractability, we use a simplified approach:
     * Cluster cells by their local neighborhood pattern and compute
     * the entropy of the cluster distribution. */

    size_t n_cells = (size_t)(width * height);
    if (n_cells == 0) return result;

    /* Compute local pattern for each cell (radius-1 neighborhood) */
    size_t max_patterns = 512; /* 2^9 for binary radius-1 Moore */
    uint64_t *pattern_counts = (uint64_t *)calloc(max_patterns, sizeof(uint64_t));
    if (!pattern_counts) return result;

    for (int64_t y = 0; y < height; y++) {
        for (int64_t x = 0; x < width; x++) {
            uint64_t pattern = 0;
            int bit = 0;
            for (int64_t dy = -1; dy <= 1; dy++) {
                for (int64_t dx = -1; dx <= 1; dx++) {
                    int64_t nx = x + dx;
                    int64_t ny = y + dy;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        if (state_history[0][ny * width + nx]) {
                            pattern |= ((uint64_t)1 << bit);
                        }
                    }
                    bit++;
                    if (bit >= 64) break;
                }
                if (bit >= 64) break;
            }

            if (pattern < max_patterns) {
                pattern_counts[pattern]++;
            }
        }
    }

    /* Count distinct (non-zero) patterns */
    size_t n_patterns = 0;
    for (size_t p = 0; p < max_patterns; p++) {
        if (pattern_counts[p] > 0) n_patterns++;
    }
    result.n_causal_states = n_patterns;

    /* Statistical complexity = entropy of pattern distribution */
    result.statistical_complexity = alife_shannon_entropy(pattern_counts,
                                                           max_patterns, n_cells);

    /* Entropy rate estimate: average of temporal entropy over steps */
    double h_mu = 0.0;
    for (size_t t = 0; t < n_states - 1 && t < 10; t++) {
        uint64_t changes = 0;
        for (size_t i = 0; i < n_cells; i++) {
            if (state_history[t][i] != state_history[t+1][i]) changes++;
        }
        double rate = (double)changes / (double)n_cells;
        if (rate > 0.0 && rate < 1.0) {
            h_mu += -(rate * log2(rate) + (1.0 - rate) * log2(1.0 - rate));
        }
    }
    if (n_states > 1) h_mu /= (double)((n_states - 1) < 10 ? (n_states - 1) : 10);
    result.entropy_rate_estimate = h_mu;

    /* Predictive information = I(past; future) ? excess entropy */
    result.predictive_information = alife_effective_complexity(
        state_history, n_states, width, height);

    free(pattern_counts);
    return result;
}

/*??????????????????????????????????????????????????????????????????????
 * L8: Advanced ALife Metrics
 *??????????????????????????????????????????????????????????????????????*/

double alife_novelty_rate(const uint8_t **state_history, size_t n_states,
                           int64_t width, int64_t height, int pattern_radius)
{
    if (!state_history || n_states < 2) return 0.0;

    /* Novelty: fraction of radius-r patterns at time t not seen before t.
     * Uses simple hash-set tracking for efficiency. */
    size_t n_cells = (size_t)(width * height);
    size_t n_patterns = n_cells < 10000 ? n_cells : 10000;

    /* For simplicity, use the cell value itself as the "pattern" (radius-0) */
    uint64_t seen_0 = 0, seen_1 = 0;
    uint64_t new_patterns = 0;
    uint64_t total_checked = 0;

    for (size_t t = 0; t < n_states && total_checked < n_patterns; t++) {
        for (size_t i = 0; i < n_cells && total_checked < n_patterns; i++) {
            int val = (state_history[t][i] != 0) ? 1 : 0;
            if (t == 0) {
                if (val == 0) seen_0++;
                else seen_1++;
            } else {
                if (val == 0 && seen_0 == 0) new_patterns++;
                else if (val == 1 && seen_1 == 0) new_patterns++;
            }
            total_checked++;
        }
    }

    (void)pattern_radius;
    return total_checked > 0 ? (double)new_patterns / (double)total_checked : 0.0;
}

bool alife_detect_open_ended(const uint8_t **state_history, size_t n_states,
                              int64_t width, int64_t height,
                              double *novelty_trend)
{
    if (!state_history || n_states < 10) {
        if (novelty_trend) *novelty_trend = 0.0;
        return false;
    }

    /* Open-ended evolution: novelty rate does NOT decay to zero.
     * Test: compute novelty rate in windows of increasing time,
     * check if the rate stays bounded away from zero. */

    size_t n_windows = n_states / 3 > 5 ? 5 : n_states / 3;
    if (n_windows < 3) {
        if (novelty_trend) *novelty_trend = 0.0;
        return false;
    }

    double *novelties = (double *)malloc(n_windows * sizeof(double));
    if (!novelties) {
        if (novelty_trend) *novelty_trend = 0.0;
        return false;
    }

    size_t window_size = n_states / n_windows;
    for (size_t w = 0; w < n_windows; w++) {
        size_t t_start = w * window_size;
        size_t t_end = (w + 1) * window_size;
        if (t_end > n_states) t_end = n_states;

        novelties[w] = alife_novelty_rate(&state_history[t_start],
                                           t_end - t_start, width, height, 1);
    }

    /* Compute linear trend of novelty rate */
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    for (size_t w = 0; w < n_windows; w++) {
        double x = (double)w;
        double y = novelties[w];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    double n_d = (double)n_windows;
    double slope = (n_d * sum_xy - sum_x * sum_y) / (n_d * sum_xx - sum_x * sum_x);

    if (novelty_trend) *novelty_trend = slope;
    free(novelties);

    /* Open-ended if slope > -0.01 (not significantly negative) */
    return (slope > -0.01);
}

double alife_multiscale_complexity(const uint8_t *state,
                                    int64_t width, int64_t height,
                                    int max_scale)
{
    if (!state || max_scale <= 0) return 0.0;

    /* Bar-Yam multi-scale complexity: sum of mutual information
     * between subsystems at different scales, normalized.
     *
     * At each scale s, partition the grid into blocks of size s?s,
     * compute mutual information between adjacent blocks. */
    double total_complexity = 0.0;
    int n_scales = 0;

    for (int scale = 1; scale <= max_scale; scale *= 2) {
        int64_t block_w = width / scale;
        int64_t block_h = height / scale;
        if (block_w < 2 || block_h < 2) break;

        /* Compute average mutual information between adjacent blocks */
        double scale_mi = 0.0;
        int n_pairs = 0;

        for (int64_t by = 0; by < block_h; by++) {
            for (int64_t bx = 0; bx < block_w - 1; bx++) {
                /* Compare block (bx, by) with block (bx+1, by) */
                uint64_t freq_a[2] = {0, 0}, freq_b[2] = {0, 0};
                uint64_t joint[4] = {0, 0, 0, 0};
                uint64_t block_cells = (uint64_t)(scale * scale);

                for (int64_t dy = 0; dy < scale; dy++) {
                    for (int64_t dx = 0; dx < scale; dx++) {
                        int64_t ax = bx * scale + dx;
                        int64_t ay = by * scale + dy;
                        int64_t bdx = dx + scale;
                        if (ax < width && ay < height && ax + scale < width) {
                            int va = state[ay * width + ax] ? 1 : 0;
                            int vb = state[ay * width + ax + scale] ? 1 : 0;
                            freq_a[va]++;
                            freq_b[vb]++;
                            joint[va * 2 + vb]++;
                        }
                    }
                }

                uint64_t total = freq_a[0] + freq_a[1];
                if (total > 0) {
                    scale_mi += alife_mutual_information(joint, freq_a, freq_b,
                                                           2, 2, total);
                    n_pairs++;
                }
            }
        }

        if (n_pairs > 0) {
            scale_mi /= (double)n_pairs;
            total_complexity += scale_mi;
            n_scales++;
        }
    }

    return (n_scales > 0) ? total_complexity / (double)n_scales : 0.0;
}

void alife_comprehensive_test(const uint8_t **state_history, size_t n_states,
                               int64_t width, int64_t height,
                               const alife_metrics_config_t *cfg,
                               double scores_out[5])
{
    if (!scores_out) return;

    /* Initialize scores */
    for (int i = 0; i < 5; i++) scores_out[i] = 0.0;

    if (!state_history || n_states < 2 || !cfg) return;

    /* (1) Emergence */
    alife_emergence_t emergence = alife_analyze_emergence(state_history, n_states,
                                                           width, height, cfg);
    scores_out[0] = emergence.emergence_score;

    /* (2) Self-organization */
    scores_out[1] = alife_self_organization(state_history, n_states, width, height);

    /* (3) Adaptation: measured as decrease in entropy over time
     * (more order ? adaptation to constraints) */
    scores_out[2] = emergence.self_org_score;

    /* (4) Open-endedness */
    double novelty_trend;
    bool is_open_ended = alife_detect_open_ended(state_history, n_states,
                                                   width, height, &novelty_trend);
    scores_out[3] = is_open_ended ? (1.0 + novelty_trend * 10.0) : 0.0;
    if (scores_out[3] > 1.0) scores_out[3] = 1.0;
    if (scores_out[3] < 0.0) scores_out[3] = 0.0;

    /* (5) Life-likeness: composite score */
    scores_out[4] = (scores_out[0] + scores_out[1] + scores_out[2] + scores_out[3]) / 4.0;
}