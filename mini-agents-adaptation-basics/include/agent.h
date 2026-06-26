/**
 * agent.h — Core Agent Definitions and Adaptation Primitives
 *
 * Complex Adaptive Systems: An agent is an autonomous entity that perceives
 * its environment through sensors and acts upon it through effectors, adapting
 * its behavior to maximize a fitness/reward signal.
 *
 * Reference: Russell & Norvig "AI: A Modern Approach" Ch2 (Intelligent Agents)
 *            Holland "Hidden Order" (1995) — Adaptive agents in CAS
 *            Mitchell "Complexity: A Guided Tour" Ch12
 *
 * Knowledge coverage: L1 (Definitions), L2 (Core Concepts), L3 (Math Structures)
 */

#ifndef AGENT_H
#define AGENT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ----------------------------------------------------------------------------
 * L1: Core Definitions — Agent Models
 * ----------------------------------------------------------------------------
 */

/** Percept: raw sensory input vector from environment (L1) */
#define MAX_PERCEPT_DIM 64
#define MAX_ACTION_DIM  32
#define MAX_STATE_DIM   48

typedef struct {
    int      dim;
    double   values[MAX_PERCEPT_DIM];
    uint64_t timestamp;
} Percept;

/** Action: discrete or continuous effector output (L1) */
typedef enum {
    ACTION_DISCRETE,
    ACTION_CONTINUOUS,
    ACTION_SYMBOLIC
} ActionType;

typedef struct {
    ActionType type;
    int        discrete_choice;
    double     continuous_vec[MAX_ACTION_DIM];
    int        dim;
} Action;

/** AgentState: internal memory/state of the agent (L1) */
typedef struct {
    int      state_id;
    double   belief_vec[MAX_STATE_DIM];
    double   value_estimate;
    int      state_dim;
    uint64_t visit_count;
} AgentState;

/** PEAS description: Performance measure, Environment, Actuators, Sensors */
typedef struct {
    char     performance_measure[128];
    char     environment_type[128];
    char     actuators[256];
    char     sensors[256];
} PEAS;

typedef struct Agent Agent;

typedef Action   (*AgentPolicy)(Agent *self, const Percept *p);
typedef void     (*AgentLearn)(Agent *self, const Percept *p,
                               const Action *a, double reward);
typedef Percept  (*AgentSense)(Agent *self);
typedef void     (*AgentActuate)(Agent *self, const Action *a);

struct Agent {
    int         agent_id;
    char        name[64];
    PEAS        peas;
    AgentState  state;
    double      cumulative_reward;
    double      fitness;
    uint64_t    age;
    double      adaptation_rate;
    AgentPolicy  policy;
    AgentLearn   learn;
    AgentSense   sense;
    AgentActuate actuate;
    double       memory[256];
    int          memory_size;
    void        *private_data;
};

Agent* agent_create(int id, const char *name, double adaptation_rate);
Action agent_step(Agent *a, const Percept *p);
void   agent_learn_from_experience(Agent *a, const Percept *p,
                                    const Action *act, double reward);
double agent_fitness(const Agent *a);
void   agent_reset(Agent *a);
void   agent_destroy(Agent *a);

Action agent_satisfice(Agent *a, const Percept *p,
                       double aspiration_level, int max_lookahead);
double agent_adapt_aspiration(Agent *a, double recent_reward_history[],
                               int history_len);

int    agent_epsilon_greedy(const double *action_values, int n_actions,
                             double epsilon);
int    agent_boltzmann_selection(const double *action_values, int n_actions,
                                  double temperature);
double agent_policy_entropy(const double *action_probs, int n_actions);

double agent_state_similarity(const AgentState *a, const AgentState *b);
AgentState agent_state_merge(const AgentState *a, const AgentState *b,
                              double weight_a);
int agent_discretize_percept(const Percept *p, int bins_per_dim);

void agent_send_message(Agent *sender, Agent *receiver,
                         const char *message, double intensity);
void agent_observe_peer(Agent *observer, const Agent *peer,
                         const Percept *context, const Action *peer_action);
void agent_imitate(Agent *learner, const Agent *model, double imitation_rate);

#endif /* AGENT_H */
