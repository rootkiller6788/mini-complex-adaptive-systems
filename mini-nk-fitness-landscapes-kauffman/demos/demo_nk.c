#include "nk_core.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include "nk_statistics.h"
#include "nk_coevolution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * NK Fitness Landscapes -- Comprehensive Demo
 *
 * Demonstrates a complete workflow:
 *   1. Create NK landscape at the "edge of chaos"
 *   2. Perform adaptive walks with multiple strategies
 *   3. Compute landscape statistics
 *   4. Demonstrate coevolution with Red Queen dynamics
 *
 * References:
 *   Kauffman, S.A. (1993). The Origins of Order. Oxford University Press.
 *   Kauffman, S.A. & Johnsen, S. (1991). J. Theor. Biol., 149(4), 467-505.
 * ============================================================================ */

static void print_section(const char *title) {
    printf("\n============================================================\n");
    printf("  %s\n", title);
    printf("============================================================\n");
}

int main(void) {
    print_section("NK Fitness Landscapes (Kauffman 1993) -- Full Demo");

    /* === Part 1: Landscape at the Edge of Chaos === */
    print_section("Part 1: Creating an NK Landscape at the Edge of Chaos");

    int N = 12, K = 3;
    printf("Parameters: N=%d, K=%d\n", N, K);
    printf("K/N = %.2f -- this is the critical regime where complex\n", (double)K/N);
    printf("adaptations are most likely to emerge (Kauffman 1993, Ch. 5).\n");

    NKLandscape *land = nk_landscape_create(N, K,
        NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM, 42);

    if (!land) {
        printf("ERROR: Failed to create landscape.\n");
        return 1;
    }

    printf("Landscape name: %s\n", land->name);
    printf("Phase: %s\n\n",
           nk_landscape_phase(land) == NK_PHASE_CRITICAL ? "CRITICAL (edge of chaos)" :
           nk_landscape_phase(land) == NK_PHASE_ORDERED ? "ORDERED" : "CHAOTIC");

    /* === Part 2: Landscape Exploration === */
    print_section("Part 2: Fitness Landscape Sampling");

    int n_samples = 500;
    NKGenotype *g = nk_genotype_create(N);
    uint64_t rng_state = 12345;

    double sum_f = 0.0, min_f = 1.0, max_f = 0.0;
    int n_local_opt = 0;

    for (int i = 0; i < n_samples; i++) {
        nk_genotype_randomize(g, &rng_state);
        double f = nk_fitness(land, g);
        sum_f += f;
        if (f < min_f) min_f = f;
        if (f > max_f) max_f = f;
        if (nk_is_local_optimum(land, g)) n_local_opt++;
    }

    printf("Sampled %d random genotypes:\n", n_samples);
    printf("  Fitness range:     [%.4f, %.4f]\n", min_f, max_f);
    printf("  Mean fitness:      %.4f\n", sum_f / n_samples);
    printf("  Local optima:      %d / %d (%.1f%%)\n",
           n_local_opt, n_samples, 100.0 * n_local_opt / n_samples);

    double ruggedness = nk_ruggedness_estimate(land, 200);
    printf("  Ruggedness:        %.4f (0=smooth, 1=random)\n", ruggedness);

    /* === Part 3: Adaptive Walks === */
    print_section("Part 3: Adaptive Walks on the Landscape");

    nk_genotype_randomize(g, &rng_state);
    char *start_str = nk_genotype_to_string(g);
    printf("Starting genotype:  %s\n", start_str);
    printf("Starting fitness:   %.6f\n\n", nk_fitness(land, g));
    free(start_str);

    const char *names[] = {"Greedy", "Steepest Ascent", "Next Ascent", "Random Better"};
    NKWalkResult* (*funcs[])(const NKLandscape*, const NKGenotype*, int) = {
        nk_walk_greedy, nk_walk_steepest_ascent,
        nk_walk_next_ascent, nk_walk_random_better
    };

    for (int s = 0; s < 4; s++) {
        NKGenotype *start_copy = nk_genotype_clone(g);
        NKWalkResult *wr = funcs[s](land, start_copy, 100);
        printf("  %-16s: %3d steps, final fitness = %.6f, optimum = %s\n",
               names[s], wr ? wr->steps : -1,
               wr ? wr->path[wr->steps]->fitness : 0.0,
               wr && wr->reached_optimum ? "yes" : "no");
        nk_walk_result_free(wr);
        nk_genotype_free(start_copy);
    }

    /* === Part 4: Landscape Statistics === */
    print_section("Part 4: Comprehensive Landscape Statistics");

    uint64_t stat_rng = 99999;
    NKLandscapeStatistics *stats = nk_statistics_compute(land, 500, 50, &stat_rng);
    if (stats) {
        nk_statistics_print(stats);
        nk_statistics_free(stats);
    }

    /* === Part 5: Coevolution Demo === */
    print_section("Part 5: NKCS Coevolution (Kauffman & Johnsen 1991)");

    int C = 2;
    printf("Creating NKCS landscape: N=%d, K=%d, C=%d\n", N, K, C);
    NKCSLandscape *nkcs = nkcs_create(N, K, C, 0.4, 77777);

    if (nkcs) {
        NKGenotype *ga = nk_genotype_create(N);
        NKGenotype *gb = nk_genotype_create(N);
        nk_genotype_randomize(ga, &rng_state);
        nk_genotype_randomize(gb, &rng_state);

        double fa_init = nkcs_fitness_a(nkcs, ga, gb);
        double fb_init = nkcs_fitness_b(nkcs, ga, gb);
        printf("Initial:  F_A=%.4f, F_B=%.4f\n", fa_init, fb_init);

        /* Find Nash equilibrium */
        int iters = nkcs_find_nash_equilibrium(nkcs, ga, gb, 100, &rng_state);
        double fa_eq = nkcs_fitness_a(nkcs, ga, gb);
        double fb_eq = nkcs_fitness_b(nkcs, ga, gb);
        printf("Nash eq:  F_A=%.4f, F_B=%.4f (converged in %d iters)\n",
               fa_eq, fb_eq, iters);

        /* Red Queen measure */
        double rq = nkcs_red_queen_measure(nkcs, 100, &rng_state);
        printf("Red Queen measure: %.4f (fraction of compensatory steps)\n", rq);

        /* Mutual information */
        double mi = nkcs_mutual_information(nkcs, 200, &rng_state);
        printf("Mutual information: %.4f (coevolutionary coupling)\n", mi);

        nk_genotype_free(ga);
        nk_genotype_free(gb);
        nkcs_free(nkcs);
    }

    /* === Wrap-up === */
    print_section("Demo Complete");
    printf("Key takeaways:\n");
    printf("  1. K controls landscape ruggedness (K=0 smooth, K=N-1 random)\n");
    printf("  2. The 'edge of chaos' at K/N ~ 0.2-0.5 enables complex adaptation\n");
    printf("  3. Different adaptive walk strategies reach different local optima\n");
    printf("  4. Complexity catastrophe: higher K lowers mean optimum fitness\n");
    printf("  5. Coevolution creates Red Queen dynamics and Nash equilibria\n");
    printf("\nReference: Kauffman, S.A. (1993). The Origins of Order.\n");

    nk_genotype_free(g);
    nk_landscape_free(land);

    return 0;
}
