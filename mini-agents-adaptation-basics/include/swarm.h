/**
 * swarm.h — Swarm Intelligence Algorithms
 *
 * Decentralized self-organized systems: collective behavior emerges from
 * local interactions of simple agents.
 *
 * Reference: Kennedy & Eberhart "PSO" (1995)
 *            Dorigo & Stutzle "ACO" (2004)
 *            Reynolds "Flocks, Herds, and Schools" (1987)
 *            Yang "Nature-Inspired Metaheuristic Algorithms" (2010)
 *
 * Knowledge coverage: L1 (Definitions), L2 (Stigmergy, Emergence),
 *                     L5 (Algorithms), L6 (Function optimization)
 */

#ifndef SWARM_H
#define SWARM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

/* ====================================================================
 * PART I: Particle Swarm Optimization (PSO)
 * ==================================================================== */

typedef struct {
    int       particle_id;
    int       dim;
    double   *position;
    double   *velocity;
    double   *personal_best;
    double    personal_best_value;
    double    current_value;
} PSO_Particle;

typedef struct {
    int            num_particles;
    int            dim;
    PSO_Particle  *particles;
    double        *global_best;
    double         global_best_value;
    double         inertia_weight;
    double         cognitive_weight;
    double         social_weight;
    double         max_velocity;
    double        *lower_bound;
    double        *upper_bound;
    int            iterations;
    double         history[100];
    int            history_len;
} PSO_Swarm;

PSO_Swarm*   pso_create(int num_particles, int dim,
                         const double *lb, const double *ub,
                         double inertia, double cognitive, double social);
void         pso_random_init(PSO_Swarm *swarm);
void         pso_evaluate(PSO_Swarm *swarm,
                           double (*obj)(const double *x, int dim, void *ctx),
                           void *ctx);
void         pso_update(PSO_Swarm *swarm);
int          pso_run(PSO_Swarm *swarm, int n_iterations,
                      double (*obj)(const double *x, int dim, void *ctx),
                      void *ctx, bool verbose);
const double* pso_get_best(const PSO_Swarm *swarm);
const double* pso_history(const PSO_Swarm *swarm, int *len_out);
void         pso_linear_inertia(PSO_Swarm *swarm, double w_start, double w_end,
                                 int current_iter, int total_iter);
double       pso_constriction_coefficient(double c1, double c2);
void         pso_set_constriction(PSO_Swarm *swarm, double c1, double c2);
void         pso_destroy(PSO_Swarm *swarm);

/* ====================================================================
 * PART II: Ant Colony Optimization (ACO)
 * ==================================================================== */

typedef struct {
    int       ant_id;
    int       num_cities;
    int      *tour;
    double    tour_length;
    int       tour_size;
    bool     *visited;
} ACO_Ant;

typedef struct {
    int         num_ants;
    int         num_cities;
    ACO_Ant    *ants;
    double     *distance_matrix;
    double    **pheromone;
    double    **heuristic;
    double      alpha;
    double      beta;
    double      evaporation_rate;
    double      q0;
    double      best_tour_length;
    int        *best_tour;
    int         iterations;
} ACO_Colony;

ACO_Colony* aco_create(int num_ants, int num_cities,
                        const double *distance_matrix,
                        double alpha, double beta, double evaporation);
void        aco_init_pheromone(ACO_Colony *colony, double tau0);
void        aco_construct_tour(ACO_Colony *colony, int ant_id);
void        aco_construct_all_tours(ACO_Colony *colony);
void        aco_evaporate(ACO_Colony *colony);
void        aco_deposit_pheromone(ACO_Colony *colony);
void        aco_elitist_deposit(ACO_Colony *colony, double elitist_weight);
int         aco_run(ACO_Colony *colony, int n_iterations, bool verbose);
const int*  aco_get_best_tour(const ACO_Colony *colony);
double      aco_get_best_length(const ACO_Colony *colony);
bool        aco_validate_tour(const ACO_Ant *ant);
double      aco_lower_bound(const ACO_Colony *colony);
void        aco_destroy(ACO_Colony *colony);

/* ====================================================================
 * PART III: Flocking — Reynolds Boids (1987)
 * ==================================================================== */

typedef struct {
    int       id;
    double   *position;
    double   *velocity;
    int       dim;
} Boid;

typedef struct {
    int       num_boids;
    int       dim;
    Boid     *boids;
    double    cohesion_weight;
    double    separation_weight;
    double    alignment_weight;
    double    perception_radius;
    double    separation_radius;
    double    max_speed;
    double    max_force;
    double   *bound_min;
    double   *bound_max;
    double    boundary_margin;
} Flock;

Flock* flock_create(int n, int dim, double perception_radius,
                     double cohesion, double separation, double alignment,
                     double max_speed);
void   flock_random_init(Flock *f, const double *min_bounds,
                          const double *max_bounds);
void   flock_step(Flock *f, double dt);
void   flock_center_of_mass(const Flock *f, double *center_out);
double flock_polarization(const Flock *f);
void   flock_run(Flock *f, int n_steps, double dt);
void   flock_destroy(Flock *f);

/* ====================================================================
 * PART IV: Firefly Algorithm (Yang, 2010)
 * ==================================================================== */

typedef struct {
    int       id;
    int       dim;
    double   *position;
    double    brightness;
} Firefly;

typedef struct {
    int        num_fireflies;
    int        dim;
    Firefly   *fireflies;
    double     alpha;
    double     beta0;
    double     gamma;
    double    *lower_bound;
    double    *upper_bound;
    int        iterations;
} FireflySwarm;

FireflySwarm* firefly_create(int n, int dim, const double *lb, const double *ub,
                              double alpha, double beta0, double gamma_abs);
void          firefly_random_init(FireflySwarm *fs);
void          firefly_evaluate(FireflySwarm *fs,
                                double (*obj)(const double *x, int dim, void *ctx),
                                void *ctx);
void          firefly_move(FireflySwarm *fs);
int           firefly_run(FireflySwarm *fs, int n_iterations,
                           double (*obj)(const double *x, int dim, void *ctx),
                           void *ctx, bool verbose);
const Firefly* firefly_get_best(const FireflySwarm *fs);
void          firefly_destroy(FireflySwarm *fs);

/* ====================================================================
 * PART V: Utility Functions
 * ==================================================================== */

double swarm_euclidean_distance(const double *a, const double *b, int dim);
double swarm_clamp(double value, double low, double high);
double swarm_uniform(double low, double high);
double swarm_gaussian(double mean, double std);
double swarm_test_rastrigin(const double *x, int dim, void *ctx);
double swarm_test_rosenbrock(const double *x, int dim, void *ctx);
double swarm_test_griewank(const double *x, int dim, void *ctx);

#endif /* SWARM_H */
