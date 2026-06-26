/**
 * environment.c â€” Environment Models for Agent-Based Systems
 *
 * Implements GridWorld (classic RL environment), general MDP environment,
 * and Kauffman NK fitness landscapes for studying co-evolutionary adaptation.
 *
 * Reference: Sutton & Barto "Reinforcement Learning" Ch3 (MDPs)
 *            Kauffman "The Origins of Order" (1993) â€” NK landscapes
 *            Mitchell "Complexity: A Guided Tour" Ch10-12
 *
 * Knowledge: L1 Definitions, L2 Core Concepts, L3 Math Structures,
 *            L6 Canonical Problems (GridWorld)
 */

#include "environment.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

/* ===================================================================
 * L2: General Environment Lifecycle
 * =================================================================== */

Environment* env_create(const char *name, int num_states, int num_actions,
                         Observability obs, DynamicsType dyn) {
    Environment *env = (Environment*)calloc(1, sizeof(Environment));
    if (!env) return NULL;

    strncpy(env->name, name, 127);
    env->name[127] = '\0';
    env->observability = obs;
    env->dynamics = dyn;
    env->current_state = 0;
    env->time_step = 0;
    env->max_steps = 1000;

    /* State space */
    env->state_space.num_states = num_states;
    env->state_space.state_dim = 1;
    env->state_space.num_terminal = 0;
    env->state_space.state_matrix = (double*)calloc(num_states, sizeof(double));
    for (int i = 0; i < num_states; i++) {
        env->state_space.state_matrix[i] = (double)i;
    }

    /* Action space */
    env->action_space.num_actions = num_actions;
    env->action_space.action_dim = 1;
    env->action_space.action_names = (char**)calloc(num_actions, sizeof(char*));
    for (int i = 0; i < num_actions; i++) {
        env->action_space.action_names[i] = (char*)calloc(32, sizeof(char));
        snprintf(env->action_space.action_names[i], 31, "action_%d", i);
    }

    /* Transition model */
    env->transitions.num_states = num_states;
    env->transitions.num_actions = num_actions;
    int tsize = num_states * num_actions * num_states;
    env->transitions.probs = (double*)calloc(tsize, sizeof(double));

    /* Reward function */
    env->rewards.num_states = num_states;
    env->rewards.num_actions = num_actions;
    env->rewards.rewards = (double*)calloc(num_states * num_actions, sizeof(double));
    env->rewards.depends_on_next_state = false;
    env->rewards.rewards_sas = NULL;

    /* Multi-agent */
    env->max_agents = 16;
    env->num_agents = 0;
    env->agents = (Agent**)calloc(env->max_agents, sizeof(Agent*));

    return env;
}

double env_step(Environment *env, int agent_id, const Action *action,
                Percept *percept_out) {
    if (!env) return 0.0;
    if (action->type == ACTION_DISCRETE && action->discrete_choice >= env->action_space.num_actions)
        return 0.0;

    int s = env->current_state;
    int a = (action->type == ACTION_DISCRETE) ? action->discrete_choice : 0;
    double reward = 0.0;

    /* Use custom transition function if set, else use static table */
    if (env->transition_fn) {
        int next = env->transition_fn(env, s, action, &reward);
        env->current_state = next;
    } else {
        /* Sample from transition table */
        int next = env_sample_transition(env, s, a);
        reward = env->rewards.rewards[s * env->action_space.num_actions + a];
        env->current_state = next;
    }

    /* Provide percept */
    if (percept_out) {
        if (env->observe_fn) {
            *percept_out = env->observe_fn(env, env->current_state, agent_id);
        } else {
            *percept_out = env_observe(env, agent_id);
        }
    }

    env->time_step++;
    return reward;
}

Percept env_observe(const Environment *env, int agent_id) {
    Percept p;
    memset(&p, 0, sizeof(p));
    p.timestamp = env->time_step;

    int s = env->current_state;

    if (env->observability == ENV_FULLY_OBSERVABLE) {
        /* Full state observation: percept = one-hot of current state */
        p.dim = env->state_space.state_dim > 0 ? env->state_space.state_dim : 1;
        if (p.dim > MAX_PERCEPT_DIM) p.dim = MAX_PERCEPT_DIM;
        if (env->state_space.state_matrix) {
            for (int i = 0; i < p.dim; i++) {
                p.values[i] = env->state_space.state_matrix[s * env->state_space.state_dim + i];
            }
        }
    } else if (env->observability == ENV_PARTIALLY_OBSERVABLE) {
        /* Partial: observation = state with noise */
        p.dim = 1;
        p.values[0] = (double)s + 0.1 * ((double)rand() / RAND_MAX - 0.5);
    }
    /* ENV_HIDDEN: return empty percept */

    (void)agent_id;
    return p;
}

int env_register_agent(Environment *env, Agent *agent) {
    if (!env || !agent) return -1;
    if (env->num_agents >= env->max_agents) {
        int new_max = env->max_agents * 2;
        Agent **new_agents = (Agent**)realloc(env->agents, new_max * sizeof(Agent*));
        if (!new_agents) return -1;
        env->agents = new_agents;
        env->max_agents = new_max;
    }
    env->agents[env->num_agents] = agent;
    return env->num_agents++;
}

void env_remove_agent(Environment *env, int agent_id) {
    if (!env || agent_id < 0 || agent_id >= env->num_agents) return;
    for (int i = agent_id; i < env->num_agents - 1; i++) {
        env->agents[i] = env->agents[i + 1];
    }
    env->num_agents--;
    env->agents[env->num_agents] = NULL;
}

void env_reset(Environment *env) {
    if (!env) return;
    env->current_state = 0;
    env->time_step = 0;
}

void env_destroy(Environment *env) {
    if (!env) return;
    free(env->state_space.state_matrix);
    if (env->action_space.action_names) {
        for (int i = 0; i < env->action_space.num_actions; i++) {
            free(env->action_space.action_names[i]);
        }
        free(env->action_space.action_names);
    }
    free(env->transitions.probs);
    free(env->rewards.rewards);
    free(env->rewards.rewards_sas);
    free(env->agents);
    free(env);
}

/* ===================================================================
 * L2: Transition & Reward Setup
 * =================================================================== */

void env_set_transition(Environment *env, int from_state, int action_idx,
                         int to_state, double probability) {
    if (!env) return;
    int NA = env->action_space.num_actions;
    int NS = env->state_space.num_states;
    int idx = from_state * NA * NS + action_idx * NS + to_state;
    if (idx >= 0 && idx < NS * NA * NS) {
        env->transitions.probs[idx] = probability;
    }
}

void env_set_reward(Environment *env, int state, int action_idx, double reward) {
    if (!env) return;
    int NA = env->action_space.num_actions;
    int idx = state * NA + action_idx;
    if (idx >= 0 && idx < env->state_space.num_states * NA) {
        env->rewards.rewards[idx] = reward;
    }
}

void env_set_reward_sas(Environment *env, int state, int action_idx,
                         int next_state, double reward) {
    if (!env) return;
    int NA = env->action_space.num_actions;
    int NS = env->state_space.num_states;
    /* Allocate sas rewards if first use */
    if (!env->rewards.rewards_sas) {
        env->rewards.rewards_sas = (double*)calloc(NS * NA * NS, sizeof(double));
        env->rewards.depends_on_next_state = true;
    }
    int idx = state * NA * NS + action_idx * NS + next_state;
    if (idx >= 0 && idx < NS * NA * NS) {
        env->rewards.rewards_sas[idx] = reward;
    }
}

double env_expected_reward(const Environment *env, int state, int action) {
    if (!env) return 0.0;
    int NA = env->action_space.num_actions;
    if (env->rewards.depends_on_next_state && env->rewards.rewards_sas) {
        int NS = env->state_space.num_states;
        double expected = 0.0;
        for (int s_next = 0; s_next < NS; s_next++) {
            double prob = env->transitions.probs[state * NA * NS + action * NS + s_next];
            double r = env->rewards.rewards_sas[state * NA * NS + action * NS + s_next];
            expected += prob * r;
        }
        return expected;
    }
    return env->rewards.rewards[state * NA + action];
}

int env_sample_transition(const Environment *env, int state, int action) {
    if (!env) return 0;
    int NS = env->state_space.num_states;
    int NA = env->action_space.num_actions;

    /* Check for deterministic transition */
    if (env->dynamics == ENV_DETERMINISTIC) {
        for (int s_next = 0; s_next < NS; s_next++) {
            int idx = state * NA * NS + action * NS + s_next;
            if (env->transitions.probs[idx] > 0.5) return s_next;
        }
    }

    /* Stochastic sampling via cumulative distribution */
    double rand_val = (double)rand() / (double)RAND_MAX;
    double cumulative = 0.0;
    for (int s_next = 0; s_next < NS; s_next++) {
        int idx = state * NA * NS + action * NS + s_next;
        cumulative += env->transitions.probs[idx];
        if (rand_val <= cumulative) return s_next;
    }
    /* Fallback: stay in current state */
    return state;
}

bool env_is_terminal(const Environment *env, int state) {
    if (!env) return false;
    for (int i = 0; i < env->state_space.num_terminal; i++) {
        if (env->state_space.terminal_states[i] == state) return true;
    }
    return false;
}
/* ===================================================================
 * L3: Grid World ¡ª Canonical Agent Environment (L6: Canonical Problems)
 *
 * GridWorld is the standard testbed for RL agents. An agent navigates a
 * 2D grid with walls, rewards, traps, and goals using 4-directional movement.
 *
 * State = row * cols + col (linear index)
 * Actions = {UP, RIGHT, DOWN, LEFT, STAY}
 * =================================================================== */

GridWorld* gw_create(int rows, int cols) {
    if (rows < 1 || cols < 1) return NULL;
    GridWorld *gw = (GridWorld*)calloc(1, sizeof(GridWorld));
    if (!gw) return NULL;

    gw->rows = rows;
    gw->cols = cols;
    gw->cells = (GridCell*)calloc(rows * cols, sizeof(GridCell));
    gw->start_row = 0;
    gw->start_col = 0;
    gw->agent_row = 0;
    gw->agent_col = 0;
    gw->step_penalty = -0.04;
    gw->goal_reward = 1.0;
    gw->trap_penalty = -1.0;
    gw->max_steps = 100;
    gw->current_step = 0;
    gw->done = false;
    gw->agent_rows = NULL;
    gw->agent_cols = NULL;
    gw->num_agents = 1;

    return gw;
}

void gw_set_cell(GridWorld *gw, int row, int col, GridCell type) {
    if (!gw || row < 0 || row >= gw->rows || col < 0 || col >= gw->cols) return;
    gw->cells[row * gw->cols + col] = type;
}

GridCell gw_get_cell(const GridWorld *gw, int row, int col) {
    if (!gw || row < 0 || row >= gw->rows || col < 0 || col >= gw->cols)
        return CELL_EMPTY;
    return gw->cells[row * gw->cols + col];
}

void gw_add_reward(GridWorld *gw, int row, int col, double value) { (void)value;
    if (!gw || row < 0 || row >= gw->rows || col < 0 || col >= gw->cols) return;
    gw->cells[row * gw->cols + col] = CELL_REWARD;
}

void gw_add_trap(GridWorld *gw, int row, int col, double penalty) { (void)penalty;
    if (!gw || row < 0 || row >= gw->rows || col < 0 || col >= gw->cols) return;
    gw->cells[row * gw->cols + col] = CELL_TRAP;
}

void gw_set_start(GridWorld *gw, int row, int col) {
    if (!gw || row < 0 || row >= gw->rows || col < 0 || col >= gw->cols) return;
    gw->start_row = row;
    gw->start_col = col;
    gw->agent_row = row;
    gw->agent_col = col;
    gw->cells[row * gw->cols + col] = CELL_START;
}

void gw_set_goal(GridWorld *gw, int row, int col) {
    if (!gw || row < 0 || row >= gw->rows || col < 0 || col >= gw->cols) return;
    gw->cells[row * gw->cols + col] = CELL_GOAL;
}

double gw_step(GridWorld *gw, GridAction action) {
    if (!gw || gw->done) return 0.0;

    int new_row = gw->agent_row;
    int new_col = gw->agent_col;

    switch (action) {
        case GRID_UP:    new_row--; break;
        case GRID_RIGHT: new_col++; break;
        case GRID_DOWN:  new_row++; break;
        case GRID_LEFT:  new_col--; break;
        case GRID_STAY:  break;
        default: break;
    }

    /* Check boundaries */
    if (new_row < 0) new_row = 0;
    if (new_row >= gw->rows) new_row = gw->rows - 1;
    if (new_col < 0) new_col = 0;
    if (new_col >= gw->cols) new_col = gw->cols - 1;

    /* Check wall collision ¡ª agent bounces back */
    if (gw->cells[new_row * gw->cols + new_col] == CELL_WALL) {
        new_row = gw->agent_row;
        new_col = gw->agent_col;
    }

    gw->agent_row = new_row;
    gw->agent_col = new_col;
    gw->current_step++;

    /* Determine reward */
    double reward = gw->step_penalty; /* living cost */
    GridCell cell = gw->cells[new_row * gw->cols + new_col];

    if (cell == CELL_GOAL) {
        reward = gw->goal_reward;
        gw->done = true;
    } else if (cell == CELL_TRAP) {
        reward = gw->trap_penalty;
    } else if (cell == CELL_REWARD) {
        reward = gw->goal_reward * 0.5;
    }

    if (gw->current_step >= gw->max_steps) {
        gw->done = true;
    }

    return reward;
}

Percept gw_perceive(const GridWorld *gw) {
    Percept p;
    memset(&p, 0, sizeof(p));
    p.timestamp = gw->current_step;

    /* 5D percept: [agent_row/rows, agent_col/cols,
     *               has_wall_up, has_wall_right, has_wall_down, has_wall_left,
     *               goal_nearby, trap_nearby] */
    p.dim = 8;
    if (gw->rows > 0 && gw->cols > 0) {
        p.values[0] = (double)gw->agent_row / (double)gw->rows;
        p.values[1] = (double)gw->agent_col / (double)gw->cols;

        /* Adjacent cell types */
        int r = gw->agent_row, c = gw->agent_col;
        p.values[2] = (r > 0 && gw->cells[(r-1)*gw->cols + c] == CELL_WALL) ? 1.0 : 0.0;
        p.values[3] = (c < gw->cols-1 && gw->cells[r*gw->cols + c+1] == CELL_WALL) ? 1.0 : 0.0;
        p.values[4] = (r < gw->rows-1 && gw->cells[(r+1)*gw->cols + c] == CELL_WALL) ? 1.0 : 0.0;
        p.values[5] = (c > 0 && gw->cells[r*gw->cols + c-1] == CELL_WALL) ? 1.0 : 0.0;

        /* Goal proximity (3x3 window) */
        p.values[6] = 0.0;
        p.values[7] = 0.0;
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                int nr = r + dr, nc = c + dc;
                if (nr >= 0 && nr < gw->rows && nc >= 0 && nc < gw->cols) {
                    GridCell cell = gw->cells[nr * gw->cols + nc];
                    if (cell == CELL_GOAL) p.values[6] = 1.0;
                    if (cell == CELL_TRAP) p.values[7] = 1.0;
                }
            }
        }
    }
    return p;
}

bool gw_is_done(const GridWorld *gw) {
    return gw ? gw->done : true;
}

void gw_reset(GridWorld *gw) {
    if (!gw) return;
    gw->agent_row = gw->start_row;
    gw->agent_col = gw->start_col;
    gw->current_step = 0;
    gw->done = false;
}

void gw_render(const GridWorld *gw, char *buffer, int buf_size) {
    if (!gw || !buffer || buf_size <= 0) return;
    int pos = 0;
    for (int r = 0; r < gw->rows && pos < buf_size - 2; r++) {
        for (int c = 0; c < gw->cols && pos < buf_size - 2; c++) {
            char ch = '.';
            if (r == gw->agent_row && c == gw->agent_col) {
                ch = 'A';
            } else {
                switch (gw->cells[r * gw->cols + c]) {
                    case CELL_WALL:   ch = '#'; break;
                    case CELL_REWARD: ch = '$'; break;
                    case CELL_TRAP:   ch = 'X'; break;
                    case CELL_START:  ch = 'S'; break;
                    case CELL_GOAL:   ch = 'G'; break;
                    default:          ch = '.'; break;
                }
            }
            buffer[pos++] = ch;
        }
        buffer[pos++] = '\n';
    }
    buffer[pos] = '\0';
}

Environment* gw_to_mdp_environment(const GridWorld *gw, const char *env_name) {
    if (!gw) return NULL;
    int S = gw->rows * gw->cols;
    int A = 5; /* 5 grid actions */
    Environment *env = env_create(env_name, S, A, ENV_FULLY_OBSERVABLE, ENV_DETERMINISTIC);

    /* Build transition table: for each (s,a), map to next state s' */
    for (int r = 0; r < gw->rows; r++) {
        for (int c = 0; c < gw->cols; c++) {
            int s = r * gw->cols + c;
            /* Skip walls ¡ª treat as absorbing */
            if (gw->cells[s] == CELL_WALL) {
                for (int a = 0; a < A; a++) {
                    env_set_transition(env, s, a, s, 1.0);
                    env_set_reward(env, s, a, 0.0);
                }
                continue;
            }
            /* For each action, compute resulting state */
            for (int a = 0; a < A; a++) {
                int nr = r, nc = c;
                switch (a) {
                    case GRID_UP:    nr--; break;
                    case GRID_RIGHT: nc++; break;
                    case GRID_DOWN:  nr++; break;
                    case GRID_LEFT:  nc--; break;
                    default: break;
                }
                if (nr < 0) nr = 0;
                if (nr >= gw->rows) nr = gw->rows - 1;
                if (nc < 0) nc = 0;
                if (nc >= gw->cols) nc = gw->cols - 1;

                int s_next = nr * gw->cols + nc;
                if (gw->cells[s_next] == CELL_WALL) s_next = s;

                env_set_transition(env, s, a, s_next, 1.0);

                /* Set reward for transition */
                double r = gw->step_penalty;
                GridCell cell = gw->cells[s_next];
                if (cell == CELL_GOAL) r = gw->goal_reward;
                else if (cell == CELL_TRAP) r = gw->trap_penalty;
                env_set_reward(env, s, a, r);
            }
        }
    }
    /* Mark terminal states */
    int nt = 0;
    for (int s = 0; s < S && nt < 64; s++) {
        GridCell cell = gw->cells[s];
        if (cell == CELL_GOAL || cell == CELL_TRAP) {
            env->state_space.terminal_states[nt++] = s;
        }
    }
    env->state_space.num_terminal = nt;

    return env;
}

void gw_print(const GridWorld *gw) {
    if (!gw) return;
    printf("GridWorld %dx%d, agent at (%d,%d), step %d/%d, %s\n",
           gw->rows, gw->cols, gw->agent_row, gw->agent_col,
           gw->current_step, gw->max_steps, gw->done ? "DONE" : "ACTIVE");
    char buf[4096] = {0};
    gw_render(gw, buf, sizeof(buf));
    printf("%s", buf);
}

void gw_destroy(GridWorld *gw) {
    if (!gw) return;
    free(gw->cells);
    free(gw->agent_rows);
    free(gw->agent_cols);
    free(gw);
}

/* ===================================================================
 * L3: NK Fitness Landscape ¡ª Kauffman (1993)
 *
 * N loci, each affected by K other loci (epistasis).
 * Generates rugged fitness landscapes for studying adaptation
 * on complex surfaces. Key properties:
 *   - K=0: smooth, single-peaked (Mount Fuji)
 *   - K=N-1: maximally rugged, random (uncorrelated)
 *   - Intermediate K: realistic ruggedness with many local optima
 *
 * Fitness of locus i: f_i depends on itself + K other loci
 * Total fitness = (1/N) * sum_i f_i(gene_i, gene_epistatic1, ...)
 * =================================================================== */

/* Internal: generate random fitness contribution for a given K+1-bit pattern */
static double nk_random_contribution(void) {
    return (double)rand() / (double)RAND_MAX;
}

NKLandscape* nk_create(int N, int K) {
    if (N < 1 || K < 0 || K >= N) return NULL;
    NKLandscape *nk = (NKLandscape*)calloc(1, sizeof(NKLandscape));
    if (!nk) return NULL;

    nk->N = N;
    nk->K = K;
    int patterns = 1 << (K + 1); /* 2^(K+1) possible input patterns per locus */
    nk->fitness_table = (double*)calloc(N * patterns, sizeof(double));
    nk->epistasis_map = (int*)calloc(N * K, sizeof(int));

    /* Set up epistasis map: each locus i interacts with itself + K random others */
    for (int i = 0; i < N; i++) {
        /* First entry is always the locus itself */
        nk->epistasis_map[i * K + 0] = i;
        /* K-1 other randomly chosen loci (excluding i) */
        for (int k = 1; k < K; k++) {
            int other;
            int tries = 0;
            do {
                other = rand() % N;
                tries++;
            } while ((other == i || other == nk->epistasis_map[i * K + 0])
                     && tries < 100);
            if (tries >= 100) other = (i + k) % N;
            nk->epistasis_map[i * K + k] = other;
        }
        /* Generate random fitness contributions for all 2^(K+1) patterns */
        for (int p = 0; p < patterns; p++) {
            nk->fitness_table[i * patterns + p] = nk_random_contribution();
        }
    }

    /* Pre-compute global optimum if N is small enough for exhaustive search */
    nk->global_optimum = 0.0;
    nk->optimum_genome = NULL;
    if (N <= 20) {
        nk->optimum_genome = (int*)calloc(N, sizeof(int));
        int total_genomes = 1 << N;
        double best_fitness = -1.0;
        int *genome = (int*)calloc(N, sizeof(int));
        for (int g = 0; g < total_genomes; g++) {
            /* Convert integer to binary genome */
            for (int i = 0; i < N; i++) {
                genome[i] = (g >> i) & 1;
            }
            double fit = nk_fitness(nk, genome);
            if (fit > best_fitness) {
                best_fitness = fit;
                memcpy(nk->optimum_genome, genome, N * sizeof(int));
            }
        }
        nk->global_optimum = best_fitness;
        free(genome);
    }

    return nk;
}

double nk_fitness(const NKLandscape *nk, const int *genome) {
    if (!nk || !genome) return 0.0;

    int patterns = 1 << (nk->K + 1);
    double total_fitness = 0.0;

    for (int i = 0; i < nk->N; i++) {
        /* Build the K+1 bit pattern for locus i */
        int pattern = 0;
        for (int k = 0; k < nk->K + 1; k++) {
            int locus = nk->epistasis_map[i * nk->K + k];
            if (genome[locus]) {
                pattern |= (1 << k);
            }
        }
        total_fitness += nk->fitness_table[i * patterns + pattern];
    }

    return total_fitness / (double)nk->N;
}

int* nk_hill_climb(const NKLandscape *nk, const int *start_genome,
                    int *steps_out) {
    if (!nk || !start_genome) return NULL;

    int *current = (int*)calloc(nk->N, sizeof(int));
    int *candidate = (int*)calloc(nk->N, sizeof(int));
    memcpy(current, start_genome, nk->N * sizeof(int));

    double current_fitness = nk_fitness(nk, current);
    int steps = 0;
    bool improved;

    do {
        improved = false;
        /* Try flipping each bit */
        for (int i = 0; i < nk->N; i++) {
            memcpy(candidate, current, nk->N * sizeof(int));
            candidate[i] = 1 - candidate[i]; /* flip bit i */
            double candidate_fitness = nk_fitness(nk, candidate);

            if (candidate_fitness > current_fitness) {
                /* Accept improvement immediately (greedy ascent) */
                current[i] = 1 - current[i];
                current_fitness = candidate_fitness;
                improved = true;
                steps++;
                break; /* restart from new point */
            }
        }
    } while (improved);

    if (steps_out) *steps_out = steps;
    free(candidate);
    return current;
}

double nk_ruggedness(const NKLandscape *nk, int walk_length) {
    /* Estimate ruggedness via autocorrelation of a random walk.
     * Corr(1) = autocorrelation at lag 1 of fitness values along walk.
     * Smoother landscapes have Corr(1) closer to 1.0.
     * Rugged landscapes have Corr(1) closer to 0.0. */
    if (!nk || walk_length < 2) return 0.0;

    int *genome = (int*)calloc(nk->N, sizeof(int));
    double *fitness_vals = (double*)calloc(walk_length, sizeof(double));

    /* Random starting genome */
    for (int i = 0; i < nk->N; i++) genome[i] = rand() % 2;
    fitness_vals[0] = nk_fitness(nk, genome);

    /* Random walk: flip one random bit each step */
    for (int t = 1; t < walk_length; t++) {
        int flip_locus = rand() % nk->N;
        genome[flip_locus] = 1 - genome[flip_locus];
        fitness_vals[t] = nk_fitness(nk, genome);
    }

    /* Compute lag-1 autocorrelation */
    double mean = 0.0;
    for (int t = 0; t < walk_length; t++) mean += fitness_vals[t];
    mean /= walk_length;

    double cov = 0.0, var = 0.0;
    for (int t = 0; t < walk_length - 1; t++) {
        cov += (fitness_vals[t] - mean) * (fitness_vals[t+1] - mean);
    }
    for (int t = 0; t < walk_length; t++) {
        var += (fitness_vals[t] - mean) * (fitness_vals[t] - mean);
    }
    cov /= (walk_length - 1);
    var /= walk_length;

    free(genome);
    free(fitness_vals);

    if (var < 1e-12) return 1.0;
    return cov / var;
}

int nk_count_local_optima(const NKLandscape *nk) {
    /* Exhaustive enumeration of all 2^N genomes. Only feasible for N <= 15. */
    if (!nk || nk->N > 15) return -1;

    int *genome = (int*)calloc(nk->N, sizeof(int));
    int *neighbor = (int*)calloc(nk->N, sizeof(int));
    int total_genomes = 1 << nk->N;
    int count = 0;

    for (int g = 0; g < total_genomes; g++) {
        for (int i = 0; i < nk->N; i++) {
            genome[i] = (g >> i) & 1;
        }
        double fit = nk_fitness(nk, genome);

        /* Check all N one-bit-flip neighbors */
        bool is_local_optimum = true;
        for (int i = 0; i < nk->N; i++) {
            memcpy(neighbor, genome, nk->N * sizeof(int));
            neighbor[i] = 1 - neighbor[i];
            double neighbor_fit = nk_fitness(nk, neighbor);
            if (neighbor_fit > fit) {
                is_local_optimum = false;
                break;
            }
        }
        if (is_local_optimum) count++;
    }

    free(genome);
    free(neighbor);
    return count;
}

const int* nk_global_optimum(const NKLandscape *nk) {
    return nk ? nk->optimum_genome : NULL;
}

void nk_destroy(NKLandscape *nk) {
    if (!nk) return;
    free(nk->fitness_table);
    free(nk->epistasis_map);
    free(nk->optimum_genome);
    free(nk);
}
