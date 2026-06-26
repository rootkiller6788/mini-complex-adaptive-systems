#include "ace_core.h"
#include "ace_dynamics.h"
#include "ace_market.h"
#include "ace_evolution.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    unsigned int seed = 999;
    printf("============================================================
");
    printf("  CAS in Economics -- W. Brian Arthur Demo Overview
");
    printf("============================================================

");
    printf("1. CORE DEFINITIONS (L1):
");
    printf("   Market regimes: Equilibrium, Complex, Chaotic, Evolutionary
");
    printf("   Reasoning modes: Deductive, Inductive, Abductive, Bounded
");
    printf("   Feedback types: Diminishing, Constant, Increasing, Network
");
    printf("   Lock states: Not, Partial, Full, Escape

");
    printf("2. POLYA URN (L4-L5):
");
    int init[] = {1,1};
    ACEPolyaUrn* u = ace_polya_urn_alloc(2, init, 1.0);
    for (int i = 0; i < 50; i++) ace_polya_urn_draw(u, &seed);
    double props[2]; ace_polya_urn_proportions(u, props);
    printf("   After 50 draws: share_A=%.4f share_B=%.4f
", props[0], props[1]);
    ace_polya_urn_free(u);
    printf("
3. ADOPTION DYNAMICS (L5):
");
    ACEAdoptionState* st = ace_adoption_state_alloc(2);
    for (int i = 0; i < 100; i++)
        ace_adoption_step_binary(st, 1.0, 1.0, 2.0, 2.0, &seed);
    printf("   After 100 adoptions: A=%.4f B=%.4f
", st->shares[0], st->shares[1]);
    ace_adoption_state_free(st);
    printf("
4. NK FITNESS LANDSCAPE (L8):
");
    ACENKFitnessLandscape* nk = ace_nk_landscape_alloc(6, 2);
    if (nk) {
        ace_nk_generate(nk, &seed);
        printf("   N=6 K=2: peaks=%d ruggedness=%.4f
", nk->n_peaks, nk->ruggedness);
        ace_nk_landscape_free(nk);
    }
    printf("
5. CLASSIFIER SYSTEM (L5):
");
    ACEClassifierSystem* cs = ace_classifier_system_alloc(50, 0.1, 0.01);
    ace_classifier_add(cs, "01#10#", "BUY", &seed);
    ace_classifier_add(cs, "#01##0", "SELL", &seed);
    printf("   Rules in system: %d
", cs->n_rules);
    ace_classifier_system_free(cs);
    printf("
6. CRISIS DETECTION (L7):
");
    ACEMacroState ms = {0};
    ms.market_volatility = 0.4; ms.herfindahl_index = 0.5;
    ms.entropy = 1.5; ms.n_bankruptcies = 30; ms.n_active_agents = 100;
    printf("   Crisis detected: %s
",
           ace_detect_crisis(&ms, 0.3, 0.25) ? "YES" : "NO");
    printf("
============================================================
");
    printf("  All systems operational. See examples/ for detailed runs.
");
    printf("============================================================
");
    return 0;
}
