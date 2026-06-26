#include "nk_core.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * Example 1: Basic NK Landscape Construction and Exploration
 *
 * Demonstrates L1-L3: creating landscapes with different K values,
 * evaluating fitness, and detecting local optima.
 *
 * Kauffman's key insight: K tunes ruggedness.
 * - K=0: single smooth peak
 * - K=2: "edge of chaos" - moderate ruggedness
 * - K=N-1: maximally rugged random landscape
 * ============================================================================ */

int main(void) {
    printf("=== NK Fitness Landscape: Basic Exploration ===\n\n");

    int N = 8;
    int K_values[] = {0, 1, 3, 7}; /* K=N-1 = maximally rugged */
    int nK = 4;

    for (int ki = 0; ki < nK; ki++) {
        int K = K_values[ki];
        printf("--- Landscape N=%d, K=%d ---\n", N, K);

        NKLandscape *land = nk_landscape_create(N, K,
            NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM, 42 + ki * 100);

        if (!land) {
            printf("  Failed to create landscape.\n");
            continue;
        }

        /* Phase classification */
        NKPhaseClassification phase = nk_landscape_phase(land);
        const char *phase_names[] = {"ORDERED", "CRITICAL", "CHAOTIC"};
        printf("  Phase: %s\n", phase_names[phase]);

        /* Sample some genotypes */
        NKGenotype *g = nk_genotype_create(N);
        uint64_t rng = 999 + ki;
        double min_f = 1.0, max_f = 0.0, sum_f = 0.0;
        int n_local_opt = 0;

        for (int s = 0; s < 200; s++) {
            nk_genotype_randomize(g, &rng);
            double f = nk_fitness(land, g);
            if (f < min_f) min_f = f;
            if (f > max_f) max_f = f;
            sum_f += f;
            if (nk_is_local_optimum(land, g)) n_local_opt++;
        }

        printf("  Fitness range: [%.4f, %.4f]\n", min_f, max_f);
        printf("  Mean fitness:  %.4f\n", sum_f / 200.0);
        printf("  Local optima in sample: %d/200\n", n_local_opt);

        /* Ruggedness estimate */
        double ruggedness = nk_ruggedness_estimate(land, 100);
        printf("  Ruggedness (1-rho(1)): %.4f\n", ruggedness);

        nk_genotype_free(g);
        nk_landscape_free(land);
        printf("\n");
    }

    printf("Observe: as K increases, ruggedness increases and more local optima appear.\n");
    printf("This is Kauffman's tunable ruggedness (The Origins of Order, 1993).\n");

    return 0;
}
