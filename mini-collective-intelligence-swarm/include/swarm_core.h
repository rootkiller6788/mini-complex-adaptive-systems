#ifndef SWARM_CORE_H
#define SWARM_CORE_H

#include "swarm_types.h"
#include <math.h>
#include <stdlib.h>

/* ============================================================================
 * Swarm Core — Mathematical Utilities for Collective Intelligence
 *
 * Provides: random number generation, vector operations, distance metrics,
 * graph Laplacian construction, selection operators, diversity metrics.
 * ============================================================================ */

/* ── Mathematical Constants ── */
#define SWARM_PI           3.14159265358979323846
#define SWARM_E             2.71828182845904523536
#define SWARM_EPSILON       1e-12
#define SWARM_EPSILON_MED   1e-8
#define SWARM_EPSILON_COARSE 1e-6
#define SWARM_GOLDEN_RATIO  1.61803398874989484820

/* ── PSO Constants (from literature) ── */
#define SWARM_PSO_INERTIA_DEFAULT    0.7298   /* Clerc-Kennedy constriction */
#define SWARM_PSO_COGNITIVE_DEFAULT  1.49618  /* c1 default */
#define SWARM_PSO_SOCIAL_DEFAULT     1.49618  /* c2 default */
#define SWARM_PSO_CHI                0.7298   /* Constriction factor χ */

/* ── ACO Constants ── */
#define SWARM_ACO_EVAP_DEFAULT       0.1      /* Evaporation rate ρ */
#define SWARM_ACO_ALPHA_DEFAULT      1.0      /* Pheromone exponent */
#define SWARM_ACO_BETA_DEFAULT       5.0      /* Heuristic exponent */

/* ── Random Number Generation ──
 *
 * Uses xoshiro256** for fast, high-quality pseudorandom numbers.
 * Period = 2^256 - 1. State size = 256 bits.
 * Reference: Blackman & Vigna (2021), "Scrambled Linear Pseudorandom Number Generators"
 */
void       swarm_rng_seed(uint64_t seed);
uint64_t   swarm_rng_next(void);
double     swarm_rng_uniform(void);          /* Uniform [0, 1)                */
double     swarm_rng_uniform_range(double a, double b); /* Uniform [a, b)     */
double     swarm_rng_gaussian(void);         /* N(0,1) Box-Muller             */
double     swarm_rng_cauchy(double scale);   /* Cauchy(0, scale) for fat-tail */
int        swarm_rng_int(int max_exclusive); /* Uniform int [0, max)          */
void       swarm_rng_shuffle(int* arr, int n); /* Fisher-Yates shuffle        */

/* ── Vector Operations (D-dimensional) ── */
void       swarm_vec_copy(const double* src, double* dst, int dim);
void       swarm_vec_set(double* v, int dim, double value);
void       swarm_vec_add(const double* a, const double* b, double* c, int dim);
void       swarm_vec_sub(const double* a, const double* b, double* c, int dim);
void       swarm_vec_scale(double* v, int dim, double scalar);
void       swarm_vec_mul(const double* a, const double* b, double* c, int dim); /* Hadamard */
double     swarm_vec_dot(const double* a, const double* b, int dim);
double     swarm_vec_norm(const double* v, int dim);          /* Euclidean L2 */
double     swarm_vec_norm_sq(const double* v, int dim);      /* Squared L2   */
double     swarm_vec_dist(const double* a, const double* b, int dim); /* L2   */
double     swarm_vec_dist_manhattan(const double* a, const double* b, int dim);
double     swarm_vec_dist_infinity(const double* a, const double* b, int dim);
void       swarm_vec_normalize(double* v, int dim);
void       swarm_vec_clamp(double* v, int dim, double min_val, double max_val);
void       swarm_vec_reflect(const double* v, double* r, int dim, 
                             const double* bounds_low, const double* bounds_high);
void       swarm_vec_lerp(const double* a, const double* b, double t, double* c, int dim);
int        swarm_vec_argmin(const double* v, int dim);
int        swarm_vec_argmax(const double* v, int dim);
double     swarm_vec_mean(const double* v, int dim);
double     swarm_vec_variance(const double* v, int dim, double mean);
double     swarm_vec_std(const double* v, int dim);
void       swarm_vec_print(const double* v, int dim, const char* label);

/* ── Matrix Operations (for pheromone, adjacency, weight matrices) ── */
double**   swarm_matrix_alloc(int rows, int cols);
void       swarm_matrix_free(double** m, int rows);
void       swarm_matrix_copy(double** src, double** dst, int rows, int cols);
void       swarm_matrix_set(double** m, int rows, int cols, double value);
void       swarm_matrix_normalize_rows(double** m, int rows, int cols); /* Row-stochastic */
double     swarm_matrix_row_sum(const double* row, int cols);
bool       swarm_matrix_is_stochastic(double** m, int rows, int cols, double tol);

/* ── Graph & Topology Operations ── */
void       swarm_topology_ring(int* neighbors, int agent_id, int n_agents, int k);
void       swarm_topology_von_neumann(int* neighbors, int agent_id, int grid_w, int grid_h);
void       swarm_topology_star(int* neighbors, int agent_id, int n_agents);
void       swarm_topology_random(int* neighbors, int agent_id, int n_agents, int k);
void       swarm_topology_dynamic_build(int* neighbors, const double* positions,
                   int agent_id, int n_agents, int dim, double radius, int max_neighbors);
double**   swarm_graph_laplacian(double** adjacency, int n_nodes);
double**   swarm_graph_adjacency_from_positions(const double* positions, int n,
                   int dim, double radius);
double*    swarm_graph_degree_centrality(double** adjacency, int n);
double     swarm_graph_clustering_coefficient(double** adjacency, int n, int node);
double     swarm_graph_average_path_length(double** adjacency, int n);
bool       swarm_graph_is_connected(double** adjacency, int n);

/* ── Selection Operators (for evolutionary swarm methods) ── */
int        swarm_select_roulette(const double* fitness, int n, bool minimize);
int        swarm_select_tournament(const double* fitness, int n, int k, bool minimize);
int        swarm_select_rank(const double* fitness, int n, int select_rank, bool minimize);
void       swarm_compute_ranks(const double* fitness, int* ranks, int n, bool minimize);

/* ── Diversity Metrics ── */
double     swarm_diversity_pairwise(const double* positions, int n_agents, int dim);
double     swarm_diversity_entropy(const double* positions, int n_agents, int dim,
                   const double* bounds_low, const double* bounds_high, int bins);
double     swarm_diversity_swarm_radius(const double* positions, int n_agents, int dim);
void       swarm_centroid(const double* positions, int n_agents, int dim, double* centroid);

/* ── Fitness Landscape Analysis ── */
void       swarm_landscape_random_walk(FitnessLandscape* landscape,
                   double (*fitness_fn)(const double*, int, void*),
                   const double* bounds_low, const double* bounds_high,
                   int dim, int n_steps, double step_size, void* context);
void       swarm_landscape_free(FitnessLandscape* landscape);

/* ── Convergence Diagnostics ── */
void       swarm_diagnostics_update(ConvergenceDiagnostics* diag,
                   const double* positions, const double* velocities,
                   const double* fitnesses, int n_agents, int dim,
                   const double* bounds_low, const double* bounds_high,
                   double best_fitness_prev, int iteration);
void       swarm_diagnostics_reset(ConvergenceDiagnostics* diag);
const char* swarm_diagnostics_summary(const ConvergenceDiagnostics* diag);

/* ── Agent Management ── */
SwarmAgent* swarm_agent_create(int dim);
void        swarm_agent_free(SwarmAgent* agent);
void        swarm_agent_init_random(SwarmAgent* agent, int dim, int id,
                   const double* bounds_low, const double* bounds_high);
void        swarm_agent_copy(const SwarmAgent* src, SwarmAgent* dst);
void        swarm_agent_update_fitness(SwarmAgent* agent,
                   double (*fitness_fn)(const double*, int, void*),
                   void* context, bool minimize);

/* ── Memory Allocation Helpers ── */
double*     swarm_alloc_vector(int dim);
void        swarm_free_vector(double* v);
int*        swarm_alloc_int_vector(int n);
void        swarm_free_int_vector(int* v);

#endif /* SWARM_CORE_H */
