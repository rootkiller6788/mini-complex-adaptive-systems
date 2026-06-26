/**
 * environment.h — Environment Models for Agent-Based Systems
 *
 * Defines grid worlds, general Markov environments, fitness landscapes,
 * and multi-agent simulation containers.
 *
 * Reference: Sutton & Barto "Reinforcement Learning" Ch3 (MDPs)
 *            Mitchell "Complexity" Ch10-12
 *            Kauffman "The Origins of Order" (1993) — NK landscapes
 *
 * Knowledge coverage: L1 (Definitions), L2 (Core Concepts), L3 (Math Structures)
 */

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "agent.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* L1: Environment observability classification */
typedef enum {
    ENV_FULLY_OBSERVABLE,
    ENV_PARTIALLY_OBSERVABLE,
    ENV_HIDDEN
} Observability;

/* L1: Environment dynamics type */
typedef enum {
    ENV_DETERMINISTIC,
    ENV_STOCHASTIC,
    ENV_ADVERSARIAL
} DynamicsType;

/* L1: State space */
typedef struct {
    int      num_states;
    int      state_dim;
    double  *state_matrix;
    int      terminal_states[64];
    int      num_terminal;
} StateSpace;

/* L1: Action space */
typedef struct {
    int      num_actions;
    int      action_dim;
    char   **action_names;
} ActionSpace;

/* L3: Transition tensor P(s' | s, a) */
typedef struct {
    int      num_states;
    int      num_actions;
    double  *probs;
} TransitionModel;

/* L3: Reward function R(s, a) */
typedef struct {
    int      num_states;
    int      num_actions;
    double  *rewards;
    bool     depends_on_next_state;
    double  *rewards_sas;
} RewardFunction;

typedef struct Environment Environment;

struct Environment {
    char           name[128];
    Observability  observability;
    DynamicsType   dynamics;
    int            current_state;
    StateSpace     state_space;
    ActionSpace    action_space;
    TransitionModel transitions;
    RewardFunction   rewards;
    int   (*transition_fn)(struct Environment *env, int state,
                            const Action *action, double *reward_out);
    Percept (*observe_fn)(struct Environment *env, int state, int agent_id);
    uint64_t       time_step;
    int            max_steps;
    Agent        **agents;
    int            num_agents;
    int            max_agents;
};

Environment* env_create(const char *name, int num_states, int num_actions,
                         Observability obs, DynamicsType dyn);
double env_step(Environment *env, int agent_id, const Action *action,
                Percept *percept_out);
Percept env_observe(const Environment *env, int agent_id);
int    env_register_agent(Environment *env, Agent *agent);
void   env_remove_agent(Environment *env, int agent_id);
void   env_reset(Environment *env);
void   env_destroy(Environment *env);

void   env_set_transition(Environment *env, int from_state, int action_idx,
                           int to_state, double probability);
void   env_set_reward(Environment *env, int state, int action_idx, double reward);
void   env_set_reward_sas(Environment *env, int state, int action_idx,
                           int next_state, double reward);
double env_expected_reward(const Environment *env, int state, int action);
int    env_sample_transition(const Environment *env, int state, int action);
bool   env_is_terminal(const Environment *env, int state);

/* L3: Grid World */
typedef enum {
    CELL_EMPTY   = 0,
    CELL_WALL    = 1,
    CELL_REWARD  = 2,
    CELL_TRAP    = 3,
    CELL_START   = 4,
    CELL_AGENT   = 5,
    CELL_GOAL    = 6
} GridCell;

typedef struct {
    int      rows;
    int      cols;
    GridCell *cells;
    int      agent_row;
    int      agent_col;
    int      start_row;
    int      start_col;
    double   step_penalty;
    double   goal_reward;
    double   trap_penalty;
    int      max_steps;
    int      current_step;
    bool     done;
    int     *agent_rows;
    int     *agent_cols;
    int      num_agents;
} GridWorld;

typedef enum {
    GRID_UP    = 0,
    GRID_RIGHT = 1,
    GRID_DOWN  = 2,
    GRID_LEFT  = 3,
    GRID_STAY  = 4
} GridAction;

GridWorld* gw_create(int rows, int cols);
void       gw_set_cell(GridWorld *gw, int row, int col, GridCell type);
GridCell   gw_get_cell(const GridWorld *gw, int row, int col);
void       gw_add_reward(GridWorld *gw, int row, int col, double value);
void       gw_add_trap(GridWorld *gw, int row, int col, double penalty);
void       gw_set_start(GridWorld *gw, int row, int col);
void       gw_set_goal(GridWorld *gw, int row, int col);
double     gw_step(GridWorld *gw, GridAction action);
Percept    gw_perceive(const GridWorld *gw);
bool       gw_is_done(const GridWorld *gw);
void       gw_reset(GridWorld *gw);
void       gw_render(const GridWorld *gw, char *buffer, int buf_size);
Environment* gw_to_mdp_environment(const GridWorld *gw, const char *env_name);
void       gw_destroy(GridWorld *gw);
void       gw_print(const GridWorld *gw);

/* L3: NK Fitness Landscape (Kauffman, 1993) */
typedef struct {
    int       N;
    int       K;
    double   *fitness_table;
    int      *epistasis_map;
    double    global_optimum;
    int      *optimum_genome;
} NKLandscape;

NKLandscape* nk_create(int N, int K);
double       nk_fitness(const NKLandscape *nk, const int *genome);
int*         nk_hill_climb(const NKLandscape *nk, const int *start_genome,
                           int *steps_out);
double       nk_ruggedness(const NKLandscape *nk, int walk_length);
int          nk_count_local_optima(const NKLandscape *nk);
const int*   nk_global_optimum(const NKLandscape *nk);
void         nk_destroy(NKLandscape *nk);

#endif /* ENVIRONMENT_H */
