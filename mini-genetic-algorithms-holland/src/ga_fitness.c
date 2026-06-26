/**
 * ga_fitness.c — Fitness Functions and Landscape Analysis
 *
 * Implements canonical fitness functions for testing GAs and landscape
 * analysis tools for understanding problem difficulty. Fitness landscapes
 * are the mathematical structure linking optimization theory to GA dynamics.
 *
 * Reference: Kauffman "The Origins of Order" (1993)
 *            Jones & Forrest "Fitness Distance Correlation" (1995)
 *            Deb "Multi-Objective Optimization using EAs" (2001)
 *            Stadler "Fitness Landscapes" (2002)
 *
 * Knowledge: L1 Definitions, L3 Math Structures, L6 Canonical Problems
 */

#include "ga_fitness.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

/* ====================================================================
 * Local PRNG
 * ==================================================================== */

static uint64_t _fit_rand_state = 314159265ULL;

static uint64_t _fit_rand64(void) {
    _fit_rand_state ^= _fit_rand_state >> 12;
    _fit_rand_state ^= _fit_rand_state << 25;
    _fit_rand_state ^= _fit_rand_state >> 27;
    return _fit_rand_state * 0x2545F4914F6CDD1DULL;
}

static double _fit_rand_double(void) {
    return (double)(_fit_rand64() >> 11) / (double)(1ULL << 53);
}

/* ====================================================================
 * L1/L6: Binary-encoded Fitness Functions
 * ==================================================================== */

double ga_fitness_onemax(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < c->num_genes; i++) {
        if (c->alleles[i].binary_val) sum += 1.0;
    }
    return sum;
}

double ga_fitness_needle(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;

    /* Compare against target provided in user_data or default all-ones */
    int *target = (int*)user_data;

    for (int i = 0; i < c->num_genes; i++) {
        int expected = (target) ? target[i] : 1;
        if (c->alleles[i].binary_val != (expected != 0)) return 0.0;
    }
    return 1.0;
}

double ga_fitness_deceptive_trap(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;

    /* Deceptive trap: partition chromosome into k-bit blocks.
     * For each block, count ones u.
     * trap(u) = k if u==k, else (k-1-u).
     * Default k=4. */
    int k = 4; /* trap size */
    double total = 0.0;
    int num_blocks = c->num_genes / k;

    for (int b = 0; b < num_blocks; b++) {
        int u = 0;
        for (int i = 0; i < k; i++) {
            int idx = b * k + i;
            if (idx < c->num_genes && c->alleles[idx].binary_val) u++;
        }
        if (u == k) {
            total += (double)k;
        } else {
            total += (double)(k - 1 - u);
        }
    }
    return total;
}

double ga_fitness_royal_road(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;

    /* Royal Road: fitness = sum of bonuses for each fully-1 block.
     * Block sizes: 8, 16, 32 (building block hierarchy).
     */
    double total = 0.0;
    int pos = 0;

    /* Level 1 blocks (size 8) */
    while (pos + 8 <= c->num_genes) {
        bool all_one = true;
        for (int i = 0; i < 8; i++) {
            if (!c->alleles[pos + i].binary_val) { all_one = false; break; }
        }
        if (all_one) total += 8.0;
        pos += 8;
    }

    /* Level 2 blocks (size 16) */
    pos = 0;
    while (pos + 16 <= c->num_genes) {
        bool all_one = true;
        for (int i = 0; i < 16; i++) {
            if (!c->alleles[pos + i].binary_val) { all_one = false; break; }
        }
        if (all_one) total += 16.0;
        pos += 16;
    }

    /* Level 3 blocks (size 32) */
    pos = 0;
    while (pos + 32 <= c->num_genes) {
        bool all_one = true;
        for (int i = 0; i < 32; i++) {
            if (!c->alleles[pos + i].binary_val) { all_one = false; break; }
        }
        if (all_one) total += 32.0;
        pos += 32;
    }

    return total;
}

/* NK Landscape sub-function lookup table (generated pseudo-randomly) */
typedef struct {
    int N, K;
    double **subfunctions; /* [gene][neighbor_config] → contribution */
} NKData;

double ga_fitness_nk_landscape(const Chromosome *c, void *user_data) {
    if (!c) return 0.0;

    NKData *nk = (NKData*)user_data;
    int N = c->num_genes;
    int K = (nk) ? nk->K : 3;
    if (K >= N) K = N - 1;
    if (K < 0) K = 0;

    double total = 0.0;

    /* Use deterministic hash-based NK contributions so we don't need user_data */
    for (int i = 0; i < N; i++) {
        /* Build K+1 bit neighborhood key */
        uint64_t key = 0;
        for (int k = 0; k <= K; k++) {
            int neighbor = (i + k) % N;
            if (c->alleles[neighbor].binary_val)
                key |= (1ULL << (uint64_t)k);
        }
        /* Deterministic pseudo-random contribution from key */
        key = key * 0x517cc1b727220a95ULL + (uint64_t)i;
        double contrib = (double)(key % 10000) / 10000.0;
        total += contrib;
    }
    return total;
}

/* ====================================================================
 * L6: Real-valued Optimization Functions
 * ==================================================================== */

double ga_fitness_sphere(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < c->num_genes; i++) {
        double x = (c->loci[i].type == GENE_REAL)
                   ? c->alleles[i].real_val
                   : (double)c->alleles[i].integer_val;
        sum += x * x;
    }
    return -sum; /* maximize = minimize sphere */
}

double ga_fitness_rosenbrock(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;
    double sum = 0.0;
    int n = c->num_genes;
    for (int i = 0; i < n - 1; i++) {
        double xi = (c->loci[i].type == GENE_REAL)
                     ? c->alleles[i].real_val
                     : (double)c->alleles[i].integer_val;
        double xj = (c->loci[i+1].type == GENE_REAL)
                     ? c->alleles[i+1].real_val
                     : (double)c->alleles[i+1].integer_val;
        sum += 100.0 * (xj - xi * xi) * (xj - xi * xi) + (1.0 - xi) * (1.0 - xi);
    }
    return -sum; /* maximize = minimize Rosenbrock */
}

double ga_fitness_rastrigin(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c || c->num_genes <= 0) return 0.0;
    int n = c->num_genes;
    double sum = 10.0 * (double)n;
    for (int i = 0; i < n; i++) {
        double x = (c->loci[i].type == GENE_REAL)
                   ? c->alleles[i].real_val
                   : (double)c->alleles[i].integer_val;
        sum += x * x - 10.0 * cos(2.0 * M_PI * x);
    }
    return -sum; /* maximize = minimize Rastrigin */
}

double ga_fitness_ackley(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c || c->num_genes <= 0) return 0.0;
    int n = c->num_genes;
    double sum1 = 0.0, sum2 = 0.0;
    for (int i = 0; i < n; i++) {
        double x = (c->loci[i].type == GENE_REAL)
                   ? c->alleles[i].real_val
                   : (double)c->alleles[i].integer_val;
        sum1 += x * x;
        sum2 += cos(2.0 * M_PI * x);
    }
    double val = -20.0 * exp(-0.2 * sqrt(sum1 / (double)n))
                 - exp(sum2 / (double)n) + 20.0 + M_E;
    return -val; /* maximize = minimize Ackley */
}

double ga_fitness_schwefel(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;
    int n = c->num_genes;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double x = (c->loci[i].type == GENE_REAL)
                   ? c->alleles[i].real_val
                   : (double)c->alleles[i].integer_val;
        sum += x * sin(sqrt(fabs(x)));
    }
    return sum; /* maximize to approach 418.9829n */
}

double ga_fitness_griewank(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c || c->num_genes <= 0) return 0.0;
    int n = c->num_genes;
    double sum = 0.0;
    double prod = 1.0;
    for (int i = 0; i < n; i++) {
        double x = (c->loci[i].type == GENE_REAL)
                   ? c->alleles[i].real_val
                   : (double)c->alleles[i].integer_val;
        sum += x * x / 4000.0;
        prod *= cos(x / sqrt((double)(i + 1)));
    }
    return -(1.0 + sum - prod);
}

/* ====================================================================
 * L6: Combinatorial Optimization Functions
 * ==================================================================== */

static double _tsp_distance(int from, int to, double **distance_matrix,
                             int n_cities) {
    if (!distance_matrix) {
        /* Generate consistent pseudo-random distances */
        uint64_t key = ((uint64_t)from << 32) | (uint64_t)to;
        key = key * 0x517cc1b727220a95ULL + 0x9e3779b97f4a7c15ULL;
        return (double)(key % 1000) / 10.0 + 1.0; /* 0.1 to 100.0 */
    }
    if (from < 0 || to < 0 || from >= n_cities || to >= n_cities) return 0.0;
    return distance_matrix[from][to];
}

double ga_fitness_tsp(const Chromosome *c, void *user_data) {
    if (!c) return 0.0;
    double **dist_matrix = (double**)user_data;

    double total = 0.0;
    for (int i = 0; i < c->num_genes; i++) {
        int city_a = c->alleles[i].integer_val;
        int city_b = c->alleles[(i + 1) % c->num_genes].integer_val;
        total += _tsp_distance(city_a, city_b, dist_matrix, c->num_genes);
    }
    return -total; /* minimize tour length */
}

double ga_fitness_knapsack(const Chromosome *c, void *user_data) {
    if (!c) return 0.0;

    /* User data: array of [n, W, v0,w0, v1,w1, ...] */
    double *params = (double*)user_data;
    int n = (params) ? (int)params[0] : c->num_genes;
    double capacity = (params) ? params[1] : (double)c->num_genes * 5.0;

    double total_value = 0.0;
    double total_weight = 0.0;

    for (int i = 0; i < n && i < c->num_genes; i++) {
        if (c->alleles[i].binary_val) {
            double vi = (params) ? params[2 + 2*i] : 1.0;
            double wi = (params) ? params[3 + 2*i] : 1.0;
            total_value += vi;
            total_weight += wi;
        }
    }

    /* Penalty for exceeding capacity */
    double penalty = (total_weight > capacity)
                     ? 100.0 * (total_weight - capacity) * (total_weight - capacity)
                     : 0.0;
    return total_value - penalty;
}

double ga_fitness_graph_coloring(const Chromosome *c, void *user_data) {
    if (!c) return 0.0;

    /* User data: [n_nodes, n_edges, u0,v0, u1,v1, ...] */
    int *data = (int*)user_data;
    int n_nodes = (data) ? data[0] : c->num_genes;
    int n_edges = (data) ? data[1] : 0;

    int conflicts = 0;
    for (int e = 0; e < n_edges; e++) {
        int u = (data) ? data[2 + 2*e] : e;
        int v = (data) ? data[3 + 2*e] : e + 1;
        if (u < n_nodes && v < n_nodes && u < c->num_genes && v < c->num_genes) {
            if (c->alleles[u].integer_val == c->alleles[v].integer_val) {
                conflicts++;
            }
        }
    }
    return -(double)conflicts; /* minimize conflicts */
}

double ga_fitness_jobshop(const Chromosome *c, void *user_data) {
    (void)user_data;
    if (!c) return 0.0;

    /* Simplified JSSP makespan estimate from operation priorities.
     * Each gene is a priority; jobs are scheduled in priority order. */
    int n_jobs = c->num_genes;
    double makespan = 0.0;

    /* Simulate cumulative processing */
    double current_time = 0.0;
    for (int i = 0; i < n_jobs; i++) {
        /* Processing time proportional to priority value */
        double proc_time = fabs((double)c->alleles[i].integer_val) * 0.1 + 1.0;
        current_time += proc_time;
    }
    makespan = current_time;
    return -makespan; /* minimize makespan */
}

/* ====================================================================
 * L3: Fitness Landscape Analysis
 * ==================================================================== */

double ga_fdc(const Population *pop, const Chromosome *optimum) {
    if (!pop || !optimum || pop->size <= 1) return 0.0;

    double mean_f = pop->avg_fitness;
    double mean_d = 0.0;

    /* Compute distances to optimum */
    double sum_fd = 0.0, sum_f2 = 0.0, sum_d2 = 0.0;

    for (int i = 0; i < pop->size; i++) {
        double d = ga_euclidean_distance(&pop->individuals[i], optimum);
        double f = pop->individuals[i].fitness;
        mean_d += d;
        sum_fd += (f - mean_f) * d;
    }
    mean_d /= (double)pop->size;

    for (int i = 0; i < pop->size; i++) {
        double d = ga_euclidean_distance(&pop->individuals[i], optimum);
        double f = pop->individuals[i].fitness;
        sum_fd = (i == 0) ? (f - mean_f) * (d - mean_d) : sum_fd + (f - mean_f) * (d - mean_d);
        sum_f2 += (f - mean_f) * (f - mean_f);
        sum_d2 += (d - mean_d) * (d - mean_d);
    }

    /* Recompute sum_fd properly */
    sum_fd = 0.0;
    for (int i = 0; i < pop->size; i++) {
        double d = ga_euclidean_distance(&pop->individuals[i], optimum);
        double f = pop->individuals[i].fitness;
        sum_fd += (f - mean_f) * (d - mean_d);
    }

    double denom = sqrt(sum_f2 * sum_d2);
    if (denom < 1e-12) return 0.0;
    return sum_fd / denom;
}

double ga_fitness_autocorrelation(const Population *pop, int walk_length) {
    if (!pop || pop->size <= 1 || walk_length <= 1) return 0.0;

    /* Generate fitness values along a random walk */
    double *fitness_walk = (double*)malloc(sizeof(double) * (size_t)walk_length);
    if (!fitness_walk) return 0.0;

    /* Use first individual as starting point, mutate step by step */
    Chromosome current = ga_chromosome_copy(&pop->individuals[0]);
    /* Simulate walk with small perturbations */
    for (int step = 0; step < walk_length; step++) {
        fitness_walk[step] = current.fitness;
        /* Make a small random change */
        int idx = (int)(_fit_rand64() % (uint64_t)current.num_genes);
        if (current.loci[idx].type == GENE_BINARY) {
            current.alleles[idx].binary_val = !current.alleles[idx].binary_val;
        }
    }

    /* Compute lag-1 autocorrelation */
    double mean = 0.0;
    for (int i = 0; i < walk_length; i++) mean += fitness_walk[i];
    mean /= (double)walk_length;

    double num = 0.0, den = 0.0;
    for (int i = 0; i < walk_length - 1; i++) {
        num += (fitness_walk[i] - mean) * (fitness_walk[i+1] - mean);
    }
    for (int i = 0; i < walk_length; i++) {
        den += (fitness_walk[i] - mean) * (fitness_walk[i] - mean);
    }

    free(fitness_walk);
    if (fabs(den) < 1e-12) return 0.0;
    return num / den;
}

double ga_epistasis_measure(const Population *pop) {
    if (!pop || pop->size <= 1) return 0.0;

    /* Epistasis = variance not explained by additive gene effects.
     * Fit linear additive model: f ≈ Σ a_i * g_i + b
     * Compute residual variance / total variance. */

    /* Simple approximation: compute variance of per-locus effects */
    double *locus_effects = (double*)calloc((size_t)pop->num_genes, sizeof(double));
    if (!locus_effects) return 0.0;

    /* Estimate additive effect of each locus */
    for (int g = 0; g < pop->num_genes; g++) {
        double mean_1 = 0.0, mean_0 = 0.0;
        int count_1 = 0, count_0 = 0;
        for (int i = 0; i < pop->size; i++) {
            double fit = pop->individuals[i].fitness;
            if (pop->loci_def[g].type == GENE_BINARY) {
                if (pop->individuals[i].alleles[g].binary_val) {
                    mean_1 += fit; count_1++;
                } else {
                    mean_0 += fit; count_0++;
                }
            }
        }
        if (count_1 > 0) mean_1 /= (double)count_1;
        if (count_0 > 0) mean_0 /= (double)count_0;
        locus_effects[g] = mean_1 - mean_0;
    }

    /* Compute additive model prediction and residual */
    double ss_res = 0.0, ss_tot = 0.0;
    for (int i = 0; i < pop->size; i++) {
        double pred = pop->avg_fitness;
        for (int g = 0; g < pop->num_genes; g++) {
            if (pop->loci_def[g].type == GENE_BINARY) {
                pred += (pop->individuals[i].alleles[g].binary_val ? 0.5 : -0.5)
                        * locus_effects[g];
            }
        }
        double residual = pop->individuals[i].fitness - pred;
        ss_res += residual * residual;
        ss_tot += (pop->individuals[i].fitness - pop->avg_fitness)
                  * (pop->individuals[i].fitness - pop->avg_fitness);
    }

    free(locus_effects);
    if (ss_tot < 1e-12) return 0.0;
    return ss_res / ss_tot; /* 1 - R² = epistasis measure */
}

int ga_count_local_optima(const Population *pop, double epsilon) {
    if (!pop || pop->size <= 0) return 0;

    int count = 0;
    /* Check each individual: is it a local optimum in its neighborhood? */
    for (int i = 0; i < pop->size; i++) {
        bool is_local_opt = true;
        /* Compare with nearby individuals in population */
        for (int j = 0; j < pop->size; j++) {
            if (i == j) continue;
            double dist = ga_euclidean_distance(&pop->individuals[i],
                                                 &pop->individuals[j]);
            if (dist > epsilon) continue;
            if (pop->individuals[j].fitness > pop->individuals[i].fitness) {
                is_local_opt = false;
                break;
            }
        }
        if (is_local_opt) count++;
    }
    return count;
}

double ga_information_content(const double *fitness_walk, int n) {
    if (!fitness_walk || n <= 1) return 0.0;

    /* Entropic measure of landscape ruggedness.
     * Count sign changes in first differences as information. */
    int sign_changes = 0;
    for (int i = 1; i < n - 1; i++) {
        double d1 = fitness_walk[i] - fitness_walk[i-1];
        double d2 = fitness_walk[i+1] - fitness_walk[i];
        if (d1 * d2 < 0.0) sign_changes++;
    }
    /* Normalize to [0, 1] */
    return (double)sign_changes / (double)(n - 2);
}

double ga_signal_noise_ratio(const double *fitness_walk, int n) {
    if (!fitness_walk || n <= 1) return 0.0;

    /* Signal = variance of smoothed walk; Noise = variance of residuals */
    double mean = 0.0;
    for (int i = 0; i < n; i++) mean += fitness_walk[i];
    mean /= (double)n;

    double var_total = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = fitness_walk[i] - mean;
        var_total += diff * diff;
    }
    var_total /= (double)n;

    /* Simple moving average (window 3) as signal estimate */
    double var_noise = 0.0;
    for (int i = 1; i < n - 1; i++) {
        double smooth = (fitness_walk[i-1] + fitness_walk[i] + fitness_walk[i+1]) / 3.0;
        double resid = fitness_walk[i] - smooth;
        var_noise += resid * resid;
    }
    var_noise /= (double)(n - 2);

    if (var_noise < 1e-12) return var_total / 1e-12; /* cap */
    return (var_total - var_noise) / var_noise;
}

/* ====================================================================
 * L3: Multi-Objective Fitness
 * ==================================================================== */

bool ga_pareto_dominates(const double *obj_a, const double *obj_b,
                          int num_objectives) {
    /* a dominates b if a is no worse in all objectives AND strictly better in at least one */
    bool better_in_any = false;
    for (int i = 0; i < num_objectives; i++) {
        if (obj_a[i] < obj_b[i]) return false; /* a is worse in objective i */
        if (obj_a[i] > obj_b[i]) better_in_any = true;
    }
    return better_in_any;
}

void ga_crowding_distance(double *distances,
                           const double *objectives,
                           int n_individuals, int n_objectives) {
    if (!distances || !objectives || n_individuals <= 0) return;

    /* Initialize distances to zero */
    for (int i = 0; i < n_individuals; i++) distances[i] = 0.0;

    for (int obj = 0; obj < n_objectives; obj++) {
        /* Sort individuals by this objective */
        int *sorted = (int*)malloc(sizeof(int) * (size_t)n_individuals);
        if (!sorted) return;

        for (int i = 0; i < n_individuals; i++) sorted[i] = i;
        for (int i = 0; i < n_individuals; i++) {
            for (int j = i + 1; j < n_individuals; j++) {
                if (objectives[sorted[j] * n_objectives + obj] >
                    objectives[sorted[i] * n_objectives + obj]) {
                    int tmp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = tmp;
                }
            }
        }

        /* Boundary points get infinite distance */
        double f_min = objectives[sorted[0] * n_objectives + obj];
        double f_max = objectives[sorted[n_individuals - 1] * n_objectives + obj];
        double range = f_max - f_min;
        if (fabs(range) < 1e-12) range = 1.0;

        distances[sorted[0]] = DBL_MAX;
        distances[sorted[n_individuals - 1]] = DBL_MAX;

        for (int i = 1; i < n_individuals - 1; i++) {
            double next = objectives[sorted[i + 1] * n_objectives + obj];
            double prev = objectives[sorted[i - 1] * n_objectives + obj];
            distances[sorted[i]] += (next - prev) / range;
        }

        free(sorted);
    }
}

double ga_hypervolume(const double *pareto_front, int n_points,
                       int n_objectives, const double *reference_point) {
    if (!pareto_front || n_points <= 0 || !reference_point) return 0.0;

    /* Simple hypervolume by Lebesgue measure approximation
     * For 2-objective case: sum of rectangle areas */
    if (n_objectives == 2) {
        /* Sort by first objective ascending */
        int *sorted = (int*)malloc(sizeof(int) * (size_t)n_points);
        if (!sorted) return 0.0;
        for (int i = 0; i < n_points; i++) sorted[i] = i;

        for (int i = 0; i < n_points; i++) {
            for (int j = i + 1; j < n_points; j++) {
                if (pareto_front[sorted[j] * 2] < pareto_front[sorted[i] * 2]) {
                    int tmp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = tmp;
                }
            }
        }

        double hv = 0.0;
        double prev_f1 = reference_point[0];
        for (int i = 0; i < n_points; i++) {
            double f1 = pareto_front[sorted[i] * 2];
            double f2 = pareto_front[sorted[i] * 2 + 1];
            double width = f1 - prev_f1;
            double height = reference_point[1] - f2;
            if (width > 0 && height > 0) hv += width * height;
            prev_f1 = f1;
        }

        free(sorted);
        return hv;
    }

    /* Higher dimensions: Monte Carlo approximation */
    int samples = 10000;
    int hits = 0;
    for (int s = 0; s < samples; s++) {
        /* Generate random point in hyper-rectangle [ref_point, bounds] */
        double point[4]; /* assume max 4 objectives */
        for (int o = 0; o < n_objectives && o < 4; o++) {
            point[o] = _fit_rand_double() * reference_point[o];
        }
        /* Check if dominated by any pareto point */
        for (int p = 0; p < n_points; p++) {
            bool dominated = true;
            for (int o = 0; o < n_objectives && o < 4; o++) {
                if (point[o] > pareto_front[p * n_objectives + o]) {
                    dominated = false;
                    break;
                }
            }
            if (dominated) { hits++; break; }
        }
    }

    /* Volume of reference hyper-rectangle */
    double ref_vol = 1.0;
    for (int o = 0; o < n_objectives && o < 4; o++) {
        ref_vol *= reference_point[o];
    }

    return ref_vol * (double)hits / (double)samples;
}
