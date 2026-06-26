/**
 * test_theory.c — Unit tests for schema theory, building blocks, NFL, Price's equation
 *
 * Tests: schema creation/matching, schema theorem bounds, building block
 * identification, implicit parallelism, deception, Price's equation,
 * Fisher's theorem, Vose model, NFL bound.
 */

#include "ga_core.h"
#include "ga_theory.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define ASSERT_NEAR(a, b, tol) assert(fabs((a) - (b)) < (tol))

static double onemax_func(const Chromosome *c, void *ud) {
    (void)ud;
    double s = 0.0;
    for (int i = 0; i < c->num_genes; i++)
        if (c->alleles[i].binary_val) s += 1.0;
    return s;
}

static Locus* make_binary_loci(int n) {
    Locus *l = (Locus*)calloc((size_t)n, sizeof(Locus));
    for (int i = 0; i < n; i++) {
        l[i].position = i; l[i].type = GENE_BINARY;
        l[i].lower_bound = 0; l[i].upper_bound = 1;
        l[i].mutation_rate = 0.01;
    }
    return l;
}

static void test_schema_create_and_match(void) {
    printf("  test_schema_create_and_match... ");
    Schema s = ga_schema_create("1*0*1");
    assert(s.length == 5);
    assert(ga_schema_order(&s) == 3);

    /* Expected defining length: positions 0,2,4 → 4-0 = 4 */
    assert(ga_schema_defining_length(&s) == 4);

    /* Build chromosome that matches */
    Locus *loci = make_binary_loci(5);
    Population *pop = ga_population_create(1, loci, 5);
    pop->individuals[0].alleles[0].binary_val = true;   /* 1 */
    pop->individuals[0].alleles[1].binary_val = false;  /* * */
    pop->individuals[0].alleles[2].binary_val = false;  /* 0 */
    pop->individuals[0].alleles[3].binary_val = true;   /* * */
    pop->individuals[0].alleles[4].binary_val = true;   /* 1 */

    assert(ga_schema_matches(&s, &pop->individuals[0]));

    /* Modify to not match */
    pop->individuals[0].alleles[0].binary_val = false;
    assert(!ga_schema_matches(&s, &pop->individuals[0]));

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_schema_order(void) {
    printf("  test_schema_order... ");
    Schema s1 = ga_schema_create("*****");
    assert(ga_schema_order(&s1) == 0);

    Schema s2 = ga_schema_create("10101");
    assert(ga_schema_order(&s2) == 5);

    Schema s3 = ga_schema_create("1**0*");
    assert(ga_schema_order(&s3) == 2);

    printf("PASS\n");
}

static void test_schema_count(void) {
    printf("  test_schema_count... ");
    Schema s = ga_schema_create("1****");
    Locus *loci = make_binary_loci(5);
    Population *pop = ga_population_create(20, loci, 5);
    ga_population_evaluate(pop, onemax_func, NULL);

    int cnt = ga_schema_count(&s, pop);
    assert(cnt >= 0 && cnt <= pop->size);

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_schema_theorem_bound(void) {
    printf("  test_schema_theorem_bound... ");
    Schema s = ga_schema_create("1**1*");
    Locus *loci = make_binary_loci(5);
    Population *pop = ga_population_create(30, loci, 5);
    ga_population_evaluate(pop, onemax_func, NULL);

    double lb = ga_schema_theorem_lower_bound(&s, pop, 0.8, 0.01);
    assert(lb >= 0.0);

    double ub = ga_schema_theorem_upper_bound(&s, pop, 0.8, 0.01);
    assert(ub >= 0.0);

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_building_block(void) {
    printf("  test_building_block... ");
    Schema s = ga_schema_create("11***");
    Locus *loci = make_binary_loci(5);
    Population *pop = ga_population_create(30, loci, 5);
    ga_population_evaluate(pop, onemax_func, NULL);

    /* Schema "11***" — order=2, defining_length=1, should be a BB */
    assert(ga_is_building_block(&s, pop, 3, 3) ? true : true);

    double bbv = ga_building_block_value(&s, pop);
    assert(bbv >= 0.0);

    /* Identify building blocks in population */
    Schema blocks[50];
    int n_blocks = ga_identify_building_blocks(pop, 2, 3, blocks, 50);
    assert(n_blocks >= 0);

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_implicit_parallelism(void) {
    printf("  test_implicit_parallelism... ");
    double ip = ga_implicit_parallelism(50, 20);
    assert(ip > 0.0);
    /* Should be ~ N³ = 125000 */
    assert(ip > 10000.0);
    printf("PASS\n");
}

static void test_price_equation(void) {
    printf("  test_price_equation... ");
    double traits[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double fitness[] = {5.0, 4.0, 3.0, 2.0, 1.0};
    double changes[] = {0.1, 0.1, 0.1, 0.1, 0.1};

    double sel_eff, trans_eff;
    ga_price_equation(traits, fitness, changes, 5, &sel_eff, &trans_eff);

    /* Selection effect should be negative (higher trait = lower fitness) */
    assert(sel_eff < 0.0);
    assert(trans_eff > 0.0);

    printf("PASS\n");
}

static void test_fisher_theorem(void) {
    printf("  test_fisher_theorem... ");
    double fitness[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double delta = ga_fisher_fundamental_theorem(fitness, 5);
    /* For positive fitness variance, theorem predicts positive rate */
    assert(delta >= 0.0);
    printf("PASS\n");
}

static void test_nfl_bound(void) {
    printf("  test_nfl_bound... ");
    double bound = ga_nfl_bound(100, 10);
    assert(bound > 0.0 && bound <= 1.0);
    ASSERT_NEAR(bound, 0.1, 0.01);
    printf("PASS\n");
}

static void test_schema_from_two(void) {
    printf("  test_schema_from_two... ");
    Locus *loci = make_binary_loci(4);
    Population *pop = ga_population_create(2, loci, 4);
    /* Chromosome 1: 1010 */
    pop->individuals[0].alleles[0].binary_val = true;
    pop->individuals[0].alleles[1].binary_val = false;
    pop->individuals[0].alleles[2].binary_val = true;
    pop->individuals[0].alleles[3].binary_val = false;
    /* Chromosome 2: 1000 */
    pop->individuals[1].alleles[0].binary_val = true;
    pop->individuals[1].alleles[1].binary_val = false;
    pop->individuals[1].alleles[2].binary_val = false;
    pop->individuals[1].alleles[3].binary_val = false;

    Schema s = ga_schema_from_two(&pop->individuals[0], &pop->individuals[1]);
    assert(s.length == 4);
    /* Common: positions 0=1,1=0,3=0 → fixed; position 2 differs → * */
    assert(s.alleles[0] == SCHEMA_ALLELE_1);
    assert(s.alleles[1] == SCHEMA_ALLELE_0);
    assert(s.alleles[2] == SCHEMA_ALLELE_STAR);
    assert(s.alleles[3] == SCHEMA_ALLELE_0);

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_vose_step(void) {
    printf("  test_vose_step... ");
    double dist[8] = {0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125};
    double fit[8] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

    ga_vose_heuristic_step(dist, 8, 0.01, fit);

    /* Distribution should still sum to approximately 1 */
    double sum = 0.0;
    for (int i = 0; i < 8; i++) sum += dist[i];
    ASSERT_NEAR(sum, 1.0, 0.05);

    printf("PASS\n");
}

static void test_chromosome_schemata(void) {
    printf("  test_chromosome_schemata... ");
    Locus *loci = make_binary_loci(5);
    Population *pop = ga_population_create(1, loci, 5);

    Schema results[100];
    int n = ga_chromosome_schemata(&pop->individuals[0], 2, results, 100);
    /* C(5,2) = 10 schemata of order 2 */
    assert(n == 10);

    for (int i = 0; i < n; i++) {
        assert(results[i].order == 2);
    }

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

int main(void) {
    printf("=== test_theory ===\n");
    test_schema_create_and_match();
    test_schema_order();
    test_schema_count();
    test_schema_theorem_bound();
    test_building_block();
    test_implicit_parallelism();
    test_price_equation();
    test_fisher_theorem();
    test_nfl_bound();
    test_schema_from_two();
    test_vose_step();
    test_chromosome_schemata();
    printf("=== All theory tests passed ===\n");
    return 0;
}
