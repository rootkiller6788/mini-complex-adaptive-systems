#include "swarm_aco.h"
#include "swarm_core.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Ant Colony Optimization — Core Implementation
 *
 * References:
 *   Dorigo, Maniezzo & Colorni (1991) — Original Ant System
 *   Dorigo & Gambardella (1997)   — Ant Colony System (ACS)
 *   Stützle & Hoos (2000)          — MAX-MIN Ant System (MMAS)
 *   Dorigo & Stützle (2004)        — ACO book
 * ============================================================================ */

/* ── Pheromone Matrix ── */
PheromoneMatrix* swarm_aco_pheromone_create(int n_cities, double tau0) {
    if (n_cities <= 0) return NULL;
    PheromoneMatrix* p = (PheromoneMatrix*)calloc(1, sizeof(PheromoneMatrix));
    if (!p) return NULL;
    p->tau = swarm_matrix_alloc(n_cities, n_cities);
    p->heuristic = swarm_matrix_alloc(n_cities, n_cities);
    if (!p->tau || !p->heuristic) {
        swarm_matrix_free(p->tau, n_cities);
        swarm_matrix_free(p->heuristic, n_cities);
        free(p); return NULL;
    }
    p->rows = p->cols = n_cities;
    p->evaporation_rate = SWARM_ACO_EVAP_DEFAULT;
    p->alpha = SWARM_ACO_ALPHA_DEFAULT;
    p->beta = SWARM_ACO_BETA_DEFAULT;
    p->deposit_amount = 1.0;
    p->tau_min = 0.001;
    p->tau_max = 1.0;
    swarm_aco_pheromone_reset(p, tau0);
    return p;
}

void swarm_aco_pheromone_free(PheromoneMatrix* phero) {
    if (!phero) return;
    swarm_matrix_free(phero->tau, phero->rows);
    swarm_matrix_free(phero->heuristic, phero->rows);
    free(phero);
}

void swarm_aco_pheromone_reset(PheromoneMatrix* phero, double tau0) {
    if (!phero) return;
    for (int i = 0; i < phero->rows; i++)
        for (int j = 0; j < phero->cols; j++)
            phero->tau[i][j] = (i != j) ? tau0 : 0.0;
}

/* ── ACO Problem ── */
ACOProblem* swarm_aco_problem_from_coords(const double* x, const double* y,
                                           int n_cities, bool symmetric) {
    if (!x || !y || n_cities <= 0) return NULL;
    ACOProblem* problem = (ACOProblem*)calloc(1, sizeof(ACOProblem));
    if (!problem) return NULL;
    problem->n_cities = n_cities;
    problem->symmetric = symmetric;
    
    problem->distances = swarm_matrix_alloc(n_cities, n_cities);
    problem->heuristics = swarm_matrix_alloc(n_cities, n_cities);
    if (!problem->distances || !problem->heuristics) {
        swarm_aco_problem_free(problem); return NULL;
    }
    
    for (int i = 0; i < n_cities; i++) {
        for (int j = 0; j < n_cities; j++) {
            if (i == j) {
                problem->distances[i][j] = 0.0;
                problem->heuristics[i][j] = 0.0;
            } else {
                double dx = x[i] - x[j], dy = y[i] - y[j];
                double d = sqrt(dx * dx + dy * dy);
                problem->distances[i][j] = d;
                problem->heuristics[i][j] = (d > SWARM_EPSILON) ? 1.0 / d : 1e10;
            }
        }
    }
    return problem;
}

ACOProblem* swarm_aco_problem_from_distances(const double** distances,
                                              int n_cities, bool symmetric) {
    if (!distances || n_cities <= 0) return NULL;
    ACOProblem* problem = (ACOProblem*)calloc(1, sizeof(ACOProblem));
    if (!problem) return NULL;
    problem->n_cities = n_cities;
    problem->symmetric = symmetric;
    problem->distances = swarm_matrix_alloc(n_cities, n_cities);
    problem->heuristics = swarm_matrix_alloc(n_cities, n_cities);
    if (!problem->distances || !problem->heuristics) {
        swarm_aco_problem_free(problem); return NULL;
    }
    for (int i = 0; i < n_cities; i++) {
        for (int j = 0; j < n_cities; j++) {
            problem->distances[i][j] = distances[i][j];
            problem->heuristics[i][j] = (distances[i][j] > SWARM_EPSILON) ? 1.0 / distances[i][j] : 1e10;
        }
    }
    return problem;
}

void swarm_aco_problem_free(ACOProblem* problem) {
    if (!problem) return;
    swarm_matrix_free(problem->distances, problem->n_cities);
    swarm_matrix_free(problem->heuristics, problem->n_cities);
    free(problem);
}

/* ── Ant Tour ── */
AntTour* swarm_aco_tour_create(int n_cities) {
    if (n_cities <= 0) return NULL;
    AntTour* tour = (AntTour*)calloc(1, sizeof(AntTour));
    if (!tour) return NULL;
    tour->tour = swarm_alloc_int_vector(n_cities);
    if (!tour->tour) { free(tour); return NULL; }
    tour->n_cities = n_cities;
    tour->feasible = true;
    return tour;
}

void swarm_aco_tour_free(AntTour* tour) {
    if (!tour) return;
    swarm_free_int_vector(tour->tour);
    free(tour);
}

void swarm_aco_tour_copy(const AntTour* src, AntTour* dst) {
    if (!src || !dst || src->n_cities != dst->n_cities) return;
    memcpy(dst->tour, src->tour, (size_t)src->n_cities * sizeof(int));
    dst->tour_length = src->tour_length;
    dst->feasible = src->feasible;
}

void swarm_aco_tour_print(const AntTour* tour) {
    if (!tour) { printf("(null)\n"); return; }
    printf("Tour [%d cities, length=%.2f, feasible=%d]: ", 
           tour->n_cities, tour->tour_length, tour->feasible);
    for (int i = 0; i < tour->n_cities && i < 20; i++)
        printf("%d ", tour->tour[i]);
    if (tour->n_cities > 20) printf("...");
    printf("\n");
}

double swarm_aco_tour_length(const AntTour* tour, const ACOProblem* problem) {
    if (!tour || !problem || !tour->feasible) return 1e100;
    double length = 0.0;
    for (int i = 0; i < tour->n_cities; i++) {
        int from = tour->tour[i], to = tour->tour[(i + 1) % tour->n_cities];
        if (from < 0 || from >= problem->n_cities || to < 0 || to >= problem->n_cities)
            return 1e100;
        length += problem->distances[from][to];
    }
    return length;
}

/* ── Solution Construction: Ant System ── */
void swarm_aco_construct_solution_AS(AntTour* tour, PheromoneMatrix* phero,
           const ACOProblem* problem, const ACOConfig* config) {
    if (!tour || !phero || !problem || !config) return;
    int n = problem->n_cities;
    bool* visited = (bool*)calloc((size_t)n, sizeof(bool));
    if (!visited) { tour->feasible = false; return; }
    
    /* Start at random city */
    int start = swarm_rng_int(n);
    tour->tour[0] = start;
    visited[start] = true;
    
    for (int step = 1; step < n; step++) {
        int current = tour->tour[step - 1];
        /* Compute probabilities for unvisited cities */
        double total = 0.0;
        double probs[SWARM_MAX_CITIES];
        for (int j = 0; j < n; j++) {
            if (visited[j]) { probs[j] = 0.0; continue; }
            double tau_ij = pow(phero->tau[current][j], config->alpha);
            double eta_ij = pow(problem->heuristics[current][j], config->beta);
            probs[j] = tau_ij * eta_ij;
            total += probs[j];
        }
        /* Select next city via roulette wheel */
        if (total < SWARM_EPSILON) {
            /* Fallback: random unvisited */
            for (int j = 0; j < n; j++) if (!visited[j]) { tour->tour[step] = j; visited[j] = true; break; }
        } else {
            double r = swarm_rng_uniform() * total, cum = 0.0;
            for (int j = 0; j < n; j++) {
                cum += probs[j];
                if (r <= cum && !visited[j]) { tour->tour[step] = j; visited[j] = true; break; }
            }
        }
    }
    tour->tour_length = swarm_aco_tour_length(tour, problem);
    tour->feasible = true;
    free(visited);
}

/* ── Solution Construction: ACS (pseudo-random proportional rule) ── */
void swarm_aco_construct_solution_ACS(AntTour* tour, PheromoneMatrix* phero,
           const ACOProblem* problem, const ACOConfig* config) {
    if (!tour || !phero || !problem || !config) return;
    int n = problem->n_cities;
    bool* visited = (bool*)calloc((size_t)n, sizeof(bool));
    if (!visited) { tour->feasible = false; return; }
    
    int start = swarm_rng_int(n);
    tour->tour[0] = start;
    visited[start] = true;
    
    for (int step = 1; step < n; step++) {
        int current = tour->tour[step - 1];
        double q = swarm_rng_uniform();
        int next;
        
        if (q <= config->q0) {
            /* Exploitation: select best according to τ^α·η^β */
            double best_val = -1.0;
            next = -1;
            for (int j = 0; j < n; j++) {
                if (visited[j]) continue;
                double val = pow(phero->tau[current][j], config->alpha) *
                             pow(problem->heuristics[current][j], config->beta);
                if (val > best_val) { best_val = val; next = j; }
            }
        } else {
            /* Exploration: proportional selection (same as AS) */
            /* Reuse AS construction logic */
            swarm_aco_construct_solution_AS(tour, phero, problem, config);
            free(visited);
            return; /* AS filled in the rest */
        }
        
        if (next < 0) {
            /* Fallback */
            for (int j = 0; j < n; j++) if (!visited[j]) { next = j; break; }
        }
        tour->tour[step] = next;
        visited[next] = true;
        
        /* Local pheromone update (ACS) */
        if (config->local_rho > 0.0)
            swarm_aco_local_pheromone_update_ACS(phero, current, next,
                                                  config->local_rho, config->tau0);
    }
    tour->tour_length = swarm_aco_tour_length(tour, problem);
    tour->feasible = true;
    free(visited);
}

/* ── Global Pheromone Update: Ant System ── */
void swarm_aco_global_pheromone_update_AS(PheromoneMatrix* phero,
           AntTour** tours, int n_ants, const ACOConfig* config) {
    if (!phero || !tours) return;
    int n = phero->rows;
    /* Evaporation */
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (i != j) phero->tau[i][j] *= (1.0 - config->rho);
    
    /* Deposit */
    for (int k = 0; k < n_ants; k++) {
        if (!tours[k] || !tours[k]->feasible) continue;
        double deposit = config->Q / (tours[k]->tour_length + SWARM_EPSILON);
        for (int i = 0; i < tours[k]->n_cities; i++) {
            int from = tours[k]->tour[i], to = tours[k]->tour[(i + 1) % tours[k]->n_cities];
            if (from >= 0 && from < n && to >= 0 && to < n)
                phero->tau[from][to] += deposit;
        }
    }
}

/* ── Global Pheromone Update: MMAS ── */
void swarm_aco_global_pheromone_update_MMAS(PheromoneMatrix* phero,
           const AntTour* best_tour, const ACOConfig* config) {
    if (!phero || !best_tour || !best_tour->feasible) return;
    int n = phero->rows;
    /* Evaporation */
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (i != j) phero->tau[i][j] *= (1.0 - config->rho);
    
    /* Only best tour deposits */
    double deposit = config->Q / (best_tour->tour_length + SWARM_EPSILON);
    for (int i = 0; i < best_tour->n_cities; i++) {
        int from = best_tour->tour[i], to = best_tour->tour[(i + 1) % best_tour->n_cities];
        if (from >= 0 && from < n && to >= 0 && to < n)
            phero->tau[from][to] += deposit;
    }
    
    /* Enforce bounds */
    swarm_aco_enforce_pheromone_limits(phero);
}

/* ── Local Pheromone Update (ACS) ── */
void swarm_aco_local_pheromone_update_ACS(PheromoneMatrix* phero,
           int city_i, int city_j, double local_rho, double tau0) {
    if (!phero || city_i < 0 || city_j < 0) return;
    phero->tau[city_i][city_j] = (1.0 - local_rho) * phero->tau[city_i][city_j] + local_rho * tau0;
}

/* ── Pheromone Bounds (MMAS) ── */
void swarm_aco_update_pheromone_bounds(PheromoneMatrix* phero,
           double best_tour_length, const ACOConfig* config, double rho,
           int n_cities) {
    if (!phero || !config) return;
    (void)rho; (void)n_cities;
    phero->tau_max = 1.0 / (best_tour_length + SWARM_EPSILON);
    /* τ_min = τ_max / (2n) heuristic */
    phero->tau_min = phero->tau_max / (2.0 * (double)phero->rows);
    if (phero->tau_min < SWARM_EPSILON) phero->tau_min = SWARM_EPSILON;
}

void swarm_aco_enforce_pheromone_limits(PheromoneMatrix* phero) {
    if (!phero) return;
    for (int i = 0; i < phero->rows; i++) {
        for (int j = 0; j < phero->cols; j++) {
            if (i == j) continue;
            if (phero->tau[i][j] < phero->tau_min) phero->tau[i][j] = phero->tau_min;
            if (phero->tau[i][j] > phero->tau_max) phero->tau[i][j] = phero->tau_max;
        }
    }
}

/* ── Nearest Neighbor Tour ── */
void swarm_aco_nearest_neighbor_tour(AntTour* tour, const ACOProblem* problem,
                                      int start_city) {
    if (!tour || !problem) return;
    int n = problem->n_cities;
    bool* visited = (bool*)calloc((size_t)n, sizeof(bool));
    if (!visited) { tour->feasible = false; return; }
    
    tour->tour[0] = start_city;
    visited[start_city] = true;
    
    for (int step = 1; step < n; step++) {
        int current = tour->tour[step - 1];
        int nearest = -1;
        double min_dist = 1e100;
        for (int j = 0; j < n; j++) {
            if (visited[j]) continue;
            if (problem->distances[current][j] < min_dist) {
                min_dist = problem->distances[current][j];
                nearest = j;
            }
        }
        if (nearest >= 0) {
            tour->tour[step] = nearest;
            visited[nearest] = true;
        }
    }
    tour->tour_length = swarm_aco_tour_length(tour, problem);
    tour->feasible = true;
    free(visited);
}

/* ── 2-opt Local Search Improvement ── */
bool swarm_aco_2opt_improve(AntTour* tour, const ACOProblem* problem) {
    if (!tour || !problem || !tour->feasible) return false;
    int n = tour->n_cities;
    bool improved = false;
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 2; j < n; j++) {
            if (j == n - 1 && i == 0) continue; /* Don't break complete tour */
            int a = tour->tour[i], b = tour->tour[i + 1];
            int c = tour->tour[j], d = tour->tour[(j + 1) % n];
            double old_len = problem->distances[a][b] + problem->distances[c][d];
            double new_len = problem->distances[a][c] + problem->distances[b][d];
            if (new_len < old_len - SWARM_EPSILON) {
                /* Reverse segment i+1 .. j */
                int left = i + 1, right = j;
                while (left < right) {
                    int tmp = tour->tour[left];
                    tour->tour[left] = tour->tour[right];
                    tour->tour[right] = tmp;
                    left++; right--;
                }
                improved = true;
            }
        }
    }
    if (improved) tour->tour_length = swarm_aco_tour_length(tour, problem);
    return improved;
}

/* ── 3-opt Local Search (simplified) ── */
bool swarm_aco_3opt_improve(AntTour* tour, const ACOProblem* problem) {
    if (!tour || !problem || !tour->feasible) return false;
    /* Apply 2-opt repeatedly as a simpler alternative */
    bool improved = false, local = true;
    int tries = 0;
    while (local && tries < 10) {
        local = swarm_aco_2opt_improve(tour, problem);
        if (local) improved = true;
        tries++;
    }
    return improved;
}

/* ── Or-opt Improvement ── */
bool swarm_aco_oropt_improve(AntTour* tour, const ACOProblem* problem, int max_segment) {
    if (!tour || !problem || !tour->feasible) return false;
    int n = tour->n_cities;
    bool improved = false;
    for (int seg = 1; seg <= max_segment && seg < n; seg++) {
        for (int i = 0; i < n - seg; i++) {
            for (int j = 0; j < n; j++) {
                if (j >= i && j <= i + seg) continue;
                /* Compute delta cost */
                int a = tour->tour[(i > 0) ? i - 1 : n - 1], b = tour->tour[i];
                int c = tour->tour[i + seg - 1], d = tour->tour[(i + seg) % n];
                int before_j = tour->tour[(j > 0) ? j - 1 : n - 1], at_j = tour->tour[j % n];
                double old_edges = problem->distances[a][b] + problem->distances[c][d] + problem->distances[before_j][at_j];
                double new_edges = problem->distances[a][d] + problem->distances[before_j][b] + problem->distances[c][at_j];
                if (new_edges < old_edges - SWARM_EPSILON) {
                    /* Move segment [i, i+seg-1] to before j */
                    /* Simplified implementation via 2-opt */
                    improved |= swarm_aco_2opt_improve(tour, problem);
                }
            }
        }
    }
    return improved;
}

/* ── Pheromone Entropy (convergence measure) ── */
double swarm_aco_pheromone_entropy(const PheromoneMatrix* phero) {
    if (!phero) return 0.0;
    double entropy = 0.0;
    int n = phero->rows, count = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            double p = phero->tau[i][j] + SWARM_EPSILON;
            entropy -= p * log(p);
            count++;
        }
    }
    return count > 0 ? entropy / (double)count : 0.0;
}

double swarm_aco_convergence_factor(const PheromoneMatrix* phero,
                                     const ACOConfig* config) {
    if (!phero || !config) return 0.0;
    /* Fraction of τ values close to τ_max or τ_min */
    int n = phero->rows, extreme = 0, total = 0;
    double mid = (phero->tau_min + phero->tau_max) / 2.0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            if (fabs(phero->tau[i][j] - phero->tau_min) < 0.01 * mid ||
                fabs(phero->tau[i][j] - phero->tau_max) < 0.01 * mid) extreme++;
            total++;
        }
    }
    return total > 0 ? (double)extreme / (double)total : 0.0;
}

void swarm_aco_adapt_pheromone_bounds(PheromoneMatrix* phero,
           double best_tour_length, ACOConfig* config, int iteration) {
    if (!phero || !config) return;
    (void)iteration;
    swarm_aco_update_pheromone_bounds(phero, best_tour_length, config, 
                                       config->rho, phero->rows);
}

/* ── Full ACO Run for TSP ── */
SwarmResult* swarm_aco_run_tsp(const double* x, const double* y,
                                int n_cities, ACOConfig* config) {
    if (!x || !y || n_cities <= 1 || !config) return NULL;
    
    ACOProblem* problem = swarm_aco_problem_from_coords(x, y, n_cities, true);
    if (!problem) return NULL;
    
    /* Initialize pheromone */
    double tau0 = 1.0 / (double)(n_cities * n_cities);
    PheromoneMatrix* phero = swarm_aco_pheromone_create(n_cities, tau0);
    if (!phero) { swarm_aco_problem_free(problem); return NULL; }
    phero->alpha = config->alpha;
    phero->beta = config->beta;
    phero->evaporation_rate = config->rho;
    
    /* Create ants */
    int n_ants = config->n_ants;
    AntTour** tours = (AntTour**)calloc((size_t)n_ants, sizeof(AntTour*));
    AntTour* best_tour = swarm_aco_tour_create(n_cities);
    if (!tours || !best_tour) {
        swarm_aco_pheromone_free(phero); swarm_aco_problem_free(problem);
        free(tours); swarm_aco_tour_free(best_tour); return NULL;
    }
    for (int i = 0; i < n_ants; i++) {
        tours[i] = swarm_aco_tour_create(n_cities);
        if (!tours[i]) {
            for (int j = 0; j < i; j++) swarm_aco_tour_free(tours[j]);
            free(tours); swarm_aco_tour_free(best_tour);
            swarm_aco_pheromone_free(phero); swarm_aco_problem_free(problem); return NULL;
        }
    }
    best_tour->tour_length = 1e100;
    
    SwarmResult* result = (SwarmResult*)calloc(1, sizeof(SwarmResult));
    if (!result) { /* cleanup */ return NULL; }
    result->best_position = swarm_alloc_vector(n_cities); /* Store tour order */
    result->history = swarm_alloc_vector(config->n_ants > 0 ? 1000 : 0);
    if (!result->best_position || !result->history) {
        free(result); /* partial cleanup */ return NULL;
    }
    
    int max_iter = config->n_ants > 0 ? 1000 : 0; /* Default */
    
    for (int iter = 0; iter < max_iter; iter++) {
        /* Construct solutions */
        for (int a = 0; a < n_ants; a++) {
            if (config->strategy == ACO_ACS)
                swarm_aco_construct_solution_ACS(tours[a], phero, problem, config);
            else
                swarm_aco_construct_solution_AS(tours[a], phero, problem, config);
            
            /* Update best tour */
            if (tours[a]->feasible && tours[a]->tour_length < best_tour->tour_length)
                swarm_aco_tour_copy(tours[a], best_tour);
        }
        
        /* Apply local search if daemon enabled */
        if (config->use_daemon) {
            for (int a = 0; a < n_ants; a++)
                swarm_aco_2opt_improve(tours[a], problem);
            if (best_tour->tour_length > 0)
                swarm_aco_2opt_improve(best_tour, problem);
        }
        
        /* Pheromone update */
        if (config->strategy == ACO_MMAS)
            swarm_aco_global_pheromone_update_MMAS(phero, best_tour, config);
        else
            swarm_aco_global_pheromone_update_AS(phero, tours, n_ants, config);
        
        /* Record history */
        result->history[iter] = best_tour->tour_length;
        
        if (iter % 50 == 0 && iter > 0)
            printf("ACO iter %d: best=%.4f\n", iter, best_tour->tour_length);
    }
    
    result->best_fitness = best_tour->tour_length;
    for (int i = 0; i < n_cities; i++)
        result->best_position[i] = (double)best_tour->tour[i];
    result->iterations_run = max_iter;
    result->converged = true;
    
    /* Cleanup */
    for (int i = 0; i < n_ants; i++) swarm_aco_tour_free(tours[i]);
    free(tours);
    swarm_aco_tour_free(best_tour);
    swarm_aco_pheromone_free(phero);
    swarm_aco_problem_free(problem);
    
    return result;
}
