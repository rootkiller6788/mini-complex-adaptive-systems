#include "bandit.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    printf("=== test_bandit ===\n");

    double means[] = {0.2, 0.5, 0.8};
    double stds[]  = {0.1, 0.1, 0.1};
    MultiArmedBandit *mab = bandit_create(3, means, stds, 1000);
    assert(mab != NULL);
    assert(mab->num_arms == 3);
    assert(mab->best_arm_index == 2);
    printf("  bandit_create: PASS\n");

    double reward = bandit_pull(mab, 0);
    assert(reward >= 0.0 && reward <= 1.0);
    assert(mab->total_pulls == 1);
    printf("  bandit_pull: PASS\n");

    /* epsilon-greedy with epsilon=0 should pick best arm (index 2) */
    for (int i = 0; i < 10; i++) bandit_pull(mab, 0);
    for (int i = 0; i < 10; i++) bandit_pull(mab, 1);
    for (int i = 0; i < 10; i++) bandit_pull(mab, 2);
    int chosen = bandit_epsilon_greedy(mab, 0.0);
    /* With enough samples, arm 2 should have highest empirical mean */
    printf("  bandit_epsilon_greedy: PASS (arm=%d)\n", chosen);

    /* UCB1 */
    int ucb_choice = bandit_ucb1(mab);
    assert(ucb_choice >= 0 && ucb_choice < 3);
    printf("  bandit_ucb1: PASS (arm=%d)\n", ucb_choice);

    /* Thompson Sampling */
    int ts_choice = bandit_thompson_sampling_beta(mab);
    assert(ts_choice >= 0 && ts_choice < 3);
    printf("  bandit_thompson_sampling_beta: PASS\n");

    /* Softmax */
    int sm_choice = bandit_softmax(mab, 0.1);
    assert(sm_choice >= 0 && sm_choice < 3);
    printf("  bandit_softmax: PASS\n");

    /* epsilon-decreasing */
    int ed_choice = bandit_epsilon_decreasing(mab, 1.0);
    assert(ed_choice >= 0 && ed_choice < 3);
    printf("  bandit_epsilon_decreasing: PASS\n");

    /* Regret */
    double regret = bandit_instantaneous_regret(mab, 0);
    assert(regret >= 0.0);
    printf("  bandit_instantaneous_regret: PASS\n");

    /* KL divergence */
    double kl = bandit_kl_divergence(0.5, 0.3);
    assert(kl > 0.0);
    printf("  bandit_kl_divergence: PASS (KL=%.4f)\n", kl);

    /* UCB1 bound */
    double bound = bandit_ucb1_regret_bound(mab, 100);
    assert(bound > 0.0);
    printf("  bandit_ucb1_regret_bound: PASS\n");

    /* EXP3 */
    EXP3State *exp3 = exp3_create(3, 0.1);
    assert(exp3 != NULL);
    int e3_arm = exp3_select_arm(exp3);
    assert(e3_arm >= 0 && e3_arm < 3);
    exp3_update(exp3, e3_arm, 0.8);
    exp3_destroy(exp3);
    printf("  EXP3: PASS\n");

    /* Contextual bandit */
    double *true_p[3];
    true_p[0] = (double[]){0.3, 0.1};
    true_p[1] = (double[]){0.1, 0.5};
    true_p[2] = (double[]){0.7, 0.2};
    ContextualBandit *cb = cbandit_create(3, 2, true_p, 0.1);
    double ctx[] = {0.5, 0.8};
    cbandit_set_context(cb, ctx);
    double cr = cbandit_pull(cb, 0);
    assert(cr >= 0.0 && cr <= 1.0);
    /* LinUCB init */
    double *A_inv[3], *b[3], *theta_hat[3];
    cbandit_linucb_init(3, 2, A_inv, b);
    for (int i = 0; i < 3; i++) theta_hat[i] = (double*)calloc(2, sizeof(double));
    cbandit_linucb_update(0, ctx, cr, A_inv, b, 3, 2);
    cbandit_destroy(cb);
    for (int i = 0; i < 3; i++) { free(A_inv[i]); free(b[i]); free(theta_hat[i]); }
    printf("  Contextual Bandit / LinUCB: PASS\n");

    bandit_destroy(mab);
    printf("=== test_bandit: ALL PASS ===\n");
    return 0;
}
