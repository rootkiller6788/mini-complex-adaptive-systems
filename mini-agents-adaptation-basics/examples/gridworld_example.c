/**
 * gridworld_example.c — Q-Learning Agent Navigates a Grid World
 *
 * End-to-end example demonstrating an RL agent learning to navigate
 * a 6x6 grid world with walls, traps, and a goal using Q-learning.
 *
 * Knowledge: L6 Canonical Problem — Grid World navigation
 */

#include "rl_agent.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== GridWorld Q-Learning Example ===\n\n");

    /* Create a 6x6 grid with obstacles */
    GridWorld *gw = gw_create(6, 6);
    gw->step_penalty = -0.04;
    gw->goal_reward = 1.0;
    gw->max_steps = 200;

    gw_set_start(gw, 0, 0);
    gw_set_goal(gw, 5, 5);

    /* Add walls to create a maze-like structure */
    gw_set_cell(gw, 1, 1, CELL_WALL);
    gw_set_cell(gw, 1, 2, CELL_WALL);
    gw_set_cell(gw, 2, 3, CELL_WALL);
    gw_set_cell(gw, 3, 1, CELL_WALL);
    gw_set_cell(gw, 3, 4, CELL_WALL);
    gw_set_cell(gw, 4, 4, CELL_WALL);

    /* Add a trap */
    gw_add_trap(gw, 2, 2, -1.0);

    printf("Initial grid:\n");
    gw_print(gw);
    printf("\n");

    /* Train Q-learning agent */
    printf("Training Q-learning for 500 episodes...\n");
    QTable *qt = gridworld_train_qlearning(gw, 500, 0.1, 0.95, 0.3, 200);

    printf("Learned %d non-zero Q-values out of %d\n",
           qtable_nonzero_entries(qt), gw->rows * gw->cols * 5);

    /* Extract and display learned path */
    int path_len;
    GridAction *path = gridworld_extract_path(qt, gw, 100, &path_len);

    printf("\nLearned path (%d steps):\n", path_len);
    const char *action_names[] = {"UP", "RIGHT", "DOWN", "LEFT", "STAY"};
    for (int i = 0; i < path_len; i++) {
        printf("  Step %d: %s\n", i + 1, action_names[path[i]]);
    }

    /* Evaluate the learned policy */
    double avg_reward = gridworld_evaluate_policy(qt, gw, 100, 200);
    printf("\nAverage reward over 100 evaluation episodes: %.4f\n", avg_reward);

    /* Also train with SARSA for comparison */
    printf("\nTraining SARSA for 500 episodes...\n");
    QTable *qt_sarsa = gridworld_train_sarsa(gw, 500, 0.1, 0.95, 0.3, 200);
    double avg_sarsa = gridworld_evaluate_policy(qt_sarsa, gw, 100, 200);
    printf("SARSA average reward: %.4f\n", avg_sarsa);

    /* Convert to MDP and run Value Iteration for optimal comparison */
    Environment *env = gw_to_mdp_environment(gw, "grid_mdp");
    double *V = (double*)calloc(env->state_space.num_states, sizeof(double));
    Policy *optimal = value_iteration(&env->transitions, &env->rewards,
                                       0.95, 0.001, V, 1000);
    printf("\nValue Iteration converged. Optimal V*(start) = %.4f\n", V[0]);

    /* Cleanup */
    free(V);
    free(path);
    policy_destroy(optimal);
    qtable_destroy(qt);
    qtable_destroy(qt_sarsa);
    env_destroy(env);
    gw_destroy(gw);

    printf("\n=== GridWorld Example Complete ===\n");
    return 0;
}