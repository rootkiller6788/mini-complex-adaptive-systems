#include "nk_core.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include "nk_statistics.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================================
 * Performance Benchmarks for NK Fitness Landscapes
 *
 * Measures throughput for key operations to establish computational
 * feasibility limits for different (N, K) parameter combinations.
 * ============================================================================ */

static double wall_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== NK Fitness Landscapes -- Performance Benchmarks ===\n\n");

    int N_values[] = {8, 12, 16, 20};
    int K_values[] = {1, 2, 3, 4};
    int n_trials = 10000;

    printf("%-6s %-4s %-12s %-12s %-12s\n",
           "N", "K", "fitness/sec", "walk/sec", "local_opt/sec");
    printf("------ ---- ------------ ------------ ------------\n");

    for (int ni = 0; ni < 4; ni++) {
        for (int ki = 0; ki < 4; ki++) {
            int N = N_values[ni];
            int K = K_values[ki];
            if (K >= N) continue;

            NKLandscape *land = nk_landscape_create(N, K,
                NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM, 12345);
            if (!land) continue;

            NKGenotype *g = nk_genotype_create(N);
            uint64_t rng = 99999;
            nk_genotype_randomize(g, &rng);

            /* Benchmark fitness evaluation */
            double t0 = wall_seconds();
            for (int t = 0; t < n_trials; t++) {
                nk_genotype_flip(g, t % N);
                nk_fitness(land, g);
            }
            double t1 = wall_seconds();
            double fitness_per_sec = n_trials / (t1 - t0 + 1e-9);

            /* Benchmark greedy walk */
            t0 = wall_seconds();
            int n_walk_trials = (n_trials > 1000) ? 1000 : n_trials;
            for (int t = 0; t < n_walk_trials; t++) {
                NKGenotype *start = nk_genotype_create(N);
                uint64_t rng2 = rng + t;
                nk_genotype_randomize(start, &rng2);
                NKWalkResult *wr = nk_walk_greedy(land, start, 50);
                nk_walk_result_free(wr);
                nk_genotype_free(start);
            }
            t1 = wall_seconds();
            double walk_per_sec = n_walk_trials / (t1 - t0 + 1e-9);

            /* Benchmark local optimum check */
            t0 = wall_seconds();
            int opt_trials = (n_trials > 2000) ? 2000 : n_trials;
            for (int t = 0; t < opt_trials; t++) {
                nk_genotype_randomize(g, &rng);
                nk_is_local_optimum(land, g);
            }
            t1 = wall_seconds();
            double opt_per_sec = opt_trials / (t1 - t0 + 1e-9);

            printf("%-6d %-4d %-12.0f %-12.0f %-12.0f\n",
                   N, K, fitness_per_sec, walk_per_sec, opt_per_sec);

            nk_genotype_free(g);
            nk_landscape_free(land);
        }
    }

    printf("\nBenchmark complete.\n");
    return 0;
}
