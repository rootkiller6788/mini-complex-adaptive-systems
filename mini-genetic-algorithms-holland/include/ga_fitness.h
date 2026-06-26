/**
 * ga_fitness.h — Fitness Functions and Landscape Analysis
 *
 * Fitness is the objective function that the GA optimizes. The structure
 * of the fitness landscape determines problem difficulty: ruggedness,
 * deceptiveness, neutrality, and epistasis all affect GA performance.
 *
 * Reference: Kauffman "The Origins of Order" (1993) — NK landscapes
 *            Stadler "Fitness Landscapes" (2002) — Mathematical theory
 *            Jones "Evolutionary Algorithms, Fitness Landscapes and Search" (1995)
 *
 * Knowledge: L1 Definitions, L3 Math Structures, L6 Canonical Problems
 */

#ifndef GA_FITNESS_H
#define GA_FITNESS_H

#include "ga_core.h"

/* ====================================================================
 * L1/L6: Canonical Fitness Functions
 * ==================================================================== */

/* === Binary-encoded Fitness Functions === */

/**
 * OneMax: f(x) = Σ x_i  (count number of 1s)
 * The simplest test function; no local optima, no deception.
 * Reference: Ackley (1987)
 */
double  ga_fitness_onemax(const Chromosome *c, void *user_data);

/**
 * Needle-in-a-Haystack: f(x) = 1 if x == target, else 0.
 * Extremely hard: provides no gradient information.
 */
double  ga_fitness_needle(const Chromosome *c, void *user_data);

/**
 * Deceptive Trap Function:
 * f(x) = trap(u) where u = Σ x_i, trap(u) = k if u=k, else (k-1-u)
 * Local optimum at u=0, global at u=k. Deceptive because low-order
 * schemata mislead the search away from the global optimum.
 *
 * Reference: Deb & Goldberg (1993)
 */
double  ga_fitness_deceptive_trap(const Chromosome *c, void *user_data);

/**
 * Royal Road Function: fitness = sum of bonuses for each fully-1 block.
 * Tests the Building Block Hypothesis: GA should assemble short
 * building blocks into longer ones.
 *
 * Reference: Mitchell, Forrest & Holland (1992)
 */
double  ga_fitness_royal_road(const Chromosome *c, void *user_data);

/**
 * NK Fitness Landscape: epistatic interactions among N genes,
 * each gene's contribution depends on K other genes.
 * Tunable ruggedness: K=0 smooth, K=N-1 maximally rugged.
 *
 * Reference: Kauffman (1993)
 */
double  ga_fitness_nk_landscape(const Chromosome *c, void *user_data);

/* === Real-valued Optimization Functions === */

/**
 * Sphere Function: f(x) = Σ x_i²
 * Smooth, convex, separable. Global minimum at origin.
 * Bounds: [-5.12, 5.12]^n
 */
double  ga_fitness_sphere(const Chromosome *c, void *user_data);

/**
 * Rosenbrock Function (Banana Function):
 * f(x) = Σ [100(x_{i+1} - x_i²)² + (1 - x_i)²]
 * Non-convex. Global minimum at x=(1,1,...,1). Narrow curved valley.
 *
 * Reference: Rosenbrock (1960)
 */
double  ga_fitness_rosenbrock(const Chromosome *c, void *user_data);

/**
 * Rastrigin Function:
 * f(x) = 10n + Σ [x_i² - 10 cos(2π x_i)]
 * Highly multimodal with regular local minima.
 * Global minimum at x=0.
 *
 * Reference: Rastrigin (1974)
 */
double  ga_fitness_rastrigin(const Chromosome *c, void *user_data);

/**
 * Ackley Function:
 * f(x) = -20 exp(-0.2√(Σx_i²/n)) - exp(Σcos(2πx_i)/n) + 20 + e
 * Nearly flat outer region with many local minima, deep hole at center.
 *
 * Reference: Ackley (1987)
 */
double  ga_fitness_ackley(const Chromosome *c, void *user_data);

/**
 * Schwefel Function:
 * f(x) = 418.9829n - Σ x_i sin(√|x_i|)
 * Complex multimodal, deceptive: global minimum far from origin.
 *
 * Reference: Schwefel (1981)
 */
double  ga_fitness_schwefel(const Chromosome *c, void *user_data);

/**
 * Griewank Function:
 * f(x) = 1 + Σ x_i²/4000 - Π cos(x_i/√i)
 * Highly multimodal. Product term introduces interdependencies.
 *
 * Reference: Griewank (1981)
 */
double  ga_fitness_griewank(const Chromosome *c, void *user_data);

/* === Combinatorial Optimization Functions === */

/**
 * Traveling Salesman Problem (TSP) fitness: total tour length.
 * Chromosome encodes a permutation (tour order).
 * lower is better (minimization).
 *
 * Reference: Lawler et al. "The Traveling Salesman Problem" (1985)
 */
double  ga_fitness_tsp(const Chromosome *c, void *user_data);

/**
 * 0/1 Knapsack Problem:
 * Maximize Σ v_i x_i subject to Σ w_i x_i ≤ W.
 * Fitness = total value with penalty for weight violation.
 *
 * Reference: Martello & Toth (1990)
 */
double  ga_fitness_knapsack(const Chromosome *c, void *user_data);

/**
 * Graph Coloring Problem: minimize number of color conflicts.
 * Penalty for adjacent vertices with same color.
 *
 * Reference: Jensen & Toft "Graph Coloring Problems" (1995)
 */
double  ga_fitness_graph_coloring(const Chromosome *c, void *user_data);

/**
 * Job Shop Scheduling (JSSP): minimize makespan (completion time).
 * Chromosome encodes operation priorities.
 *
 * Reference: Pinedo "Scheduling: Theory, Algorithms, and Systems" (2016)
 */
double  ga_fitness_jobshop(const Chromosome *c, void *user_data);

/* ====================================================================
 * L3: Fitness Landscape Analysis
 * ==================================================================== */

/**
 * Compute fitness-distance correlation (FDC):
 * r_fd = Cov(f_i, d_i) / (σ_f * σ_d)
 * Measures how well fitness correlates with distance to optimum.
 * r_fd ≈ -1: easy problem; r_fd ≈ 0: hard/deceptive.
 *
 * Reference: Jones & Forrest (1995)
 */
double  ga_fdc(const Population *pop, const Chromosome *optimum);

/**
 * Compute autocorrelation of fitness along a random walk through
 * the search space. The correlation length measures landscape smoothness.
 *
 * Reference: Weinberger (1990)
 */
double  ga_fitness_autocorrelation(const Population *pop, int walk_length);

/**
 * Epistasis measure: variance of fitness not explained by additive model.
 * High epistasis → non-separable, harder for GA.
 *
 * Reference: Davidor (1991)
 */
double  ga_epistasis_measure(const Population *pop);

/**
 * Fitness Landscape Ruggedness: count of local optima in neighborhood.
 * Uses basin of attraction analysis.
 */
int     ga_count_local_optima(const Population *pop, double epsilon);

/**
 * Compute the information content of a fitness landscape from
 * a random walk (entropic measure).
 *
 * Reference: Vassilev et al. (2000)
 */
double  ga_information_content(const double *fitness_walk, int n);

/** Signal-to-noise ratio of fitness landscape */
double  ga_signal_noise_ratio(const double *fitness_walk, int n);

/* ====================================================================
 * L3: Multi-Objective Fitness
 * ==================================================================== */

/** Pareto dominance check: a dominates b if a is no worse in all objectives
 *  and strictly better in at least one. */
bool    ga_pareto_dominates(const double *obj_a, const double *obj_b,
                             int num_objectives);

/** Compute crowding distance for NSGA-II diversity preservation */
void    ga_crowding_distance(double *distances,
                              const double *objectives,
                              int n_individuals, int n_objectives);

/** Hypervolume indicator: volume of objective space dominated by the set */
double  ga_hypervolume(const double *pareto_front, int n_points,
                        int n_objectives, const double *reference_point);

#endif /* GA_FITNESS_H */
