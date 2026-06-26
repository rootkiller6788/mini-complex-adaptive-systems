#include "ace_core.h"
#include "ace_dynamics.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    unsigned int seed = 123;
    printf("=== Polya Urn: Increasing Returns and Path Dependence ===
");
    printf("(Arthur, Ermoliev, Kaniovski 1987)

");
    int n_colors = 2;
    int init[] = {1, 1};
    int n_draws = 1000;
    int n_trials = 5;
    printf("Running %d trials of %d draws each...

", n_trials, n_draws);
    for (int trial = 0; trial < n_trials; trial++) {
        ACEPolyaUrn* urn = ace_polya_urn_alloc(n_colors, init, 1.0);
        ACEPathHistory* hist = ace_path_history_alloc(n_draws, n_colors);
        for (int d = 0; d < n_draws; d++) {
            ace_polya_urn_draw(urn, &seed);
            double total = (double)urn->total_balls;
            hist->market_shares[d * n_colors] =
                (double)urn->current_balls[0] / total;
            hist->market_shares[d * n_colors + 1] =
                (double)urn->current_balls[1] / total;
            hist->time_points[d] = (double)(d + 1);
        }
        hist->n_steps = n_draws;
        double final_share = hist->market_shares[(n_draws-1) * n_colors];
        printf("  Trial %d: final share of A = %.4f (B = %.4f)
",
               trial + 1, final_share, 1.0 - final_share);
        ace_path_history_free(hist);
        ace_polya_urn_free(urn);
    }
    printf("
With gamma > 1 (increasing returns), the urn locks in
");
    printf("to either color A or B depending on early random draws.
");
    printf("This is path dependence: history matters.
");
    printf("
Now computing urn function f(p) for various gamma:
");
    for (double gamma = 0.5; gamma <= 3.0; gamma += 0.5) {
        printf("  gamma=%.1f: f(0.3)=%.4f, f(0.5)=%.4f, f(0.7)=%.4f
",
               gamma,
               ace_arthur_urn_function(0.3, gamma),
               ace_arthur_urn_function(0.5, gamma),
               ace_arthur_urn_function(0.7, gamma));
    }
    printf("
Gamma > 1: f(p) > p for p > 0.5 -> self-reinforcing -> lock-in
");
    printf("Gamma < 1: f(p) crosses identity -> stable equilibrium at 0.5
");
    return 0;
}
