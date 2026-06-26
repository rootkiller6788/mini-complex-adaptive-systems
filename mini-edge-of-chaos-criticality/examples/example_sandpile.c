#include "eoc_core.h"
#include "eoc_sandpile.h"
#include "eoc_powerlaw.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * Example 1: BTW Sandpile ? Self-Organized Criticality
 *
 * Demonstrates the Bak-Tang-Wiesenfeld sandpile model reaching a
 * self-organized critical state and producing power-law avalanche
 * distributions.
 *
 * Key concepts:
 *   - SOC: system naturally evolves to critical state without fine-tuning
 *   - Avalanche size distribution: P(s) ~ s^{-tau} with tau ~ 1.1-1.3
 *   - 1/f noise: S(f) ~ f^{-alpha} with alpha ~ 1.0
 * ============================================================================ */

int main(void) {
    printf("=== BTW Sandpile ? Self-Organized Criticality ===\n\n");

    int L = 32;
    int n_grains = 2000;

    printf("Creating %dx%d sandpile, driving with %d grains...\n", L, L, n_grains);
    EOCSandpileModel* sp = eoc_sandpile_create("BTW Demo", L, L, 4.0);
    eoc_sandpile_drive(sp, n_grains, eoc_sandpile_add_grain_btw);

    printf("Total avalanches: %d\n", sp->total_avalanches);
    printf("Recorded avalanches: %d\n", sp->n_avalanches_recorded);

    /* Average avalanche size */
    double avg_size = eoc_sandpile_average_size(sp, n_grains);
    printf("Average avalanche size: %.2f\n", avg_size);

    /* Branching ratio */
    double sigma = eoc_sandpile_branching_ratio(sp, n_grains);
    printf("Branching ratio (sigma): %.3f", sigma);
    if (sigma > 0.9 && sigma < 1.1) {
        printf(" [CRITICAL - sigma ~ 1]\n");
    } else if (sigma < 0.9) {
        printf(" [SUBCRITICAL]\n");
    } else {
        printf(" [SUPERCRITICAL]\n");
    }

    /* Power-law fit to avalanche distribution */
    EOCPowerLawFit fit = eoc_sandpile_fit_powerlaw(sp);
    printf("\nPower-law fit:\n");
    printf("  tau = %.3f +/- %.3f\n", fit.alpha, fit.alpha_std_error);
    printf("  Samples above xmin=%.0f: %d\n", fit.xmin, fit.n_samples);
    printf("  Power-law hypothesis: %s\n", fit.is_power_law ? "ACCEPTED" : "REJECTED");

    /* Avalanche size distribution */
    double* sizes, *probs;
    int n_bins;
    eoc_sandpile_distribution(sp, &sizes, &probs, &n_bins);

    printf("\nAvalanche Size Distribution (log-binned):\n");
    printf("  Size       |  Probability\n");
    printf("  -----------|-------------\n");
    int shown = 0;
    for (int i = 0; i < n_bins && shown < 8; i++) {
        if (probs[i] > 1e-10) {
            printf("  %10.1f |  %.6f\n", sizes[i], probs[i]);
            shown++;
        }
    }

    /* 1/f noise spectrum */
    double *freqs, *power;
    int n_freqs;
    eoc_sandpile_flicker_noise(sp, &freqs, &power, &n_freqs);
    printf("\n1/f Noise Spectrum:\n");
    printf("  Frequency   |  Power\n");
    printf("  ------------|--------\n");
    shown = 0;
    for (int i = 0; i < n_freqs && shown < 6; i++) {
        if (power[i] > 1e-15) {
            printf("  %11.6f | %.4f\n", freqs[i], power[i]);
            shown++;
        }
    }

    /* Check if critical */
    printf("\nCritical state: %s\n",
           eoc_sandpile_is_critical(sp) ? "YES (tau consistent with SOC)" : "NOT CRITICAL");

    free(sizes);
    free(probs);
    free(freqs);
    free(power);
    eoc_sandpile_free(sp);

    printf("\n=== Example Complete ===\n");
    return 0;
}
