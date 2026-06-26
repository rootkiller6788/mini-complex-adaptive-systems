/**
 * swarm_example.c — Swarm Intelligence on Classic Test Functions
 *
 * Demonstrates PSO, ACO, and Firefly Algorithm on standard benchmark
 * functions (Rastrigin, TSP, Rosenbrock).
 *
 * Knowledge: L6 Canonical Problem — Global function optimization
 *            L7 Application — Swarm-based optimization
 */

#include "swarm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== Swarm Intelligence Examples ===\n\n");

    /* ================================================================
     * PSO on Rastrigin function (2D)
     * Global minimum: f(0,0)=0, within [-5.12, 5.12]
     * ================================================================ */
    printf("--- PSO on Rastrigin (2D) ---\n");
    double lb2[] = {-5.12, -5.12};
    double ub2[] = {5.12, 5.12};
    PSO_Swarm *pso = pso_create(30, 2, lb2, ub2, 0.7, 1.5, 1.5);
    pso_random_init(pso);
    pso_run(pso, 100, swarm_test_rastrigin, NULL, true);

    const double *best = pso_get_best(pso);
    printf("PSO best position: (%.4f, %.4f)\n", best[0], best[1]);
    printf("PSO best value: %.6f (optimum = 0.0)\n", pso->global_best_value);

    /* Show convergence history */
    int hist_len;
    const double *hist = pso_history(pso, &hist_len);
    printf("Convergence: ");
    for (int i = 0; i < hist_len && i < 10; i++) {
        printf("%.3f ", hist[i]);
    }
    printf("\n");
    pso_destroy(pso);

    /* PSO with constriction coefficient */
    printf("\n--- PSO with Constriction (Clerc & Kennedy, 2002) ---\n");
    PSO_Swarm *pso2 = pso_create(30, 2, lb2, ub2, 0.7, 2.05, 2.05);
    pso_random_init(pso2);
    pso_set_constriction(pso2, 2.05, 2.05);
    double chi = pso_constriction_coefficient(2.05, 2.05);
    printf("Constriction coefficient chi = %.4f\n", chi);
    pso_run(pso2, 100, swarm_test_rastrigin, NULL, false);
    printf("PSO-Constriction best value: %.6f\n", pso2->global_best_value);
    pso_destroy(pso2);

    /* ================================================================
     * ACO on TSP (10 cities, randomly placed in a circle)
     * ================================================================ */
    printf("\n--- ACO on TSP (10 cities) ---\n");
    int n = 10;
    /* Generate random city coordinates on a circle + noise */
    double cx[10], cy[10];
    for (int i = 0; i < n; i++) {
        double angle = 2.0 * 3.14159265 * (double)i / (double)n;
        cx[i] = 10.0 * cos(angle) + ((double)rand()/RAND_MAX - 0.5) * 4.0;
        cy[i] = 10.0 * sin(angle) + ((double)rand()/RAND_MAX - 0.5) * 4.0;
    }

    /* Compute distance matrix */
    double *dists = (double*)calloc(n * n, sizeof(double));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double dx = cx[i] - cx[j];
            double dy = cy[i] - cy[j];
            dists[i * n + j] = sqrt(dx*dx + dy*dy);
        }
    }

    ACO_Colony *aco = aco_create(20, n, dists, 1.0, 3.0, 0.1);
    aco_init_pheromone(aco, 1.0 / ((double)n * 50.0));
    aco_run(aco, 100, true);

    printf("ACO best tour length: %.4f\n", aco->best_tour_length);
    printf("ACO lower bound (NN heuristic): %.4f\n", aco_lower_bound(aco));
    const int *tour = aco_get_best_tour(aco);
    printf("Best tour: ");
    for (int i = 0; i < n; i++) printf("%d ", tour[i]);
    printf("\n");

    aco_destroy(aco);
    free(dists);

    /* ================================================================
     * Firefly Algorithm on Rosenbrock (5D)
     * Global minimum: f(1,1,...,1)=0
     * ================================================================ */
    printf("\n--- Firefly Algorithm on Rosenbrock (5D) ---\n");
    double lb5[] = {-5, -5, -5, -5, -5};
    double ub5[] = {10, 10, 10, 10, 10};
    FireflySwarm *fs = firefly_create(25, 5, lb5, ub5, 0.2, 1.0, 0.1);
    firefly_random_init(fs);
    firefly_run(fs, 80, swarm_test_rosenbrock, NULL, true);

    const Firefly *best_fly = firefly_get_best(fs);
    printf("Firefly best value: %.6f (optimum=0.0 at x_i=1.0)\n",
           best_fly ? best_fly->brightness : -1.0);
    firefly_destroy(fs);

    /* ================================================================
     * Flocking simulation (Boids)
     * ================================================================ */
    printf("\n--- Flocking (Boids) Simulation ---\n");
    double fb_min[] = {-20, -20};
    double fb_max[] = {20, 20};
    Flock *flock = flock_create(20, 2, 5.0, 1.0, 1.5, 1.0, 3.0);
    flock_random_init(flock, fb_min, fb_max);

    double init_polar = flock_polarization(flock);
    printf("Initial polarization: %.4f (0=random, 1=aligned)\n", init_polar);

    /* Run 50 steps */
    for (int t = 0; t < 50; t++) {
        flock_step(flock, 0.1);
        if (t % 10 == 9) {
            double polar = flock_polarization(flock);
            double center[2];
            flock_center_of_mass(flock, center);
            printf("  step %d: polarization=%.4f, center=(%.1f,%.1f)\n",
                   t+1, polar, center[0], center[1]);
        }
    }
    flock_destroy(flock);

    printf("\n=== Swarm Example Complete ===\n");
    return 0;
}