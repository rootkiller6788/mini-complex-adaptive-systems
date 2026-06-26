#include "game_theory.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int main(void) {
    printf("=== test_game ===\n");

    /* Prisoner's Dilemma */
    NormalFormGame *pd = nfg_prisoners_dilemma(5.0, 3.0, 1.0, 0.0);
    assert(pd != NULL);
    /* Check (D,D) is Nash in PD */
    MixedStrategy s1_d = {2, (double[]){0.0, 1.0}};
    MixedStrategy s2_d = {2, (double[]){0.0, 1.0}};
    StrategyProfile sp_dd = {s1_d, s2_d};
    assert(nfg_is_nash(pd, &sp_dd, 0.001));
    printf("  PD (D,D) is Nash: PASS\n");
    nfg_destroy(pd);

    /* Battle of Sexes */
    NormalFormGame *bos = nfg_battle_of_sexes();
    /* (Opera,Opera) is pure NE */
    MixedStrategy s1_o = {2, (double[]){1.0, 0.0}};
    MixedStrategy s2_o = {2, (double[]){1.0, 0.0}};
    StrategyProfile sp_oo = {s1_o, s2_o};
    assert(nfg_is_nash(bos, &sp_oo, 0.001));
    printf("  BoS (O,O) is Nash: PASS\n");

    /* Lemke-Howson for BoS */
    StrategyProfile *sp = nfg_lemke_howson(bos);
    assert(sp != NULL);
    printf("  Lemke-Howson: P1=(%.3f,%.3f) P2=(%.3f,%.3f)\n",
           sp->p1.probs[0], sp->p1.probs[1],
           sp->p2.probs[0], sp->p2.probs[1]);
    free(sp->p1.probs);
    free(sp->p2.probs);
    free(sp);
    nfg_destroy(bos);

    /* Rock-Paper-Scissors: uniform (1/3,1/3,1/3) is unique NE */
    NormalFormGame *rps = nfg_rock_paper_scissors();
    StrategyProfile *rps_eq = nfg_lemke_howson(rps);
    assert(rps_eq != NULL);
    printf("  RPS NE: P1=(%.3f,%.3f,%.3f) P2=(%.3f,%.3f,%.3f)\n",
           rps_eq->p1.probs[0], rps_eq->p1.probs[1], rps_eq->p1.probs[2],
           rps_eq->p2.probs[0], rps_eq->p2.probs[1], rps_eq->p2.probs[2]);
    free(rps_eq->p1.probs); free(rps_eq->p2.probs); free(rps_eq);

    /* Enumeration */
    StrategyProfile *profiles[10];
    int n_found = nfg_enumeration_find_all(rps, profiles, 10);
    printf("  RPS pure NE count: %d\n", n_found);
    for (int i = 0; i < n_found; i++) {
        free(profiles[i]->p1.probs);
        free(profiles[i]->p2.probs);
        free(profiles[i]);
    }
    nfg_destroy(rps);

    /* Replicator dynamics */
    double payoff[] = {3.0, 0.0, 5.0, 1.0}; /* PD payoff for row player */
    double init[] = {0.5, 0.5};
    ReplicatorState *rs = replicator_create(2, payoff, init);
    assert(rs != NULL);

    double hist[100];
    double avg_fit[50];
    replicator_run(rs, 50, 0.1, hist, avg_fit);
    /* In PD replicator, defection evolves toward fixation */
    assert(rs->frequencies[1] > rs->frequencies[0]); /* defectors increase */
    printf("  Replicator: after 50 steps frequencies=(%.4f,%.4f)\n",
           rs->frequencies[0], rs->frequencies[1]);

    /* ESS check */
    bool ess0 = replicator_is_ess(rs, 0, 0.01);
    bool ess1 = replicator_is_ess(rs, 1, 0.01);
    printf("  ESS: strategy0=%d, strategy1=%d\n", ess0, ess1);

    double fp[10];
    int n_fp = replicator_find_fixed_points(rs, fp, 5, 0.001);
    printf("  Fixed points found: %d\n", n_fp);

    replicator_destroy(rs);

    /* PD tournament */
    NormalFormGame *pd2 = nfg_prisoners_dilemma(5.0, 3.0, 1.0, 0.0);
    PDStrategy strategies[] = {
        pd_tit_for_tat, pd_always_defect,
        pd_grim_trigger, pd_pavlov, pd_random_strategy
    };
    const char *names[] = {"TFT", "ALLD", "Grim", "Pavlov", "Random"};
    double scores[5] = {0};
    pd_round_robin(pd2, strategies, names, 5, 50, scores);
    printf("  PD Round-Robin scores:\n");
    for (int i = 0; i < 5; i++) {
        printf("    %s: %.1f\n", names[i], scores[i]);
    }
    nfg_destroy(pd2);

    /* TFT vs TFT: cooperation emerges */
    PDHistory *h1 = pd_history_create(10);
    PDHistory *h2 = pd_history_create(10);
    NormalFormGame *pd3 = nfg_prisoners_dilemma(5.0, 3.0, 1.0, 0.0);
    for (int r = 0; r < 10; r++) {
        double p1, p2;
        PDAction a1, a2;
        pd_play_round(pd3, pd_tit_for_tat, pd_tit_for_tat,
                       h1, h2, &a1, &a2, &p1, &p2);
        assert(a1 == PD_COOPERATE);
        assert(a2 == PD_COOPERATE);
    }
    printf("  TFT vs TFT: mutual cooperation sustained for 10 rounds: PASS\n");
    pd_history_destroy(h1);
    pd_history_destroy(h2);
    nfg_destroy(pd3);

    printf("=== test_game: ALL PASS ===\n");
    return 0;
}
