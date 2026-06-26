#include "nk_core.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include "nk_statistics.h"
#include "nk_epistasis.h"
#include "nk_coevolution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  TEST %s... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASSED\n"); } while(0)
#define FAIL(msg) do { printf("FAILED: %s\n", msg); } while(0)
#define ASSERT_EQ_INT(a, b, msg) do { \
    if ((a) != (b)) { FAIL(msg); return; } \
} while(0)
#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)
#define ASSERT_FLOAT_NEAR(a, b, eps, msg) do { \
    if (fabs((a) - (b)) > (eps)) { \
        printf("FAILED: %s (%.6f vs %.6f)\n", msg, a, b); return; \
    } \
} while(0)
#define ASSERT_NOT_NULL(ptr, msg) do { \
    if ((ptr) == NULL) { FAIL(msg); return; } \
} while(0)

/* ============================================================================
 * L1: Genotype Lifecycle Tests
 * ============================================================================ */
static void test_genotype_create_free(void) {
    TEST("genotype_create/free");
    NKGenotype *g = nk_genotype_create(8);
    ASSERT_NOT_NULL(g, "create failed");
    ASSERT_EQ_INT(g->N, 8, "N mismatch");
    nk_genotype_free(g);
    nk_genotype_free(NULL);  /* Should not crash */
    PASS();
}

static void test_genotype_set_get(void) {
    TEST("genotype_set/get");
    NKGenotype *g = nk_genotype_create(16);
    ASSERT_NOT_NULL(g, "create failed");

    nk_genotype_set(g, 0, 1);
    ASSERT_EQ_INT(nk_genotype_get(g, 0), 1, "bit 0 should be 1");

    nk_genotype_set(g, 0, 0);
    ASSERT_EQ_INT(nk_genotype_get(g, 0), 0, "bit 0 should be 0");

    nk_genotype_set(g, 15, 1);
    ASSERT_EQ_INT(nk_genotype_get(g, 15), 1, "bit 15 should be 1");

    /* Out of bounds should be safe */
    nk_genotype_set(g, -1, 1);
    nk_genotype_set(g, 16, 1);
    ASSERT_EQ_INT(nk_genotype_get(g, -1), 0, "OOB get returns 0");
    ASSERT_EQ_INT(nk_genotype_get(g, 16), 0, "OOB get returns 0");

    nk_genotype_free(g);
    PASS();
}

static void test_genotype_flip(void) {
    TEST("genotype_flip");
    NKGenotype *g = nk_genotype_create(8);
    nk_genotype_flip(g, 3);
    ASSERT_EQ_INT(nk_genotype_get(g, 3), 1, "flip 0->1 failed");
    nk_genotype_flip(g, 3);
    ASSERT_EQ_INT(nk_genotype_get(g, 3), 0, "flip 1->0 failed");
    nk_genotype_free(g);
    PASS();
}

static void test_genotype_clone_equal(void) {
    TEST("genotype_clone/equal");
    NKGenotype *g1 = nk_genotype_create(8);
    nk_genotype_set(g1, 2, 1);
    nk_genotype_set(g1, 5, 1);

    NKGenotype *g2 = nk_genotype_clone(g1);
    ASSERT_NOT_NULL(g2, "clone failed");
    ASSERT_TRUE(nk_genotype_equal(g1, g2), "clones should be equal");

    nk_genotype_flip(g2, 2);
    ASSERT_TRUE(!nk_genotype_equal(g1, g2), "different should not be equal");

    nk_genotype_free(g1);
    nk_genotype_free(g2);
    PASS();
}

static void test_genotype_hamming(void) {
    TEST("genotype_hamming");
    NKGenotype *g1 = nk_genotype_create(8);
    NKGenotype *g2 = nk_genotype_create(8);
    ASSERT_EQ_INT(nk_genotype_hamming(g1, g2), 0, "identical => 0");

    nk_genotype_flip(g2, 0);
    ASSERT_EQ_INT(nk_genotype_hamming(g1, g2), 1, "one flip => 1");

    nk_genotype_flip(g2, 3);
    nk_genotype_flip(g2, 7);
    ASSERT_EQ_INT(nk_genotype_hamming(g1, g2), 3, "three flips => 3");

    nk_genotype_free(g1);
    nk_genotype_free(g2);
    PASS();
}

static void test_genotype_to_from_string(void) {
    TEST("genotype_to/from_string");
    NKGenotype *g = nk_genotype_create(6);
    nk_genotype_set(g, 0, 1);
    nk_genotype_set(g, 2, 1);
    nk_genotype_set(g, 5, 1);

    char *s = nk_genotype_to_string(g);
    ASSERT_NOT_NULL(s, "to_string returned NULL");
    ASSERT_EQ_INT((int)strlen(s), 6, "string length mismatch");
    ASSERT_TRUE(strcmp(s, "101001") == 0 || strcmp(s, "100101") == 0, "string content wrong");

    NKGenotype *g2 = nk_genotype_from_string(s);
    ASSERT_NOT_NULL(g2, "from_string returned NULL");
    ASSERT_TRUE(nk_genotype_equal(g, g2), "round-trip failed");

    free(s);
    nk_genotype_free(g);
    nk_genotype_free(g2);
    PASS();
}

/* ============================================================================
 * L2/L3: Landscape and Fitness Tests
 * ============================================================================ */
static void test_landscape_create(void) {
    TEST("landscape_create");
    NKLandscape *land = nk_landscape_create(8, 2, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 42);
    ASSERT_NOT_NULL(land, "landscape create failed");
    ASSERT_EQ_INT(land->N, 8, "N mismatch");
    ASSERT_EQ_INT(land->K, 2, "K mismatch");
    ASSERT_NOT_NULL(land->epistasis, "epistasis NULL");
    ASSERT_NOT_NULL(land->fitness, "fitness NULL");
    nk_landscape_free(land);
    PASS();
}

static void test_fitness_range(void) {
    TEST("fitness_range");
    NKLandscape *land = nk_landscape_create(10, 2, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 123);
    ASSERT_NOT_NULL(land, "create failed");

    NKGenotype *g = nk_genotype_create(10);
    double f = nk_fitness(land, g);
    ASSERT_TRUE(f >= 0.0 && f <= 1.0, "fitness out of [0,1]");

    nk_genotype_free(g);
    nk_landscape_free(land);
    PASS();
}

static void test_local_optimum_detection(void) {
    TEST("local_optimum_detection");
    /* For K=0 (no epistasis), there should be exactly one local optimum */
    NKLandscape *land = nk_landscape_create(5, 0, NK_INTERACTION_ADJACENT,
                                              NK_CONTRIB_UNIFORM, 99);
    ASSERT_NOT_NULL(land, "create failed");

    NKGenotype *g = nk_genotype_create(5);
    /* Enumerate all 32 genotypes, count local optima */
    int n_optima = 0;
    for (int gi = 0; gi < 32; gi++) {
        for (int i = 0; i < 5; i++) {
            nk_genotype_set(g, i, (gi >> i) & 1);
        }
        if (nk_is_local_optimum(land, g)) n_optima++;
    }
    ASSERT_EQ_INT(n_optima, 1, "K=0 landscape should have exactly 1 local optimum");

    nk_genotype_free(g);
    nk_landscape_free(land);
    PASS();
}

static void test_phase_classification(void) {
    TEST("phase_classification");
    /* Ordered phase */
    NKLandscape *l1 = nk_landscape_create(20, 1, NK_INTERACTION_RANDOM,
                                            NK_CONTRIB_UNIFORM, 1);
    ASSERT_EQ_INT(nk_landscape_phase(l1), NK_PHASE_ORDERED, "K=1,N=20 should be ordered");
    nk_landscape_free(l1);

    /* Critical phase */
    NKLandscape *l2 = nk_landscape_create(10, 3, NK_INTERACTION_RANDOM,
                                            NK_CONTRIB_UNIFORM, 2);
    ASSERT_EQ_INT(nk_landscape_phase(l2), NK_PHASE_CRITICAL, "K=3,N=10 should be critical");
    nk_landscape_free(l2);

    /* Chaotic phase */
    NKLandscape *l3 = nk_landscape_create(8, 6, NK_INTERACTION_RANDOM,
                                            NK_CONTRIB_UNIFORM, 3);
    ASSERT_EQ_INT(nk_landscape_phase(l3), NK_PHASE_CHAOTIC, "K=6,N=8 should be chaotic");
    nk_landscape_free(l3);

    PASS();
}

/* ============================================================================
 * L5: Adaptive Walk Tests
 * ============================================================================ */
static void test_greedy_walk(void) {
    TEST("greedy_walk");
    NKLandscape *land = nk_landscape_create(12, 2, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 100);
    NKGenotype *start = nk_genotype_create(12);
    uint64_t rng = 200;
    nk_genotype_randomize(start, &rng);

    NKWalkResult *wr = nk_walk_greedy(land, start, 100);
    ASSERT_NOT_NULL(wr, "walk returned NULL");
    ASSERT_TRUE(wr->reached_optimum, "walk did not reach optimum");
    ASSERT_TRUE(wr->steps >= 0, "negative steps");
    ASSERT_TRUE(wr->steps <= 100, "walk exceeded max_steps");

    /* Verify fitness is non-decreasing */
    for (int i = 1; i <= wr->steps; i++) {
        ASSERT_TRUE(wr->path[i]->fitness >= wr->path[i-1]->fitness,
                    "fitness decreased during walk");
    }

    /* Verify final state is local optimum */
    ASSERT_TRUE(nk_is_local_optimum(land, wr->path[wr->steps]->genotype),
                "final state not local optimum");

    nk_walk_result_free(wr);
    nk_genotype_free(start);
    nk_landscape_free(land);
    PASS();
}

static void test_steepest_ascent(void) {
    TEST("steepest_ascent");
    NKLandscape *land = nk_landscape_create(10, 2, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 300);
    NKGenotype *start = nk_genotype_create(10);
    uint64_t rng = 400;
    nk_genotype_randomize(start, &rng);

    NKWalkResult *wr = nk_walk_steepest_ascent(land, start, 100);
    ASSERT_NOT_NULL(wr, "walk returned NULL");
    ASSERT_TRUE(wr->reached_optimum, "did not reach optimum");

    /* Verify final state is local optimum */
    ASSERT_TRUE(nk_is_local_optimum(land, wr->path[wr->steps]->genotype),
                "final state not local optimum");

    nk_walk_result_free(wr);
    nk_genotype_free(start);
    nk_landscape_free(land);
    PASS();
}

static void test_metropolis_walk(void) {
    TEST("metropolis_walk");
    NKLandscape *land = nk_landscape_create(10, 3, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 500);
    NKGenotype *start = nk_genotype_create(10);
    uint64_t rng = 600;
    nk_genotype_randomize(start, &rng);

    NKWalkResult *wr = nk_walk_metropolis(land, start, 200, 0.1);
    ASSERT_NOT_NULL(wr, "walk returned NULL");
    ASSERT_TRUE(wr->steps == 200, "Metropolis should run all steps");

    nk_walk_result_free(wr);
    nk_genotype_free(start);
    nk_landscape_free(land);
    PASS();
}

/* ============================================================================
 * L4: Statistics and Epistasis Tests
 * ============================================================================ */
static void test_fitness_histogram(void) {
    TEST("fitness_histogram");
    NKLandscape *land = nk_landscape_create(10, 2, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 700);
    int hist[10] = {0};
    uint64_t rng = 800;
    nk_fitness_histogram(land, 1000, 10, hist, &rng);

    int sum = 0;
    for (int b = 0; b < 10; b++) sum += hist[b];
    ASSERT_EQ_INT(sum, 1000, "histogram should sum to sample count");

    nk_landscape_free(land);
    PASS();
}

static void test_epistasis_pairwise(void) {
    TEST("epistasis_pairwise");
    NKLandscape *land = nk_landscape_create(10, 3, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 900);
    NKGenotype *g = nk_genotype_create(10);
    uint64_t rng = 1000;
    nk_genotype_randomize(g, &rng);

    double e = nk_epistasis_pairwise_magnitude(land, g, 0, 1);
    ASSERT_TRUE(e >= 0.0, "epistasis magnitude should be non-negative");

    /* Self-epistasis should be zero */
    double e_self = nk_epistasis_pairwise_magnitude(land, g, 0, 0);
    ASSERT_FLOAT_NEAR(e_self, 0.0, 1e-9, "self-epistasis should be 0");

    nk_genotype_free(g);
    nk_landscape_free(land);
    PASS();
}

static void test_complexity_catastrophe(void) {
    TEST("complexity_catastrophe");
    /* For N=10, test K=0..5: mean optimum fitness should decrease */
    NKLandscape *base = nk_landscape_create(10, 2, NK_INTERACTION_RANDOM,
                                              NK_CONTRIB_UNIFORM, 1100);
    int ks[6];
    double fits[6];
    uint64_t rng = 1200;
    nk_complexity_catastrophe(base, 20, 5, ks, fits, &rng);

    /* Verify fitness at K=0 is >= fitness at K=5 (complexity catastrophe) */
    ASSERT_TRUE(fits[0] >= fits[5] - 0.05,
                "complexity catastrophe not observed (K=0 should >= K=5)");

    nk_landscape_free(base);
    PASS();
}

/* ============================================================================
 * L7/L8: Coevolution Tests
 * ============================================================================ */
static void test_nkcs_create(void) {
    TEST("nkcs_create");
    NKCSLandscape *nkcs = nkcs_create(8, 2, 1, 0.3, 1300);
    ASSERT_NOT_NULL(nkcs, "nkcs create failed");
    ASSERT_NOT_NULL(nkcs->species_a, "species_a NULL");
    ASSERT_NOT_NULL(nkcs->species_b, "species_b NULL");
    ASSERT_EQ_INT(nkcs->C, 1, "C mismatch");
    nkcs_free(nkcs);
    PASS();
}

static void test_nkcs_fitness(void) {
    TEST("nkcs_fitness");
    NKCSLandscape *nkcs = nkcs_create(8, 1, 1, 0.5, 1400);
    ASSERT_NOT_NULL(nkcs, "create failed");

    NKGenotype *ga = nk_genotype_create(8);
    NKGenotype *gb = nk_genotype_create(8);
    uint64_t rng = 1500;
    nk_genotype_randomize(ga, &rng);
    nk_genotype_randomize(gb, &rng);

    double fa = nkcs_fitness_a(nkcs, ga, gb);
    double fb = nkcs_fitness_b(nkcs, ga, gb);
    ASSERT_TRUE(fa >= 0.0 && fa <= 1.0, "fitness_a out of range");
    ASSERT_TRUE(fb >= 0.0 && fb <= 1.0, "fitness_b out of range");

    nk_genotype_free(ga);
    nk_genotype_free(gb);
    nkcs_free(nkcs);
    PASS();
}

static void test_nkcs_nash_equilibrium(void) {
    TEST("nkcs_nash_equilibrium");
    NKCSLandscape *nkcs = nkcs_create(6, 1, 1, 0.3, 1600);
    ASSERT_NOT_NULL(nkcs, "create failed");

    NKGenotype *ga = nk_genotype_create(6);
    NKGenotype *gb = nk_genotype_create(6);
    uint64_t rng = 1700;
    nk_genotype_randomize(ga, &rng);
    nk_genotype_randomize(gb, &rng);

    int iters = nkcs_find_nash_equilibrium(nkcs, ga, gb, 50, &rng);
    ASSERT_TRUE(iters >= 1, "Nash equilibrium search failed to start");
    ASSERT_TRUE(iters <= 50, "Nash search exceeded max iterations");

    /* Verify it is actually a Nash equilibrium */
    ASSERT_TRUE(nkcs_is_nash_equilibrium(nkcs, ga, gb), "not a Nash equilibrium");

    nk_genotype_free(ga);
    nk_genotype_free(gb);
    nkcs_free(nkcs);
    PASS();
}

/* ============================================================================
 * Edge Case Tests
 * ============================================================================ */
static void test_null_safety(void) {
    TEST("null_safety");
    /* All functions should handle NULL gracefully */
    nk_genotype_free(NULL);
    ASSERT_EQ_INT(nk_genotype_hamming(NULL, NULL), -1, "NULL hamming");
    ASSERT_TRUE(nk_genotype_equal(NULL, NULL), "NULL == NULL");
    ASSERT_TRUE(!nk_is_local_optimum(NULL, NULL), "NULL not optimum");

    double buf[1];
    nk_neighbor_fitnesses(NULL, NULL, NULL);
    nk_neighbor_fitnesses(NULL, NULL, buf);

    nk_walk_result_free(NULL);
    nk_walk_print_trajectory(NULL);

    nk_statistics_free(NULL);
    nk_statistics_print(NULL);

    nkcs_free(NULL);

    PASS();
}

int main(void) {
    printf("=== NK Fitness Landscapes Test Suite ===\n\n");

    /* L1: Genotype */
    test_genotype_create_free();
    test_genotype_set_get();
    test_genotype_flip();
    test_genotype_clone_equal();
    test_genotype_hamming();
    test_genotype_to_from_string();

    /* L2/L3: Landscape */
    test_landscape_create();
    test_fitness_range();
    test_local_optimum_detection();
    test_phase_classification();

    /* L5: Adaptive walks */
    test_greedy_walk();
    test_steepest_ascent();
    test_metropolis_walk();

    /* L4/L5: Statistics and epistasis */
    test_fitness_histogram();
    test_epistasis_pairwise();
    test_complexity_catastrophe();

    /* L7/L8: Coevolution */
    test_nkcs_create();
    test_nkcs_fitness();
    test_nkcs_nash_equilibrium();

    /* Edge cases */
    test_null_safety();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
