#include "swarm_pso.h"
#include "swarm_core.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

/* ============================================================================
 * Particle Swarm Optimization — Core Implementation
 *
 * References:
 *   Kennedy & Eberhart (1995)  — Original PSO
 *   Shi & Eberhart (1998)       — Inertia weight
 *   Clerc & Kennedy (2002)      — Constriction factor
 *   Mendes, Kennedy & Neves (2004) — FIPS
 *   Sun, Feng & Xu (2004)       — Quantum-behaved PSO
 *   Zhan et al. (2009)          — Adaptive PSO (APSO)
 * ============================================================================ */

/* ── Particle Initialization ── */
SwarmAgent** swarm_pso_create(SwarmConfig* config) {
    if (!config || config->n_agents <= 0 || config->dimensions <= 0) return NULL;
    int n = config->n_agents, dim = config->dimensions;
    SwarmAgent** swarm = (SwarmAgent**)calloc((size_t)n, sizeof(SwarmAgent*));
    if (!swarm) return NULL;
    for (int i = 0; i < n; i++) {
        swarm[i] = swarm_agent_create(dim);
        if (!swarm[i]) {
            for (int j = 0; j < i; j++) swarm_agent_free(swarm[j]);
            free(swarm); return NULL;
        }
        swarm_agent_init_random(swarm[i], dim, i, config->bounds_low, config->bounds_high);
        swarm_agent_update_fitness(swarm[i], config->fitness_function, 
                                    config->fitness_context, config->minimize);
        swarm[i]->neighbor_count = 0;
    }
    return swarm;
}

void swarm_pso_free(SwarmAgent** swarm, int n_agents) {
    if (!swarm) return;
    for (int i = 0; i < n_agents; i++) swarm_agent_free(swarm[i]);
    free(swarm);
}

/* ── Topology Construction ── */
void swarm_pso_build_topology(SwarmAgent** swarm, int n_agents,
                               SwarmTopology topology, int topology_param) {
    if (!swarm) return;
    for (int i = 0; i < n_agents; i++) {
        switch (topology) {
            case SWARM_TOPOLOGY_GLOBAL:
                /* All agents are neighbors */
                swarm[i]->neighbor_count = 0;
                for (int j = 0; j < n_agents && swarm[i]->neighbor_count < SWARM_MAX_NEIGHBORS; j++)
                    if (j != i) swarm[i]->neighbors[swarm[i]->neighbor_count++] = j;
                break;
            case SWARM_TOPOLOGY_RING:
                swarm_topology_ring(swarm[i]->neighbors, i, n_agents, topology_param);
                swarm[i]->neighbor_count = topology_param;
                if (swarm[i]->neighbor_count > SWARM_MAX_NEIGHBORS)
                    swarm[i]->neighbor_count = SWARM_MAX_NEIGHBORS;
                break;
            case SWARM_TOPOLOGY_VON_NEUMANN:
                { int gw = (int)sqrt((double)n_agents), gh = n_agents / gw;
                  if (gw * gh < n_agents) gh++;
                  swarm_topology_von_neumann(swarm[i]->neighbors, i, gw, gh); }
                swarm[i]->neighbor_count = 4;
                break;
            case SWARM_TOPOLOGY_RANDOM:
                swarm_topology_random(swarm[i]->neighbors, i, n_agents, topology_param);
                swarm[i]->neighbor_count = topology_param;
                break;
            case SWARM_TOPOLOGY_STAR:
                swarm_topology_star(swarm[i]->neighbors, i, n_agents);
                swarm[i]->neighbor_count = (i == 0) ? n_agents - 1 : 1;
                break;
            case SWARM_TOPOLOGY_DYNAMIC:
                swarm[i]->neighbor_count = 0;
                break;
        }
        /* Cleanup: ensure neighbor_count reflects valid entries */
        int cnt = 0;
        for (int j = 0; j < SWARM_MAX_NEIGHBORS; j++)
            if (swarm[i]->neighbors[j] >= 0) cnt = j + 1;
        swarm[i]->neighbor_count = cnt;
    }
}

/* ── Local Best Update ── */
void swarm_pso_update_lbest(SwarmAgent** swarm, int n_agents,
                             SwarmTopology topology, int topology_param) {
    if (!swarm) return;
    (void)topology_param;
    if (topology == SWARM_TOPOLOGY_GLOBAL) {
        /* lbest = gbest for all */
        int gbest = swarm_pso_find_gbest(swarm, n_agents, true);
        for (int i = 0; i < n_agents; i++)
            swarm[i]->neighbors[swarm[i]->neighbor_count] = gbest;
    }
    /* For other topologies, lbest is the best among neighbors */
    /* This is handled implicitly in velocity update by iterating neighbors */
}

/* ── Global Best ── */
int swarm_pso_find_gbest(SwarmAgent** swarm, int n_agents, bool minimize) {
    if (!swarm || n_agents <= 0) return -1;
    int best = 0;
    for (int i = 1; i < n_agents; i++) {
        bool better = minimize ? (swarm[i]->best_fitness < swarm[best]->best_fitness)
                               : (swarm[i]->best_fitness > swarm[best]->best_fitness);
        if (better) best = i;
    }
    return best;
}

/* ── Standard Velocity Update (Shi-Eberhart 1998) ── */
void swarm_pso_update_velocity_standard(SwarmAgent* agent,
    const double* lbest_position, const SwarmConfig* config) {
    if (!agent || !lbest_position || !config) return;
    int dim = agent->dim;
    for (int d = 0; d < dim; d++) {
        double r1 = swarm_rng_uniform(), r2 = swarm_rng_uniform();
        double cognitive = config->cognitive_weight * r1 * (agent->best_position[d] - agent->position[d]);
        double social = config->social_weight * r2 * (lbest_position[d] - agent->position[d]);
        agent->velocity[d] = config->inertia_weight * agent->velocity[d] + cognitive + social;
    }
}

/* ── Constricted Velocity Update (Clerc-Kennedy 2002) ── */
void swarm_pso_update_velocity_constricted(SwarmAgent* agent,
    const double* lbest_position, const SwarmConfig* config) {
    if (!agent || !lbest_position || !config) return;
    double phi = config->cognitive_weight + config->social_weight;
    double chi = SWARM_PSO_CHI; /* Default constriction factor */
    if (phi > 4.0) {
        /* Clerc's Type 1" constriction */
        chi = 2.0 / fabs(2.0 - phi - sqrt(phi * phi - 4.0 * phi));
    }
    int dim = agent->dim;
    for (int d = 0; d < dim; d++) {
        double r1 = swarm_rng_uniform(), r2 = swarm_rng_uniform();
        double cognitive = config->cognitive_weight * r1 * (agent->best_position[d] - agent->position[d]);
        double social = config->social_weight * r2 * (lbest_position[d] - agent->position[d]);
        agent->velocity[d] = chi * (agent->velocity[d] + cognitive + social);
    }
}

/* ── FIPS Velocity Update (Mendes et al. 2004) ── */
void swarm_pso_update_velocity_fips(SwarmAgent* agent,
    SwarmAgent** swarm, int n_agents, const SwarmConfig* config) {
    if (!agent || !swarm || !config) return;
    int dim = agent->dim;
    /* Compute weighted average of all neighbors' best positions */
    double sum_weights = 0.0, weighted_pos[SWARM_MAX_DIMENSIONS];
    memset(weighted_pos, 0, sizeof(double) * dim);
    /* Include self */
    for (int d = 0; d < dim; d++) weighted_pos[d] += agent->best_position[d];
    sum_weights += 1.0;
    for (int n = 0; n < agent->neighbor_count; n++) {
        int nb = agent->neighbors[n];
        if (nb < 0 || nb >= n_agents) continue;
        for (int d = 0; d < dim; d++)
            weighted_pos[d] += swarm[nb]->best_position[d];
        sum_weights += 1.0;
    }
    if (sum_weights > SWARM_EPSILON) {
        for (int d = 0; d < dim; d++) weighted_pos[d] /= sum_weights;
    }
    double phi_total = config->cognitive_weight + config->social_weight;
    for (int d = 0; d < dim; d++) {
        double r = swarm_rng_uniform();
        agent->velocity[d] = SWARM_PSO_CHI * (agent->velocity[d] + 
            phi_total * r * (weighted_pos[d] - agent->position[d]));
    }
}

/* ── Quantum-behaved PSO (Sun et al. 2004) ── */
void swarm_pso_update_position_quantum(SwarmAgent* agent,
    const double* mbest_position, const SwarmConfig* config) {
    if (!agent || !mbest_position || !config) return;
    /* QPSO: no velocity, position sampled from quantum well */
    double alpha = 1.0 - 0.5 * (1.0 / (double)config->max_iterations);
    int dim = agent->dim;
    for (int d = 0; d < dim; d++) {
        double phi = swarm_rng_uniform();
        double u = swarm_rng_uniform();
        /* Attractor: random combination of pbest and gbest */
        double p_id = phi * agent->best_position[d] + (1.0 - phi) * mbest_position[d];
        /* Mean best position influence */
        double L = 2.0 * alpha * fabs(mbest_position[d] - agent->position[d]);
        if (u > 0.5) agent->position[d] = p_id - 0.5 * L * log(1.0 / swarm_rng_uniform());
        else         agent->position[d] = p_id + 0.5 * L * log(1.0 / swarm_rng_uniform());
    }
}

void swarm_pso_mean_best(SwarmAgent** swarm, int n_agents, double* mbest, int dim) {
    if (!swarm || !mbest) return;
    swarm_vec_set(mbest, dim, 0.0);
    for (int i = 0; i < n_agents; i++)
        for (int d = 0; d < dim; d++)
            mbest[d] += swarm[i]->best_position[d];
    for (int d = 0; d < dim; d++) mbest[d] /= (double)n_agents;
}

/* ── Velocity Clamping ── */
void swarm_pso_clamp_velocity(SwarmAgent* agent, const SwarmConfig* config) {
    if (!agent || !config) return;
    for (int d = 0; d < agent->dim; d++) {
        double vmax = config->max_velocity;
        if (vmax <= 0.0) {
            double range = config->bounds_high[d] - config->bounds_low[d];
            vmax = 0.5 * range;
        }
        if (agent->velocity[d] > vmax) agent->velocity[d] = vmax;
        if (agent->velocity[d] < -vmax) agent->velocity[d] = -vmax;
    }
}

/* ── Boundary Reflection ── */
void swarm_pso_reflect_boundary(SwarmAgent* agent, const double* bounds_low,
                                 const double* bounds_high, double restitution) {
    if (!agent || !bounds_low || !bounds_high) return;
    for (int d = 0; d < agent->dim; d++) {
        if (agent->position[d] < bounds_low[d]) {
            agent->position[d] = 2.0 * bounds_low[d] - agent->position[d];
            agent->velocity[d] *= -restitution;
        }
        if (agent->position[d] > bounds_high[d]) {
            agent->position[d] = 2.0 * bounds_high[d] - agent->position[d];
            agent->velocity[d] *= -restitution;
        }
    }
}

/* ── Turbulence ── */
void swarm_pso_turbulence(SwarmAgent* agent, const SwarmConfig* config) {
    if (!agent || !config) return;
    (void)config;
    /* Add random perturbation to a random dimension */
    int d = swarm_rng_int(agent->dim);
    double range = config->bounds_high[d] - config->bounds_low[d];
    agent->velocity[d] += swarm_rng_gaussian() * range * 0.05;
}

/* ── Adaptive Inertia Weight (Zhan et al. 2009) ── */
double swarm_pso_inertia_linear(double omega_max, double omega_min,
                                 int iteration, int max_iterations) {
    if (max_iterations <= 0) return omega_min;
    double t = (double)iteration / (double)max_iterations;
    return omega_max - (omega_max - omega_min) * t;
}

double swarm_pso_inertia_adaptive(double diversity, double stagnation_ratio) {
    /* Higher inertia when diversity is low (encourage exploration) */
    double base = 0.4;
    double explore = 0.5 * (1.0 - diversity);
    double stagnant = 0.1 * stagnation_ratio;
    return base + explore + stagnant;
}

void swarm_pso_adaptive_coefficients(double* c1, double* c2,
                                      int iteration, int max_iterations) {
    if (!c1 || !c2 || max_iterations <= 0) return;
    double t = (double)iteration / (double)max_iterations;
    /* Cognitive decreases, social increases over time */
    *c1 = 2.5 - 2.0 * t;
    *c2 = 0.5 + 2.0 * t;
}

/* ── Single PSO Iteration ── */
void swarm_pso_iterate(SwarmAgent** swarm, const SwarmConfig* config, int iteration) {
    if (!swarm || !config) return;
    int n = config->n_agents, dim = config->dimensions;
    
    /* Find neighborhood bests */
    int gbest_idx = swarm_pso_find_gbest(swarm, n, config->minimize);
    if (gbest_idx < 0) return;
    
    for (int i = 0; i < n; i++) {
        if (!swarm[i]->is_active) continue;
        
        /* Determine lbest for this agent */
        const double* lbest;
        if (config->topology == SWARM_TOPOLOGY_GLOBAL) {
            lbest = swarm[gbest_idx]->best_position;
        } else {
            /* Best among neighbors (or self if no neighbors) */
            int local_best = i;
            for (int j = 0; j < swarm[i]->neighbor_count; j++) {
                int nb = swarm[i]->neighbors[j];
                if (nb < 0 || nb >= n) continue;
                bool better = config->minimize ? 
                    (swarm[nb]->best_fitness < swarm[local_best]->best_fitness) :
                    (swarm[nb]->best_fitness > swarm[local_best]->best_fitness);
                if (better) local_best = nb;
            }
            lbest = swarm[local_best]->best_position;
        }
        
        /* Update velocity */
        if (config->use_constriction)
            swarm_pso_update_velocity_constricted(swarm[i], lbest, config);
        else
            swarm_pso_update_velocity_standard(swarm[i], lbest, config);
        
        /* Clamp velocity */
        swarm_pso_clamp_velocity(swarm[i], config);
        
        /* Update position: x = x + v */
        for (int d = 0; d < dim; d++)
            swarm[i]->position[d] += swarm[i]->velocity[d];
        
        /* Boundary handling */
        swarm_pso_reflect_boundary(swarm[i], config->bounds_low, config->bounds_high, 0.5);
        
        /* Evaluate fitness */
        swarm_agent_update_fitness(swarm[i], config->fitness_function,
                                    config->fitness_context, config->minimize);
    }
}

/* ── SPSO-2011 Iteration ── */
void swarm_pso_sps2011_iterate(SwarmAgent** swarm, const SwarmConfig* config,
                                int iteration, double* gbest, double* gbest_fitness) {
    if (!swarm || !config) return;
    swarm_pso_iterate(swarm, config, iteration);
    int gb = swarm_pso_find_gbest(swarm, config->n_agents, config->minimize);
    if (gb >= 0 && gbest && gbest_fitness) {
        swarm_vec_copy(swarm[gb]->best_position, gbest, config->dimensions);
        *gbest_fitness = swarm[gb]->best_fitness;
    }
}

/* ── Convergence Check ── */
bool swarm_pso_is_converged(SwarmAgent** swarm, int n_agents, int dim, double epsilon) {
    if (!swarm || n_agents <= 1) return true;
    /* Collect all positions into flat array */
    double* positions = swarm_alloc_vector(n_agents * dim);
    if (!positions) return false;
    for (int i = 0; i < n_agents; i++)
        swarm_vec_copy(swarm[i]->position, positions + i * dim, dim);
    double radius = swarm_diversity_swarm_radius(positions, n_agents, dim);
    swarm_free_vector(positions);
    return radius < epsilon;
}

double swarm_pso_convergence_rate(const double* history, int n_iterations) {
    if (!history || n_iterations < 2) return 0.0;
    /* Exponential fit: find rate α where f(t) ≈ f* + A·exp(-α·t) */
    double f_final = history[n_iterations - 1];
    double f_initial = history[0];
    if (fabs(f_initial - f_final) < SWARM_EPSILON) return 0.0;
    /* Linear regression on log(|f(t) - f_final|) */
    double sum_t = 0, sum_log = 0, sum_tt = 0, sum_tlog = 0;
    int count = 0;
    for (int i = 0; i < n_iterations; i++) {
        double diff = fabs(history[i] - f_final);
        if (diff < SWARM_EPSILON) diff = SWARM_EPSILON;
        sum_t += i; sum_log += log(diff);
        sum_tt += i * i; sum_tlog += i * log(diff);
        count++;
    }
    if (count < 2) return 0.0;
    double denom = count * sum_tt - sum_t * sum_t;
    if (fabs(denom) < SWARM_EPSILON) return 0.0;
    double slope = (count * sum_tlog - sum_t * sum_log) / denom;
    return -slope; /* Convergence rate = -slope of log-diff */
}

int swarm_pso_speciation_count(SwarmAgent** swarm, int n_agents, int dim, double niche_radius) {
    if (!swarm || n_agents <= 0) return 0;
    int* species = swarm_alloc_int_vector(n_agents);
    if (!species) return 0;
    for (int i = 0; i < n_agents; i++) species[i] = -1;
    int n_species = 0;
    for (int i = 0; i < n_agents; i++) {
        if (species[i] >= 0) continue;
        species[i] = n_species;
        for (int j = i + 1; j < n_agents; j++) {
            if (species[j] >= 0) continue;
            double d = swarm_vec_dist(swarm[i]->position, swarm[j]->position, dim);
            if (d <= niche_radius) species[j] = n_species;
        }
        n_species++;
    }
    swarm_free_int_vector(species);
    return n_species;
}

void swarm_pso_speciate(SwarmAgent** swarm, int n_agents, int dim,
                         double niche_radius, int* species_id) {
    if (!swarm || !species_id) return;
    for (int i = 0; i < n_agents; i++) species_id[i] = -1;
    int next_id = 0;
    for (int i = 0; i < n_agents; i++) {
        if (species_id[i] >= 0) continue;
        species_id[i] = next_id;
        for (int j = i + 1; j < n_agents; j++) {
            if (species_id[j] >= 0) continue;
            double d = swarm_vec_dist(swarm[i]->position, swarm[j]->position, dim);
            if (d <= niche_radius) species_id[j] = next_id;
        }
        next_id++;
    }
}

void swarm_pso_update_dynamic_topology(SwarmAgent** swarm, int n_agents,
                                        int dim, double radius) {
    if (!swarm) return;
    for (int i = 0; i < n_agents; i++) {
        int count = 0;
        for (int j = 0; j < n_agents && count < SWARM_MAX_NEIGHBORS; j++) {
            if (j == i) continue;
            double d = swarm_vec_dist(swarm[i]->position, swarm[j]->position, dim);
            if (d <= radius) swarm[i]->neighbors[count++] = j;
        }
        swarm[i]->neighbor_count = count;
        for (int k = count; k < SWARM_MAX_NEIGHBORS; k++) swarm[i]->neighbors[k] = -1;
    }
}

/* ── Full PSO Run ── */
SwarmResult* swarm_pso_run(SwarmConfig* config) {
    if (!config || config->n_agents <= 0 || config->dimensions <= 0) return NULL;
    int n = config->n_agents, dim = config->dimensions, max_iter = config->max_iterations;
    
    SwarmAgent** swarm = swarm_pso_create(config);
    if (!swarm) return NULL;
    
    SwarmResult* result = (SwarmResult*)calloc(1, sizeof(SwarmResult));
    if (!result) { swarm_pso_free(swarm, n); return NULL; }
    result->best_position = swarm_alloc_vector(dim);
    result->history = swarm_alloc_vector(max_iter);
    if (!result->best_position || !result->history) {
        swarm_pso_free(swarm, n); swarm_free_vector(result->best_position);
        swarm_free_vector(result->history); free(result); return NULL;
    }
    
    /* Build initial topology */
    swarm_pso_build_topology(swarm, n, config->topology, config->topology_param);
    
    clock_t t_start = clock();
    int gbest_idx = swarm_pso_find_gbest(swarm, n, config->minimize);
    
    for (int iter = 0; iter < max_iter; iter++) {
        swarm_pso_iterate(swarm, config, iter);
        
        gbest_idx = swarm_pso_find_gbest(swarm, n, config->minimize);
        result->history[iter] = swarm[gbest_idx]->best_fitness;
        
        /* Dynamic topology update */
        if (config->topology == SWARM_TOPOLOGY_DYNAMIC) {
            double radius = 1.0;
            swarm_pso_update_dynamic_topology(swarm, n, dim, radius);
        }
        
        /* Early convergence check */
        if (swarm_pso_is_converged(swarm, n, dim, 1e-6)) {
            result->convergence_iter = iter;
            result->converged = true;
            result->iterations_run = iter + 1;
            break;
        }
        
        /* Target fitness reached? */
        if (config->target_fitness != 0.0 && 
            swarm[gbest_idx]->best_fitness <= config->target_fitness) {
            result->convergence_iter = iter;
            result->converged = true;
            result->iterations_run = iter + 1;
            break;
        }
        
        if (iter == max_iter - 1) {
            result->iterations_run = max_iter;
            result->converged = result->convergence_iter > 0;
        }
    }
    
    result->best_fitness = swarm[gbest_idx]->best_fitness;
    swarm_vec_copy(swarm[gbest_idx]->best_position, result->best_position, dim);
    result->n_evaluations = n + result->iterations_run * n;
    
    /* Compute final diversity */
    double* positions = swarm_alloc_vector(n * dim);
    if (positions) {
        for (int i = 0; i < n; i++)
            swarm_vec_copy(swarm[i]->position, positions + i * dim, dim);
        result->diversity = swarm_diversity_pairwise(positions, n, dim);
        swarm_free_vector(positions);
    }
    
    result->time_seconds = (double)(clock() - t_start) / (double)CLOCKS_PER_SEC;
    
    swarm_pso_free(swarm, n);
    return result;
}

/* ── Multi-Swarm Cooperative PSO ── */
SwarmResult* swarm_pso_cooperative_run(SwarmConfig* base_config,
                                        MultiSwarmConfig* multi_config) {
    if (!base_config || !multi_config || multi_config->n_subswarms <= 0) return NULL;
    /* Simplified: run sequential PSO per sub-swarm, sharing best */
    int total_dim = base_config->dimensions;
    SwarmResult* best_result = NULL;
    double best_global_fitness = 1e100;
    
    for (int s = 0; s < multi_config->n_subswarms; s++) {
        SwarmConfig sub_config = *base_config;
        sub_config.dimensions = multi_config->dims_per_swarm[s];
        SwarmResult* sub_result = swarm_pso_run(&sub_config);
        if (sub_result && sub_result->best_fitness < best_global_fitness) {
            best_global_fitness = sub_result->best_fitness;
            if (best_result) { free(best_result->best_position); 
                free(best_result->history); free(best_result); }
            best_result = sub_result;
        } else if (sub_result) {
            free(sub_result->best_position); free(sub_result->history); free(sub_result);
        }
    }
    
    if (best_result) {
        /* Pad best position to full dimension */
        if (best_result->best_position) {
            double* full = swarm_alloc_vector(total_dim);
            if (full) {
                memcpy(full, best_result->best_position, 
                       (size_t)best_result->best_position == 0 ? 0 : 
                       sizeof(double) * (size_t)base_config->dimensions);
                swarm_free_vector(best_result->best_position);
                best_result->best_position = full;
            }
        }
    }
    return best_result;
}
