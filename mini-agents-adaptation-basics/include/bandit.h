/**
 * bandit.h — Multi-Armed Bandit Algorithms
 *
 * The multi-armed bandit (MAB) embodies the exploration-exploitation dilemma.
 * An agent chooses among K arms, each providing stochastic rewards from an
 * unknown distribution. The goal is to maximize cumulative reward.
 *
 * Reference: Robbins (1952), Lai & Robbins (1985), Auer et al. (2002)
 *            Thompson (1933), Bubeck & Cesa-Bianchi (2012)
 *
 * Knowledge coverage: L1 (Definitions), L2 (Explore/Exploit),
 *                     L4 (Regret bounds), L5 (Algorithms), L6 (Canonical Problem)
 */

#ifndef BANDIT_H
#define BANDIT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

/* L1: BanditArm — one action with unknown reward distribution */
typedef struct {
    int      arm_id;
    char     name[32];
    double   true_mean;
    double   true_std;
    double   empirical_mean;
    double   sum_rewards;
    int      pull_count;
    double   sum_sq_rewards;
    double   beta_alpha;
    double   beta_beta;
} BanditArm;

/* L1: MultiArmedBandit */
typedef struct {
    int        num_arms;
    BanditArm *arms;
    int        horizon;
    int        total_pulls;
    double     cumulative_regret;
    int        best_arm_index;
    double     best_arm_mean;
} MultiArmedBandit;

MultiArmedBandit* bandit_create(int K, const double *true_means,
                                 const double *true_stds, int horizon);
double bandit_pull(MultiArmedBandit *mab, int arm_index);
void   bandit_update_arm(BanditArm *arm, double reward);
double bandit_instantaneous_regret(const MultiArmedBandit *mab, int arm_chosen);
void   bandit_reset(MultiArmedBandit *mab);
double bandit_expected_regret(const MultiArmedBandit *mab, int arm_index);
void   bandit_destroy(MultiArmedBandit *mab);

/* L5: Bandit strategies */
int    bandit_epsilon_greedy(const MultiArmedBandit *mab, double epsilon);
int    bandit_ucb1(const MultiArmedBandit *mab);
int    bandit_ucb(const MultiArmedBandit *mab, double c);
int    bandit_thompson_sampling_beta(MultiArmedBandit *mab);
int    bandit_thompson_sampling_gaussian(const MultiArmedBandit *mab);
int    bandit_softmax(const MultiArmedBandit *mab, double temperature);
int    bandit_epsilon_decreasing(MultiArmedBandit *mab, double epsilon_0);
int    bandit_optimistic_init(MultiArmedBandit *mab, double optimistic_value);

/* L4: Regret analysis */
double bandit_cumulative_regret_sim(MultiArmedBandit *mab, int T,
                                     int (*strategy_fn)(const MultiArmedBandit*));
double bandit_lai_robbins_bound(const MultiArmedBandit *mab);
double bandit_ucb1_regret_bound(const MultiArmedBandit *mab, int T);
double bandit_kl_divergence(double p, double q);
double bandit_gap(const MultiArmedBandit *mab, int arm_index);

/* L6: Contextual Bandit */
typedef struct {
    int        num_arms;
    int        context_dim;
    double   **true_params;
    double     noise_std;
    double    *current_context;
} ContextualBandit;

ContextualBandit* cbandit_create(int num_arms, int context_dim,
                                  double **true_params, double noise_std);
void   cbandit_set_context(ContextualBandit *cb, const double *context);
double cbandit_pull(ContextualBandit *cb, int arm);
int    cbandit_linucb_select(ContextualBandit *cb, double alpha,
                              double **A_inv, double **b, double **theta_hat);
void   cbandit_linucb_update(int arm, const double *context, double reward,
                              double **A_inv, double **b,
                              int num_arms, int context_dim);
void   cbandit_linucb_init(int num_arms, int context_dim,
                            double **A_inv, double **b);
void   cbandit_destroy(ContextualBandit *cb);

/* L7: Adversarial Bandit — EXP3 (Auer et al., 2002) */
typedef struct {
    int        num_arms;
    double    *weights;
    double     gamma;
    double     eta;
    int        time_step;
} EXP3State;

EXP3State* exp3_create(int K, double gamma);
int    exp3_select_arm(EXP3State *state);
void   exp3_update(EXP3State *state, int chosen_arm, double reward);
void   exp3_destroy(EXP3State *state);
double exp3_regret_bound(int K, int T);

#endif /* BANDIT_H */
