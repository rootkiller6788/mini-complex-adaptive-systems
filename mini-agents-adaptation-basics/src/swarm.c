/**
 * swarm.c — Swarm Intelligence Implementation
 *
 * Implements four major swarm intelligence paradigms:
 *   1. PSO (Particle Swarm Optimization) — Kennedy & Eberhart (1995)
 *   2. ACO (Ant Colony Optimization) — Dorigo (1992), Dorigo & Stutzle (2004)
 *   3. Boids (Flocking) — Reynolds (1987)
 *   4. Firefly Algorithm — Yang (2010)
 *
 * Plus utility functions and standard test functions for optimization.
 *
 * Reference: Kennedy & Eberhart (1995), Shi & Eberhart (1998) — inertia weight
 *            Clerc & Kennedy (2002) — constriction coefficient
 *            Dorigo & Stutzle (2004) — ACO for TSP
 *            Reynolds (1987) — Boids
 *            Yang (2010) — Firefly Algorithm
 *
 * Knowledge: L1 Definitions, L2 Core Concepts (stigmergy, emergence),
 *            L5 Algorithms, L6 Function optimization problems
 */

#include "swarm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===================================================================
 * PART V: Utility Functions
 * =================================================================== */

double swarm_euclidean_distance(const double *a, const double *b, int dim) {
    double sum = 0.0;
    for (int i = 0; i < dim; i++) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

double swarm_clamp(double value, double low, double high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

double swarm_uniform(double low, double high) {
    return low + (high - low) * (double)rand() / (double)RAND_MAX;
}

double swarm_gaussian(double mean, double std) {
    double u1 = (double)rand() / RAND_MAX;
    double u2 = (double)rand() / RAND_MAX;
    double z = sqrt(-2.0 * log(u1 < 1e-12 ? 1e-12 : u1))
               * cos(2.0 * M_PI * u2);
    return mean + std * z;
}

/* ===================================================================
 * PART V: Standard Test Functions
 * =================================================================== */

/* Rastrigin function: f(x) = 10n + sum(x_i^2 - 10 cos(2*pi*x_i))
 * Global minimum at x_i = 0, f = 0. Highly multimodal. */
double swarm_test_rastrigin(const double *x, int dim, void *ctx) {
    (void)ctx;
    double sum = 10.0 * (double)dim;
    for (int i = 0; i < dim; i++) {
        sum += x[i] * x[i] - 10.0 * cos(2.0 * M_PI * x[i]);
    }
    return sum;
}

/* Rosenbrock (banana) function: f(x) = sum_{i=1}^{n-1} [100(x_{i+1}-x_i^2)^2 + (1-x_i)^2]
 * Global minimum at x_i = 1, f = 0. Narrow curved valley. */
double swarm_test_rosenbrock(const double *x, int dim, void *ctx) {
    (void)ctx;
    double sum = 0.0;
    for (int i = 0; i < dim - 1; i++) {
        double t1 = x[i+1] - x[i] * x[i];
        double t2 = 1.0 - x[i];
        sum += 100.0 * t1 * t1 + t2 * t2;
    }
    return sum;
}

/* Griewank function: f(x) = 1 + sum(x_i^2/4000) - prod(cos(x_i/sqrt(i+1)))
 * Global minimum at x_i = 0, f = 0. Many widespread local minima. */
double swarm_test_griewank(const double *x, int dim, void *ctx) {
    (void)ctx;
    double sum = 0.0;
    double prod = 1.0;
    for (int i = 0; i < dim; i++) {
        sum += x[i] * x[i] / 4000.0;
        prod *= cos(x[i] / sqrt((double)(i + 1)));
    }
    return 1.0 + sum - prod;
}

/* ===================================================================
 * PART I: Particle Swarm Optimization (PSO)
 * =================================================================== */

PSO_Swarm* pso_create(int num_particles, int dim,
                       const double *lb, const double *ub,
                       double inertia, double cognitive, double social) {
    PSO_Swarm *swarm = (PSO_Swarm*)calloc(1, sizeof(PSO_Swarm));
    if (!swarm) return NULL;

    swarm->num_particles = num_particles;
    swarm->dim = dim;
    swarm->inertia_weight = inertia;
    swarm->cognitive_weight = cognitive;
    swarm->social_weight = social;
    swarm->max_velocity = 1.0;
    swarm->global_best_value = DBL_MAX;

    /* Allocate bounds */
    swarm->lower_bound = (double*)calloc(dim, sizeof(double));
    swarm->upper_bound = (double*)calloc(dim, sizeof(double));
    if (lb) memcpy(swarm->lower_bound, lb, dim * sizeof(double));
    if (ub) memcpy(swarm->upper_bound, ub, dim * sizeof(double));

    /* Allocate global best */
    swarm->global_best = (double*)calloc(dim, sizeof(double));

    /* Allocate particles */
    swarm->particles = (PSO_Particle*)calloc(num_particles, sizeof(PSO_Particle));
    for (int i = 0; i < num_particles; i++) {
        PSO_Particle *p = &swarm->particles[i];
        p->particle_id = i;
        p->dim = dim;
        p->position = (double*)calloc(dim, sizeof(double));
        p->velocity = (double*)calloc(dim, sizeof(double));
        p->personal_best = (double*)calloc(dim, sizeof(double));
        p->personal_best_value = DBL_MAX;
    }

    swarm->iterations = 0;
    swarm->history_len = 0;

    return swarm;
}

void pso_random_init(PSO_Swarm *swarm) {
    if (!swarm) return;

    for (int i = 0; i < swarm->num_particles; i++) {
        PSO_Particle *p = &swarm->particles[i];
        for (int d = 0; d < swarm->dim; d++) {
            p->position[d] = swarm_uniform(swarm->lower_bound[d],
                                            swarm->upper_bound[d]);
            double range = swarm->upper_bound[d] - swarm->lower_bound[d];
            p->velocity[d] = swarm_uniform(-range * 0.1, range * 0.1);
        }
    }
}

void pso_evaluate(PSO_Swarm *swarm,
                   double (*obj)(const double *x, int dim, void *ctx),
                   void *ctx) {
    if (!swarm || !obj) return;

    for (int i = 0; i < swarm->num_particles; i++) {
        PSO_Particle *p = &swarm->particles[i];
        p->current_value = obj(p->position, swarm->dim, ctx);

        /* Update personal best */
        if (p->current_value < p->personal_best_value) {
            p->personal_best_value = p->current_value;
            memcpy(p->personal_best, p->position,
                   swarm->dim * sizeof(double));

            /* Update global best */
            if (p->current_value < swarm->global_best_value) {
                swarm->global_best_value = p->current_value;
                memcpy(swarm->global_best, p->position,
                       swarm->dim * sizeof(double));
            }
        }
    }

    /* Record history */
    if (swarm->history_len < 100) {
        swarm->history[swarm->history_len++] = swarm->global_best_value;
    }
}

void pso_update(PSO_Swarm *swarm) {
    if (!swarm) return;

    for (int i = 0; i < swarm->num_particles; i++) {
        PSO_Particle *p = &swarm->particles[i];

        for (int d = 0; d < swarm->dim; d++) {
            double r1 = swarm_uniform(0.0, 1.0);
            double r2 = swarm_uniform(0.0, 1.0);

            /* PSO velocity update:
             * v_i(t+1) = w * v_i(t)
             *          + c1 * r1 * (pbest_i - x_i(t))
             *          + c2 * r2 * (gbest - x_i(t))
             */
            double cognitive_term = swarm->cognitive_weight * r1
                                    * (p->personal_best[d] - p->position[d]);
            double social_term = swarm->social_weight * r2
                                 * (swarm->global_best[d] - p->position[d]);

            p->velocity[d] = swarm->inertia_weight * p->velocity[d]
                             + cognitive_term + social_term;

            /* Clamp velocity */
            p->velocity[d] = swarm_clamp(p->velocity[d],
                                          -swarm->max_velocity,
                                          swarm->max_velocity);

            /* Update position */
            p->position[d] += p->velocity[d];

            /* Clamp position to bounds */
            p->position[d] = swarm_clamp(p->position[d],
                                          swarm->lower_bound[d],
                                          swarm->upper_bound[d]);
        }
    }

    swarm->iterations++;
}

int pso_run(PSO_Swarm *swarm, int n_iterations,
             double (*obj)(const double *x, int dim, void *ctx),
             void *ctx, bool verbose) {
    if (!swarm || !obj) return 0;

    for (int iter = 0; iter < n_iterations; iter++) {
        pso_evaluate(swarm, obj, ctx);
        pso_update(swarm);

        if (verbose && iter % 10 == 0) {
            printf("PSO iter %d: best=%f\n", iter, swarm->global_best_value);
        }
    }
    return swarm->iterations;
}

const double* pso_get_best(const PSO_Swarm *swarm) {
    return swarm ? swarm->global_best : NULL;
}

const double* pso_history(const PSO_Swarm *swarm, int *len_out) {
    if (!swarm) { *len_out = 0; return NULL; }
    *len_out = swarm->history_len;
    return swarm->history;
}

void pso_linear_inertia(PSO_Swarm *swarm, double w_start, double w_end,
                         int current_iter, int total_iter) {
    if (!swarm || total_iter <= 0) return;
    double ratio = (double)current_iter / (double)total_iter;
    swarm->inertia_weight = w_start - (w_start - w_end) * ratio;
}

double pso_constriction_coefficient(double c1, double c2) {
    double phi = c1 + c2;
    if (phi <= 4.0) {
        /* Formula requires phi > 4 */
        return 0.729; /* Clerc's kappa = 1 default */
    }
    double chi = 2.0 / fabs(2.0 - phi - sqrt(phi * phi - 4.0 * phi));
    return chi;
}

void pso_set_constriction(PSO_Swarm *swarm, double c1, double c2) {
    if (!swarm) return;
    double chi = pso_constriction_coefficient(c1, c2);
    swarm->inertia_weight = chi;
    swarm->cognitive_weight = chi * c1;
    swarm->social_weight = chi * c2;
}

void pso_destroy(PSO_Swarm *swarm) {
    if (!swarm) return;
    for (int i = 0; i < swarm->num_particles; i++) {
        free(swarm->particles[i].position);
        free(swarm->particles[i].velocity);
        free(swarm->particles[i].personal_best);
    }
    free(swarm->particles);
    free(swarm->global_best);
    free(swarm->lower_bound);
    free(swarm->upper_bound);
    free(swarm);
}
/* ===================================================================
 * PART II: Ant Colony Optimization (ACO) for TSP
 * =================================================================== */

ACO_Colony* aco_create(int num_ants, int num_cities,
                        const double *distance_matrix,
                        double alpha, double beta, double evaporation) {
    ACO_Colony *colony = (ACO_Colony*)calloc(1, sizeof(ACO_Colony));
    if (!colony) return NULL;

    colony->num_ants = num_ants;
    colony->num_cities = num_cities;
    colony->alpha = alpha;
    colony->beta = beta;
    colony->evaporation_rate = evaporation;
    colony->q0 = 0.9; /* exploitation probability */
    colony->best_tour_length = DBL_MAX;

    /* Store distance matrix */
    colony->distance_matrix = (double*)calloc(num_cities * num_cities,
                                               sizeof(double));
    if (distance_matrix) {
        memcpy(colony->distance_matrix, distance_matrix,
               num_cities * num_cities * sizeof(double));
    }

    /* Allocate pheromone matrix */
    colony->pheromone = (double**)calloc(num_cities, sizeof(double*));
    for (int i = 0; i < num_cities; i++) {
        colony->pheromone[i] = (double*)calloc(num_cities, sizeof(double));
    }

    /* Allocate heuristic matrix: eta_ij = 1 / d_ij */
    colony->heuristic = (double**)calloc(num_cities, sizeof(double*));
    for (int i = 0; i < num_cities; i++) {
        colony->heuristic[i] = (double*)calloc(num_cities, sizeof(double));
        for (int j = 0; j < num_cities; j++) {
            if (i != j) {
                double d = colony->distance_matrix[i * num_cities + j];
                colony->heuristic[i][j] = (d > 1e-12) ? (1.0 / d) : 1e10;
            }
        }
    }

    /* Allocate ants */
    colony->ants = (ACO_Ant*)calloc(num_ants, sizeof(ACO_Ant));
    for (int k = 0; k < num_ants; k++) {
        colony->ants[k].ant_id = k;
        colony->ants[k].num_cities = num_cities;
        colony->ants[k].tour = (int*)calloc(num_cities, sizeof(int));
        colony->ants[k].visited = (bool*)calloc(num_cities, sizeof(bool));
        colony->ants[k].tour_size = 0;
        colony->ants[k].tour_length = 0.0;
    }

    /* Allocate best tour */
    colony->best_tour = (int*)calloc(num_cities, sizeof(int));

    return colony;
}

void aco_init_pheromone(ACO_Colony *colony, double tau0) {
    if (!colony) return;

    for (int i = 0; i < colony->num_cities; i++) {
        for (int j = 0; j < colony->num_cities; j++) {
            colony->pheromone[i][j] = tau0;
        }
    }
}

void aco_construct_tour(ACO_Colony *colony, int ant_id) {
    if (!colony || ant_id < 0 || ant_id >= colony->num_ants) return;

    ACO_Ant *ant = &colony->ants[ant_id];
    int n = colony->num_cities;

    /* Reset ant state */
    ant->tour_size = 0;
    ant->tour_length = 0.0;
    memset(ant->visited, 0, n * sizeof(bool));

    /* Start at a random city */
    int start = rand() % n;
    ant->tour[0] = start;
    ant->visited[start] = true;
    ant->tour_size = 1;

    /* Build tour: for each step, select next unvisited city */
    for (int step = 1; step < n; step++) {
        int current = ant->tour[step - 1];

        /* Compute denominator (sum of tau^alpha * eta^beta for unvisited) */
        double denom = 0.0;
        for (int j = 0; j < n; j++) {
            if (ant->visited[j]) continue;
            double tau_ij = pow(colony->pheromone[current][j], colony->alpha);
            double eta_ij = pow(colony->heuristic[current][j], colony->beta);
            denom += tau_ij * eta_ij;
        }

        if (denom < 1e-12) {
            /* Fallback: pick first unvisited */
            for (int j = 0; j < n; j++) {
                if (!ant->visited[j]) {
                    ant->tour[step] = j;
                    ant->visited[j] = true;
                    ant->tour_size++;
                    ant->tour_length += colony->distance_matrix[current * n + j];
                    break;
                }
            }
            continue;
        }

        /* Probabilistic selection or pseudo-random proportional rule */
        double q = swarm_uniform(0.0, 1.0);
        int next_city = -1;

        if (q < colony->q0) {
            /* Exploit: select city with max tau^alpha * eta^beta */
            double best_val = -1.0;
            for (int j = 0; j < n; j++) {
                if (ant->visited[j]) continue;
                double val = pow(colony->pheromone[current][j], colony->alpha)
                             * pow(colony->heuristic[current][j], colony->beta);
                if (val > best_val) {
                    best_val = val;
                    next_city = j;
                }
            }
        } else {
            /* Explore: probabilistic selection (roulette wheel) */
            double r = swarm_uniform(0.0, 1.0);
            double cum = 0.0;
            for (int j = 0; j < n; j++) {
                if (ant->visited[j]) continue;
                double prob = pow(colony->pheromone[current][j], colony->alpha)
                              * pow(colony->heuristic[current][j], colony->beta)
                              / denom;
                cum += prob;
                if (r <= cum) {
                    next_city = j;
                    break;
                }
            }
        }

        /* Fallback */
        if (next_city < 0) {
            for (int j = 0; j < n; j++) {
                if (!ant->visited[j]) { next_city = j; break; }
            }
        }

        ant->tour[step] = next_city;
        ant->visited[next_city] = true;
        ant->tour_size++;
        ant->tour_length += colony->distance_matrix[current * n + next_city];
    }

    /* Add return to start to complete the tour */
    int last = ant->tour[n - 1];
    ant->tour_length += colony->distance_matrix[last * n + ant->tour[0]];

    /* Update best tour */
    if (ant->tour_length < colony->best_tour_length) {
        colony->best_tour_length = ant->tour_length;
        memcpy(colony->best_tour, ant->tour, n * sizeof(int));
    }
}

void aco_construct_all_tours(ACO_Colony *colony) {
    if (!colony) return;
    for (int k = 0; k < colony->num_ants; k++) {
        aco_construct_tour(colony, k);
    }
}

void aco_evaporate(ACO_Colony *colony) {
    if (!colony) return;
    double rho = colony->evaporation_rate;
    int n = colony->num_cities;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            colony->pheromone[i][j] *= (1.0 - rho);
            /* Minimum pheromone to prevent stagnation */
            if (colony->pheromone[i][j] < 1e-15) {
                colony->pheromone[i][j] = 1e-15;
            }
        }
    }
}

void aco_deposit_pheromone(ACO_Colony *colony) {
    if (!colony) return;
    int n = colony->num_cities;

    /* Each ant deposits pheromone proportional to 1/tour_length */
    for (int k = 0; k < colony->num_ants; k++) {
        ACO_Ant *ant = &colony->ants[k];
        if (ant->tour_length < 1e-12) continue;

        double deposit = 1.0 / ant->tour_length;

        for (int step = 0; step < n - 1; step++) {
            int i = ant->tour[step];
            int j = ant->tour[step + 1];
            colony->pheromone[i][j] += deposit;
            colony->pheromone[j][i] += deposit; /* symmetric TSP */
        }
        /* Close the tour */
        int last = ant->tour[n - 1];
        colony->pheromone[last][ant->tour[0]] += deposit;
        colony->pheromone[ant->tour[0]][last] += deposit;
    }
}

void aco_elitist_deposit(ACO_Colony *colony, double elitist_weight) {
    if (!colony) return;
    int n = colony->num_cities;

    /* Extra pheromone for the best-so-far tour */
    if (colony->best_tour_length < DBL_MAX && colony->best_tour_length > 1e-12) {
        double deposit = elitist_weight / colony->best_tour_length;

        for (int step = 0; step < n - 1; step++) {
            int i = colony->best_tour[step];
            int j = colony->best_tour[step + 1];
            colony->pheromone[i][j] += deposit;
            colony->pheromone[j][i] += deposit;
        }
        int last = colony->best_tour[n - 1];
        colony->pheromone[last][colony->best_tour[0]] += deposit;
        colony->pheromone[colony->best_tour[0]][last] += deposit;
    }
}

int aco_run(ACO_Colony *colony, int n_iterations, bool verbose) {
    if (!colony) return 0;

    for (int iter = 0; iter < n_iterations; iter++) {
        aco_construct_all_tours(colony);
        aco_evaporate(colony);
        aco_deposit_pheromone(colony);
        aco_elitist_deposit(colony, 2.0); /* elitist weight */

        colony->iterations++;

        if (verbose && iter % 10 == 0) {
            printf("ACO iter %d: best=%f\n", iter, colony->best_tour_length);
        }
    }
    return colony->iterations;
}

const int* aco_get_best_tour(const ACO_Colony *colony) {
    return colony ? colony->best_tour : NULL;
}

double aco_get_best_length(const ACO_Colony *colony) {
    return colony ? colony->best_tour_length : DBL_MAX;
}

bool aco_validate_tour(const ACO_Ant *ant) {
    if (!ant) return false;

    /* Check: all cities visited exactly once */
    int n = ant->num_cities;
    int *counts = (int*)calloc(n, sizeof(int));
    for (int i = 0; i < n; i++) {
        int city = ant->tour[i];
        if (city < 0 || city >= n) { free(counts); return false; }
        counts[city]++;
    }
    for (int i = 0; i < n; i++) {
        if (counts[i] != 1) { free(counts); return false; }
    }
    free(counts);

    /* Tour structure validated: all cities visited exactly once */
    (void)n; /* n used only in validation above, suppress warning */
    return true;
}

double aco_lower_bound(const ACO_Colony *colony) {
    /* Nearest-neighbor heuristic lower bound: sum of minimum edge per vertex */
    if (!colony) return 0.0;
    int n = colony->num_cities;
    double bound = 0.0;

    for (int i = 0; i < n; i++) {
        double min_dist = DBL_MAX;
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            double d = colony->distance_matrix[i * n + j];
            if (d < min_dist) min_dist = d;
        }
        if (min_dist < DBL_MAX) bound += min_dist;
    }
    return bound / 2.0; /* each edge counted twice */
}

void aco_destroy(ACO_Colony *colony) {
    if (!colony) return;
    for (int i = 0; i < colony->num_cities; i++) {
        free(colony->pheromone[i]);
        free(colony->heuristic[i]);
    }
    free(colony->pheromone);
    free(colony->heuristic);
    free(colony->distance_matrix);
    for (int k = 0; k < colony->num_ants; k++) {
        free(colony->ants[k].tour);
        free(colony->ants[k].visited);
    }
    free(colony->ants);
    free(colony->best_tour);
    free(colony);
}

/* ===================================================================
 * PART III: Flocking �� Reynolds Boids (1987)
 *
 * Three steering behaviors:
 *   1. Cohesion: steer toward average position of local neighbors
 *   2. Separation: steer away from neighbors that are too close
 *   3. Alignment: steer toward average heading of local neighbors
 *
 * Together these produce emergent flocking behavior from simple
 * local rules �� a hallmark of complex adaptive systems.
 * =================================================================== */

Flock* flock_create(int n, int dim, double perception_radius,
                     double cohesion, double separation, double alignment,
                     double max_speed) {
    Flock *f = (Flock*)calloc(1, sizeof(Flock));
    if (!f) return NULL;

    f->num_boids = n;
    f->dim = dim;
    f->perception_radius = perception_radius;
    f->separation_radius = perception_radius * 0.3;
    f->cohesion_weight = cohesion;
    f->separation_weight = separation;
    f->alignment_weight = alignment;
    f->max_speed = max_speed;
    f->max_force = 0.5;
    f->boundary_margin = 2.0;

    f->boids = (Boid*)calloc(n, sizeof(Boid));
    for (int i = 0; i < n; i++) {
        f->boids[i].id = i;
        f->boids[i].dim = dim;
        f->boids[i].position = (double*)calloc(dim, sizeof(double));
        f->boids[i].velocity = (double*)calloc(dim, sizeof(double));
    }

    f->bound_min = (double*)calloc(dim, sizeof(double));
    f->bound_max = (double*)calloc(dim, sizeof(double));

    return f;
}

void flock_random_init(Flock *f, const double *min_bounds,
                        const double *max_bounds) {
    if (!f) return;

    for (int d = 0; d < f->dim; d++) {
        f->bound_min[d] = min_bounds ? min_bounds[d] : -10.0;
        f->bound_max[d] = max_bounds ? max_bounds[d] : 10.0;
    }

    for (int i = 0; i < f->num_boids; i++) {
        for (int d = 0; d < f->dim; d++) {
            f->boids[i].position[d] = swarm_uniform(f->bound_min[d],
                                                      f->bound_max[d]);
            f->boids[i].velocity[d] = swarm_uniform(-1.0, 1.0);
        }
    }
}

void flock_step(Flock *f, double dt) {
    if (!f) return;

    int n = f->num_boids;
    int dim = f->dim;

    for (int i = 0; i < n; i++) {
        Boid *b = &f->boids[i];

        /* Compute steering forces */
        double cohesion_force[4] = {0};
        double separation_force[4] = {0};
        double alignment_force[4] = {0};
        int cohesion_count = 0;
        int separation_count = 0;
        int alignment_count = 0;

        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            Boid *other = &f->boids[j];

            double dist = swarm_euclidean_distance(b->position,
                                                    other->position, dim);

            /* Cohesion: all neighbors within perception radius */
            if (dist < f->perception_radius && dist > 0.0) {
                for (int d = 0; d < dim; d++) {
                    cohesion_force[d] += other->position[d];
                }
                cohesion_count++;

                /* Alignment: match heading of neighbors */
                for (int d = 0; d < dim; d++) {
                    alignment_force[d] += other->velocity[d];
                }
                alignment_count++;
            }

            /* Separation: only very close neighbors */
            if (dist < f->separation_radius && dist > 1e-8) {
                for (int d = 0; d < dim; d++) {
                    double diff = b->position[d] - other->position[d];
                    separation_force[d] += diff / (dist * dist + 1e-8);
                }
                separation_count++;
            }
        }

        /* Accumulate total steering force with weights */
        double steering[4] = {0};

        /* Cohesion: steer toward average position of neighbors */
        if (cohesion_count > 0) {
            for (int d = 0; d < dim; d++) {
                cohesion_force[d] = cohesion_force[d] / (double)cohesion_count
                                    - b->position[d];
                steering[d] += f->cohesion_weight * cohesion_force[d];
            }
        }

        /* Separation: steer away from too-close neighbors */
        if (separation_count > 0) {
            for (int d = 0; d < dim; d++) {
                steering[d] += f->separation_weight * separation_force[d];
            }
        }

        /* Alignment: steer toward average velocity of neighbors */
        if (alignment_count > 0) {
            for (int d = 0; d < dim; d++) {
                alignment_force[d] = alignment_force[d] / (double)alignment_count
                                     - b->velocity[d];
                steering[d] += f->alignment_weight * alignment_force[d];
            }
        }

        /* Boundary avoidance: steer away from edges */
        for (int d = 0; d < dim; d++) {
            double margin = f->boundary_margin;
            if (b->position[d] < f->bound_min[d] + margin) {
                steering[d] += f->max_force * 2.0;
            } else if (b->position[d] > f->bound_max[d] - margin) {
                steering[d] -= f->max_force * 2.0;
            }
        }

        /* Clamp steering force */
        double force_mag = 0.0;
        for (int d = 0; d < dim; d++) force_mag += steering[d] * steering[d];
        force_mag = sqrt(force_mag);
        if (force_mag > f->max_force && force_mag > 1e-8) {
            for (int d = 0; d < dim; d++) {
                steering[d] = steering[d] / force_mag * f->max_force;
            }
        }

        /* Euler integration */
        for (int d = 0; d < dim; d++) {
            b->velocity[d] += steering[d] * dt;
            /* Clamp speed */
            double speed = fabs(b->velocity[d]);
            if (speed > f->max_speed) {
                b->velocity[d] = b->velocity[d] / speed * f->max_speed;
            }
            b->position[d] += b->velocity[d] * dt;

            /* Keep within bounds */
            b->position[d] = swarm_clamp(b->position[d],
                                          f->bound_min[d], f->bound_max[d]);
        }
    }
}

void flock_center_of_mass(const Flock *f, double *center_out) {
    if (!f || !center_out) return;
    int n = f->num_boids;
    int dim = f->dim;

    memset(center_out, 0, dim * sizeof(double));
    if (n <= 0) return;

    for (int i = 0; i < n; i++) {
        for (int d = 0; d < dim; d++) {
            center_out[d] += f->boids[i].position[d];
        }
    }
    for (int d = 0; d < dim; d++) {
        center_out[d] /= (double)n;
    }
}

double flock_polarization(const Flock *f) {
    /* Order parameter: |sum v_i| / sum |v_i|
     * 1.0 = perfect alignment, 0.0 = random directions */
    if (!f || f->num_boids <= 0) return 0.0;

    int n = f->num_boids;
    int dim = f->dim;

    double sum_v[4] = {0};
    double sum_abs = 0.0;

    for (int i = 0; i < n; i++) {
        double speed = 0.0;
        for (int d = 0; d < dim; d++) {
            double v = f->boids[i].velocity[d];
            sum_v[d] += v;
            speed += v * v;
        }
        sum_abs += sqrt(speed);
    }

    double net_mag = 0.0;
    for (int d = 0; d < dim; d++) {
        net_mag += sum_v[d] * sum_v[d];
    }
    net_mag = sqrt(net_mag);

    if (sum_abs < 1e-12) return 1.0;
    return net_mag / sum_abs;
}

void flock_run(Flock *f, int n_steps, double dt) {
    for (int t = 0; t < n_steps; t++) {
        flock_step(f, dt);
    }
}

void flock_destroy(Flock *f) {
    if (!f) return;
    for (int i = 0; i < f->num_boids; i++) {
        free(f->boids[i].position);
        free(f->boids[i].velocity);
    }
    free(f->boids);
    free(f->bound_min);
    free(f->bound_max);
    free(f);
}

/* ===================================================================
 * PART IV: Firefly Algorithm (Yang, 2010)
 *
 * Inspired by the flashing behavior of fireflies:
 *   1. All fireflies are unisex �� any can attract any other
 *   2. Attractiveness is proportional to brightness (fitness)
 *   3. Brighter fireflies attract dimmer ones
 *   4. Attraction decreases with distance (light absorption gamma)
 *   5. If no brighter firefly nearby, move randomly
 *
 * Movement rule: x_i = x_i + beta0 * exp(-gamma * r^2) * (x_j - x_i) + alpha * rand
 * =================================================================== */

FireflySwarm* firefly_create(int n, int dim, const double *lb, const double *ub,
                              double alpha, double beta0, double gamma_abs) {
    FireflySwarm *fs = (FireflySwarm*)calloc(1, sizeof(FireflySwarm));
    if (!fs) return NULL;

    fs->num_fireflies = n;
    fs->dim = dim;
    fs->alpha = alpha;
    fs->beta0 = beta0;
    fs->gamma = gamma_abs;
    fs->iterations = 0;

    fs->lower_bound = (double*)calloc(dim, sizeof(double));
    fs->upper_bound = (double*)calloc(dim, sizeof(double));
    if (lb) memcpy(fs->lower_bound, lb, dim * sizeof(double));
    if (ub) memcpy(fs->upper_bound, ub, dim * sizeof(double));

    fs->fireflies = (Firefly*)calloc(n, sizeof(Firefly));
    for (int i = 0; i < n; i++) {
        fs->fireflies[i].id = i;
        fs->fireflies[i].dim = dim;
        fs->fireflies[i].position = (double*)calloc(dim, sizeof(double));
        fs->fireflies[i].brightness = DBL_MAX;
    }

    return fs;
}

void firefly_random_init(FireflySwarm *fs) {
    if (!fs) return;
    for (int i = 0; i < fs->num_fireflies; i++) {
        for (int d = 0; d < fs->dim; d++) {
            fs->fireflies[i].position[d] = swarm_uniform(fs->lower_bound[d],
                                                           fs->upper_bound[d]);
        }
    }
}

void firefly_evaluate(FireflySwarm *fs,
                       double (*obj)(const double *x, int dim, void *ctx),
                       void *ctx) {
    if (!fs || !obj) return;
    for (int i = 0; i < fs->num_fireflies; i++) {
        fs->fireflies[i].brightness = obj(fs->fireflies[i].position,
                                           fs->dim, ctx);
    }
}

void firefly_move(FireflySwarm *fs) {
    if (!fs) return;

    int n = fs->num_fireflies;
    int dim = fs->dim;

    for (int i = 0; i < n; i++) {
        Firefly *fi = &fs->fireflies[i];

        for (int j = 0; j < n; j++) {
            Firefly *fj = &fs->fireflies[j];

            /* Firefly i moves toward firefly j if j is brighter */
            if (fj->brightness < fi->brightness) {
                double dist = swarm_euclidean_distance(fi->position,
                                                        fj->position, dim);
                double r2 = dist * dist;

                /* Attractiveness: beta = beta0 * exp(-gamma * r^2) */
                double beta = fs->beta0 * exp(-fs->gamma * r2);

                /* Move toward brighter firefly with random perturbation */
                for (int d = 0; d < dim; d++) {
                    double attraction = beta * (fj->position[d] - fi->position[d]);
                    double randomization = fs->alpha * swarm_gaussian(0.0, 1.0);
                    fi->position[d] += attraction + randomization;

                    /* Clamp to bounds */
                    fi->position[d] = swarm_clamp(fi->position[d],
                                                   fs->lower_bound[d],
                                                   fs->upper_bound[d]);
                }
            }
        }
    }

    fs->iterations++;
}

int firefly_run(FireflySwarm *fs, int n_iterations,
                 double (*obj)(const double *x, int dim, void *ctx),
                 void *ctx, bool verbose) {
    if (!fs || !obj) return 0;

    for (int iter = 0; iter < n_iterations; iter++) {
        firefly_evaluate(fs, obj, ctx);
        firefly_move(fs);

        if (verbose && iter % 10 == 0) {
            const Firefly *best = firefly_get_best(fs);
            printf("Firefly iter %d: best=%f\n", iter,
                   best ? best->brightness : DBL_MAX);
        }
    }

    /* Final evaluation */
    firefly_evaluate(fs, obj, ctx);
    return fs->iterations;
}

const Firefly* firefly_get_best(const FireflySwarm *fs) {
    if (!fs || fs->num_fireflies <= 0) return NULL;

    const Firefly *best = &fs->fireflies[0];
    for (int i = 1; i < fs->num_fireflies; i++) {
        if (fs->fireflies[i].brightness < best->brightness) {
            best = &fs->fireflies[i];
        }
    }
    return best;
}

void firefly_destroy(FireflySwarm *fs) {
    if (!fs) return;
    for (int i = 0; i < fs->num_fireflies; i++) {
        free(fs->fireflies[i].position);
    }
    free(fs->fireflies);
    free(fs->lower_bound);
    free(fs->upper_bound);
    free(fs);
}
