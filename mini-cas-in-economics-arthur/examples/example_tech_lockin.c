#include "ace_core.h"
#include "ace_dynamics.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    unsigned int seed = 456;
    printf("=== Technology Lock-in under Increasing Returns ===
");
    printf("(Arthur 1989: Competing Technologies and Lock-in)

");
    int n_steps = 200;
    printf("Simulating adoption competition between Tech A and Tech B
");
    printf("over %d adoption decisions...

", n_steps);
    ACEAdoptionState* state = ace_adoption_state_alloc(2);
    ACEPathHistory* hist = ace_path_history_alloc(n_steps, 2);
    printf("%-6s %10s %10s %10s
", "Step", "Share A", "Share B", "Winner");
    printf("------ ---------- ---------- ----------
");
    for (int s = 0; s < n_steps; s++) {
        ace_adoption_step_binary(state, 1.0, 1.0, 2.0, 2.0, &seed);
        hist->market_shares[s*2] = state->shares[0];
        hist->market_shares[s*2+1] = state->shares[1];
        hist->time_points[s] = state->time;
        if (s % 20 == 0 || s == n_steps - 1) {
            const char* winner = (state->shares[0] > 0.5) ? "Tech A" : "Tech B";
            printf("%-6d %10.4f %10.4f %10s
",
                   s + 1, state->shares[0], state->shares[1], winner);
        }
    }
    hist->n_steps = n_steps;
    printf("
--- Lock-in Analysis ---
");
    ACELockState ls = ace_detect_lockin(hist, 0.7, 10);
    const char* ls_str[] = {"Not Locked", "Partial Lock", "Full Lock", "Lock Escape"};
    printf("Lock state: %s
", ls_str[ls]);
    double final_share_a = state->shares[0];
    printf("Final share of Tech A: %.4f
", final_share_a);
    printf("Final share of Tech B: %.4f
", 1.0 - final_share_a);
    printf("
Explanation: Under increasing returns (gamma=2.0),
");
    printf("small random advantages in early adoption compound.
");
    printf("The market eventually locks into one technology.
");
    printf("Which technology wins depends on stochastic early history --
");
    printf("this is path dependence in action.
");
    ace_path_history_free(hist);
    ace_adoption_state_free(state);
    return 0;
}
