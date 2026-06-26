#include "eoc_core.h"
#include "eoc_sandpile.h"
#include "eoc_dynamics.h"
#include "eoc_powerlaw.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================================
 * Performance Benchmarks for Edge of Chaos & Criticality
 * ============================================================================ */

static double wall_time(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== Performance Benchmarks ===\n\n");

    double t_start, t_end;

    /* Benchmark 1: Sandpile Drive */
    printf("1. BTW Sandpile (64x64, 10000 grains)...\n");
    EOCSandpileModel* sp = eoc_sandpile_create("bench", 64, 64, 4.0);
    t_start = wall_time();
    eoc_sandpile_drive(sp, 10000, eoc_sandpile_add_grain_btw);
    t_end = wall_time();
    printf("   Time: %.3f s, avalanches: %d\n", t_end - t_start, sp->total_avalanches);
    eoc_sandpile_free(sp);

    /* Benchmark 2: Logistic Trajectory */
    printf("2. Logistic Map (10000 iterations)...\n");
    t_start = wall_time();
    double* traj = eoc_logistic_trajectory(0.5, 3.9, 100, 10000);
    t_end = wall_time();
    printf("   Time: %.6f s\n", t_end - t_start);
    free(traj);

    /* Benchmark 3: Bifurcation Diagram */
    printf("3. Bifurcation Diagram (200 r values)...\n");
    EOCBifurcationPoint* pts = NULL;
    int np = 0;
    t_start = wall_time();
    eoc_bifurcation_diagram(2.5, 4.0, 200, 200, 100, &pts, &np);
    t_end = wall_time();
    printf("   Time: %.3f s, %d points\n", t_end - t_start, np);
    for (int i = 0; i < np; i++) eoc_bifurcation_free(&pts[i]);
    free(pts);

    /* Benchmark 4: Power-Law Fitting */
    printf("4. Power-law MLE (10000 samples)...\n");
    double* data = eoc_powerlaw_generate_continuous(10000, 1.0, 2.5);
    t_start = wall_time();
    EOCPowerLawFit fit = eoc_powerlaw_fit_continuous(data, 10000, 1.0);
    t_end = wall_time();
    printf("   Time: %.6f s, alpha=%.3f\n", t_end - t_start, fit.alpha);
    free(data);

    /* Benchmark 5: Lyapunov Exponent */
    printf("5. Lyapunov Exponent (Rosenstein, N=2000)...\n");
    double* ts = eoc_logistic_trajectory(0.5, 4.0, 100, 2000);
    t_start = wall_time();
    double lam = eoc_lyapunov_rosenstein(ts, 2000, 3, 1, 20);
    t_end = wall_time();
    printf("   Time: %.3f s, lambda=%.4f\n", t_end - t_start, lam);
    free(ts);

    /* Benchmark 6: DFA */
    printf("6. DFA (N=1024)...\n");
    double* dfa_ts = (double*)malloc(1024 * sizeof(double));
    for (int i = 0; i < 1024; i++) dfa_ts[i] = (double)rand() / RAND_MAX;
    t_start = wall_time();
    double alpha = eoc_dfa_exponent(dfa_ts, 1024, 4, 256);
    t_end = wall_time();
    printf("   Time: %.6f s, alpha=%.3f\n", t_end - t_start, alpha);
    free(dfa_ts);

    printf("\n=== Benchmarks Complete ===\n");
    return 0;
}
