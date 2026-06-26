#include "nk_core.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * Example 2: Adaptive Walks on Rugged Landscapes
 *
 * Demonstrates L5: compares four adaptive walk strategies on the
 * same NK landscape. Shows how different search strategies lead
 * to different local optima.
 *
 * Key result: steepest ascent typically reaches higher fitness
 * optima but at higher computational cost per step.
 * Random-better explores more diverse optima.
 * ============================================================================ */

int main(void) {
    printf("=== Adaptive Walks on NK Fitness Landscape ===\n\n");

    int N = 12, K = 3;
    printf("Landscape: N=%d, K=%d (critical regime)\n\n", N, K);

    NKLandscape *land = nk_landscape_create(N, K,
        NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM, 12345);

    if (!land) {
        printf("Failed to create landscape.\n");
        return 1;
    }

    /* Use the same starting genotype for all strategies */
    NKGenotype *start = nk_genotype_create(N);
    uint64_t rng = 99999;
    nk_genotype_randomize(start, &rng);

    char *start_str = nk_genotype_to_string(start);
    printf("Starting genotype: %s\n", start_str);
    printf("Starting fitness:  %.6f\n\n", nk_fitness(land, start));
    free(start_str);

    /* Strategy names */
    const char *strat_names[] = {
        "Greedy (first improvement)",
        "Steepest Ascent (best improvement)",
        "Next Ascent (with memory)",
        "Random Better (drift among improvements)"
    };

    NKWalkStrategy strategies[] = {
        NK_WALK_GREEDY, NK_WALK_STEEPEST_ASCENT,
        NK_WALK_NEXT_ASCENT, NK_WALK_RANDOM_BETTER
    };

    NKWalkResult* (*walk_funcs[])(const NKLandscape*, const NKGenotype*, int) = {
        nk_walk_greedy, nk_walk_steepest_ascent,
        nk_walk_next_ascent, nk_walk_random_better
    };

    int n_strategies = 4;

    for (int s = 0; s < n_strategies; s++) {
        printf("--- %s ---\n", strat_names[s]);
        NKWalkResult *wr = walk_funcs[s](land, start, 200);

        if (!wr) {
            printf("  Walk failed.\n\n");
            continue;
        }

        printf("  Steps taken:     %d\n", wr->steps);
        printf("  Reached optimum: %s\n", wr->reached_optimum ? "yes" : "no");
        printf("  Initial fitness: %.6f\n", wr->path[0]->fitness);
        printf("  Final fitness:   %.6f\n", wr->path[wr->steps]->fitness);
        printf("  Fitness gain:    %.6f\n",
               wr->path[wr->steps]->fitness - wr->path[0]->fitness);

        char *final_str = nk_genotype_to_string(wr->path[wr->steps]->genotype);
        printf("  Final genotype:  %s\n", final_str);
        free(final_str);

        int best_step = nk_walk_best_step(wr);
        printf("  Best step:       %d (fitness=%.6f)\n",
               best_step, wr->path[best_step]->fitness);

        nk_walk_result_free(wr);
        printf("\n");
    }

    /* Compare: do different strategies reach different optima? */
    printf("--- Optimum Diversity ---\n");
    for (int s = 0; s < n_strategies; s++) {
        uint64_t rng2 = 10000 + s * 1000;
        int n_distinct = nk_walk_count_distinct_optima(land, 50, strategies[s], &rng2);
        printf("  %s: %d distinct optima in 50 walks\n", strat_names[s], n_distinct);
    }

    nk_genotype_free(start);
    nk_landscape_free(land);

    return 0;
}
