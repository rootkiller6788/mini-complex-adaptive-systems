#ifndef SWARM_EMERGENT_H
#define SWARM_EMERGENT_H

#include "swarm_types.h"

/* ============================================================================
 * Emergent Behavior & Complex Collective Dynamics
 *
 * Holland (1998): "Emergence: From Chaos to Order"
 *   — Emergence = much coming from little. Global patterns from local rules.
 *
 * Crutchfield (1994): "The Calculi of Emergence: Computation, Dynamics, and Induction"
 *   — ϵ-machine formalism for detecting emergent patterns.
 *
 * Wolfram (2002): "A New Kind of Science"
 *   — Simple rules → complex behavior (Class 4 CA).
 *
 * Bak, Tang & Wiesenfeld (1987): "Self-Organized Criticality"
 *   — Systems naturally evolve to critical state (sandpile model).
 *
 * Key concepts:
 *   - Self-organized criticality (SOC): power-law distributions without parameter tuning
 *   - Phase transitions in collective systems
 *   - Information transfer between scales (micro → macro)
 *   - Computational mechanics (ϵ-machines, causal states)
 * ============================================================================ */

/* ── Emergence Metrics ── */

/* Gestalt emergence measure: how much does the whole exceed the sum of parts?
 * E = I_global - Σ I_local_i  (where I = mutual information or complexity) */
double swarm_emergence_gestalt(const double* local_complexities, int n_parts,
                                double global_complexity);

/* Synergy-redundancy decomposition (Williams & Beer 2010):
 * I(X;Y) = Redundancy + Unique_X + Unique_Y + Synergy */
void swarm_emergence_pid(const double* joint_probs, int n_states,
           double* redundancy, double* unique_a, double* unique_b, double* synergy);

/* Excess entropy (Crutchfield & Feldman 2003):
 * E = lim_{L→∞} [H(L) - L·h_μ]
 * where H(L) is block entropy and h_μ is entropy rate */
double swarm_emergence_excess_entropy(const int* time_series, int length, int alphabet);

/* Statistical complexity (Crutchfield-Young complexity):
 * C_μ = -Σ_s P(s) log₂ P(s)  where s are causal states */
double swarm_emergence_statistical_complexity(const double* state_dist, int n_states);

/* ── Collective Behavior Analysis ── */

/* Detect phase transition in order parameter vs noise */
typedef struct {
    double* noise_levels;
    double* order_parameters;
    double* susceptibilities;  /* dφ/dη */
    int     n_samples;
    double  critical_noise;     /* Estimated critical noise η_c */
    bool    transition_detected;
} PhaseTransition;

void swarm_phase_transition_analyze(PhaseTransition* pt,
           double (*order_fn)(void* data, double noise),
           void* data, double noise_min, double noise_max, int n_points,
           int trials_per_point);

void swarm_phase_transition_free(PhaseTransition* pt);

/* Compute Binder cumulant for precise critical point location */
double swarm_binder_cumulant(const double* order_samples, int n_samples, int order);

/* ── Self-Organized Criticality ── */

/* Sandpile model (Bak-Tang-Wiesenfeld 1987) */
typedef struct {
    int**  grid;              /* Heights grid [height][width]                     */
    int    width, height;
    int    critical_height;    /* Threshold for toppling                           */
    int    n_avalanches;       /* Total avalanches observed                        */
    int*   avalanche_sizes;    /* Size of each avalanche                           */
    int    avalanche_capacity;
    double total_mass;         /* Total grains in system                           */
} Sandpile;

Sandpile* swarm_sandpile_create(int width, int height, int critical_height);
void      swarm_sandpile_free(Sandpile* sp);
void      swarm_sandpile_drop(Sandpile* sp, int x, int y);
int       swarm_sandpile_avalanche(Sandpile* sp, int x, int y);
double*   swarm_sandpile_avalanche_distribution(Sandpile* sp, int n_bins, 
                double* bin_edges);
double    swarm_sandpile_power_law_exponent(const int* sizes, int n_events);

/* ── Cellular Automata Analysis ── */

/* Wolfram classification (Class 1-4) via entropy rate and stability */
int  swarm_ca_classify(const int* states, int length, int n_steps, int n_rules);

/* Langton's λ parameter: fraction of non-quiescent outputs */
double swarm_langton_lambda(const int* rule_table, int n_inputs, int n_outputs, 
                             int quiescent);

/* ── Agent-Based Model Framework ── */

/* Generic ABM: agent behaviors defined by callback functions */
typedef void (*agent_init_fn)(void* agent, int id, void* config);
typedef void (*agent_update_fn)(void* agent, void** neighbors, int n_neighbors, 
                                 void* environment, void* config);
typedef double (*agent_fitness_fn)(const void* agent, void* config);

typedef struct {
    void**           agents;        /* Array of agent pointers                   */
    void*            environment;    /* Shared environment                        */
    int              n_agents;
    int              agent_type_size;/* Size of each agent struct                 */
    agent_init_fn    init;
    agent_update_fn  update;
    agent_fitness_fn fitness;
    ABMConfig        config;
    double**         positions;      /* Spatial positions [n][dim]                */
    double**         velocities;
    int*             agent_types;
    int              step;
    double           time;
} AgentBasedModel;

AgentBasedModel* swarm_abm_create(const ABMConfig* config, int agent_struct_size,
                    agent_init_fn init, agent_update_fn update, agent_fitness_fn fitness);
void             swarm_abm_free(AgentBasedModel* abm);
void             swarm_abm_step(AgentBasedModel* abm);
void             swarm_abm_run(AgentBasedModel* abm, int n_steps);
void             swarm_abm_print_stats(const AgentBasedModel* abm);

/* ── Information-Theoretic Measures ── */

/* Transfer entropy: T_{Y→X} = I(X_{t+1}; Y_t | X_t) */
double swarm_transfer_entropy(const int* x_series, const int* y_series, 
           int length, int x_alphabet, int y_alphabet, int lag);

/* Active information storage: A_X = I(X_{t+1}; X_t^{(k)}) */
double swarm_active_info_storage(const int* series, int length, int alphabet, int k);

/* Average pairwise mutual information in swarm */
double swarm_swarm_mutual_information(const double** trajectories, int n_agents,
           int length, int n_bins);

/* ── Critical Slowing Down Detection ── */
double swarm_critical_slowing_down(const double* time_series, int length, 
           int window_size, double* ar1_coeff, double* variance);
bool   swarm_early_warning_signal(const double* time_series, int length,
           double* kendall_tau, double* p_value);

/* ── Robustness & Evolvability ── */
double swarm_robustness_mutation(const void** agents, int n_agents, int n_trials,
           double mutation_rate, double threshold, agent_fitness_fn fitness_fn,
           void* config);
double swarm_evolvability_estimate(const double* fitness_history, int length);

#endif /* SWARM_EMERGENT_H */
