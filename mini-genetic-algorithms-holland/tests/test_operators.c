/**
 * test_operators.c — Unit tests for selection, crossover, mutation, replacement
 *
 * Tests: roulette/tournament/rank selection, one/two-point/uniform crossover,
 * bit-flip/gaussian/uniform mutation, replacement strategies.
 */

#include "ga_core.h"
#include "ga_operators.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

static double onemax_func(const Chromosome *c, void *ud) {
    (void)ud;
    double s = 0.0;
    for (int i = 0; i < c->num_genes; i++)
        if (c->alleles[i].binary_val) s += 1.0;
    return s;
}

static Locus* make_loci(int n) {
    Locus *l = (Locus*)calloc((size_t)n, sizeof(Locus));
    for (int i = 0; i < n; i++) {
        l[i].position = i;
        l[i].type = GENE_BINARY;
        l[i].lower_bound = 0;
        l[i].upper_bound = 1;
        l[i].mutation_rate = 0.01;
    }
    return l;
}

static void test_roulette_selection(void) {
    printf("  test_roulette_selection... ");
    Locus *loci = make_loci(4);
    Population *pop = ga_population_create(10, loci, 4);
    ga_population_evaluate(pop, onemax_func, NULL);

    /* Selection should return valid index */
    for (int i = 0; i < 20; i++) {
        int idx = ga_select_roulette(pop);
        assert(idx >= 0 && idx < pop->size);
    }

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_tournament_selection(void) {
    printf("  test_tournament_selection... ");
    Locus *loci = make_loci(4);
    Population *pop = ga_population_create(20, loci, 4);
    ga_population_evaluate(pop, onemax_func, NULL);

    for (int i = 0; i < 30; i++) {
        int idx = ga_select_tournament(pop, 3);
        assert(idx >= 0 && idx < pop->size);
    }

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_rank_selection(void) {
    printf("  test_rank_selection... ");
    Locus *loci = make_loci(4);
    Population *pop = ga_population_create(15, loci, 4);
    ga_population_evaluate(pop, onemax_func, NULL);

    for (int i = 0; i < 20; i++) {
        int idx = ga_select_rank(pop, 1.5);
        assert(idx >= 0 && idx < pop->size);
    }

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_sus_selection(void) {
    printf("  test_sus_selection... ");
    Locus *loci = make_loci(4);
    Population *pop = ga_population_create(20, loci, 4);
    ga_population_evaluate(pop, onemax_func, NULL);

    int selected[5];
    ga_select_sus(pop, selected, 5);
    for (int i = 0; i < 5; i++) {
        assert(selected[i] >= 0 && selected[i] < pop->size);
    }

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_crossover_one_point(void) {
    printf("  test_crossover_one_point... ");
    Locus *loci = make_loci(8);
    Population *pop = ga_population_create(2, loci, 8);
    /* Parent 1: all zeros */
    for (int i = 0; i < 8; i++) pop->individuals[0].alleles[i].binary_val = false;
    /* Parent 2: all ones */
    for (int i = 0; i < 8; i++) pop->individuals[1].alleles[i].binary_val = true;

    Chromosome c1, c2;
    memset(&c1, 0, sizeof(c1));
    memset(&c2, 0, sizeof(c2));
    c1.num_genes = 8; c2.num_genes = 8;
    memcpy(c1.loci, loci, sizeof(Locus)*8);
    memcpy(c2.loci, loci, sizeof(Locus)*8);

    ga_crossover_one_point(&pop->individuals[0], &pop->individuals[1], &c1, &c2);

    /* After crossover, children must still be valid binary chromosomes */
    assert(ga_is_valid(&c1));
    assert(ga_is_valid(&c2));

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_crossover_two_point(void) {
    printf("  test_crossover_two_point... ");
    Locus *loci = make_loci(10);
    Population *pop = ga_population_create(2, loci, 10);
    for (int i = 0; i < 10; i++) pop->individuals[0].alleles[i].binary_val = false;
    for (int i = 0; i < 10; i++) pop->individuals[1].alleles[i].binary_val = true;

    Chromosome c1, c2;
    memset(&c1, 0, sizeof(c1)); memset(&c2, 0, sizeof(c2));
    c1.num_genes = 10; c2.num_genes = 10;
    memcpy(c1.loci, loci, sizeof(Locus)*10);
    memcpy(c2.loci, loci, sizeof(Locus)*10);

    ga_crossover_two_point(&pop->individuals[0], &pop->individuals[1], &c1, &c2);
    assert(ga_is_valid(&c1));
    assert(ga_is_valid(&c2));

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_crossover_uniform(void) {
    printf("  test_crossover_uniform... ");
    Locus *loci = make_loci(10);
    Population *pop = ga_population_create(2, loci, 10);
    for (int i = 0; i < 10; i++) pop->individuals[0].alleles[i].binary_val = false;
    for (int i = 0; i < 10; i++) pop->individuals[1].alleles[i].binary_val = true;

    Chromosome c1, c2;
    memset(&c1, 0, sizeof(c1)); memset(&c2, 0, sizeof(c2));
    c1.num_genes = 10; c2.num_genes = 10;
    memcpy(c1.loci, loci, sizeof(Locus)*10);
    memcpy(c2.loci, loci, sizeof(Locus)*10);

    ga_crossover_uniform(&pop->individuals[0], &pop->individuals[1], &c1, &c2, 0.7);
    assert(ga_is_valid(&c1));
    assert(ga_is_valid(&c2));

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_mutate_bit_flip(void) {
    printf("  test_mutate_bit_flip... ");
    Locus *loci = make_loci(20);
    Population *pop = ga_population_create(1, loci, 20);
    for (int i = 0; i < 20; i++) pop->individuals[0].alleles[i].binary_val = false;

    /* With mutation_rate = 1.0, all bits should flip */
    ga_mutate_bit_flip(&pop->individuals[0], 1.0);
    for (int i = 0; i < 20; i++) {
        assert(pop->individuals[0].alleles[i].binary_val == true);
    }

    ga_population_destroy(pop);
    free(loci);
    printf("PASS\n");
}

static void test_replacement_elitist(void) {
    printf("  test_replacement_elitist... ");
    Locus *loci = make_loci(4);
    Population *parent = ga_population_create(10, loci, 4);
    Population *offspring = ga_population_create(10, loci, 4);

    ga_population_evaluate(parent, onemax_func, NULL);
    ga_population_evaluate(offspring, onemax_func, NULL);

    double best_before = parent->max_fitness;
    ga_replace_elitist(parent, offspring, 2);

    /* After elitism, best fitness should not decrease */
    ga_population_evaluate(parent, onemax_func, NULL);
    assert(parent->max_fitness >= best_before);

    ga_population_destroy(parent);
    ga_population_destroy(offspring);
    free(loci);
    printf("PASS\n");
}

static void test_operator_analysis(void) {
    printf("  test_operator_analysis... ");
    double dr = ga_crossover_disruption_rate(20, CROSSOVER_SINGLE_POINT);
    assert(dr > 0.0 && dr < 1.0);

    dr = ga_crossover_disruption_rate(20, CROSSOVER_TWO_POINT);
    assert(dr > 0.0 && dr < 1.0);

    double surv = ga_schema_survival_probability(5, 3, 0.01, 20);
    assert(surv > 0.0 && surv <= 1.0);

    double bias = ga_positional_bias(CROSSOVER_SINGLE_POINT, 20);
    assert(bias > 0.0);

    printf("PASS\n");
}

int main(void) {
    printf("=== test_operators ===\n");
    test_roulette_selection();
    test_tournament_selection();
    test_rank_selection();
    test_sus_selection();
    test_crossover_one_point();
    test_crossover_two_point();
    test_crossover_uniform();
    test_mutate_bit_flip();
    test_replacement_elitist();
    test_operator_analysis();
    printf("=== All operator tests passed ===\n");
    return 0;
}
