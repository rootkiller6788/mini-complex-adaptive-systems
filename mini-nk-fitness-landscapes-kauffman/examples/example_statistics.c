#include "nk_core.h"
#include "nk_landscape.h"
#include "nk_statistics.h"
#include "nk_epistasis.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * Example 3: Landscape Statistics and Epistasis Analysis
 *
 * Demonstrates L4-L6: verifies the complexity catastrophe,
 * computes autocorrelation functions, and analyzes epistasis.
 *
 * Weinberger (1990) prediction: rho(d) = (1 - d/N)^K
 * Kauffman (1993) prediction: mean optimum fitness decreases with K
 * ============================================================================ */

int main(void) {
    printf("=== NK Landscape Statistics and Epistasis Analysis ===\n\n");

    int N = 14;

    /* --- Complexity Catastrophe --- */
    printf("--- Complexity Catastrophe (Kauffman 1993, Ch. 4) ---\n");
    printf("N=%d, varying K from 0 to %d\n\n", N, N-1);
    printf("  K   | Mean Optimum Fitness\n");
    printf("  ----|--------------------\n");

    for (int K = 0; K < N; K++) {
        NKLandscape *land = nk_landscape_create(N, K,
            NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM, 55555 + K * 1000);
        if (!land) continue;

        NKLandscapeStatistics *stats = nk_statistics_compute(land, 300, 30,
                                                              &(uint64_t){77777 + K});
        if (stats) {
            printf("  %-3d | %.6f\n", K, stats->fitness_at_optimum);
            nk_statistics_free(stats);
        }
        nk_landscape_free(land);
    }
    printf("\nObservation: mean optimum fitness generally DECREASES as K increases.\n");
    printf("This is the complexity catastrophe -- more epistasis => lower peaks.\n\n");

    /* --- Autocorrelation Function --- */
    printf("--- Autocorrelation Function rho(d) ---\n");
    printf("N=14, K=2: Weinberger predicts rho(d) = (1 - d/14)^2\n\n");
    printf("  d   | Measured rho(d) | Predicted rho(d)\n");
    printf("  ----|-----------------|-----------------\n");

    NKLandscape *land2 = nk_landscape_create(14, 2,
        NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM, 88888);

    if (land2) {
        double rho_out[15];
        uint64_t rng = 99999;
        nk_autocrelation_full(land2, 200, rho_out, &rng);

        for (int d = 0; d <= 14; d++) {
            double predicted = pow(1.0 - (double)d / 14.0, 2.0);
            printf("  %-3d | %-15.6f | %.6f\n", d, rho_out[d], predicted);
        }
        nk_landscape_free(land2);
    }

    printf("\nObservation: measured autocorrelation approximates Weinberger's formula.\n");
    printf("This confirms the fundamental law of NK landscape correlation decay.\n\n");

    /* --- Sign Epistasis Analysis --- */
    printf("--- Sign Epistasis Analysis ---\n");
    printf("N=10, K=3 landscape:\n");

    NKLandscape *land3 = nk_landscape_create(10, 3,
        NK_INTERACTION_RANDOM, NK_CONTRIB_UNIFORM, 11111);

    if (land3) {
        uint64_t rng = 22222;
        int n_rse = nk_count_reciprocal_sign_epistasis(land3, 20, &rng);
        printf("  Reciprocal sign epistasis pairs found: %d (in 20 samples)\n", n_rse);

        double accessibility = nk_optimum_accessibility(land3, 100, &rng);
        printf("  Global optimum accessibility: %.4f (fraction of starts reaching it)\n",
               accessibility);

        nk_landscape_free(land3);
    }

    printf("\nReciprocal sign epistasis creates ruggedness by blocking evolutionary paths.\n");
    printf("Higher reciprocal sign epistasis => lower optimum accessibility.\n");

    return 0;
}
