/**
 * agent.c â€” Core Agent Implementation
 *
 * Implements fundamental agent abstraction: perception-action loop,
 * bounded rationality (satisficing), exploration/exploitation tradeoffs,
 * social learning, and state-space metrics.
 *
 * Reference: Russell & Norvig "AI: A Modern Approach" (Ch2)
 *            Simon "Models of Man" (1957) â€” Bounded Rationality
 *            Holland "Hidden Order" (1995) â€” Adaptive Agents in CAS
 *
 * Knowledge: L1 Definitions, L2 Core Concepts, L3 Math Structures
 */

#include "agent.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

/* ===================================================================
 * L2: Agent Lifecycle
 * =================================================================== */

Agent* agent_create(int id, const char *name, double adaptation_rate) {
    Agent *a = (Agent*)calloc(1, sizeof(Agent));
    if (!a) return NULL;
    a->agent_id = id;
    strncpy(a->name, name, 63);
    a->name[63] = '\0';
    a->adaptation_rate = adaptation_rate;
    a->fitness = 0.0;
    a->cumulative_reward = 0.0;
    a->age = 0;
    a->memory_size = 0;
    a->private_data = NULL;
    a->policy = NULL;
    a->learn = NULL;
    a->sense = NULL;
    a->actuate = NULL;
    /* Initialize state */
    a->state.state_id = 0;
    a->state.value_estimate = 0.0;
    a->state.visit_count = 0;
    a->state.state_dim = 0;
    memset(a->state.belief_vec, 0, sizeof(a->state.belief_vec));
    /* Default PEAS */
    snprintf(a->peas.performance_measure, 127, "maximize cumulative reward");
    snprintf(a->peas.environment_type, 127, "general MDP");
    snprintf(a->peas.actuators, 255, "actions in action space");
    snprintf(a->peas.sensors, 255, "full state observation");
    return a;
}

Action agent_step(Agent *a, const Percept *p) {
    a->age++;
    if (a->policy) {
        return a->policy(a, p);
    }
    /* Default: no-op action */
    Action act;
    act.type = ACTION_DISCRETE;
    act.discrete_choice = 0;
    act.dim = 0;
    memset(act.continuous_vec, 0, sizeof(act.continuous_vec));
    return act;
}

void agent_learn_from_experience(Agent *a, const Percept *p,
                                  const Action *act, double reward) {
    a->cumulative_reward += reward;
    a->fitness = a->cumulative_reward / (double)(a->age > 0 ? a->age : 1);
    a->state.visit_count++;
    a->state.value_estimate = (1.0 - a->adaptation_rate) * a->state.value_estimate
                              + a->adaptation_rate * reward;
    /* Update state belief from percept */
    for (int i = 0; i < p->dim && i < MAX_STATE_DIM; i++) {
        a->state.belief_vec[i] = (1.0 - a->adaptation_rate) * a->state.belief_vec[i]
                                 + a->adaptation_rate * p->values[i];
    }
    a->state.state_dim = p->dim < MAX_STATE_DIM ? p->dim : MAX_STATE_DIM;
    if (a->learn) {
        a->learn(a, p, act, reward);
    }
}

double agent_fitness(const Agent *a) {
    return a->fitness;
}

void agent_reset(Agent *a) {
    a->cumulative_reward = 0.0;
    a->fitness = 0.0;
    a->age = 0;
    a->state.state_id = 0;
    a->state.value_estimate = 0.0;
    a->state.visit_count = 0;
    a->state.state_dim = 0;
    memset(a->state.belief_vec, 0, sizeof(a->state.belief_vec));
    memset(a->memory, 0, sizeof(a->memory));
    a->memory_size = 0;
}

void agent_destroy(Agent *a) {
    if (a) {
        free(a->private_data);
        free(a);
    }
}
/* ===================================================================
 * L2: Bounded Rationality ¡ª Simon (1957)
 *
 * Bounded rationality recognizes that agents have limited cognitive
 * resources. Rather than optimizing, they satisfice: choose the first
 * option that meets an aspiration level.
 * =================================================================== */

Action agent_satisfice(Agent *a, const Percept *p,
                        double aspiration_level, int max_lookahead) {
    Action best_action;
    best_action.type = ACTION_DISCRETE;
    best_action.discrete_choice = 0;
    best_action.dim = 0;
    memset(best_action.continuous_vec, 0, sizeof(best_action.continuous_vec));

    double best_value = -DBL_MAX;
    int found_satisficing = 0;

    /* Evaluate actions up to max_lookahead */
    for (int action_id = 0; action_id < max_lookahead && action_id < MAX_ACTION_DIM;
         action_id++) {
        /* Heuristic action evaluation from current belief state */
        double estimated_value = 0.0;
        for (int d = 0; d < p->dim && d < MAX_STATE_DIM; d++) {
            estimated_value += p->values[d] * (double)(action_id + 1) / (double)max_lookahead;
        }
        estimated_value /= (p->dim > 0 ? (double)p->dim : 1.0);

        if (estimated_value > best_value) {
            best_value = estimated_value;
            best_action.discrete_choice = action_id;
        }
        /* Check aspiration: stop search if this action is "good enough" */
        if (estimated_value >= aspiration_level) {
            found_satisficing = 1;
            best_action.discrete_choice = action_id;
            break;
        }
    }

    /* Store deliberation cost in memory */
    a->memory[a->memory_size % 256] = (double)(found_satisficing ? 1 : max_lookahead);
    a->memory_size++;
    return best_action;
}

double agent_adapt_aspiration(Agent *a, double recent_reward_history[],
                               int history_len) {
    if (history_len <= 0) return 0.0;

    /* Compute exponentially weighted moving average of recent rewards */
    double ewma = recent_reward_history[0];
    double alpha = 0.3;
    for (int i = 1; i < history_len; i++) {
        ewma = alpha * recent_reward_history[i] + (1.0 - alpha) * ewma;
        alpha *= 0.95; /* decay the smoothing factor */
    }

    /* Compute variance for exploration bonus */
    double variance = 0.0;
    for (int i = 0; i < history_len; i++) {
        double diff = recent_reward_history[i] - ewma;
        variance += diff * diff;
    }
    variance /= (double)history_len;
    double exploration_bonus = sqrt(variance > 0.0 ? variance : 0.01);

    /* Adapt agent aspiration: aspiration rises with success, falls with failure */
    a->memory[a->memory_size % 256] = ewma;
    a->memory_size++;
    return ewma + 0.5 * exploration_bonus;
}

/* ===================================================================
 * L2: Exploration vs Exploitation (Sutton & Barto, 1998)
 * =================================================================== */

int agent_epsilon_greedy(const double *action_values, int n_actions,
                          double epsilon) {
    if (n_actions <= 0) return -1;
    if (n_actions == 1) return 0;

    /* Exploration: random action with probability epsilon */
    double rand_val = (double)rand() / (double)RAND_MAX;
    if (rand_val < epsilon) {
        return rand() % n_actions;
    }
    /* Exploitation: best estimated action */
    int best = 0;
    double best_val = action_values[0];
    for (int i = 1; i < n_actions; i++) {
        if (action_values[i] > best_val) {
            best_val = action_values[i];
            best = i;
        }
    }
    return best;
}

int agent_boltzmann_selection(const double *action_values, int n_actions,
                               double temperature) {
    if (n_actions <= 0) return -1;
    if (n_actions == 1) return 0;

    /* Compute softmax probabilities with numerical stability (subtract max) */
    double max_val = action_values[0];
    for (int i = 1; i < n_actions; i++) {
        if (action_values[i] > max_val) max_val = action_values[i];
    }

    double sum_exp = 0.0;
    double probs[64];
    double safe_temp = (temperature < 1e-10) ? 1e-10 : temperature;

    for (int i = 0; i < n_actions && i < 64; i++) {
        probs[i] = exp((action_values[i] - max_val) / safe_temp);
        sum_exp += probs[i];
    }

    /* Categorical sampling from softmax distribution */
    double rand_val = (double)rand() / (double)RAND_MAX;
    double cumulative = 0.0;
    for (int i = 0; i < n_actions && i < 64; i++) {
        cumulative += probs[i] / sum_exp;
        if (rand_val <= cumulative) return i;
    }
    return n_actions - 1;
}

double agent_policy_entropy(const double *action_probs, int n_actions) {
    /* H(pi) = -sum_i p_i log(p_i) ¡ª measures exploration/noise in policy */
    double entropy = 0.0;
    for (int i = 0; i < n_actions; i++) {
        if (action_probs[i] > 1e-12) {
            entropy -= action_probs[i] * log(action_probs[i]);
        }
    }
    return entropy;
}

/* ===================================================================
 * L3: Mathematical Structures ¡ª Agent State Spaces
 * =================================================================== */

double agent_state_similarity(const AgentState *a, const AgentState *b) {
    /* Cosine similarity between belief vectors */
    int dim = (a->state_dim < b->state_dim) ? a->state_dim : b->state_dim;
    if (dim <= 0) return 0.0;
    if (dim > MAX_STATE_DIM) dim = MAX_STATE_DIM;

    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (int i = 0; i < dim; i++) {
        dot += a->belief_vec[i] * b->belief_vec[i];
        norm_a += a->belief_vec[i] * a->belief_vec[i];
        norm_b += b->belief_vec[i] * b->belief_vec[i];
    }
    double denom = sqrt(norm_a) * sqrt(norm_b);
    if (denom < 1e-12) return 0.0;
    return dot / denom;
}

AgentState agent_state_merge(const AgentState *a, const AgentState *b,
                              double weight_a) {
    /* Weighted Bayesian-style fusion of two belief states */
    AgentState merged;
    memset(&merged, 0, sizeof(merged));
    int dim = (a->state_dim > b->state_dim) ? a->state_dim : b->state_dim;
    if (dim > MAX_STATE_DIM) dim = MAX_STATE_DIM;
    merged.state_dim = dim;

    double w_a = weight_a;
    double w_b = 1.0 - weight_a;
    for (int i = 0; i < dim; i++) {
        merged.belief_vec[i] = w_a * a->belief_vec[i] + w_b * b->belief_vec[i];
    }
    merged.value_estimate = w_a * a->value_estimate + w_b * b->value_estimate;
    merged.visit_count = a->visit_count + b->visit_count;
    merged.state_id = a->state_id;
    return merged;
}

int agent_discretize_percept(const Percept *p, int bins_per_dim) {
    /* Convert continuous percept vector to a discrete state index.
     * State index = sum_i floor(value_i * bins) * (bins)^i
     * (L3: Discretization/tiling ¡ª fundamental state representation) */
    if (bins_per_dim <= 1) return 0;
    int state_index = 0;
    int multiplier = 1;
    for (int i = 0; i < p->dim && i < MAX_PERCEPT_DIM; i++) {
        double clamped = p->values[i];
        if (clamped < 0.0) clamped = 0.0;
        if (clamped > 1.0) clamped = 1.0;
        int bin = (int)(clamped * (double)bins_per_dim);
        if (bin >= bins_per_dim) bin = bins_per_dim - 1;
        state_index += bin * multiplier;
        multiplier *= bins_per_dim;
        if (multiplier > 1000000) break; /* prevent overflow */
    }
    return state_index;
}

/* ===================================================================
 * L2: Multi-Agent Interactions ¡ª Social Learning
 * =================================================================== */

void agent_send_message(Agent *sender, Agent *receiver,
                         const char *message, double intensity) {
    /* Stigmergic communication: environment-mediated indirect communication.
     * Intensity modulates the impact weight on receiver. */
    if (!sender || !receiver || !message) return;

    int msg_len = (int)strlen(message);
    int slot = receiver->memory_size % 256;
    receiver->memory[slot] = intensity * (double)msg_len;
    receiver->memory_size++;

    /* Modulate receiver adaptation rate based on message intensity */
    double old_rate = receiver->adaptation_rate;
    receiver->adaptation_rate = old_rate * (1.0 - intensity * 0.1)
                                + intensity * 0.1 * 0.5;
}

void agent_observe_peer(Agent *observer, const Agent *peer,
                         const Percept *context, const Action *peer_action) {
    if (!observer || !peer || !context) return;

    /* Observational (vicarious) learning:
     * Update own state estimates based on peer action and context.
     * Only learn from peers with higher fitness. */
    double peer_fitness = peer->fitness;
    double self_fitness = observer->fitness;

    if (peer_fitness > self_fitness) {
        double gap = peer_fitness - self_fitness;
        double influence = gap / (fabs(self_fitness) + 1.0);

        /* Update belief toward peer successful context */
        for (int i = 0; i < context->dim && i < MAX_STATE_DIM; i++) {
            observer->state.belief_vec[i] =
                (1.0 - influence * 0.1) * observer->state.belief_vec[i]
                + influence * 0.1 * context->values[i];
        }

        /* Store peer action pattern in observer memory */
        int mem_slot = observer->memory_size % 256;
        observer->memory[mem_slot] = (double)peer_action->discrete_choice;
        observer->memory_size++;
    }
}

void agent_imitate(Agent *learner, const Agent *model, double imitation_rate) {
    if (!learner || !model) return;

    /* Direct imitation: copy model parameters proportional to imitation_rate.
     * Social learning bias: only imitate more successful models. */
    if (model->fitness <= learner->fitness) return;

    double fitness_ratio = model->fitness / (learner->fitness + 1e-8);
    double effective_rate = imitation_rate * (fitness_ratio - 1.0);
    if (effective_rate > 1.0) effective_rate = 1.0;
    if (effective_rate < 0.0) effective_rate = 0.0;

    /* Imitate state beliefs */
    int dim = (learner->state.state_dim > model->state.state_dim)
              ? learner->state.state_dim : model->state.state_dim;
    if (dim > MAX_STATE_DIM) dim = MAX_STATE_DIM;

    for (int i = 0; i < dim; i++) {
        learner->state.belief_vec[i] =
            (1.0 - effective_rate) * learner->state.belief_vec[i]
            + effective_rate * model->state.belief_vec[i];
    }

    /* Imitate adaptation rate */
    learner->adaptation_rate =
        (1.0 - effective_rate) * learner->adaptation_rate
        + effective_rate * model->adaptation_rate;
}
