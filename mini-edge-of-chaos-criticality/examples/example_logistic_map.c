#include "eoc_core.h"
#include "eoc_dynamics.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * Example 2: Logistic Map ? Route to Chaos via Period-Doubling
 *
 * Demonstrates the canonical period-doubling route to chaos in the
 * logistic map: x_{n+1} = r * x_n * (1 - x_n).
 *
 * Key concepts:
 *   - Bifurcation diagram revealing period-doubling cascade
 *   - Feigenbaum universal constants (delta ~ 4.669, alpha ~ 2.502)
 *   - Lyapunov exponent as chaos indicator
 *   - Strange attractors in continuous systems (Lorenz)
 * ============================================================================ */

int main(void) {
    printf("=== Logistic Map ? Period-Doubling Route to Chaos ===\n\n");

    /* Part 1: Bifurcation diagram */
    printf("--- Bifurcation Diagram (r in [2.5, 4.0]) ---\n");

    EOCBifurcationPoint* points = NULL;
    int n_points = 0;
    eoc_bifurcation_diagram(2.5, 4.0, 60, 500, 100, &points, &n_points);

    printf("r       | period | lyapunov  | regime\n");
    printf("--------|--------|-----------|---------\n");
    int shown = 0;
    for (int i = 0; i < n_points; i += 4) {
        if (shown >= 15) break;
        const char* regime;
        if (points[i].period == 1) regime = "Fixed point";
        else if (points[i].period == 2) regime = "Period-2";
        else if (points[i].period == 4) regime = "Period-4";
        else if (points[i].period == 8) regime = "Period-8";
        else if (points[i].is_chaotic) regime = "CHAOTIC";
        else regime = "Complex";

        printf("%.4f  | %-6d | %+.6f | %s\n",
               points[i].parameter, points[i].period,
               points[i].lyapunov, regime);
        shown++;
    }

    /* Part 2: Trajectories at specific r values */
    printf("\n--- Sample Trajectories ---\n");

    struct { double r; const char* label; } cases[] = {
        {2.8, "Fixed point (r=2.8)"},
        {3.2, "Period-2 (r=3.2)"},
        {3.5, "Period-4 (r=3.5)"},
        {3.9, "Chaos (r=3.9)"},
        {0, NULL}
    };

    for (int c = 0; cases[c].label; c++) {
        double* traj = eoc_logistic_trajectory(0.5, cases[c].r, 200, 20);
        printf("\n%s:\n  ", cases[c].label);
        for (int i = 0; i < 20; i++) {
            printf("%.4f ", traj[i]);
        }
        printf("\n");
        free(traj);
    }

    /* Part 3: Lyapunov exponent scan */
    printf("\n--- Lyapunov Exponent vs. r ---\n");
    printf("r       | lambda\n");
    printf("--------|--------\n");
    double r_vals[] = {2.8, 3.2, 3.5, 3.57, 3.7, 3.83, 3.9, 4.0};
    for (int i = 0; i < 8; i++) {
        double* traj = eoc_logistic_trajectory(0.5, r_vals[i], 200, 500);
        double lam = eoc_lyapunov_rosenstein(traj, 500, 3, 1, 20);
        printf("%.4f  | %+.6f %s\n", r_vals[i], lam,
               lam > 0.01 ? "<-- CHAOTIC" : "<-- ORDERED");
        free(traj);
    }

    /* Part 4: 0-1 Test for Chaos */
    printf("\n--- 0-1 Test for Chaos ---\n");
    for (int i = 0; i < 8; i++) {
        double* traj = eoc_logistic_trajectory(0.5, r_vals[i], 200, 500);
        double K = eoc_zero_one_test(traj, 500, 1.7);
        printf("r=%.2f: K=%.4f [%s]\n", r_vals[i], K,
               K > 0.5 ? "CHAOTIC" : "REGULAR");
        free(traj);
    }

    /* Part 5: Lorenz Attractor */
    printf("\n--- Lorenz Attractor (sigma=10, rho=28, beta=8/3) ---\n");
    double lorenz_state[3] = {1.0, 1.0, 1.0};
    double lorenz_deriv[3];

    printf("t     | x        | y        | z\n");
    printf("------|----------|----------|----------\n");
    for (int t = 0; t <= 20; t++) {
        eoc_lorenz(lorenz_state, lorenz_deriv, 10.0, 28.0, 8.0/3.0);
        for (int d = 0; d < 3; d++) lorenz_state[d] += lorenz_deriv[d] * 0.01;
        if (t % 5 == 0) {
            printf("%.2f  | %+.6f | %+.6f | %+.6f\n",
                   0.01 * t, lorenz_state[0], lorenz_state[1], lorenz_state[2]);
        }
    }

    /* Part 6: Feigenbaum Constants */
    printf("\n--- Feigenbaum Universal Constants ---\n");
    double r_bif[] = {3.0, 3.44949, 3.54409, 3.5644, 3.5688, 3.5697};
    double delta = eoc_feigenbaum_delta(r_bif, 6);
    double alpha = eoc_feigenbaum_alpha(r_bif, 0.5, 6);
    printf("Feigenbaum delta: %.6f (theoretical: 4.6692016)\n", delta);
    printf("Feigenbaum alpha: %.6f (theoretical: 2.5029078)\n", alpha);

    /* Cleanup */
    for (int i = 0; i < n_points; i++) eoc_bifurcation_free(&points[i]);
    free(points);

    printf("\n=== Example Complete ===\n");
    return 0;
}
