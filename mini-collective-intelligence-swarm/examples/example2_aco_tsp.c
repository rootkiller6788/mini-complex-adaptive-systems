#include "swarm_core.h"
#include "swarm_aco.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Example 2: Ant Colony Optimization for the Traveling Salesman Problem.
 * Solves a 30-city TSP instance. Compares Ant System, MMAS, and ACS strategies.
 * Demonstrates the pheromone trail mechanism on a constructed city layout.
 */

static void generate_circle_cities(double* x, double* y, int n, double radius) {
    for (int i = 0; i < n; i++) {
        double angle = 2.0 * SWARM_PI * (double)i / (double)n;
        x[i] = radius * cos(angle);
        y[i] = radius * sin(angle);
    }
}

static void generate_cluster_cities(double* x, double* y, int n) {
    /* 4 clusters of cities */
    double cx[4] = {0, 10, 0, 10};
    double cy[4] = {0, 0, 10, 10};
    for (int i = 0; i < n; i++) {
        int cluster = i % 4;
        x[i] = cx[cluster] + swarm_rng_gaussian() * 1.5;
        y[i] = cy[cluster] + swarm_rng_gaussian() * 1.5;
    }
}

static void run_aco_on_problem(const char* name, double* x, double* y, int n_cities,
                                ACOStrategy strategy) {
    ACOConfig config;
    memset(&config, 0, sizeof(config));
    config.n_ants = n_cities;
    config.strategy = strategy;
    config.alpha = 1.0;
    config.beta = 5.0;
    config.rho = 0.1;
    config.Q = 1.0;
    config.tau0 = 1.0 / (double)(n_cities * n_cities);
    config.q0 = 0.9;
    config.local_rho = 0.1;
    config.use_daemon = true;
    config.verbose = false;
    
    SwarmResult* result = swarm_aco_run_tsp(x, y, n_cities, &config);
    if (!result) {
        printf("  %-30s [FAILED]\n", name);
        return;
    }
    
    /* Compute optimal tour lower bound */
    double lower_bound = 0.0;
    for (int i = 0; i < n_cities; i++) {
        double min_d = 1e100;
        for (int j = 0; j < n_cities; j++) {
            if (i == j) continue;
            double dx = x[i] - x[j], dy = y[i] - y[j];
            double d = sqrt(dx*dx + dy*dy);
            if (d < min_d) min_d = d;
        }
        lower_bound += min_d;
    }
    
    printf("  %-30s length=%.4f  LB=%.4f  gap=%.1f%%  iter=%d\n",
        name, result->best_fitness, lower_bound,
        100.0 * (result->best_fitness - lower_bound) / lower_bound,
        result->iterations_run);
    
    swarm_free_vector(result->best_position);
    swarm_free_vector(result->history);
    free(result);
}

int main(void) {
    printf("\n=== ACO Traveling Salesman Problem ===\n\n");
    swarm_rng_seed(12345);
    
    int n_cities = 30;
    double x[30], y[30];
    
    /* Circle layout — optimal tour is the circle itself */
    printf("[Circle city layout]\n");
    generate_circle_cities(x, y, n_cities, 10.0);
    run_aco_on_problem("Circle-AS", x, y, n_cities, ACO_ANT_SYSTEM);
    run_aco_on_problem("Circle-MMAS", x, y, n_cities, ACO_MMAS);
    run_aco_on_problem("Circle-ACS", x, y, n_cities, ACO_ACS);
    
    /* Cluster layout — more realistic */
    printf("\n[Clustered city layout]\n");
    generate_cluster_cities(x, y, n_cities);
    run_aco_on_problem("Cluster-AS", x, y, n_cities, ACO_ANT_SYSTEM);
    run_aco_on_problem("Cluster-MMAS", x, y, n_cities, ACO_MMAS);
    run_aco_on_problem("Cluster-ACS", x, y, n_cities, ACO_ACS);
    
    printf("\nTSP optimization complete.\n");
    return 0;
}
