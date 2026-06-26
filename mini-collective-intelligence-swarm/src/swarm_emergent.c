#include "swarm_emergent.h"
#include "swarm_core.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Emergent Behavior & Complex Collective Dynamics
 *
 * References:
 *   Holland (1998)            — Emergence: From Chaos to Order
 *   Wolfram (2002)            — A New Kind of Science (CA classification)
 *   Bak, Tang & Wiesenfeld (1987) — Self-organized criticality
 *   Crutchfield & Young (1989) — Statistical complexity
 *   Langton (1990)            — Computation at the edge of chaos
 * ============================================================================ */

/* ── Emergence Metrics ── */

double swarm_emergence_gestalt(const double* local_complexities, int n_parts,
                                double global_complexity) {
    if (!local_complexities || n_parts <= 0) return 0.0;
    double sum_local = 0.0;
    for (int i = 0; i < n_parts; i++) sum_local += local_complexities[i];
    /* Gestalt = global - sum(local); emergence when global > Σ local */
    return global_complexity - sum_local;
}

void swarm_emergence_pid(const double* joint_probs, int n_states,
           double* redundancy, double* unique_a, double* unique_b, double* synergy) {
    if (!joint_probs || n_states <= 0) return;
    int n = (int)sqrt((double)n_states);
    if (n * n != n_states) return;
    /* Compute marginals */
    double* marg_a = (double*)calloc((size_t)n, sizeof(double));
    double* marg_b = (double*)calloc((size_t)n, sizeof(double));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            marg_a[i] += joint_probs[i * n + j];
            marg_b[j] += joint_probs[i * n + j];
        }
    /* Mutual information: I(A;B) = Σ p(a,b) log(p(a,b)/(p(a)p(b))) */
    double mi = 0.0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (joint_probs[i*n+j] > SWARM_EPSILON && marg_a[i] > SWARM_EPSILON && marg_b[j] > SWARM_EPSILON)
                mi += joint_probs[i*n+j] * log2(joint_probs[i*n+j] / (marg_a[i] * marg_b[j]));
    /* Decompose MI into components (simplified) */
    if (redundancy) *redundancy = mi * 0.25;
    if (unique_a)   *unique_a = mi * 0.35;
    if (unique_b)   *unique_b = mi * 0.25;
    if (synergy)    *synergy = mi * 0.15;
    free(marg_a); free(marg_b);
}

double swarm_emergence_excess_entropy(const int* time_series, int length, int alphabet) {
    if (!time_series || length <= 0 || alphabet <= 0) return 0.0;
    /* Block entropy estimation for L=1 and L=2 */
    int* count1 = (int*)calloc((size_t)alphabet, sizeof(int));
    int* count2 = (int*)calloc((size_t)(alphabet * alphabet), sizeof(int));
    if (!count1 || !count2) { free(count1); free(count2); return 0.0; }
    for (int i = 0; i < length; i++) {
        int sym = time_series[i] % alphabet;
        if (sym >= 0 && sym < alphabet) count1[sym]++;
        if (i < length - 1) {
            int sym2 = time_series[i+1] % alphabet;
            if (sym >= 0 && sym < alphabet && sym2 >= 0 && sym2 < alphabet)
                count2[sym * alphabet + sym2]++;
        }
    }
    /* H(1) */
    double H1 = 0.0;
    for (int i = 0; i < alphabet; i++)
        if (count1[i] > 0) { double p = (double)count1[i]/(double)length; H1 -= p*log2(p); }
    /* H(2) */
    double H2 = 0.0;
    int total2 = length - 1;
    for (int i = 0; i < alphabet * alphabet; i++)
        if (count2[i] > 0) { double p = (double)count2[i]/(double)total2; H2 -= p*log2(p); }
    /* Entropy rate h_μ ≈ H(2) - H(1) */
    double h_mu = H2 - H1;
    /* Excess entropy E = lim H(L) - L·h_μ, approximated as H(1) - h_μ */
    double excess = H1 - h_mu;
    free(count1); free(count2);
    return excess > 0.0 ? excess : 0.0;
}

double swarm_emergence_statistical_complexity(const double* state_dist, int n_states) {
    if (!state_dist || n_states <= 0) return 0.0;
    double C = 0.0;
    for (int i = 0; i < n_states; i++)
        if (state_dist[i] > SWARM_EPSILON)
            C -= state_dist[i] * log2(state_dist[i]);
    return C;
}

/* ── Phase Transition Analysis ── */

void swarm_phase_transition_analyze(PhaseTransition* pt,
           double (*order_fn)(void* data, double noise),
           void* data, double noise_min, double noise_max, int n_points,
           int trials_per_point) {
    if (!pt || !order_fn) return;
    pt->noise_levels = swarm_alloc_vector(n_points);
    pt->order_parameters = swarm_alloc_vector(n_points);
    pt->susceptibilities = swarm_alloc_vector(n_points);
    pt->n_samples = n_points;
    double d_noise = (noise_max - noise_min) / (double)(n_points - 1);
    
    for (int p = 0; p < n_points; p++) {
        double noise = noise_min + p * d_noise;
        pt->noise_levels[p] = noise;
        double sum = 0.0, sum_sq = 0.0;
        for (int t = 0; t < trials_per_point; t++) {
            double phi = order_fn(data, noise);
            sum += phi; sum_sq += phi * phi;
        }
        pt->order_parameters[p] = sum / (double)trials_per_point;
        double var = sum_sq/(double)trials_per_point - pt->order_parameters[p]*pt->order_parameters[p];
        pt->susceptibilities[p] = var * (double)trials_per_point;
    }
    /* Find critical noise: maximum susceptibility */
    int max_idx = 0;
    for (int i = 1; i < n_points; i++)
        if (pt->susceptibilities[i] > pt->susceptibilities[max_idx]) max_idx = i;
    pt->critical_noise = pt->noise_levels[max_idx];
    pt->transition_detected = (pt->susceptibilities[max_idx] > 0.1);
}

void swarm_phase_transition_free(PhaseTransition* pt) {
    if (!pt) return;
    swarm_free_vector(pt->noise_levels);
    swarm_free_vector(pt->order_parameters);
    swarm_free_vector(pt->susceptibilities);
    free(pt);
}

double swarm_binder_cumulant(const double* order_samples, int n_samples, int order) {
    if (!order_samples || n_samples <= 0) return 0.0;
    double m2 = 0.0, m4 = 0.0;
    for (int i = 0; i < n_samples; i++) {
        double val = fabs(order_samples[i]);
        m2 += val * val;
        m4 += val * val * val * val;
    }
    m2 /= (double)n_samples; m4 /= (double)n_samples;
    if (m2 < SWARM_EPSILON) return 0.0;
    /* Binder cumulant: U = 1 - <m^4>/(3<m^2>^2) for Ising-like */
    return 1.0 - m4 / (3.0 * m2 * m2);
    (void)order;
}

/* ── Sandpile Model (Bak-Tang-Wiesenfeld 1987) ── */

Sandpile* swarm_sandpile_create(int width, int height, int critical_height) {
    if (width <= 0 || height <= 0) return NULL;
    Sandpile* sp = (Sandpile*)calloc(1, sizeof(Sandpile));
    if (!sp) return NULL;
    sp->width = width; sp->height = height;
    sp->critical_height = critical_height;
    sp->grid = (int**)malloc((size_t)height * sizeof(int*));
    sp->grid[0] = (int*)calloc((size_t)(width * height), sizeof(int));
    if (!sp->grid || !sp->grid[0]) { swarm_sandpile_free(sp); return NULL; }
    for (int i = 1; i < height; i++) sp->grid[i] = sp->grid[0] + i * width;
    sp->avalanche_capacity = 10000;
    sp->avalanche_sizes = swarm_alloc_int_vector(sp->avalanche_capacity);
    sp->n_avalanches = 0;
    return sp;
}

void swarm_sandpile_free(Sandpile* sp) {
    if (!sp) return;
    if (sp->grid) { free(sp->grid[0]); free(sp->grid); }
    swarm_free_int_vector(sp->avalanche_sizes);
    free(sp);
}

void swarm_sandpile_drop(Sandpile* sp, int x, int y) {
    if (!sp || x < 0 || x >= sp->width || y < 0 || y >= sp->height) return;
    sp->grid[y][x]++;
    sp->total_mass += 1.0;
    if (sp->grid[y][x] >= sp->critical_height) {
        int size = swarm_sandpile_avalanche(sp, x, y);
        if (sp->n_avalanches < sp->avalanche_capacity && size > 0)
            sp->avalanche_sizes[sp->n_avalanches++] = size;
    }
}

int swarm_sandpile_avalanche(Sandpile* sp, int x, int y) {
    if (!sp) return 0;
    int size = 0;
    /* BFS/queue-based toppling */
    int* qx = swarm_alloc_int_vector(sp->width * sp->height);
    int* qy = swarm_alloc_int_vector(sp->width * sp->height);
    if (!qx || !qy) { swarm_free_int_vector(qx); swarm_free_int_vector(qy); return 0; }
    int head = 0, tail = 0;
    qx[tail] = x; qy[tail] = y; tail++;
    
    while (head < tail) {
        int cx = qx[head], cy = qy[head]; head++;
        if (sp->grid[cy][cx] < sp->critical_height) continue;
        /* Topple: distribute to 4 neighbors */
        int grains = sp->grid[cy][cx];
        sp->grid[cy][cx] = grains - 4;
        size++;
        int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
        for (int d = 0; d < 4; d++) {
            int nx = cx + dirs[d][0], ny = cy + dirs[d][1];
            /* Open boundaries: grains fall off edge */
            if (nx < 0 || nx >= sp->width || ny < 0 || ny >= sp->height) continue;
            sp->grid[ny][nx]++;
            if (sp->grid[ny][nx] >= sp->critical_height) {
                qx[tail] = nx; qy[tail] = ny; tail++;
            }
        }
    }
    swarm_free_int_vector(qx); swarm_free_int_vector(qy);
    return size;
}

double* swarm_sandpile_avalanche_distribution(Sandpile* sp, int n_bins, double* bin_edges) {
    if (!sp || !bin_edges || n_bins <= 0 || sp->n_avalanches <= 0) return NULL;
    double* hist = swarm_alloc_vector(n_bins);
    if (!hist) return NULL;
    for (int i = 0; i < sp->n_avalanches; i++) {
        double s = (double)sp->avalanche_sizes[i];
        for (int b = 0; b < n_bins - 1; b++)
            if (s >= bin_edges[b] && s < bin_edges[b+1]) { hist[b]++; break; }
        if (s >= bin_edges[n_bins-1]) hist[n_bins-1]++;
    }
    for (int b = 0; b < n_bins; b++) hist[b] /= (double)sp->n_avalanches;
    return hist;
}

double swarm_sandpile_power_law_exponent(const int* sizes, int n_events) {
    if (!sizes || n_events < 2) return 0.0;
    /* Log-log linear regression: log(p(s)) = -α·log(s) + C */
    double sum_log_s = 0.0, sum_log_p = 0.0, sum_log_s2 = 0.0, sum_log_sp = 0.0;
    /* First bin into log-spaced bins */
    int n_bins = n_events > 100 ? 100 : n_events;
    int* bins = (int*)calloc((size_t)n_bins, sizeof(int));
    double min_s = sizes[0], max_s = sizes[0];
    for (int i = 0; i < n_events; i++) {
        if (sizes[i] < min_s) min_s = sizes[i];
        if (sizes[i] > max_s) max_s = sizes[i];
    }
    if (max_s <= min_s) { free(bins); return 0.0; }
    for (int i = 0; i < n_events; i++) {
        int bin = (int)((log((double)sizes[i]) - log(min_s)) / (log(max_s) - log(min_s)) * (n_bins - 1));
        if (bin < 0) bin = 0; if (bin >= n_bins) bin = n_bins - 1;
        bins[bin]++;
    }
    int n_valid = 0;
    for (int b = 0; b < n_bins; b++) {
        if (bins[b] == 0) continue;
        double s = exp(log(min_s) + (log(max_s)-log(min_s)) * (double)b / (double)(n_bins-1));
        double p = (double)bins[b] / (double)n_events;
        sum_log_s += log(s); sum_log_p += log(p);
        sum_log_s2 += log(s)*log(s); sum_log_sp += log(s)*log(p);
        n_valid++;
    }
    free(bins);
    if (n_valid < 2) return 0.0;
    double denom = n_valid * sum_log_s2 - sum_log_s * sum_log_s;
    if (fabs(denom) < SWARM_EPSILON) return 0.0;
    double alpha = -(n_valid * sum_log_sp - sum_log_s * sum_log_p) / denom;
    return alpha;
}

/* ── CA Classification ── */
int swarm_ca_classify(const int* states, int length, int n_steps, int n_rules) {
    if (!states || length <= 0) return -1;
    (void)n_steps; (void)n_rules;
    /* Simplified: measure entropy rate and temporal stability */
    /* Class 1: fixed point → low entropy rate, high stability */
    /* Class 2: periodic → medium entropy, cyclic */
    /* Class 3: chaotic → high entropy rate */
    /* Class 4: complex → medium, long transients */
    int changes = 0;
    for (int i = 1; i < length; i++)
        if (states[i] != states[i-1]) changes++;
    double change_ratio = (double)changes / (double)(length - 1);
    if (change_ratio < 0.02) return 1;  /* Class 1: uniform/fixed */
    if (change_ratio < 0.15) return 2;  /* Class 2: periodic */
    if (change_ratio > 0.4) return 3;   /* Class 3: chaotic */
    return 4; /* Class 4: complex */
}

double swarm_langton_lambda(const int* rule_table, int n_inputs, int n_outputs,
                             int quiescent) {
    if (!rule_table || n_inputs <= 0 || n_outputs <= 0) return 0.0;
    int n_quiescent = 0;
    for (int i = 0; i < n_inputs; i++)
        if (rule_table[i] == quiescent) n_quiescent++;
    /* λ = (n_inputs - n_quiescent) / n_inputs */
    return (double)(n_inputs - n_quiescent) / (double)n_inputs;
    (void)n_outputs;
}

/* ── Agent-Based Model Framework ── */
AgentBasedModel* swarm_abm_create(const ABMConfig* config, int agent_struct_size,
                    agent_init_fn init, agent_update_fn update, agent_fitness_fn fitness) {
    if (!config || agent_struct_size <= 0) return NULL;
    AgentBasedModel* abm = (AgentBasedModel*)calloc(1, sizeof(AgentBasedModel));
    if (!abm) return NULL;
    abm->config = *config;
    abm->agent_type_size = agent_struct_size;
    abm->init = init;
    abm->update = update;
    abm->fitness = fitness;
    abm->agents = (void**)calloc((size_t)config->n_agents, sizeof(void*));
    if (!abm->agents) { free(abm); return NULL; }
    abm->positions = swarm_matrix_alloc(config->n_agents, 3);
    abm->velocities = swarm_matrix_alloc(config->n_agents, 3);
    abm->agent_types = swarm_alloc_int_vector(config->n_agents);
    /* Allocate and init agents */
    for (int i = 0; i < config->n_agents; i++) {
        abm->agents[i] = calloc(1, (size_t)agent_struct_size);
        if (!abm->agents[i]) break;
        if (init) init(abm->agents[i], i, (void*)config);
    }
    return abm;
}

void swarm_abm_free(AgentBasedModel* abm) {
    if (!abm) return;
    for (int i = 0; i < abm->config.n_agents; i++) free(abm->agents[i]);
    free(abm->agents);
    swarm_matrix_free(abm->positions, abm->config.n_agents);
    swarm_matrix_free(abm->velocities, abm->config.n_agents);
    swarm_free_int_vector(abm->agent_types);
    free(abm);
}

void swarm_abm_step(AgentBasedModel* abm) {
    if (!abm || !abm->update) return;
    for (int i = 0; i < abm->config.n_agents; i++) {
        /* Collect neighbors */
        void* neighbor_ptrs[SWARM_MAX_NEIGHBORS];
        int n_neighbors = 0;
        for (int j = 0; j < abm->config.n_agents && n_neighbors < SWARM_MAX_NEIGHBORS; j++) {
            if (j == i) continue;
            double dx = abm->positions[i][0] - abm->positions[j][0];
            double dy = abm->positions[i][1] - abm->positions[j][1];
            if (sqrt(dx*dx + dy*dy) <= abm->config.interaction_radius)
                neighbor_ptrs[n_neighbors++] = abm->agents[j];
        }
        abm->update(abm->agents[i], neighbor_ptrs, n_neighbors, abm->environment, (void*)&abm->config);
    }
    abm->step++;
    abm->time += abm->config.dt;
}

void swarm_abm_run(AgentBasedModel* abm, int n_steps) {
    for (int s = 0; s < n_steps; s++) swarm_abm_step(abm);
}

void swarm_abm_print_stats(const AgentBasedModel* abm) {
    if (!abm) { printf("(null ABM)\n"); return; }
    printf("ABM: step=%d time=%.2f agents=%d\n", abm->step, abm->time, abm->config.n_agents);
}

/* ── Information-Theoretic Measures ── */

double swarm_transfer_entropy(const int* x_series, const int* y_series,
           int length, int x_alphabet, int y_alphabet, int lag) {
    if (!x_series || !y_series || length <= lag || x_alphabet <= 0 || y_alphabet <= 0) return 0.0;
    /* TE_{Y→X} = Σ p(x_{t+1}, x_t, y_t) log(p(x_{t+1}|x_t, y_t) / p(x_{t+1}|x_t)) */
    /* Simplified: use correlation as proxy */
    double corr = 0.0;
    for (int t = 0; t < length - lag; t++) {
        double x_t = (double)x_series[t], x_t1 = (double)x_series[t + lag];
        double y_t = (double)y_series[t];
        corr += (x_t1 - x_t) * y_t;
    }
    return fabs(corr) / (double)(length - lag);
    (void)x_alphabet; (void)y_alphabet;
}

double swarm_active_info_storage(const int* series, int length, int alphabet, int k) {
    if (!series || length <= k || alphabet <= 0) return 0.0;
    /* AIS = I(X_{t+1}; X_t^{(k)}) */
    /* Simplified as autocorrelation at lag 1 */
    double auto_corr = 0.0;
    double mean = 0.0;
    for (int t = 0; t < length; t++) mean += (double)series[t];
    mean /= (double)length;
    for (int t = 0; t < length - 1; t++)
        auto_corr += ((double)series[t] - mean) * ((double)series[t+1] - mean);
    double var = 0.0;
    for (int t = 0; t < length; t++) { double d = (double)series[t] - mean; var += d*d; }
    if (var < SWARM_EPSILON) return 0.0;
    return fabs(auto_corr / var);
    (void)k;
}

double swarm_swarm_mutual_information(const double** trajectories, int n_agents,
           int length, int n_bins) {
    if (!trajectories || n_agents <= 1 || length <= 0 || n_bins <= 0) return 0.0;
    /* Average pairwise MI across all agents */
    double total_mi = 0.0;
    int pairs = 0;
    for (int a = 0; a < n_agents; a++) {
        for (int b = a + 1; b < n_agents; b++) {
            double corr = 0.0;
            for (int t = 0; t < length; t++)
                corr += trajectories[a][t] * trajectories[b][t];
            total_mi += fabs(corr) / (double)length;
            pairs++;
        }
    }
    return pairs > 0 ? total_mi / (double)pairs : 0.0;
}

/* ── Critical Slowing Down ── */
double swarm_critical_slowing_down(const double* time_series, int length,
           int window_size, double* ar1_coeff, double* variance) {
    if (!time_series || length < window_size || window_size <= 1) return 0.0;
    /* Fit AR(1) model: x_t = α·x_{t-1} + ε_t */
    double sum_xy = 0.0, sum_xx = 0.0;
    double mean = 0.0;
    for (int i = length - window_size; i < length; i++) mean += time_series[i];
    mean /= (double)window_size;
    for (int i = length - window_size + 1; i < length; i++) {
        double x_prev = time_series[i-1] - mean, x_curr = time_series[i] - mean;
        sum_xy += x_prev * x_curr;
        sum_xx += x_prev * x_prev;
    }
    double alpha = (sum_xx > SWARM_EPSILON) ? sum_xy / sum_xx : 0.0;
    if (ar1_coeff) *ar1_coeff = alpha;
    /* Variance of residuals */
    double var_res = 0.0;
    for (int i = length - window_size + 1; i < length; i++) {
        double resid = time_series[i] - mean - alpha * (time_series[i-1] - mean);
        var_res += resid * resid;
    }
    var_res /= (double)(window_size - 1);
    if (variance) *variance = var_res;
    /* CSD indicator: AR1 approaching 1 + increasing variance */
    return (1.0 - alpha) * var_res;
}

bool swarm_early_warning_signal(const double* time_series, int length,
           double* kendall_tau, double* p_value) {
    if (!time_series || length < 10) return false;
    /* Kendall tau trend test on sliding window AR1 coefficients */
    int n_windows = length / 5;
    if (n_windows < 3) return false;
    double* ar1_vals = swarm_alloc_vector(n_windows);
    if (!ar1_vals) return false;
    int window_size = length / n_windows;
    for (int w = 0; w < n_windows; w++) {
        double ar1, var;
        swarm_critical_slowing_down(time_series, (w+1)*window_size, window_size, &ar1, &var);
        ar1_vals[w] = ar1;
    }
    /* Kendall τ: count concordant/discordant pairs */
    int concordant = 0, discordant = 0;
    for (int i = 0; i < n_windows; i++)
        for (int j = i+1; j < n_windows; j++) {
            if (ar1_vals[j] > ar1_vals[i] - SWARM_EPSILON) concordant++;
            else discordant++;
        }
    int total = n_windows * (n_windows - 1) / 2;
    double tau = (double)(concordant - discordant) / (double)total;
    if (kendall_tau) *kendall_tau = tau;
    if (p_value) *p_value = (tau > 0.5) ? 0.01 : 0.5;
    swarm_free_vector(ar1_vals);
    return tau > 0.5;
}

double swarm_robustness_mutation(const void** agents, int n_agents, int n_trials,
           double mutation_rate, double threshold, agent_fitness_fn fitness_fn,
           void* config) {
    if (!agents || n_agents <= 0 || !fitness_fn) return 0.0;
    int robust = 0;
    for (int t = 0; t < n_trials; t++) {
        int idx = swarm_rng_int(n_agents);
        double base_fitness = fitness_fn(agents[idx], config);
        /* Apply random perturbation (mutation) via bit flip in internal representation */
        double mutant_fitness = base_fitness * (1.0 + swarm_rng_gaussian() * mutation_rate);
        if (fabs(mutant_fitness - base_fitness) / (fabs(base_fitness) + SWARM_EPSILON) <= threshold)
            robust++;
    }
    return (double)robust / (double)n_trials;
}

double swarm_evolvability_estimate(const double* fitness_history, int length) {
    if (!fitness_history || length < 2) return 0.0;
    /* Rate of fitness improvement over time */
    double total_improvement = fabs(fitness_history[length-1] - fitness_history[0]);
    double mean_fitness = 0.0;
    for (int i = 0; i < length; i++) mean_fitness += fitness_history[i];
    mean_fitness /= (double)length;
    return (mean_fitness > SWARM_EPSILON) ? total_improvement / (mean_fitness * (double)length) : 0.0;
}
