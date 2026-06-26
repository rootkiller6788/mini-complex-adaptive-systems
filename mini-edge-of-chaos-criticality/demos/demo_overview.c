#include "eoc_core.h"
#include "eoc_sandpile.h"
#include "eoc_dynamics.h"
#include "eoc_cellular.h"
#include "eoc_fractal.h"
#include "eoc_criticality.h"
#include "eoc_powerlaw.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * Demo Overview: Edge of Chaos & Criticality
 *
 * A whirlwind tour of key concepts:
 *   1. Self-Organized Criticality (BTW Sandpile)
 *   2. Chaos and Bifurcation (Logistic Map)
 *   3. Cellular Automata at Edge of Chaos (Wolfram Classes)
 *   4. Fractal Dimension Analysis
 *   5. Universality and Critical Exponents
 * ============================================================================ */

int main(void) {
    printf("============================================================\n");
    printf("  EDGE OF CHAOS & CRITICALITY ? Demonstration Suite\n");
    printf("============================================================\n");
    printf("\nKey concepts from:\n");
    printf("  - Bak, Tang, Wiesenfeld (1987): Self-Organized Criticality\n");
    printf("  - Langton (1990): Computation at the Edge of Chaos\n");
    printf("  - Wolfram (1984): Universality in Cellular Automata\n");
    printf("  - Stanley (1987): Phase Transitions & Critical Phenomena\n");
    printf("  - Wilson (1971): Renormalization Group\n");
    printf("  - Clauset, Shalizi, Newman (2009): Power-Law Fitting\n");
    printf("\n");

    /* Quick Sandpile Demo */
    printf("--- 1. Self-Organized Criticality (BTW Sandpile) ---\n");
    EOCSandpileModel* sp = eoc_sandpile_create("demo", 16, 16, 4.0);
    eoc_sandpile_drive(sp, 500, eoc_sandpile_add_grain_btw);
    EOCPowerLawFit fit = eoc_sandpile_fit_powerlaw(sp);
    double sigma = eoc_sandpile_branching_ratio(sp, 500);
    printf("  Avalanches: %d, tau=%.3f, sigma=%.3f, critical: %s\n",
           sp->total_avalanches, fit.alpha, sigma,
           eoc_sandpile_is_critical(sp) ? "YES" : "NO");
    eoc_sandpile_free(sp);

    /* Quick Logistic Demo */
    printf("\n--- 2. Chaos & Period-Doubling (Logistic Map) ---\n");
    struct { double r; const char* label; } demos[] = {
        {2.9, "Fixed point"}, {3.2, "Period-2"}, {3.5, "Period-4"},
        {3.57, "Chaos onset"}, {3.9, "Chaos"}
    };
    for (int i = 0; i < 5; i++) {
        double* traj = eoc_logistic_trajectory(0.5, demos[i].r, 100, 100);
        double lam = eoc_lyapunov_rosenstein(traj, 100, 2, 1, 10);
        double K = eoc_zero_one_test(traj, 100, 1.7);
        printf("  r=%.2f (%s): lyap=%+.4f, 0-1 test K=%.3f\n",
               demos[i].r, demos[i].label, lam, K);
        free(traj);
    }

    /* Quick CA Demo */
    printf("\n--- 3. Cellular Automata Classes ---\n");
    int ca_rules[] = {0, 50, 30, 110};
    const char* ca_names[] = {"Rule 0", "Rule 50", "Rule 30", "Rule 110"};
    for (int i = 0; i < 4; i++) {
        WolframClass wc = eoc_ca_classify_1d(ca_rules[i], 32, 50);
        double lam = eoc_ca_lambda_1d(ca_rules[i], 0);
        printf("  %s: lambda=%.3f, class=%d\n", ca_names[i], lam, (int)wc);
    }

    /* Quick Universality Demo */
    printf("\n--- 4. Universality Classes ---\n");
    UniversalityClass uc_list[] = {UNIV_ISING_2D, UNIV_ISING_3D, UNIV_MEAN_FIELD, UNIV_BTW, UNIV_MANNA};
    for (int i = 0; i < 5; i++) {
        EOCCriticalExponents ce = eoc_universality_exponents(uc_list[i]);
        printf("  %s: d=%d\n", eoc_universality_name(uc_list[i]), ce.spatial_dimension);
    }

    /* Quick Fractal Demo */
    printf("\n--- 5. Fractal Dimension ---\n");
    double ts_wn[300];
    for (int i = 0; i < 300; i++) ts_wn[i] = (double)rand() / RAND_MAX;
    double D_hig = eoc_higuchi_fractal_dimension(ts_wn, 300, 15);
    double D_dfa = eoc_dfa_exponent(ts_wn, 300, 4, 64);
    printf("  White noise: Higuchi D=%.3f, DFA alpha=%.3f\n", D_hig, D_dfa);

    /* Quick RG Demo */
    printf("\n--- 6. Renormalization Group ---\n");
    double Kf = eoc_rg_decimation_1d_ising(1.0, 5);
    printf("  1D Ising RG: K=1.0 -> K'=%f (flows to disordered)\n", Kf);

    printf("\n============================================================\n");
    printf("  Module covers L1-L6 Complete, L7-L9 Partial\n");
    printf("============================================================\n");
    return 0;
}
