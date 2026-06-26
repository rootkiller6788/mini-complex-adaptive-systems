/**
 * rl_agent.h — Reinforcement Learning Agents
 *
 * Implements core RL algorithms: Q-learning, SARSA, TD(lambda), eligibility
 * traces, Dyna-Q integrated planning, policy iteration, and value iteration.
 *
 * Reference: Sutton & Barto "Reinforcement Learning: An Introduction" (1998,2018)
 *            Watkins & Dayan "Q-Learning" (1992) Machine Learning 8:279-292
 *            Rummery & Niranjan "On-Line Q-Learning" (1994)
 *            Howard "Dynamic Programming and Markov Processes" (1960)
 *
 * Knowledge coverage: L1-L6 (Definitions through Canonical Problems)
 */

#ifndef RL_AGENT_H
#define RL_AGENT_H

#include "agent.h"
#include "environment.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* L1: Q-Table — tabular action-value function */
typedef struct {
    int      num_states;
    int      num_actions;
    double  *values;
    int     *visit_counts;
    bool    *initialized;
} QTable;

QTable* qtable_create(int num_states, int num_actions, double init_value);
double  qtable_get(const QTable *qt, int state, int action);
void    qtable_set(QTable *qt, int state, int action, double value);
int     qtable_best_action(const QTable *qt, int state);
double  qtable_max_value(const QTable *qt, int state);
int     qtable_epsilon_greedy(const QTable *qt, int state, double epsilon);
int     qtable_boltzmann(const QTable *qt, int state, double temperature);
void    qtable_reset(QTable *qt);
void    qtable_destroy(QTable *qt);
int     qtable_nonzero_entries(const QTable *qt);

/* L4: Q-Learning (Watkins & Dayan, 1992) — off-policy TD control */
double qlearning_update(QTable *qt, int state, int action,
                         int next_state, double reward,
                         double alpha, double gamma);

/* L4: SARSA — on-policy TD control (Rummery & Niranjan, 1994) */
double sarsa_update(QTable *qt, int state, int action,
                     int next_state, int next_action, double reward,
                     double alpha, double gamma);

/* L5: Eligibility Trace for TD(lambda) (Sutton, 1988) */
typedef struct {
    int      num_states;
    int      num_actions;
    double  *traces;
    double   lambda;
    double   gamma;
} EligibilityTrace;

EligibilityTrace* etrace_create(int num_states, int num_actions,
                                 double lambda, double gamma);
void   etrace_reset(EligibilityTrace *et);
void   etrace_decay(EligibilityTrace *et);
void   etrace_set(EligibilityTrace *et, int state, int action);
double etrace_get(const EligibilityTrace *et, int state, int action);
double td_lambda_update(QTable *qt, EligibilityTrace *et,
                         int state, int action,
                         int next_state, int next_action, double reward,
                         double alpha);
void   etrace_destroy(EligibilityTrace *et);

/* L5: Dyna-Q — integrated planning and learning (Sutton, 1990) */
typedef struct {
    int      num_states;
    int      num_actions;
    int     *next_state;
    double  *reward;
    bool    *observed;
} DynaModel;

DynaModel* dyna_model_create(int num_states, int num_actions);
void   dyna_model_update(DynaModel *m, int state, int action,
                          int next_state, double reward);
bool   dyna_model_sample(const DynaModel *m, int *state_out, int *action_out);
bool   dyna_model_query(const DynaModel *m, int state, int action,
                         int *next_state_out, double *reward_out);
double dyna_q_plan_step(QTable *qt, DynaModel *model,
                          double alpha, double gamma);
void   dyna_q_step(QTable *qt, DynaModel *model,
                    int state, int action, int next_state, double reward,
                    double alpha, double gamma, int planning_steps);
void   dyna_model_destroy(DynaModel *m);

/* L5: Policy Iteration (Howard, 1960) and Value Iteration */
typedef struct {
    int      num_states;
    int     *actions;
} Policy;

Policy* policy_create_random(int num_states, int num_actions);
Policy* policy_from_qtable(const QTable *qt);
void    policy_evaluation(const TransitionModel *trans, const RewardFunction *rew,
                           double gamma, double theta,
                           double *V, int max_iterations);
bool    policy_improvement(const TransitionModel *trans,
                            const RewardFunction *rew,
                            double gamma, Policy *pi, const double *V);
Policy* policy_iteration(const TransitionModel *trans,
                          const RewardFunction *rew,
                          double gamma, double theta,
                          int max_iterations);
Policy* value_iteration(const TransitionModel *trans,
                         const RewardFunction *rew,
                         double gamma, double theta,
                         double *V_out, int max_iterations);
void    policy_destroy(Policy *p);

/* L6: Grid World RL training */
QTable*     gridworld_train_qlearning(GridWorld *gw, int n_episodes,
                                       double alpha, double gamma,
                                       double epsilon, int max_steps_per_ep);
QTable*     gridworld_train_sarsa(GridWorld *gw, int n_episodes,
                                   double alpha, double gamma,
                                   double epsilon, int max_steps_per_ep);
GridAction* gridworld_extract_path(const QTable *qt, const GridWorld *gw,
                                    int max_path_len, int *path_len);
double      gridworld_evaluate_policy(const QTable *qt, GridWorld *gw,
                                       int n_episodes, int max_steps);

#endif /* RL_AGENT_H */
