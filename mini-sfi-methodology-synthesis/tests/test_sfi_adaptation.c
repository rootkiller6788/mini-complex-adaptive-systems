/**
 * @file    test_sfi_adaptation.c
 * @brief   Tests for NK landscapes, replicator dynamics, classifiers
 */

#include "../include/sfi_adaptation.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

#define TEST_PASS() printf("  PASS: %s\n", __func__)

void test_nk_landscape_generate(void) {
    sfi_nk_landscape_t land;
    int rc = sfi_nk_landscape_generate(&land, 8, 2, 42);
    assert(rc == 0);
    assert(land.N == 8);
    assert(land.K == 2);
    assert(land.fitness_table != NULL);
    assert(land.interaction_matrix != NULL);
    assert(land.mean_fitness >= 0.0 && land.mean_fitness <= 1.0);
    assert(land.autocorr_1step >= -1.0 && land.autocorr_1step <= 1.0);

    sfi_nk_landscape_destroy(&land);
    assert(land.fitness_table == NULL);
    TEST_PASS();
}

void test_nk_landscape_invalid(void) {
    sfi_nk_landscape_t land;
    assert(sfi_nk_landscape_generate(NULL, 8, 2, 0) == -1);
    assert(sfi_nk_landscape_generate(&land, 0, 0, 0) == -1);  /* N == 0 */
    assert(sfi_nk_landscape_generate(&land, 8, 8, 0) == -1);  /* K >= N */
    TEST_PASS();
}

void test_nk_evaluate_fitness(void) {
    sfi_nk_landscape_t land;
    sfi_nk_landscape_generate(&land, 4, 1, 123);
    assert(sfi_nk_evaluate_fitness(NULL, 0) == 0.0);

    double f0 = sfi_nk_evaluate_fitness(&land, 0);
    double f1 = sfi_nk_evaluate_fitness(&land, 1);
    assert(f0 >= 0.0 && f0 <= 1.0);
    assert(f1 >= 0.0 && f1 <= 1.0);
    /* K=1 ? correlated landscape, neighbouring genotypes may differ */
    (void)f0; (void)f1;

    sfi_nk_landscape_destroy(&land);
    TEST_PASS();
}

void test_nk_greedy_walk(void) {
    sfi_nk_landscape_t land;
    sfi_nk_landscape_generate(&land, 6, 2, 99);

    double final_fit;
    uint32_t steps;
    uint64_t peak = sfi_nk_greedy_walk(&land, 0, &final_fit, &steps);
    assert(final_fit >= 0.0 && final_fit <= 1.0);
    /* Peak should have fitness ? starting fitness */
    double start_fit = sfi_nk_evaluate_fitness(&land, 0);
    assert(final_fit >= start_fit - 1e-10);

    /* Peak should be a local optimum */
    int is_optimum = 1;
    for (uint32_t b = 0; b < land.N && is_optimum; b++) {
        uint64_t mut = peak ^ (1ULL << (uint64_t)(land.N - 1 - b));
        if (sfi_nk_evaluate_fitness(&land, mut) > final_fit)
            is_optimum = 0;
    }
    assert(is_optimum == 1);

    sfi_nk_landscape_destroy(&land);
    TEST_PASS();
}

void test_nk_count_local_optima(void) {
    sfi_nk_landscape_t land;
    sfi_nk_landscape_generate(&land, 4, 1, 77);
    uint32_t count = sfi_nk_count_local_optima(&land, 1000);
    /* With N=4, total genotypes = 16, should find some local optima */
    assert(count > 0);
    assert(count <= 16);

    sfi_nk_landscape_destroy(&land);
    TEST_PASS();
}

void test_nk_autocorrelation(void) {
    sfi_nk_landscape_t land;
    sfi_nk_landscape_generate(&land, 4, 0, 42);
    double rho = sfi_nk_autocorrelation(&land);
    /* K=0: Fujiyama, should be positively correlated */
    assert(rho > 0.0);

    sfi_nk_landscape_destroy(&land);

    sfi_nk_landscape_generate(&land, 4, 3, 99);
    rho = sfi_nk_autocorrelation(&land);
    /* K=N-1: random, low correlation */
    assert(rho < 1.0);  /* May still be positive */
    (void)rho;

    sfi_nk_landscape_destroy(&land);
    TEST_PASS();
}

void test_replicator_init(void) {
    sfi_replicator_state_t rep;
    int rc = sfi_replicator_init(&rep, 3);
    assert(rc == 0);
    assert(rep.n_strategies == 3);
    assert(rep.frequencies != NULL);
    assert(fabs(rep.frequencies[0] - 1.0/3.0) < 0.001);

    sfi_replicator_destroy(&rep);
    assert(rep.frequencies == NULL);
    TEST_PASS();
}

void test_replicator_step(void) {
    sfi_replicator_state_t rep;
    sfi_replicator_init(&rep, 2);

    /* Prisoner's Dilemma payoff matrix: C vs D
       C: (3,0), D: (5,1) ? row player perspective */
    rep.payoff_matrix[0] = 3.0;  /* C,C: payoff to row C */
    rep.payoff_matrix[1] = 0.0;  /* C,D */
    rep.payoff_matrix[2] = 5.0;  /* D,C */
    rep.payoff_matrix[3] = 1.0;  /* D,D */

    /* Start with equal mix */
    rep.frequencies[0] = 0.5;
    rep.frequencies[1] = 0.5;

    sfi_replicator_step(&rep, 0.01);
    /* D should be increasing (higher payoff) */
    assert(rep.frequencies[0] + rep.frequencies[1] > 0.999);
    assert(rep.frequencies[0] + rep.frequencies[1] < 1.001);

    /* After many steps, D dominates */
    for (int i = 0; i < 1000; i++)
        sfi_replicator_step(&rep, 0.01);
    assert(rep.frequencies[1] > 0.9);

    sfi_replicator_destroy(&rep);
    TEST_PASS();
}

void test_replicator_fixed_point(void) {
    sfi_replicator_state_t rep;
    sfi_replicator_init(&rep, 2);
    rep.payoff_matrix[0] = 2.0;
    rep.payoff_matrix[1] = 2.0;
    rep.payoff_matrix[2] = 2.0;
    rep.payoff_matrix[3] = 2.0;
    rep.frequencies[0] = 1.0;
    rep.frequencies[1] = 0.0;

    /* Compute payoffs */
    rep.payoffs[0] = rep.payoff_matrix[0] * rep.frequencies[0]
                   + rep.payoff_matrix[1] * rep.frequencies[1];
    rep.payoffs[1] = rep.payoff_matrix[2] * rep.frequencies[0]
                   + rep.payoff_matrix[3] * rep.frequencies[1];
    rep.mean_population_payoff = rep.frequencies[0] * rep.payoffs[0]
                               + rep.frequencies[1] * rep.payoffs[1];

    int fp = sfi_replicator_is_fixed_point(&rep, 1e-6);
    assert(fp == 1);

    sfi_replicator_destroy(&rep);
    TEST_PASS();
}

void test_classifier_create_match(void) {
    sfi_classifier_t cl = sfi_classifier_create(1, "1##0", "01");
    assert(cl.id == 1);
    assert(cl.strength > 0.0);
    assert(cl.specificity > 0.0);

    /* Match tests */
    assert(sfi_classifier_matches(&cl, "1000") == 1);
    assert(sfi_classifier_matches(&cl, "1110") == 1);
    assert(sfi_classifier_matches(&cl, "1001") == 0);  /* Last bit doesn't match */
    assert(sfi_classifier_matches(NULL, "1000") == 0);
    assert(sfi_classifier_matches(&cl, NULL) == 0);

    TEST_PASS();
}

void test_classifier_bucket_brigade(void) {
    sfi_classifier_t prev = sfi_classifier_create(0, "###", "00");
    sfi_classifier_t winner = sfi_classifier_create(1, "1##", "11");
    double prev_strength = prev.strength;

    sfi_classifier_bucket_brigade(&winner, &prev, 0.1, 10.0);
    assert(prev.strength > prev_strength);  /* Received bid */
    assert(winner.fire_count == 1);

    /* Null winner */
    sfi_classifier_bucket_brigade(NULL, &prev, 0.1, 0.0);

    TEST_PASS();
}

void test_schema_theorem(void) {
    uint64_t pop[4] = {
        0b0000,  /* matches schema 0### */
        0b0111,  /* matches schema 0### */
        0b1000,
        0b1111
    };
    double fit[4] = {2.0, 2.0, 1.0, 1.0};

    /* Schema 0###: mask=1000 (binary), value=0000 */
    double expected = sfi_schema_theorem_expected_copies(
        pop, fit, 4, 4, 0x8, 0x0, 0.5, 0.01);
    /* Schema fitness = 2.0, avg fitness = 1.5 ? ratio = 4/3
       Expected ? 2 * (4/3) * (1 - 0.5 * 0/3) * (1-0.01)^1 ? 2.64
       With simplified survival: roughly > 2 */
    assert(expected > 1.0);

    double null_result = sfi_schema_theorem_expected_copies(NULL, fit, 4, 4, 0x8, 0x0, 0.0, 0.0);
    assert(null_result == 0.0);

    TEST_PASS();
}

int main(void) {
    printf("=== SFI Adaptation Tests ===\n");
    test_nk_landscape_generate();
    test_nk_landscape_invalid();
    test_nk_evaluate_fitness();
    test_nk_greedy_walk();
    test_nk_count_local_optima();
    test_nk_autocorrelation();
    test_replicator_init();
    test_replicator_step();
    test_replicator_fixed_point();
    test_classifier_create_match();
    test_classifier_bucket_brigade();
    test_schema_theorem();
    printf("\nAll adaptation tests passed!\n");
    return 0;
}
