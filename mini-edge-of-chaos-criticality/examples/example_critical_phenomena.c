#include "eoc_core.h"
#include "eoc_fractal.h"
#include "eoc_criticality.h"
#include "eoc_powerlaw.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * Example 4: Critical Phenomena ? Fractals, DFA, and Universality
 *
 * Demonstrates:
 *   - Fractal dimension measurement (box-counting, Higuchi, DFA)
 *   - Detrended Fluctuation Analysis for long-range correlations
 *   - Universality classes and critical exponents
 *   - Scaling relations verification
 *   - Renormalization group flow
 * ============================================================================ */

int main(void) {
    printf("=== Critical Phenomena & Fractal Analysis ===\n\n");

    /* Part 1: Box-Counting Dimension */
    printf("--- Fractal Dimension: Box-Counting on Binary Patterns ---\n");

    /* Sierpinski carpet approximation */
    int N = 64;
    int* carpet = (int*)calloc(N * N, sizeof(int));
    /* Generate a simple fractal pattern (checkerboard-like) */
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int level = 1;
            int px = x, py = y;
            while (px > 0 || py > 0) {
                if (px % 3 == 1 && py % 3 == 1) { carpet[y * N + x] = 0; break; }
                px /= 3; py /= 3;
                level++;
            }
            carpet[y * N + x] = 1;
        }
    }
    /* Actually, let's use a simpler pattern: filled square (D=2) and line (D=1) */
    int* square = (int*)calloc(N * N, sizeof(int));
    int* line = (int*)calloc(N * N, sizeof(int));
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            square[y * N + x] = (x > 10 && x < 54 && y > 10 && y < 54) ? 1 : 0;
            line[y * N + x] = (y == 32) ? 1 : 0;
        }
    }

    double D_square = eoc_box_counting_2d(square, N, N);
    double D_line = eoc_box_counting_2d(line, N, N);
    printf("  Solid square (expected D~2.0): D0 = %.3f\n", D_square);
    printf("  Horizontal line (expected D~1.0): D0 = %.3f\n", D_line);

    free(carpet);
    free(square);
    free(line);

    /* Part 2: Higuchi Fractal Dimension for Time Series */
    printf("\n--- Higuchi Fractal Dimension ---\n");

    /* Test signals with known dimensions */
    struct { const char* name; double (*gen)(int i, int n); } signals[] = {
        {"White noise", NULL},
        {"Sine wave",   NULL},
        {NULL, NULL}
    };

    /* White noise */
    int n_pts = 300;
    double* ts_wn = (double*)malloc(n_pts * sizeof(double));
    for (int i = 0; i < n_pts; i++) ts_wn[i] = (double)rand() / RAND_MAX;
    double D_wn = eoc_higuchi_fractal_dimension(ts_wn, n_pts, 15);
    printf("  White noise: D = %.3f (fractal time series)\n", D_wn);

    /* Sine wave */
    for (int i = 0; i < n_pts; i++) ts_wn[i] = sin(2.0 * 3.14159265 * i / 30.0);
    double D_sine = eoc_higuchi_fractal_dimension(ts_wn, n_pts, 15);
    printf("  Sine wave: D = %.3f (smooth ~1.0)\n", D_sine);

    free(ts_wn);

    /* Part 3: Detrended Fluctuation Analysis (DFA) */
    printf("\n--- Detrended Fluctuation Analysis (DFA) ---\n");
    printf("(alpha ~ 0.5: white noise, ~ 1.0: 1/f noise, ~ 1.5: Brownian)\n");

    int n_dfa = 512;
    double* dfa_ts = (double*)malloc(n_dfa * sizeof(double));

    /* White noise */
    for (int i = 0; i < n_dfa; i++) dfa_ts[i] = (double)rand() / RAND_MAX - 0.5;
    double alpha_wn = eoc_dfa_exponent(dfa_ts, n_dfa, 4, 128);
    printf("  White noise: alpha = %.3f\n", alpha_wn);

    /* Integrated white noise (Brownian motion) */
    double sum = 0.0;
    for (int i = 0; i < n_dfa; i++) {
        sum += (double)rand() / RAND_MAX - 0.5;
        dfa_ts[i] = sum * 0.1;
    }
    double alpha_bm = eoc_dfa_exponent(dfa_ts, n_dfa, 4, 128);
    printf("  Brownian motion: alpha = %.3f\n", alpha_bm);

    free(dfa_ts);

    /* Part 4: Universality Classes */
    printf("\n--- Universality Classes & Critical Exponents ---\n");
    printf("Class              | beta   | gamma  | nu     | eta    | alpha\n");
    printf("-------------------|--------|--------|--------|--------|--------\n");

    UniversalityClass classes[] = {
        UNIV_ISING_2D, UNIV_ISING_3D, UNIV_MEAN_FIELD,
        UNIV_XY, UNIV_PERCOLATION_2D
    };
    for (int i = 0; i < 5; i++) {
        EOCCriticalExponents ce = eoc_universality_exponents(classes[i]);
        printf("%-18s | %6.3f | %6.3f | %6.3f | %6.3f | %6.3f\n",
               eoc_universality_name(classes[i]),
               ce.beta_b, ce.gamma, ce.nu, ce.eta, ce.alpha);
    }

    /* Part 5: Scaling Relations Check */
    printf("\n--- Scaling Relations Verification ---\n");

    EOCCriticalExponents ce_2d = eoc_universality_exponents(UNIV_ISING_2D);
    printf("2D Ising:\n");
    printf("  Rushbrooke (alpha+2beta+gamma=2): %.6f (discrepancy=%.6f)\n",
           ce_2d.alpha + 2.0*ce_2d.beta_b + ce_2d.gamma,
           eoc_scaling_rushbrooke(&ce_2d));
    printf("  Widom (gamma=beta*(delta-1)): discrepancy=%.6f\n",
           eoc_scaling_widom(&ce_2d));
    printf("  Fisher (nu*(2-eta)=gamma): discrepancy=%.6f\n",
           eoc_scaling_fisher(&ce_2d));

    EOCCriticalExponents ce_3d = eoc_universality_exponents(UNIV_ISING_3D);
    printf("\n3D Ising:\n");
    printf("  Rushbrooke discrepancy: %.6f\n", eoc_scaling_rushbrooke(&ce_3d));
    printf("  Widom discrepancy: %.6f\n", eoc_scaling_widom(&ce_3d));
    printf("  Fisher discrepancy: %.6f\n", eoc_scaling_fisher(&ce_3d));

    /* Part 6: Renormalization Group Flow */
    printf("\n--- Renormalization Group Flow (1D Ising) ---\n");
    printf("  K_initial -> K_after_RG (K' = 0.5*ln(cosh(2K)))");
    printf("\n  K     |  K'     |  Flow\n");
    printf("  ------|---------|-------\n");

    double K_vals[] = {0.1, 0.5, 1.0, 2.0, 3.0};
    for (int i = 0; i < 5; i++) {
        double Kp = 0.5 * log(cosh(2.0 * K_vals[i]));
        printf("  %.2f  |  %.4f  |  %s\n", K_vals[i], Kp,
               Kp < K_vals[i] ? "-> K=0 (disordered)" : "-> K=inf");
    }

    printf("\n  The flow always goes to K=0 (disordered fixed point),\n");
    printf("  confirming that there is NO phase transition in 1D Ising\n");
    printf("  (Peierls argument).\n");

    /* Part 7: Correlation Function */
    printf("\n--- Spin Correlation Function ---\n");
    int n_spins = 100;
    int* spins = (int*)malloc(n_spins * sizeof(int));
    /* Random (high-T) configuration */
    for (int i = 0; i < n_spins; i++) spins[i] = (rand() % 2) ? 1 : -1;
    double* G_r = eoc_correlation_function(spins, n_spins, 20);
    printf("  Random (T>>Tc): G(r) ~ exp(-r/xi)\n");
    printf("  r  |  G(r)\n");
    printf("  ---|-------\n");
    for (int r = 0; r < 10; r++) {
        printf("  %2d | %+.5f\n", r+1, G_r[r]);
    }
    double xi = eoc_correlation_length(G_r, 20);
    printf("  Correlation length xi ~ %.2f\n", xi);
    free(G_r);
    free(spins);

    /* Part 8: Power-law goodness-of-fit */
    printf("\n--- Power-Law Fit Demonstration ---\n");
    double* pl_data = eoc_powerlaw_generate_continuous(1000, 1.0, 2.5);
    EOCPowerLawFit fit = eoc_powerlaw_fit_continuous(pl_data, 1000, 1.0);
    printf("  Generated alpha=2.5, fitted alpha=%.3f +/- %.3f\n",
           fit.alpha, fit.alpha_std_error);
    printf("  KS statistic: %.4f\n", fit.ks_statistic);

    /* Compare with exponential */
    double p_val;
    double logR = eoc_powerlaw_vs_exponential(pl_data, 1000, 1.0, &p_val);
    printf("  vs Exponential: log(R)=%.2f (%.2f favors power-law)\n",
           logR, logR > 0 ? 1.0 : 0.0);
    free(pl_data);

    printf("\n=== Example Complete ===\n");
    return 0;
}
