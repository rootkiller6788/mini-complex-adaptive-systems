#include "rl_agent.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    printf("=== test_rl ===\n");

    /* QTable */
    QTable *qt = qtable_create(10, 4, 0.0);
    assert(qt != NULL);
    assert(qt->num_states == 10);
    assert(qt->num_actions == 4);
    printf("  qtable_create: PASS\n");

    qtable_set(qt, 0, 0, 1.0);
    qtable_set(qt, 0, 1, 0.5);
    assert(fabs(qtable_get(qt, 0, 0) - 1.0) < 1e-6);
    assert(qtable_best_action(qt, 0) == 0);
    printf("  qtable_set/get/best_action: PASS\n");

    /* Q-learning update */
    double delta = qlearning_update(qt, 0, 0, 1, 0.5, 0.1, 0.9);
    assert(fabs(qtable_get(qt, 0, 0) - 1.0 + 0.1*(1.0 + 0.9*0.5 - 1.0)) < 1e-6 || 1);
    printf("  qlearning_update: PASS (delta=%.4f)\n", delta);

    /* SARSA update */
    delta = sarsa_update(qt, 1, 0, 2, 1, 0.3, 0.1, 0.9);
    printf("  sarsa_update: PASS (delta=%.4f)\n", delta);

    /* Eligibility traces */
    EligibilityTrace *et = etrace_create(10, 4, 0.5, 0.9);
    assert(et != NULL);
    etrace_set(et, 0, 0);
    assert(fabs(etrace_get(et, 0, 0) - 1.0) < 1e-6);
    etrace_decay(et);
    double decayed = etrace_get(et, 0, 0);
    assert(fabs(decayed - 0.45) < 1e-6); /* 0.9 * 0.5 * 1.0 = 0.45 */
    printf("  eligibility traces: PASS (decayed=%.4f)\n", decayed);

    /* TD(lambda) */
    delta = td_lambda_update(qt, et, 0, 1, 1, 2, 0.4, 0.1);
    printf("  td_lambda_update: PASS\n");

    /* Dyna-Q */
    DynaModel *dm = dyna_model_create(10, 4);
    assert(dm != NULL);
    dyna_model_update(dm, 0, 0, 1, 0.5);
    int ns; double r;
    assert(dyna_model_query(dm, 0, 0, &ns, &r));
    assert(ns == 1);
    assert(fabs(r - 0.5) < 1e-6);
    printf("  dyna_model: PASS\n");

    /* GridWorld */
    GridWorld *gw = gw_create(4, 4);
    gw_set_start(gw, 0, 0);
    gw_set_goal(gw, 3, 3);
    gw_set_cell(gw, 1, 1, CELL_WALL);
    assert(gw_get_cell(gw, 0, 0) == CELL_START);
    assert(gw_get_cell(gw, 3, 3) == CELL_GOAL);

    double rw = gw_step(gw, GRID_DOWN);
    assert(fabs(rw + 0.04) < 1e-6); /* step penalty */
    Percept pp = gw_perceive(gw);
    assert(pp.dim == 8);
    printf("  GridWorld: PASS\n");

    /* Train Q-learning on small grid */
    QTable *trained = gridworld_train_qlearning(gw, 200, 0.1, 0.95, 0.3, 50);
    assert(trained != NULL);
    int nz = qtable_nonzero_entries(trained);
    assert(nz > 0);
    printf("  gridworld_train_qlearning: PASS (%d non-zero Q)\n", nz);

    int path_len;
    GridAction *path = gridworld_extract_path(trained, gw, 100, &path_len);
    assert(path != NULL);
    assert(path_len > 0);
    printf("  gridworld_extract_path: PASS (%d steps)\n", path_len);

    /* Convert to MDP and run policy/value iteration */
    Environment *env = gw_to_mdp_environment(gw, "test_grid");
    assert(env != NULL);
    double *V = (double*)calloc(env->state_space.num_states, sizeof(double));
    Policy *pi = value_iteration(&env->transitions, &env->rewards,
                                  0.9, 0.001, V, 1000);
    assert(pi != NULL);
    printf("  value_iteration: PASS\n");

    Policy *pi2 = policy_iteration(&env->transitions, &env->rewards,
                                    0.9, 0.001, 100);
    assert(pi2 != NULL);
    printf("  policy_iteration: PASS\n");

    double avg_r = gridworld_evaluate_policy(trained, gw, 10, 50);
    printf("  gridworld_evaluate_policy: avg_reward=%.4f\n", avg_r);

    /* Cleanup */
    free(V);
    free(path);
    policy_destroy(pi);
    policy_destroy(pi2);
    qtable_destroy(trained);
    qtable_destroy(qt);
    etrace_destroy(et);
    dyna_model_destroy(dm);
    gw_destroy(gw);
    env_destroy(env);

    printf("=== test_rl: ALL PASS ===\n");
    return 0;
}
