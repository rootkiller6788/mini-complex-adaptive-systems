#include "swarm_consensus.h"
#include "swarm_core.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Distributed Consensus — Core Implementation
 *
 * References:
 *   Olfati-Saber, Fax & Murray (2007) — Consensus in multi-agent systems
 *   Tsitsiklis (1984) — Decentralized decision making
 *   Ren & Beard (2008) — Distributed consensus
 *   Boyd et al. (2006) — Gossip algorithms
 *   Bikhchandani, Hirshleifer & Welch (1992) — Information cascades
 * ============================================================================ */

/* ── Consensus State ── */
ConsensusState* swarm_consensus_create(int n_agents, const double* initial_values) {
    if (n_agents <= 0) return NULL;
    ConsensusState* cs = (ConsensusState*)calloc(1, sizeof(ConsensusState));
    if (!cs) return NULL;
    cs->n_agents = n_agents;
    cs->values = swarm_alloc_vector(n_agents);
    cs->values_new = swarm_alloc_vector(n_agents);
    cs->weights = swarm_matrix_alloc(n_agents, n_agents);
    if (!cs->values || !cs->values_new || !cs->weights) {
        swarm_consensus_free(cs); return NULL;
    }
    if (initial_values) memcpy(cs->values, initial_values, (size_t)n_agents * sizeof(double));
    cs->epsilon = 1e-6;
    return cs;
}

void swarm_consensus_free(ConsensusState* state) {
    if (!state) return;
    swarm_free_vector(state->values);
    swarm_free_vector(state->values_new);
    swarm_matrix_free(state->weights, state->n_agents);
    free(state);
}

void swarm_consensus_reset(ConsensusState* state, const double* initial_values) {
    if (!state || !initial_values) return;
    memcpy(state->values, initial_values, (size_t)state->n_agents * sizeof(double));
    state->converged = false;
    state->iterations = 0;
    state->disagreement = 1e10;
}

/* ── Weight Matrix Construction ── */

/* Max-degree weights: w_ij = 1/(d_max+1) for j∈N_i, w_ii = 1 - Σ_{j≠i} w_ij */
void swarm_consensus_weights_max_degree(double** W, int n, const double** adjacency) {
    if (!W || !adjacency || n <= 0) return;
    /* Compute degrees */
    int max_deg = 0;
    for (int i = 0; i < n; i++) {
        int deg = 0;
        for (int j = 0; j < n; j++) if (j != i && adjacency[i][j] > 0.5) deg++;
        if (deg > max_deg) max_deg = deg;
    }
    double eps = 1.0 / (double)(max_deg + 1);
    for (int i = 0; i < n; i++) {
        double off_diag_sum = 0.0;
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            if (adjacency[i][j] > 0.5) { W[i][j] = eps; off_diag_sum += eps; }
            else W[i][j] = 0.0;
        }
        W[i][i] = 1.0 - off_diag_sum;
    }
}

/* Metropolis-Hastings weights: w_ij = 1/(1+max(d_i, d_j)) */
void swarm_consensus_weights_metropolis(double** W, int n, const double** adjacency) {
    if (!W || !adjacency || n <= 0) return;
    int deg[SWARM_MAX_AGENTS];
    for (int i = 0; i < n; i++) {
        deg[i] = 0;
        for (int j = 0; j < n; j++) if (j != i && adjacency[i][j] > 0.5) deg[i]++;
    }
    for (int i = 0; i < n; i++) {
        double off_diag_sum = 0.0;
        for (int j = 0; j < n; j++) {
            if (i == j || adjacency[i][j] < 0.5) { W[i][j] = 0.0; continue; }
            int max_deg = (deg[i] > deg[j]) ? deg[i] : deg[j];
            W[i][j] = 1.0 / (1.0 + (double)max_deg);
            off_diag_sum += W[i][j];
        }
        W[i][i] = 1.0 - off_diag_sum;
        if (W[i][i] < 0.0) W[i][i] = 0.0;
    }
}

/* Laplacian-based weights: W = I - εL */
void swarm_consensus_weights_laplacian(double** W, int n, const double** adjacency, double epsilon) {
    if (!W || !adjacency || n <= 0) return;
    double** L = swarm_graph_laplacian((double**)adjacency, n);
    if (!L) return;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            W[i][j] = (i == j ? 1.0 : 0.0) - epsilon * L[i][j];
    }
    swarm_matrix_free(L, n);
}

/* ── Consensus Iteration ── */
void swarm_consensus_iterate(ConsensusState* state, double epsilon) {
    if (!state) return;
    int n = state->n_agents;
    /* x(k+1) = W · x(k) */
    for (int i = 0; i < n; i++) {
        state->values_new[i] = 0.0;
        for (int j = 0; j < n; j++)
            state->values_new[i] += state->weights[i][j] * state->values[j];
    }
    /* Swap buffers */
    double* tmp = state->values;
    state->values = state->values_new;
    state->values_new = tmp;
    state->iterations++;
    
    /* Check disagreement */
    double max_val = state->values[0], min_val = state->values[0];
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        if (state->values[i] > max_val) max_val = state->values[i];
        if (state->values[i] < min_val) min_val = state->values[i];
        sum += state->values[i];
    }
    state->disagreement = max_val - min_val;
    state->average = sum / (double)n;
    state->converged = (state->disagreement < epsilon);
}

int swarm_consensus_run(ConsensusState* state, int max_iterations) {
    if (!state || max_iterations <= 0) return 0;
    for (int iter = 0; iter < max_iterations; iter++) {
        swarm_consensus_iterate(state, state->epsilon);
        if (state->converged) return state->iterations;
    }
    return state->iterations;
}

bool swarm_consensus_is_reached(const ConsensusState* state, double threshold) {
    if (!state) return false;
    return state->disagreement < threshold;
}

double swarm_consensus_convergence_rate(const double** W, int n) {
    if (!W || n <= 1) return 0.0;
    /* Power iteration for second largest eigenvalue */
    /* Simplified: use spectral gap estimate via row sums */
    double min_row_sum = 1e10;
    for (int i = 0; i < n; i++) {
        double row_sum = 0.0;
        for (int j = 0; j < n; j++) row_sum += W[i][j];
        if (row_sum < min_row_sum) min_row_sum = row_sum;
    }
    return 1.0 - min_row_sum;
}

/* ── Collective Decision Making ── */

void swarm_decision_majority_vote(double* opinions, const int* neighbor_lists,
           const int* neighbor_counts, int n_agents, int max_neighbors) {
    if (!opinions || !neighbor_lists || !neighbor_counts) return;
    double* new_opinions = swarm_alloc_vector(n_agents);
    if (!new_opinions) return;
    for (int i = 0; i < n_agents; i++) {
        double pos = 0.0, neg = 0.0;
        /* Self included */
        if (opinions[i] > 0.5) pos++; else neg++;
        int count = neighbor_counts[i];
        if (count > max_neighbors) count = max_neighbors;
        for (int j = 0; j < count; j++) {
            int nb = neighbor_lists[i * max_neighbors + j];
            if (nb < 0 || nb >= n_agents) continue;
            if (opinions[nb] > 0.5) pos++; else neg++;
        }
        new_opinions[i] = (pos >= neg) ? 1.0 : 0.0;
    }
    memcpy(opinions, new_opinions, (size_t)n_agents * sizeof(double));
    swarm_free_vector(new_opinions);
}

void swarm_decision_voter_model(double* opinions, const int* neighbor_lists,
           const int* neighbor_counts, int n_agents, int max_neighbors,
           int steps_per_agent) {
    if (!opinions || !neighbor_lists || !neighbor_counts) return;
    for (int s = 0; s < steps_per_agent; s++) {
        int i = swarm_rng_int(n_agents);
        int count = neighbor_counts[i];
        if (count > max_neighbors) count = max_neighbors;
        if (count > 0) {
            int nb_idx = swarm_rng_int(count);
            int nb = neighbor_lists[i * max_neighbors + nb_idx];
            if (nb >= 0 && nb < n_agents)
                opinions[i] = opinions[nb];
        }
    }
}

void swarm_decision_threshold_model(double* opinions, double* thresholds,
           const int* neighbor_lists, const int* neighbor_counts,
           int n_agents, int max_neighbors) {
    if (!opinions || !thresholds || !neighbor_lists || !neighbor_counts) return;
    double* new_opinions = swarm_alloc_vector(n_agents);
    if (!new_opinions) return;
    for (int i = 0; i < n_agents; i++) {
        if (opinions[i] > 0.5) { new_opinions[i] = 1.0; continue; }
        /* Count adopting neighbors */
        int count = neighbor_counts[i];
        if (count > max_neighbors) count = max_neighbors;
        int adopters = 0;
        for (int j = 0; j < count; j++) {
            int nb = neighbor_lists[i * max_neighbors + j];
            if (nb >= 0 && nb < n_agents && opinions[nb] > 0.5) adopters++;
        }
        double fraction = (count > 0) ? (double)adopters / (double)count : 0.0;
        if (fraction >= thresholds[i]) new_opinions[i] = 1.0;
        else new_opinions[i] = 0.0;
    }
    memcpy(opinions, new_opinions, (size_t)n_agents * sizeof(double));
    swarm_free_vector(new_opinions);
}

bool swarm_decision_quorum_sensed(const double* opinions, int n_agents,
           double quorum_threshold, int* quorum_count) {
    if (!opinions || n_agents <= 0) return false;
    int count = 0;
    for (int i = 0; i < n_agents; i++) if (opinions[i] > 0.5) count++;
    if (quorum_count) *quorum_count = count;
    return (double)count / (double)n_agents >= quorum_threshold;
}

double swarm_decision_weighted_majority(const double* opinions,
           const double* weights, int n_agents) {
    if (!opinions || !weights || n_agents <= 0) return 0.0;
    double w_sum = 0.0, w_pos = 0.0;
    for (int i = 0; i < n_agents; i++) {
        w_sum += weights[i];
        if (opinions[i] > 0.5) w_pos += weights[i];
    }
    return (w_sum > SWARM_EPSILON) ? w_pos / w_sum : 0.0;
}

/* ── Gossip Algorithms ── */

void swarm_gossip_pairwise(double* values, int n_agents, int n_exchanges) {
    if (!values || n_agents <= 1) return;
    for (int k = 0; k < n_exchanges; k++) {
        int i = swarm_rng_int(n_agents);
        int j = swarm_rng_int(n_agents);
        if (i == j) continue;
        double avg = (values[i] + values[j]) * 0.5;
        values[i] = values[j] = avg;
    }
}

void swarm_gossip_broadcast(double* values, int n_agents,
           const int* neighbor_lists, const int* neighbor_counts,
           int max_neighbors, int n_broadcasts) {
    if (!values || !neighbor_lists || !neighbor_counts) return;
    for (int k = 0; k < n_broadcasts; k++) {
        int src = swarm_rng_int(n_agents);
        double val = values[src];
        int count = neighbor_counts[src];
        if (count > max_neighbors) count = max_neighbors;
        for (int j = 0; j < count; j++) {
            int nb = neighbor_lists[src * max_neighbors + j];
            if (nb >= 0 && nb < n_agents)
                values[nb] = (values[nb] + val) * 0.5;
        }
    }
}

void swarm_gossip_geographic(double* values, int n_agents,
           const int* neighbor_lists, const int* neighbor_counts,
           int max_neighbors, int n_gossips) {
    if (!values || !neighbor_lists || !neighbor_counts) return;
    /* Simplified: route by random walk, average at destination */
    for (int k = 0; k < n_gossips; k++) {
        int src = swarm_rng_int(n_agents);
        double src_val = values[src];
        /* Random walk 3 steps */
        int current = src;
        for (int hop = 0; hop < 3; hop++) {
            int count = neighbor_counts[current];
            if (count > max_neighbors) count = max_neighbors;
            if (count == 0) break;
            int nb = neighbor_lists[current * max_neighbors + swarm_rng_int(count)];
            if (nb >= 0 && nb < n_agents) current = nb;
        }
        values[current] = (values[current] + src_val) * 0.5;
    }
}

/* ── Rendezvous ── */
void swarm_rendezvous_circumcenter(double* positions, int n_agents, int dim,
           const int* neighbor_lists, const int* neighbor_counts) {
    if (!positions || !neighbor_lists || !neighbor_counts) return;
    (void)dim;
    for (int i = 0; i < n_agents; i++) {
        /* Move toward average neighbor position (simplified) */
        int cnt = neighbor_counts[i];
        if (cnt <= 0) continue;
        double sum[3] = {0.0, 0.0, 0.0};
        for (int j = 0; j < cnt; j++) {
            int nb = neighbor_lists[i * SWARM_MAX_NEIGHBORS + j];
            if (nb >= 0 && nb < n_agents)
                for (int d = 0; d < dim && d < 3; d++)
                    sum[d] += positions[nb * dim + d];
        }
        for (int d = 0; d < dim && d < 3; d++)
            positions[i * dim + d] = sum[d] / (double)cnt;
    }
}

void swarm_formation_consensus(double* positions, int n_agents, int dim,
           const double** desired_offsets, double step_size, int n_steps) {
    if (!positions || !desired_offsets || n_agents <= 1) return;
    for (int step = 0; step < n_steps; step++) {
        for (int i = 0; i < n_agents; i++) {
            for (int j = 0; j < n_agents; j++) {
                if (i == j) continue;
                /* Difference from desired offset */
                double current[3], desired[3];
                for (int d = 0; d < dim && d < 3; d++) {
                    current[d] = positions[j*dim + d] - positions[i*dim + d];
                    desired[d] = desired_offsets[i][j*dim + d];
                    double error = current[d] - desired[d];
                    positions[i*dim + d] += step_size * error;
                }
            }
        }
    }
}

/* ── Information Cascades ── */
void swarm_cascade_update(double* beliefs, const int* decisions,
           const double* private_signals, int n_agents, double signal_accuracy) {
    if (!beliefs || !decisions || !private_signals) return;
    /* Bikhchandani-Hirshleifer-Welch cascade: agents act sequentially */
    int n_high = 0, n_low = 0;
    for (int i = 0; i < n_agents; i++) {
        if (i > 0) {
            /* Observe predecessor decisions */
            double public_signal = (n_high > n_low) ? 1.0 : ((n_low > n_high) ? -1.0 : 0.0);
            /* Bayesian update: P(high|private, public) */
            double private_lr = (private_signals[i] > 0.5) ? signal_accuracy / (1.0 - signal_accuracy)
                                                           : (1.0 - signal_accuracy) / signal_accuracy;
            double public_lr = 1.0; /* Simplified */
            beliefs[i] = private_lr * public_lr / (1.0 + private_lr * public_lr);
        } else {
            beliefs[i] = (private_signals[i] > 0.5) ? signal_accuracy : 1.0 - signal_accuracy;
        }
        /* Decision: threshold at 0.5 */
        int decision = (beliefs[i] >= 0.5) ? 1 : 0;
        if (decision == 1) n_high++; else n_low++;
        ((int*)decisions)[i] = decision;
    }
}

/* ── Algebraic Connectivity ── */
double swarm_consensus_algebraic_connectivity(const double** Laplacian, int n) {
    if (!Laplacian || n <= 1) return 0.0;
    /* Power iteration approximation for λ₂ (Fiedler value) */
    /* Find λ_max first, then shift */
    double tr = 0.0;
    for (int i = 0; i < n; i++) tr += Laplacian[i][i];
    double lambda_max = tr; /* Upper bound */
    /* Estimate λ₂ via deflation (simplified) */
    double min_nonzero = lambda_max;
    for (int i = 0; i < n; i++) {
        double val = Laplacian[i][i];
        if (val > SWARM_EPSILON && val < min_nonzero) min_nonzero = val;
    }
    return min_nonzero;
}

double swarm_consensus_spectral_gap(const double** W, int n) {
    if (!W || n <= 1) return 0.0;
    /* Spectral gap = 1 - |λ₂(W)| ≈ rate of convergence */
    double trace = 0.0;
    for (int i = 0; i < n; i++) trace += W[i][i];
    /* For symmetric doubly-stochastic matrix, λ₂ ≈ 1 - 1/(diameter * mixing_time) */
    double diag_avg = trace / (double)n;
    return 1.0 - fabs(diag_avg);
}

double swarm_consensus_epsilon_time(const double** W, int n, double epsilon,
           double initial_disagreement) {
    if (!W || n <= 1) return 0.0;
    double spectral_gap = swarm_consensus_spectral_gap(W, n);
    if (spectral_gap < SWARM_EPSILON) return 1e10;
    double ratio = epsilon / (initial_disagreement + SWARM_EPSILON);
    if (ratio <= 0.0) return 0.0;
    return log(ratio) / log(1.0 - spectral_gap);
}
