#include "swarm_core.h"
#include "swarm_pso.h"
#include "swarm_applications.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Example 1: Particle Swarm Optimization on classical benchmark functions.
 * Demonstrates PSO on Sphere, Rosenbrock, Rastrigin, and Ackley functions
 * with 2D and 10D search spaces. Compares convergence behavior.
 */

static void run_pso_benchmark(const char* name,
    double (*fitness_fn)(const double*, int, void*),
    int dim, double* bounds_low, double* bounds_high, int n_agents) {
    
    SwarmConfig config;
    memset(&config, 0, sizeof(config));
    config.n_agents = n_agents;
    config.dimensions = dim;
    config.topology = SWARM_TOPOLOGY_GLOBAL;
    config.inertia_weight = 0.7298;
    config.cognitive_weight = 1.49618;
    config.social_weight = 1.49618;
    config.max_velocity = 0.5 * (bounds_high[0] - bounds_low[0]);
    config.use_constriction = true;
    config.fitness_function = fitness_fn;
    config.fitness_context = NULL;
    config.minimize = true;
    config.max_iterations = 200;
    config.seed = 42;
    config.bounds_low = bounds_low;
    config.bounds_high = bounds_high;
    
    SwarmResult* result = swarm_pso_run(&config);
    if (!result) {
        printf("  %-20s [FAILED] Could not allocate result\n", name);
        return;
    }
    
    printf("  %-20s dim=%-2d best=%.6e iter=%d/%d converged=%d time=%.3fs\n",
        name, dim, result->best_fitness, result->iterations_run,
        config.max_iterations, result->converged, result->time_seconds);
    
    swarm_free_vector(result->best_position);
    swarm_free_vector(result->history);
    free(result);
}

int main(void) {
    printf("\n=== PSO Benchmark Optimization ===\n\n");
    swarm_rng_seed(42);
    
    /* 2D Benchmarks */
    double bounds_2d_low[] = {-10.0, -10.0};
    double bounds_2d_high[] = {10.0, 10.0};
    
    run_pso_benchmark("Sphere (2D)", swarm_benchmark_sphere, 2,
                      bounds_2d_low, bounds_2d_high, 30);
    run_pso_benchmark("Rosenbrock (2D)", swarm_benchmark_rosenbrock, 2,
                      bounds_2d_low, bounds_2d_high, 30);
    run_pso_benchmark("Rastrigin (2D)", swarm_benchmark_rastrigin, 2,
                      bounds_2d_low, bounds_2d_high, 30);
    run_pso_benchmark("Ackley (2D)", swarm_benchmark_ackley, 2,
                      bounds_2d_low, bounds_2d_high, 30);
    
    /* 10D Benchmarks */
    double bounds_10d_low[] = {-5.12, -5.12, -5.12, -5.12, -5.12,
                               -5.12, -5.12, -5.12, -5.12, -5.12};
    double bounds_10d_high[] = {5.12, 5.12, 5.12, 5.12, 5.12,
                                 5.12, 5.12, 5.12, 5.12, 5.12};
    
    run_pso_benchmark("Sphere (10D)", swarm_benchmark_sphere, 10,
                      bounds_10d_low, bounds_10d_high, 50);
    run_pso_benchmark("Rastrigin (10D)", swarm_benchmark_rastrigin, 10,
                      bounds_10d_low, bounds_10d_high, 50);
    
    /* Griewank with wider bounds */
    double bounds_gw[] = {-600, -600, -600, -600, -600,
                          -600, -600, -600, -600, -600};
    double bounds_gw_h[] = {600, 600, 600, 600, 600,
                            600, 600, 600, 600, 600};
    run_pso_benchmark("Griewank (10D)", swarm_benchmark_griewank, 10,
                      bounds_gw, bounds_gw_h, 50);
    
    printf("\nOptimization complete.\n");
    return 0;
}
