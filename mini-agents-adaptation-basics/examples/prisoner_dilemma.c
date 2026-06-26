/**
 * prisoner_dilemma.c — Iterated Prisoner's Dilemma Tournament
 *
 * Axelrod-style round-robin tournament of PD strategies demonstrating
 * the emergence of cooperation in repeated interactions.
 *
 * Knowledge: L6 Canonical Problem — Prisoner's Dilemma
 *            L7 Application — Axelrod tournament / evolution of cooperation
 */

#include "game_theory.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== Iterated Prisoner's Dilemma Tournament ===\n\n");

    /* Standard PD: T=5, R=3, P=1, S=0 */
    NormalFormGame *pd = nfg_prisoners_dilemma(5.0, 3.0, 1.0, 0.0);

    PDStrategy strategies[] = {
        pd_tit_for_tat,
        pd_always_defect,
        pd_always_cooperate,
        pd_grim_trigger,
        pd_generous_tit_for_tat,
        pd_pavlov,
        pd_random_strategy,
        pd_two_tits_for_tat
    };

    const char *names[] = {
        "Tit-for-Tat",
        "Always Defect",
        "Always Cooperate",
        "Grim Trigger",
        "Generous TFT",
        "Pavlov (WSLS)",
        "Random",
        "Two-Tits-for-Tat"
    };

    int n_strategies = 8;
    int n_rounds = 100;

    printf("Strategies competing in %d-round matches:\n", n_rounds);
    for (int i = 0; i < n_strategies; i++) {
        printf("  %d. %s\n", i+1, names[i]);
    }
    printf("\n");

    /* Run round-robin tournament */
    double scores[8] = {0};
    pd_round_robin(pd, strategies, names, n_strategies, n_rounds, scores);

    printf("=== Tournament Results (total scores) ===\n");
    /* Sort and display */
    int order[8] = {0,1,2,3,4,5,6,7};
    for (int i = 0; i < n_strategies; i++) {
        for (int j = i + 1; j < n_strategies; j++) {
            if (scores[order[j]] > scores[order[i]]) {
                int tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }

    for (int i = 0; i < n_strategies; i++) {
        int idx = order[i];
        printf("  %d. %-20s: %.1f\n", i+1, names[idx], scores[idx]);
    }

    /* Head-to-head analysis: TFT vs ALLD */
    printf("\n--- Head-to-Head: TFT vs ALLD ---\n");
    double tft_score, alld_score;
    pd_tournament(pd, pd_tit_for_tat, pd_always_defect, 100,
                   &tft_score, &alld_score);
    printf("  Tit-for-Tat: %.1f\n", tft_score);
    printf("  Always Defect: %.1f\n", alld_score);
    printf("  (ALLD exploits TFT on first round, then both defect)\n");

    /* TFT vs TFT: mutual cooperation */
    printf("\n--- Head-to-Head: TFT vs TFT ---\n");
    double tft1, tft2;
    pd_tournament(pd, pd_tit_for_tat, pd_tit_for_tat, 100, &tft1, &tft2);
    printf("  TFT(1): %.1f, TFT(2): %.1f\n", tft1, tft2);
    printf("  (Mutual cooperation sustained indefinitely)\n");

    /* TFT vs Pavlov */
    printf("\n--- Head-to-Head: TFT vs Pavlov ---\n");
    double tft_p, pav_p;
    pd_tournament(pd, pd_tit_for_tat, pd_pavlov, 100, &tft_p, &pav_p);
    printf("  Tit-for-Tat: %.1f\n", tft_p);
    printf("  Pavlov: %.1f\n", pav_p);

    /* Simulation of an actual match with detailed history */
    printf("\n--- Detailed Match: TFT vs Grim Trigger (first 10 rounds) ---\n");
    PDHistory *h1 = pd_history_create(100);
    PDHistory *h2 = pd_history_create(100);
    for (int r = 0; r < 10; r++) {
        PDAction a1, a2;
        double p1, p2;
        pd_play_round(pd, pd_tit_for_tat, pd_grim_trigger,
                       h1, h2, &a1, &a2, &p1, &p2);
        printf("  Round %2d: TFT=%s, Grim=%s -> payoffs=(%.0f,%.0f)\n",
               r+1,
               a1 == PD_COOPERATE ? "C" : "D",
               a2 == PD_COOPERATE ? "C" : "D",
               p1, p2);
    }
    pd_history_destroy(h1);
    pd_history_destroy(h2);

    /* Replicator dynamics of PD */
    printf("\n--- Replicator Dynamics of PD ---\n");
    double pd_matrix[] = {3.0, 0.0, 5.0, 1.0}; /* C and D payoffs */
    double init_freq[] = {0.7, 0.3}; /* 70% cooperators initially */
    ReplicatorState *rs = replicator_create(2, pd_matrix, init_freq);

    printf("  Initial: C=%.2f, D=%.2f\n", rs->frequencies[0], rs->frequencies[1]);
    for (int t = 0; t < 50; t++) {
        replicator_step(rs, 0.1);
        if (t % 10 == 9) {
            printf("  t=%2d: C=%.3f, D=%.3f, avg_fit=%.3f\n",
                   t+1, rs->frequencies[0], rs->frequencies[1],
                   replicator_average_fitness(rs));
        }
    }
    printf("  Final:  C=%.3f, D=%.3f\n",
           rs->frequencies[0], rs->frequencies[1]);
    printf("  (Defection evolves to fixation in the one-shot PD)\n");

    /* Check ESS */
    bool ess_d = replicator_is_ess(rs, 1, 0.01);
    printf("  Is Defection an ESS? %s\n", ess_d ? "Yes" : "No");

    replicator_destroy(rs);
    nfg_destroy(pd);

    printf("\n=== Prisoner's Dilemma Example Complete ===\n");
    return 0;
}