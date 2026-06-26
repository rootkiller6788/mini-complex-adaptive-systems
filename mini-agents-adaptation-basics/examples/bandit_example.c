/**
 * bandit_example.c — Multi-Armed Bandit Algorithm Comparison
 *
 * Compares epsilon-greedy, UCB1, Thompson Sampling, and Softmax
 * on a 5-armed bandit problem over 1000 pulls.
 *
 * Knowledge: L6 Canonical Problem — Multi-armed bandit
 *            L7 Application — Algorithm selection via regret comparison
 */

#include "bandit.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== Multi-Armed Bandit Comparison ===\n\n");

    double means[] = {0.1, 0.2, 0.5, 0.55, 0.9};
    double stds[]  = {0.1, 0.1, 0.1, 0.1,  0.1};
    int T = 1000;

    printf("Arm true means: ");
    for (int i = 0; i < 5; i++) printf("%.2f ", means[i]);
    printf("\nBest arm: %d (mean=%.2f)\n\n", 4, 0.9);

    /* ---- epsilon-Greedy (epsilon=0.1) ---- */
    MultiArmedBandit *mab1 = bandit_create(5, means, stds, T);
    double reward_eg = 0.0, regret_eg = 0.0;
    for (int t = 0; t < T; t++) {
        int arm = bandit_epsilon_greedy(mab1, 0.1);
        reward_eg += bandit_pull(mab1, arm);
    }
    regret_eg = mab1->cumulative_regret;
    printf("epsilon-Greedy (eps=0.1):   total_reward=%.2f, regret=%.4f\n",
           reward_eg, regret_eg);
    bandit_destroy(mab1);

    /* ---- UCB1 ---- */
    MultiArmedBandit *mab2 = bandit_create(5, means, stds, T);
    double reward_ucb = 0.0;
    for (int t = 0; t < T; t++) {
        int arm = bandit_ucb1(mab2);
        reward_ucb += bandit_pull(mab2, arm);
    }
    double bound_ucb = bandit_ucb1_regret_bound(mab2, T);
    printf("UCB1:                       total_reward=%.2f, regret=%.4f, bound=%.2f\n",
           reward_ucb, mab2->cumulative_regret, bound_ucb);
    bandit_destroy(mab2);

    /* ---- Thompson Sampling (Beta) ---- */
    MultiArmedBandit *mab3 = bandit_create(5, means, stds, T);
    double reward_ts = 0.0;
    for (int t = 0; t < T; t++) {
        int arm = bandit_thompson_sampling_beta(mab3);
        reward_ts += bandit_pull(mab3, arm);
    }
    printf("Thompson Sampling (Beta):   total_reward=%.2f, regret=%.4f\n",
           reward_ts, mab3->cumulative_regret);
    bandit_destroy(mab3);

    /* ---- Softmax (tau=0.1) ---- */
    MultiArmedBandit *mab4 = bandit_create(5, means, stds, T);
    double reward_sm = 0.0;
    for (int t = 0; t < T; t++) {
        int arm = bandit_softmax(mab4, 0.1);
        reward_sm += bandit_pull(mab4, arm);
    }
    printf("Softmax (tau=0.1):          total_reward=%.2f, regret=%.4f\n",
           reward_sm, mab4->cumulative_regret);
    bandit_destroy(mab4);

    /* ---- epsilon-Decreasing ---- */
    MultiArmedBandit *mab5 = bandit_create(5, means, stds, T);
    double reward_ed = 0.0;
    for (int t = 0; t < T; t++) {
        int arm = bandit_epsilon_decreasing(mab5, 1.0);
        reward_ed += bandit_pull(mab5, arm);
    }
    printf("epsilon-Decreasing:         total_reward=%.2f, regret=%.4f\n",
           reward_ed, mab5->cumulative_regret);
    bandit_destroy(mab5);

    /* ---- Lai-Robbins asymptotic bound ---- */
    MultiArmedBandit *mab_lr = bandit_create(5, means, stds, T);
    double lr_bound = bandit_lai_robbins_bound(mab_lr);
    printf("\nLai-Robbins asymptotic bound coefficient: %.4f\n", lr_bound);
    bandit_destroy(mab_lr);

    /* ---- EXP3 adversarial demonstration ---- */
    printf("\nEXP3 (Adversarial Bandit) over %d rounds:\n", T);
    EXP3State *exp3 = exp3_create(5, 0.1);
    double exp3_reward = 0.0;
    for (int t = 0; t < T; t++) {
        int arm = exp3_select_arm(exp3);
        /* Simulate adversarial: reward = 1 if picking arm 4, else 0 */
        double r = (arm == 4) ? 1.0 : 0.0;
        exp3_update(exp3, arm, r);
        exp3_reward += r;
    }
    double exp3_bound = exp3_regret_bound(5, T);
    printf("  Cumulative reward: %.0f\n", exp3_reward);
    printf("  Theoretical bound: %.2f\n", exp3_bound);
    printf("  Arm weights after %d rounds: ", T);
    /* Access internal weights for diagnostics */
    exp3_destroy(exp3);

    printf("\n=== Bandit Example Complete ===\n");
    return 0;
}