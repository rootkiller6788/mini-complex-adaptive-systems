/**
 * rl_agent.c â€” Reinforcement Learning Agents
 *
 * Implements core tabular RL algorithms:
 *   Q-Learning (Watkins & Dayan, 1992) â€” off-policy TD control
 *   SARSA (Rummery & Niranjan, 1994) â€” on-policy TD control
 *   TD(lambda) with eligibility traces (Sutton, 1988)
 *   Dyna-Q integrated planning (Sutton, 1990)
 *   Policy Iteration and Value Iteration (Howard, 1960 / Bellman, 1957)
 *   GridWorld training and path extraction
 *
 * Reference: Sutton & Barto "Reinforcement Learning" (1998, 2018)
 *
 * Knowledge: L1 Definitions, L2 Core Concepts, L4 Fundamental Laws,
 *            L5 Algorithms, L6 Canonical Problems (GridWorld solver)
 */

#include "rl_agent.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

/* ===================================================================
 * L1: Q-Table Construction and Core Operations
 * =================================================================== */

QTable* qtable_create(int num_states, int num_actions, double init_value) {
    if (num_states <= 0 || num_actions <= 0) return NULL;
    QTable *qt = (QTable*)calloc(1, sizeof(QTable));
    if (!qt) return NULL;

    qt->num_states = num_states;
    qt->num_actions = num_actions;
    int total = num_states * num_actions;

    qt->values = (double*)calloc(total, sizeof(double));
    qt->visit_counts = (int*)calloc(total, sizeof(int));
    qt->initialized = (bool*)calloc(total, sizeof(bool));

    if (init_value != 0.0) {
        for (int i = 0; i < total; i++) {
            qt->values[i] = init_value;
        }
    }

    return qt;
}

double qtable_get(const QTable *qt, int state, int action) {
    if (!qt || state < 0 || state >= qt->num_states
        || action < 0 || action >= qt->num_actions)
        return 0.0;
    return qt->values[state * qt->num_actions + action];
}

void qtable_set(QTable *qt, int state, int action, double value) {
    if (!qt || state < 0 || state >= qt->num_states
        || action < 0 || action >= qt->num_actions)
        return;
    int idx = state * qt->num_actions + action;
    qt->values[idx] = value;
    qt->initialized[idx] = true;
}

int qtable_best_action(const QTable *qt, int state) {
    if (!qt || state < 0 || state >= qt->num_states) return 0;

    int best = 0;
    double best_val = qt->values[state * qt->num_actions];
    for (int a = 1; a < qt->num_actions; a++) {
        double val = qt->values[state * qt->num_actions + a];
        if (val > best_val) {
            best_val = val;
            best = a;
        }
    }
    return best;
}

double qtable_max_value(const QTable *qt, int state) {
    if (!qt || state < 0 || state >= qt->num_states) return 0.0;
    return qt->values[state * qt->num_actions + qtable_best_action(qt, state)];
}

int qtable_epsilon_greedy(const QTable *qt, int state, double epsilon) {
    if (!qt || state < 0 || state >= qt->num_states || qt->num_actions <= 0)
        return 0;
    if (qt->num_actions == 1) return 0;

    if ((double)rand() / RAND_MAX < epsilon) {
        return rand() % qt->num_actions;
    }
    return qtable_best_action(qt, state);
}

int qtable_boltzmann(const QTable *qt, int state, double temperature) {
    if (!qt || state < 0 || state >= qt->num_states || qt->num_actions <= 0)
        return 0;

    double max_val = qtable_get(qt, state, 0);
    for (int a = 1; a < qt->num_actions; a++) {
        double v = qtable_get(qt, state, a);
        if (v > max_val) max_val = v;
    }

    double sum = 0.0;
    double prob[32];
    double safe_temp = (temperature < 1e-10) ? 1e-10 : temperature;
    int na = qt->num_actions < 32 ? qt->num_actions : 32;

    for (int a = 0; a < na; a++) {
        prob[a] = exp((qtable_get(qt, state, a) - max_val) / safe_temp);
        sum += prob[a];
    }

    double r = (double)rand() / RAND_MAX;
    double cum = 0.0;
    for (int a = 0; a < na; a++) {
        cum += prob[a] / sum;
        if (r <= cum) return a;
    }
    return na - 1;
}

void qtable_reset(QTable *qt) {
    if (!qt) return;
    int total = qt->num_states * qt->num_actions;
    memset(qt->values, 0, total * sizeof(double));
    memset(qt->visit_counts, 0, total * sizeof(int));
    memset(qt->initialized, 0, total * sizeof(bool));
}

void qtable_destroy(QTable *qt) {
    if (!qt) return;
    free(qt->values);
    free(qt->visit_counts);
    free(qt->initialized);
    free(qt);
}

int qtable_nonzero_entries(const QTable *qt) {
    if (!qt) return 0;
    int count = 0;
    int total = qt->num_states * qt->num_actions;
    for (int i = 0; i < total; i++) {
        if (fabs(qt->values[i]) > 1e-12) count++;
    }
    return count;
}

/* ===================================================================
 * L4: Q-Learning â€” Watkins & Dayan (1992)
 *
 * Off-policy TD control. Updates Q(s,a) using max over next actions,
 * independent of the behavior policy.
 *
 * Update rule:
 *   Q(s,a) := Q(s,a) + alpha * [r + gamma * max_a' Q(s',a') - Q(s,a)]
 *
 * Theorem: Q-learning converges to Q* with probability 1 under:
 *   - finite MDP, bounded rewards
 *   - infinite visits to all (s,a) pairs
 *   - sum_t alpha_t = inf, sum_t alpha_t^2 < inf (Robbins-Monro conditions)
 *   (Watkins & Dayan, 1992; Jaakkola, Jordan & Singh, 1994)
 * =================================================================== */

double qlearning_update(QTable *qt, int state, int action,
                         int next_state, double reward,
                         double alpha, double gamma) {
    if (!qt) return 0.0;

    double old_q = qtable_get(qt, state, action);
    double max_next = qtable_max_value(qt, next_state);

    /* TD error: delta = r + gamma * max_a' Q(s',a') - Q(s,a) */
    double delta = reward + gamma * max_next - old_q;

    /* Update: Q(s,a) := Q(s,a) + alpha * delta */
    double new_q = old_q + alpha * delta;
    qtable_set(qt, state, action, new_q);

    /* Update visit count */
    int idx = state * qt->num_actions + action;
    qt->visit_counts[idx]++;

    return delta;
}

/* ===================================================================
 * L4: SARSA â€” Rummery & Niranjan (1994)
 *
 * On-policy TD control. Updates using the action actually taken next,
 * following the current policy (SARSA = State-Action-Reward-State-Action).
 *
 * Update rule:
 *   Q(s,a) := Q(s,a) + alpha * [r + gamma * Q(s',a') - Q(s,a)]
 *
 * SARSA converges under the same conditions as Q-learning but to the
 * optimal policy of the GLIE (Greedy in the Limit with Infinite
 * Exploration) class (Singh et al., 2000).
 * =================================================================== */

double sarsa_update(QTable *qt, int state, int action,
                     int next_state, int next_action, double reward,
                     double alpha, double gamma) {
    if (!qt) return 0.0;

    double old_q = qtable_get(qt, state, action);
    double next_q = qtable_get(qt, next_state, next_action);

    /* TD error using actual next action (on-policy) */
    double delta = reward + gamma * next_q - old_q;

    double new_q = old_q + alpha * delta;
    qtable_set(qt, state, action, new_q);

    int idx = state * qt->num_actions + action;
    qt->visit_counts[idx]++;

    return delta;
}

/* ===================================================================
 * L5: TD(lambda) â€” Eligibility Traces (Sutton, 1988)
 *
 * Backward view of TD(lambda): eligibility traces propagate TD error
 * backward through recently visited states.
 *
 * Accumulating trace: e(s,a) <- gamma*lambda*e(s,a) + 1
 * Q(s,a) <- Q(s,a) + alpha * delta * e(s,a)
 *
 * TD(0) = one-step backup (lambda=0)
 * TD(1) = Monte Carlo (lambda=1, no discounting of traces?)
 * Intermediate lambda = multi-step backup
 * =================================================================== */

EligibilityTrace* etrace_create(int num_states, int num_actions,
                                 double lambda, double gamma) {
    EligibilityTrace *et = (EligibilityTrace*)calloc(1, sizeof(EligibilityTrace));
    if (!et) return NULL;

    et->num_states = num_states;
    et->num_actions = num_actions;
    et->lambda = lambda;
    et->gamma = gamma;

    et->traces = (double*)calloc(num_states * num_actions, sizeof(double));
    return et;
}

void etrace_reset(EligibilityTrace *et) {
    if (!et) return;
    memset(et->traces, 0, et->num_states * et->num_actions * sizeof(double));
}

void etrace_decay(EligibilityTrace *et) {
    if (!et) return;
    double decay = et->gamma * et->lambda;
    int total = et->num_states * et->num_actions;
    for (int i = 0; i < total; i++) {
        et->traces[i] *= decay;
    }
}

void etrace_set(EligibilityTrace *et, int state, int action) {
    if (!et || state < 0 || state >= et->num_states
        || action < 0 || action >= et->num_actions) return;
    int idx = state * et->num_actions + action;
    /* Replacing trace: set to 1 (overwrites previous trace) */
    et->traces[idx] = 1.0;
}

double etrace_get(const EligibilityTrace *et, int state, int action) {
    if (!et || state < 0 || state >= et->num_states
        || action < 0 || action >= et->num_actions) return 0.0;
    return et->traces[state * et->num_actions + action];
}

double td_lambda_update(QTable *qt, EligibilityTrace *et,
                         int state, int action,
                         int next_state, int next_action, double reward,
                         double alpha) {
    if (!qt || !et) return 0.0;

    /* Compute TD error */
    double old_q = qtable_get(qt, state, action);
    double next_q = qtable_get(qt, next_state, next_action);
    double delta = reward + et->gamma * next_q - old_q;

    /* Set accumulating trace for current (s,a) */
    etrace_set(et, state, action);

    /* Update all Q values proportional to their eligibility traces */
    int total = qt->num_states * qt->num_actions;
    for (int i = 0; i < total; i++) {
        if (fabs(et->traces[i]) > 1e-12) {
            qt->values[i] += alpha * delta * et->traces[i];
        }
    }

    /* Decay all traces */
    etrace_decay(et);

    return delta;
}

void etrace_destroy(EligibilityTrace *et) {
    if (!et) return;
    free(et->traces);
    free(et);
}
/* ===================================================================
 * L5: Dyna-Q ¡ª Integrated Planning and Learning (Sutton, 1990)
 *
 * Dyna architecture: after each real experience, perform n simulated
 * planning updates. The learned model predicts (s', r) for each (s, a).
 * Planning is done via Q-learning updates on simulated experiences.
 *
 * Key insight: planning and learning are the same algorithmic process
 * operating on different sources of experience (real vs. simulated).
 * =================================================================== */

DynaModel* dyna_model_create(int num_states, int num_actions) {
    DynaModel *m = (DynaModel*)calloc(1, sizeof(DynaModel));
    if (!m) return NULL;

    m->num_states = num_states;
    m->num_actions = num_actions;
    int total = num_states * num_actions;

    m->next_state = (int*)calloc(total, sizeof(int));
    m->reward = (double*)calloc(total, sizeof(double));
    m->observed = (bool*)calloc(total, sizeof(bool));

    return m;
}

void dyna_model_update(DynaModel *m, int state, int action,
                        int next_state, double reward) {
    if (!m || state < 0 || state >= m->num_states
        || action < 0 || action >= m->num_actions)
        return;

    int idx = state * m->num_actions + action;
    m->next_state[idx] = next_state;
    m->reward[idx] = reward;
    m->observed[idx] = true;
}

bool dyna_model_sample(const DynaModel *m, int *state_out, int *action_out) {
    if (!m) return false;

    /* Count observed (s,a) pairs */
    int total = m->num_states * m->num_actions;
    int observed_count = 0;
    for (int i = 0; i < total; i++) {
        if (m->observed[i]) observed_count++;
    }
    if (observed_count == 0) return false;

    /* Randomly select one observed pair */
    int target = rand() % observed_count;
    int found = 0;
    for (int i = 0; i < total; i++) {
        if (m->observed[i]) {
            if (found == target) {
                *state_out = i / m->num_actions;
                *action_out = i % m->num_actions;
                return true;
            }
            found++;
        }
    }
    return false;
}

bool dyna_model_query(const DynaModel *m, int state, int action,
                       int *next_state_out, double *reward_out) {
    if (!m || state < 0 || state >= m->num_states
        || action < 0 || action >= m->num_actions)
        return false;

    int idx = state * m->num_actions + action;
    if (!m->observed[idx]) return false;

    *next_state_out = m->next_state[idx];
    *reward_out = m->reward[idx];
    return true;
}

double dyna_q_plan_step(QTable *qt, DynaModel *model,
                          double alpha, double gamma) {
    if (!qt || !model) return 0.0;

    /* Sample a previously observed (s,a) */
    int s, a;
    if (!dyna_model_sample(model, &s, &a)) return 0.0;

    /* Query model for predicted (s', r) */
    int s_next;
    double r;
    if (!dyna_model_query(model, s, a, &s_next, &r)) return 0.0;

    /* Q-learning update on simulated experience */
    return qlearning_update(qt, s, a, s_next, r, alpha, gamma);
}

void dyna_q_step(QTable *qt, DynaModel *model,
                  int state, int action, int next_state, double reward,
                  double alpha, double gamma, int planning_steps) {
    /* Step 1: direct RL update from real experience */
    qlearning_update(qt, state, action, next_state, reward, alpha, gamma);

    /* Step 2: update model */
    dyna_model_update(model, state, action, next_state, reward);

    /* Step 3: planning ¡ª n simulated Q-learning updates */
    for (int i = 0; i < planning_steps; i++) {
        dyna_q_plan_step(qt, model, alpha, gamma);
    }
}

void dyna_model_destroy(DynaModel *m) {
    if (!m) return;
    free(m->next_state);
    free(m->reward);
    free(m->observed);
    free(m);
}

/* ===================================================================
 * L5: Policy Iteration (Howard, 1960)
 *
 * Dynamic Programming algorithm for finding optimal policy in known MDP.
 *
 * Policy Evaluation: iteratively compute V^pi using Bellman expectation eq:
 *   V(s) = sum_a pi(a|s) sum_{s'} P(s'|s,a)[R(s,a,s') + gamma*V(s')]
 *
 * Policy Improvement: make policy greedy w.r.t. V:
 *   pi'(s) = argmax_a sum_{s'} P(s'|s,a)[R(s,a,s') + gamma*V(s')]
 *
 * Theorem (Howard, 1960): Policy iteration converges to optimal policy
 * in a finite number of iterations for finite MDPs.
 * =================================================================== */

Policy* policy_create_random(int num_states, int num_actions) {
    if (num_states <= 0) return NULL;
    Policy *p = (Policy*)calloc(1, sizeof(Policy));
    p->num_states = num_states;
    p->actions = (int*)calloc(num_states, sizeof(int));
    for (int s = 0; s < num_states; s++) {
        p->actions[s] = rand() % num_actions;
    }
    return p;
}

Policy* policy_from_qtable(const QTable *qt) {
    if (!qt) return NULL;
    Policy *p = (Policy*)calloc(1, sizeof(Policy));
    p->num_states = qt->num_states;
    p->actions = (int*)calloc(qt->num_states, sizeof(int));
    for (int s = 0; s < qt->num_states; s++) {
        p->actions[s] = qtable_best_action(qt, s);
    }
    return p;
}

void policy_evaluation(const TransitionModel *trans, const RewardFunction *rew,
                        double gamma, double theta,
                        double *V, int max_iterations) {
    if (!trans || !rew || !V) return;

    int S = trans->num_states;
    int A = trans->num_actions;

    for (int iter = 0; iter < max_iterations; iter++) {
        double delta = 0.0;
        for (int s = 0; s < S; s++) {
            double old_v = V[s];
            double new_v = 0.0;

            /* Compute expected value under uniform random policy
             * V(s) = (1/|A|) * sum_a sum_{s'} P(s'|s,a) [R + gamma*V(s')] */
            for (int a = 0; a < A; a++) {
                double action_value = 0.0;
                for (int s_next = 0; s_next < S; s_next++) {
                    int idx = s * A * S + a * S + s_next;
                    double prob = trans->probs[idx];
                    if (fabs(prob) < 1e-12) continue;

                    double r = rew->depends_on_next_state && rew->rewards_sas
                        ? rew->rewards_sas[idx]
                        : rew->rewards[s * A + a];

                    action_value += prob * (r + gamma * V[s_next]);
                }
                new_v += action_value / (double)A;
            }

            V[s] = new_v;
            delta = fmax(delta, fabs(old_v - new_v));
        }
        if (delta < theta) break;
    }
}

bool policy_improvement(const TransitionModel *trans,
                         const RewardFunction *rew,
                         double gamma, Policy *pi, const double *V) {
    if (!trans || !rew || !pi || !V) return false;

    int S = trans->num_states;
    int A = trans->num_actions;
    bool policy_stable = true;

    for (int s = 0; s < S; s++) {
        int old_action = pi->actions[s];
        int best_action = 0;
        double best_value = -DBL_MAX;

        for (int a = 0; a < A; a++) {
            double action_value = 0.0;
            for (int s_next = 0; s_next < S; s_next++) {
                int idx = s * A * S + a * S + s_next;
                double prob = trans->probs[idx];
                if (fabs(prob) < 1e-12) continue;

                double r = rew->depends_on_next_state && rew->rewards_sas
                    ? rew->rewards_sas[idx]
                    : rew->rewards[s * A + a];

                action_value += prob * (r + gamma * V[s_next]);
            }
            if (action_value > best_value) {
                best_value = action_value;
                best_action = a;
            }
        }

        if (best_action != old_action) {
            policy_stable = false;
        }
        pi->actions[s] = best_action;
    }

    return policy_stable;
}

Policy* policy_iteration(const TransitionModel *trans,
                          const RewardFunction *rew,
                          double gamma, double theta,
                          int max_iterations) {
    if (!trans || !rew) return NULL;

    int S = trans->num_states;
    int A = trans->num_actions;

    /* Initialize V(s) = 0, random policy */
    double *V = (double*)calloc(S, sizeof(double));
    Policy *pi = policy_create_random(S, A);

    for (int iter = 0; iter < max_iterations; iter++) {
        /* Policy Evaluation */
        for (int eval_iter = 0; eval_iter < 1000; eval_iter++) {
            double delta = 0.0;
            for (int s = 0; s < S; s++) {
                double old_v = V[s];
                int a = pi->actions[s];
                double new_v = 0.0;
                for (int s_next = 0; s_next < S; s_next++) {
                    int idx = s * A * S + a * S + s_next;
                    double prob = trans->probs[idx];
                    if (fabs(prob) < 1e-12) continue;
                    double r = rew->depends_on_next_state && rew->rewards_sas
                        ? rew->rewards_sas[idx]
                        : rew->rewards[s * A + a];
                    new_v += prob * (r + gamma * V[s_next]);
                }
                V[s] = new_v;
                delta = fmax(delta, fabs(old_v - new_v));
            }
            if (delta < theta) break;
        }

        /* Policy Improvement */
        bool stable = policy_improvement(trans, rew, gamma, pi, V);
        if (stable) break;
    }

    free(V);
    return pi;
}

/* ===================================================================
 * L5: Value Iteration (Bellman, 1957)
 *
 * Directly compute V* via Bellman optimality equation:
 *   V(s) := max_a sum_{s'} P(s'|s,a) [R(s,a,s') + gamma*V(s')]
 *
 * Converges to V* in the limit. Policy extracted greedily afterward.
 * =================================================================== */

Policy* value_iteration(const TransitionModel *trans,
                         const RewardFunction *rew,
                         double gamma, double theta,
                         double *V_out, int max_iterations) {
    if (!trans || !rew) return NULL;

    int S = trans->num_states;
    int A = trans->num_actions;
    double *V = (double*)calloc(S, sizeof(double));

    for (int iter = 0; iter < max_iterations; iter++) {
        double delta = 0.0;
        for (int s = 0; s < S; s++) {
            double old_v = V[s];

            /* max_a sum_{s'} P(s'|s,a)[R + gamma*V(s')] */
            double max_val = -DBL_MAX;
            for (int a = 0; a < A; a++) {
                double action_val = 0.0;
                for (int s_next = 0; s_next < S; s_next++) {
                    int idx = s * A * S + a * S + s_next;
                    double prob = trans->probs[idx];
                    if (fabs(prob) < 1e-12) continue;
                    double r = rew->depends_on_next_state && rew->rewards_sas
                        ? rew->rewards_sas[idx]
                        : rew->rewards[s * A + a];
                    action_val += prob * (r + gamma * V[s_next]);
                }
                if (action_val > max_val) max_val = action_val;
            }

            V[s] = max_val;
            delta = fmax(delta, fabs(old_v - max_val));
        }
        if (delta < theta) break;
    }

    /* Extract greedy policy from V* */
    Policy *pi = (Policy*)calloc(1, sizeof(Policy));
    pi->num_states = S;
    pi->actions = (int*)calloc(S, sizeof(int));

    for (int s = 0; s < S; s++) {
        int best_a = 0;
        double best_val = -DBL_MAX;
        for (int a = 0; a < A; a++) {
            double action_val = 0.0;
            for (int s_next = 0; s_next < S; s_next++) {
                int idx = s * A * S + a * S + s_next;
                double prob = trans->probs[idx];
                if (fabs(prob) < 1e-12) continue;
                double r = rew->depends_on_next_state && rew->rewards_sas
                    ? rew->rewards_sas[idx]
                    : rew->rewards[s * A + a];
                action_val += prob * (r + gamma * V[s_next]);
            }
            if (action_val > best_val) {
                best_val = action_val;
                best_a = a;
            }
        }
        pi->actions[s] = best_a;
    }

    if (V_out) memcpy(V_out, V, S * sizeof(double));
    free(V);
    return pi;
}

void policy_destroy(Policy *p) {
    if (!p) return;
    free(p->actions);
    free(p);
}

/* ===================================================================
 * L6: Grid World Solver ¡ª Training RL agents on GridWorld
 * =================================================================== */

QTable* gridworld_train_qlearning(GridWorld *gw, int n_episodes,
                                   double alpha, double gamma,
                                   double epsilon, int max_steps_per_ep) {
    if (!gw) return NULL;

    int S = gw->rows * gw->cols;
    int A = 5; /* 5 grid actions */
    QTable *qt = qtable_create(S, A, 0.0);

    for (int ep = 0; ep < n_episodes; ep++) {
        gw_reset(gw);
        int state = gw->agent_row * gw->cols + gw->agent_col;

        for (int step = 0; step < max_steps_per_ep; step++) {
            /* Select action: epsilon-greedy with decay over episodes */
            double eps_decay = epsilon / (1.0 + (double)ep / 100.0);
            int action = qtable_epsilon_greedy(qt, state, eps_decay);

            /* Execute action in grid world */
            double reward = gw_step(gw, (GridAction)action);
            int next_state = gw->agent_row * gw->cols + gw->agent_col;

            /* Q-learning update */
            qlearning_update(qt, state, action, next_state, reward, alpha, gamma);

            state = next_state;
            if (gw->done) break;
        }
    }

    return qt;
}

QTable* gridworld_train_sarsa(GridWorld *gw, int n_episodes,
                               double alpha, double gamma,
                               double epsilon, int max_steps_per_ep) {
    if (!gw) return NULL;

    int S = gw->rows * gw->cols;
    int A = 5;
    QTable *qt = qtable_create(S, A, 0.0);

    for (int ep = 0; ep < n_episodes; ep++) {
        gw_reset(gw);
        int state = gw->agent_row * gw->cols + gw->agent_col;

        /* Choose first action using current policy */
        double eps_decay = epsilon / (1.0 + (double)ep / 100.0);
        int action = qtable_epsilon_greedy(qt, state, eps_decay);

        for (int step = 0; step < max_steps_per_ep; step++) {
            double reward = gw_step(gw, (GridAction)action);
            int next_state = gw->agent_row * gw->cols + gw->agent_col;

            /* Choose next action for SARSA update */
            int next_action = qtable_epsilon_greedy(qt, next_state, eps_decay);

            /* SARSA update: on-policy TD */
            sarsa_update(qt, state, action, next_state, next_action,
                          reward, alpha, gamma);

            state = next_state;
            action = next_action;

            if (gw->done) break;
        }
    }

    return qt;
}

GridAction* gridworld_extract_path(const QTable *qt, const GridWorld *gw,
                                    int max_path_len, int *path_len) {
    if (!qt || !gw) {
        *path_len = 0;
        return NULL;
    }

    GridAction *path = (GridAction*)calloc(max_path_len, sizeof(GridAction));
    if (!path) {
        *path_len = 0;
        return NULL;
    }

    /* Create a temporary grid copy for path extraction */
    int row = gw->start_row;
    int col = gw->start_col;
    int path_idx = 0;

    while (path_idx < max_path_len) {
        int state = row * gw->cols + col;
        GridCell cell = gw->cells[state];

        /* Stop at goal or trap */
        if (cell == CELL_GOAL || cell == CELL_TRAP) break;

        int action = qtable_best_action(qt, state);
        path[path_idx++] = (GridAction)action;

        /* Move agent position */
        switch (action) {
            case GRID_UP:    row--; break;
            case GRID_RIGHT: col++; break;
            case GRID_DOWN:  row++; break;
            case GRID_LEFT:  col--; break;
            case GRID_STAY:  break;
        }

        /* Boundary/wall check */
        if (row < 0) row = 0;
        if (row >= gw->rows) row = gw->rows - 1;
        if (col < 0) col = 0;
        if (col >= gw->cols) col = gw->cols - 1;
        if (gw->cells[row * gw->cols + col] == CELL_WALL) {
            /* Undo move */
            row = (action == GRID_UP) ? row + 1 :
                  (action == GRID_DOWN) ? row - 1 : row;
            col = (action == GRID_LEFT) ? col + 1 :
                  (action == GRID_RIGHT) ? col - 1 : col;
            path_idx--; /* retract invalid step */
        }
    }

    *path_len = path_idx;
    return path;
}

double gridworld_evaluate_policy(const QTable *qt, GridWorld *gw,
                                  int n_episodes, int max_steps) {
    if (!qt || !gw) return 0.0;

    double total_reward = 0.0;

    for (int ep = 0; ep < n_episodes; ep++) {
        gw_reset(gw);

        for (int step = 0; step < max_steps; step++) {
            int state = gw->agent_row * gw->cols + gw->agent_col;
            int action = qtable_best_action(qt, state); /* greedy policy */
            total_reward += gw_step(gw, (GridAction)action);
            if (gw->done) break;
        }
    }

    return total_reward / (double)n_episodes;
}
