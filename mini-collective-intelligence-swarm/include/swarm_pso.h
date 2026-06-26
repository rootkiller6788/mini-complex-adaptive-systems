#ifndef SWARM_PSO_H
#define SWARM_PSO_H

#include "swarm_types.h"

/* ============================================================================
 * Particle Swarm Optimization (PSO)
 *
 * Kennedy & Eberhart (1995): "Particle Swarm Optimization"
 * Shi & Eberhart (1998): "A Modified Particle Swarm Optimizer" (inertia weight)
 * Clerc & Kennedy (2002): "The Particle Swarm — Explosion, Stability, and
 *   Convergence in a Multidimensional Complex Space" (constriction factor)
 * Mendes, Kennedy & Neves (2004): "The Fully Informed Particle Swarm" (FIPS)
 *
 * Core update equations for each particle i, dimension d:
 *
 *   v_{id} = ω·v_{id} + c1·r1·(pbest_{id} - x_{id}) + c2·r2·(lbest_{id} - x_{id})
 *   x_{id} = x_{id} + v_{id}
 *
 * where:
 *   ω = inertia weight (controls momentum)
 *   c1 = cognitive acceleration coefficient
 *   c2 = social acceleration coefficient
 *   r1, r2 ~ U(0,1)
 *   pbest = personal best position
 *   lbest = local/neighborhood best (or gbest for global topology)
 *
 * Constriction version (Type 1" constriction):
 *   χ = 2κ / |2 - φ - sqrt(φ² - 4φ)|, φ = c1 + c2 > 4, κ ∈ [0,1]
 *   Typically: c1 = c2 = 2.05, χ ≈ 0.7298
 *
 * Stability condition (Clerc & Kennedy, 2002):
 *   The swarm converges if |1 - ω - (c1·r1 + c2·r2)| < 1 (deterministic approx)
 * ============================================================================ */

/* ── PSO Strategy Variants ── */
typedef enum {
    PSO_STANDARD    = 0,   /* Standard PSO with inertia weight (Shi-Eberhart) */
    PSO_CONSTRICTED = 1,   /* Constriction factor (Clerc-Kennedy)             */
    PSO_FIPS         = 2,  /* Fully Informed Particle Swarm                    */
    PSO_ADAPTIVE     = 3,  /* Adaptive parameter control                       */
    PSO_QUANTUM      = 4   /* Quantum-behaved PSO (Sun et al. 2004)           */
} PSOStrategy;

/* ── PSO-specific Configuration Extension ── */
typedef struct {
    PSOStrategy strategy;      /* PSO variant to use                           */
    bool   use_velocity_clamp; /* Clamp velocities to v_max?                   */
    bool   use_reflection;     /* Reflect particles at boundaries?             */
    bool   use_turbulence;     /* Occasional random velocity boost?             */
    double turbulence_prob;    /* Probability of turbulence per particle        */
    double turbulence_scale;   /* Turbulence magnitude                          */
    double restart_stagnation; /* Restart if stagnant for this many iterations   */
    bool   track_diversity;    /* Monitor and report diversity metrics?         */
} PSOConfig;

/* ── Core PSO API ── */

/* Initialize a swarm with PSO-specific settings */
SwarmAgent** swarm_pso_create(SwarmConfig* config);
void         swarm_pso_free(SwarmAgent** swarm, int n_agents);

/* Single iteration: update velocities, positions, fitness, pbest, lbest */
void swarm_pso_iterate(SwarmAgent** swarm, const SwarmConfig* config, int iteration);

/* Full PSO run: returns result; NULL on failure */
SwarmResult* swarm_pso_run(SwarmConfig* config);

/* Compute lbest (local best) for each agent given topology */
void swarm_pso_update_lbest(SwarmAgent** swarm, int n_agents, 
                            SwarmTopology topology, int topology_param);

/* Compute gbest (global best) — single best solution across swarm */
int  swarm_pso_find_gbest(SwarmAgent** swarm, int n_agents, bool minimize);

/* ── Parameter Adaptation ── */

/* Linearly decreasing inertia weight: ω = ω_max - (ω_max - ω_min)·t/T */
double swarm_pso_inertia_linear(double omega_max, double omega_min, 
                                int iteration, int max_iterations);

/* Adaptive inertia based on swarm diversity (Zhan et al. 2009) */
double swarm_pso_inertia_adaptive(double diversity, double stagnation_ratio);

/* Time-varying acceleration coefficients */
void swarm_pso_adaptive_coefficients(double* c1, double* c2, 
                                     int iteration, int max_iterations);

/* ── Advanced PSO Operators ── */

/* Velocity clamping to prevent explosion */
void swarm_pso_clamp_velocity(SwarmAgent* agent, const SwarmConfig* config);

/* Reflection at boundary: x ← 2·bound - x, v ← -λ·v */
void swarm_pso_reflect_boundary(SwarmAgent* agent, const double* bounds_low,
                                const double* bounds_high, double restitution);

/* Turbulence: random perturbation to maintain diversity */
void swarm_pso_turbulence(SwarmAgent* agent, const SwarmConfig* config);

/* Inertia-weighted velocity update (Shi-Eberhart 1998) */
void swarm_pso_update_velocity_standard(SwarmAgent* agent, 
    const double* lbest_position, const SwarmConfig* config);

/* Constriction-factor velocity update (Clerc-Kennedy 2002) */
void swarm_pso_update_velocity_constricted(SwarmAgent* agent,
    const double* lbest_position, const SwarmConfig* config);

/* Fully Informed Particle Swarm update (Mendes et al. 2004) */
void swarm_pso_update_velocity_fips(SwarmAgent* agent,
    SwarmAgent** swarm, int n_agents, const SwarmConfig* config);

/* Quantum-behaved PSO update (Sun et al. 2004): no velocity, position sampling */
void swarm_pso_update_position_quantum(SwarmAgent* agent, 
    const double* mbest_position, const SwarmConfig* config);

/* Compute mean best position (mbest) for QPSO */
void swarm_pso_mean_best(SwarmAgent** swarm, int n_agents, double* mbest, int dim);

/* Multi-swarm cooperative PSO (Van den Bergh & Engelbrecht 2004) */
SwarmResult* swarm_pso_cooperative_run(SwarmConfig* base_config, 
                                        MultiSwarmConfig* multi_config);

/* ── Analysis and Diagnostics ── */

/* Check convergence: swarm radius < ε */
bool swarm_pso_is_converged(SwarmAgent** swarm, int n_agents, int dim, 
                             double epsilon);

/* Compute convergence rate estimate */
double swarm_pso_convergence_rate(const double* history, int n_iterations);

/* Estimate number of distinct optima in population clusters */
int  swarm_pso_speciation_count(SwarmAgent** swarm, int n_agents, 
                                 int dim, double niche_radius);

/* ── Topology Construction ── */

/* Initialize agent neighbors based on topology */
void swarm_pso_build_topology(SwarmAgent** swarm, int n_agents, 
                               SwarmTopology topology, int topology_param);

/* Update dynamic topology based on agent positions */
void swarm_pso_update_dynamic_topology(SwarmAgent** swarm, int n_agents, 
                                        int dim, double radius);

/* ── Niching and Speciation ── */

/* Determine niche for each agent (species ID) */
void swarm_pso_speciate(SwarmAgent** swarm, int n_agents, int dim,
                        double niche_radius, int* species_id);

/* SPSO-2011 standard PSO as specified by Clerc et al. */
void swarm_pso_sps2011_iterate(SwarmAgent** swarm, const SwarmConfig* config, 
                                int iteration, double* gbest, double* gbest_fitness);

#endif /* SWARM_PSO_H */
