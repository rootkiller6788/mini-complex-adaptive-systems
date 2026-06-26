#ifndef SWARM_ACO_H
#define SWARM_ACO_H

#include "swarm_types.h"

/* ============================================================================
 * Ant Colony Optimization (ACO)
 *
 * Dorigo, Maniezzo & Colorni (1991): "Positive Feedback as a Search Strategy"
 * Dorigo & Gambardella (1997): "Ant Colony System" (ACS)
 * Stützle & Hoos (2000): "MAX-MIN Ant System" (MMAS)
 * Dorigo & Stützle (2004): "Ant Colony Optimization" (comprehensive book)
 *
 * Core mechanism:
 *   m ants construct solutions incrementally, choosing next components
 *   probabilistically. The probability of choosing component j from i:
 *
 *     p_{ij}^k = [τ_{ij}]^α · [η_{ij}]^β / Σ_{l∈N_i^k} [τ_{il}]^α · [η_{il}]^β
 *
 *   where τ_{ij} = pheromone, η_{ij} = heuristic, N_i^k = feasible neighbors.
 *
 * Pheromone update (AS):
 *   τ_{ij} ← (1-ρ)·τ_{ij} + Σ_{k=1}^m Δτ_{ij}^k
 *   Δτ_{ij}^k = Q/L_k if ant k used edge (i,j), else 0
 *
 * MAX-MIN Ant System (MMAS) bounds:
 *   τ_min ≤ τ_{ij} ≤ τ_max
 *   Only the iteration-best or global-best ant deposits pheromone.
 *
 * Ant Colony System (ACS) pseudo-random proportional rule:
 *   j = argmax [τ_{il}]^α·[η_{il}]^β  if q ≤ q0 (exploitation)
 *   j = random proportional                     otherwise (exploration)
 *   where q ~ U(0,1) and q0 ∈ [0,1]
 * ============================================================================ */

/* ── ACO Strategy Variants ── */
typedef enum {
    ACO_ANT_SYSTEM   = 0,  /* Original Ant System (Dorigo 1991)               */
    ACO_ELITIST_AS   = 1,  /* Elitist AS: extra pheromone for best tour       */
    ACO_RANK_AS      = 2,  /* Rank-based AS: top-k ants deposit               */
    ACO_ACS          = 3,  /* Ant Colony System: pseudo-random + local update  */
    ACO_MMAS         = 4   /* MAX-MIN Ant System: bounded pheromone           */
} ACOStrategy;

/* ── ACO-specific Configuration ── */
typedef struct {
    ACOStrategy strategy;      /* ACO variant                                   */
    int    n_ants;             /* Number of ants per iteration                   */
    int    n_cities;           /* Number of cities (for TSP)                     */
    double alpha;              /* Pheromone weight exponent                      */
    double beta;               /* Heuristic weight exponent                      */
    double rho;                /* Evaporation rate                               */
    double q0;                 /* ACS exploitation threshold                     */
    double Q;                  /* Pheromone deposit amount                       */
    double tau0;               /* Initial pheromone level                        */
    double tau_min;            /* MMAS lower bound                               */
    double tau_max;            /* MMAS upper bound                               */
    int    n_elite;            /* Elitist AS: number of elite ants               */
    int    n_ranked;           /* Rank-based AS: number of ranked ants           */
    double local_rho;          /* ACS local pheromone decay                      */
    bool   use_nn_heuristic;   /* Use nearest-neighbor heuristic?                */
    bool   use_candidate_list; /* Use candidate list for efficiency?             */
    int    candidate_list_size;/* Size of candidate list                         */
    int    stagnation_limit;   /* MMAS: reinitialize if stagnant                 */
    bool   use_daemon;         /* Apply local search daemon actions?             */
} ACOConfig;

/* ── Ant Tour: a solution built by a single ant ── */
typedef struct {
    int*    tour;              /* City visit order [n_cities]                    */
    double  tour_length;       /* Total tour length                              */
    int     n_cities;          /* Number of cities                               */
    bool    feasible;          /* Is the tour valid?                             */
} AntTour;

/* ── ACO Problem: complete problem definition ── */
typedef struct {
    double** distances;        /* Distance matrix [n][n]                         */
    double** heuristics;       /* Heuristic matrix [n][n] (typically 1/d_ij)     */
    int      n_cities;         /* Number of cities                               */
    bool     symmetric;        /* Symmetric distances? (TSP vs ATSP)             */
    double   optimal_value;    /* Known optimal (if any), for gap calculation    */
} ACOProblem;

/* ── Core ACO API ── */

/* Initialize pheromone matrix */
PheromoneMatrix* swarm_aco_pheromone_create(int n_cities, double tau0);
void             swarm_aco_pheromone_free(PheromoneMatrix* phero);
void             swarm_aco_pheromone_reset(PheromoneMatrix* phero, double tau0);

/* Initialize ACO problem from coordinates (computes Euclidean distances) */
ACOProblem* swarm_aco_problem_from_coords(const double* x, const double* y, 
                                          int n_cities, bool symmetric);
ACOProblem* swarm_aco_problem_from_distances(const double** distances, 
                                             int n_cities, bool symmetric);
void        swarm_aco_problem_free(ACOProblem* problem);

/* Create and free ant tours */
AntTour*  swarm_aco_tour_create(int n_cities);
void      swarm_aco_tour_free(AntTour* tour);
void      swarm_aco_tour_copy(const AntTour* src, AntTour* dst);
void      swarm_aco_tour_print(const AntTour* tour);
double    swarm_aco_tour_length(const AntTour* tour, const ACOProblem* problem);

/* Ant constructs a solution */
void swarm_aco_construct_solution_AS(AntTour* tour, PheromoneMatrix* phero,
           const ACOProblem* problem, const ACOConfig* config);
void swarm_aco_construct_solution_ACS(AntTour* tour, PheromoneMatrix* phero,
           const ACOProblem* problem, const ACOConfig* config);

/* Pheromone update schemes */
void swarm_aco_global_pheromone_update_AS(PheromoneMatrix* phero,
           AntTour** tours, int n_ants, const ACOConfig* config);
void swarm_aco_global_pheromone_update_MMAS(PheromoneMatrix* phero,
           const AntTour* best_tour, const ACOConfig* config);
void swarm_aco_local_pheromone_update_ACS(PheromoneMatrix* phero,
           int city_i, int city_j, double local_rho, double tau0);

/* Full ACO run */
SwarmResult* swarm_aco_run_tsp(const double* x, const double* y, 
                                int n_cities, ACOConfig* config);

/* ── Heuristic Construction ── */
void swarm_aco_compute_distance_heuristic(double** heuristic, double** distances,
           int n_cities);
void swarm_aco_nearest_neighbor_tour(AntTour* tour, const ACOProblem* problem,
           int start_city);

/* ── Candidate List ── */
void swarm_aco_build_candidate_lists(int** candidate_lists, int candidate_size,
           const double** distances, int n_cities);

/* ── Local Search (2-opt, 3-opt) ── */
bool swarm_aco_2opt_improve(AntTour* tour, const ACOProblem* problem);
bool swarm_aco_3opt_improve(AntTour* tour, const ACOProblem* problem);
bool swarm_aco_oropt_improve(AntTour* tour, const ACOProblem* problem, int max_segment);

/* ── Pheromone Bounds (MMAS) ── */
void swarm_aco_update_pheromone_bounds(PheromoneMatrix* phero,
           double best_tour_length, const ACOConfig* config, double rho,
           int n_cities);
void swarm_aco_enforce_pheromone_limits(PheromoneMatrix* phero);

/* ── Daemon Actions (optional intensification) ── */
void swarm_aco_daemon_local_search(AntTour** tours, int n_ants,
           const ACOProblem* problem, int n_improved);
void swarm_aco_daemon_elitist(AntTour** tours, AntTour* best_so_far,
           int n_ants);

/* ── Convergence Analysis ── */
double swarm_aco_pheromone_entropy(const PheromoneMatrix* phero);
double swarm_aco_convergence_factor(const PheromoneMatrix* phero, 
           const ACOConfig* config);

/* ── Scheduler for parameter adaptation ── */
void swarm_aco_adapt_pheromone_bounds(PheromoneMatrix* phero,
           double best_tour_length, ACOConfig* config, int iteration);

#endif /* SWARM_ACO_H */
