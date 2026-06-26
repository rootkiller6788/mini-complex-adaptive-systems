/**
 * test_core.c — Unit tests for GA core (population, chromosome, GA loop)
 *
 * Tests: chromosome creation, population initialization, evaluation,
 * sorting, distance metrics, convergence detection, gene entropy.
 */

#include "ga_core.h"
#include "ga_operators.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define ASSERT_NEAR(a, b, tol) assert(fabs((a) - (b)) < (tol))

static double onemax_func(const Chromosome *c, void *ud) {
    (void)ud;
    double sum = 0.0;
    for (int i = 0; i < c->num_genes; i++)
        if (c->alleles[i].binary_val) sum += 1.0;
    return sum;
}

static void test_chromosome_init(void) {
    printf("  test_chromosome_init... ");
    Locus loci[3] = {
        {0, GENE_BINARY, 0, 1, 0.01, "bit0"},
        {1, GENE_BINARY, 0, 1, 0.01, "bit1"},
        {2, GENE_BINARY, 0, 1, 0.01, "bit2"},
    };
    Population *pop = ga_population_create(10, loci, 3);
    assert(pop != NULL);
    assert(pop->size == 10);
    assert(pop->num_genes == 3);
    for (int i = 0; i < pop->size; i++) {
        assert(pop->individuals[i].num_genes == 3);
        assert(!pop->individuals[i].evaluated);
    }
    ga_population_destroy(pop);
    printf("PASS\n");
}

static void test_population_evaluate(void) {
    printf("  test_population_evaluate... ");
    Locus loci[4] = {
        {0, GENE_BINARY, 0, 1, 0.01, ""},
        {1, GENE_BINARY, 0, 1, 0.01, ""},
        {2, GENE_BINARY, 0, 1, 0.01, ""},
        {3, GENE_BINARY, 0, 1, 0.01, ""},
    };
    Population *pop = ga_population_create(20, loci, 4);
    ga_population_evaluate(pop, onemax_func, NULL);
    assert(pop->max_fitness <= 4.0);
    assert(pop->min_fitness >= 0.0);
    assert(pop->avg_fitness >= 0.0);
    assert(pop->best_index >= 0 && pop->best_index < pop->size);
    assert(pop->individuals[pop->best_index].evaluated);
    ga_population_destroy(pop);
    printf("PASS\n");
}

static void test_population_sort(void) {
    printf("  test_population_sort... ");
    Locus loci[5] = {
        {0, GENE_BINARY, 0, 1, 0.01, ""},
        {1, GENE_BINARY, 0, 1, 0.01, ""},
        {2, GENE_BINARY, 0, 1, 0.01, ""},
        {3, GENE_BINARY, 0, 1, 0.01, ""},
        {4, GENE_BINARY, 0, 1, 0.01, ""},
    };
    Population *pop = ga_population_create(15, loci, 5);
    ga_population_evaluate(pop, onemax_func, NULL);
    ga_population_sort(pop);
    double prev = pop->individuals[0].fitness;
    for (int i = 1; i < pop->size; i++) {
        assert(pop->individuals[i].fitness <= prev);
        prev = pop->individuals[i].fitness;
    }
    ga_population_destroy(pop);
    printf("PASS\n");
}

static void test_chromosome_copy(void) {
    printf("  test_chromosome_copy... ");
    Locus loci[2] = {{0, GENE_BINARY, 0, 1, 0.01, ""}, {1, GENE_BINARY, 0, 1, 0.01, ""}};
    Population *pop = ga_population_create(1, loci, 2);
    Chromosome copy = ga_chromosome_copy(&pop->individuals[0]);
    assert(copy.num_genes == 2);
    assert(copy.loci[0].type == GENE_BINARY);
    ga_population_destroy(pop);
    printf("PASS\n");
}

static void test_hamming_distance(void) {
    printf("  test_hamming_distance... ");
    Locus loci[4] = {
        {0, GENE_BINARY, 0, 1, 0.01, ""}, {1, GENE_BINARY, 0, 1, 0.01, ""},
        {2, GENE_BINARY, 0, 1, 0.01, ""}, {3, GENE_BINARY, 0, 1, 0.01, ""},
    };
    Population *pop = ga_population_create(2, loci, 4);
    /* Set first chromosome to all zeros */
    for (int i = 0; i < 4; i++) pop->individuals[0].alleles[i].binary_val = false;
    /* Set second chromosome to all ones */
    for (int i = 0; i < 4; i++) pop->individuals[1].alleles[i].binary_val = true;

    int dist = ga_hamming_distance(&pop->individuals[0], &pop->individuals[1]);
    assert(dist == 4);

    /* Same chromosome */
    dist = ga_hamming_distance(&pop->individuals[0], &pop->individuals[0]);
    assert(dist == 0);

    ga_population_destroy(pop);
    printf("PASS\n");
}

static void test_diversity(void) {
    printf("  test_diversity... ");
    Locus loci[8] = {
        {0, GENE_BINARY, 0, 1, 0.01, ""}, {1, GENE_BINARY, 0, 1, 0.01, ""},
        {2, GENE_BINARY, 0, 1, 0.01, ""}, {3, GENE_BINARY, 0, 1, 0.01, ""},
        {4, GENE_BINARY, 0, 1, 0.01, ""}, {5, GENE_BINARY, 0, 1, 0.01, ""},
        {6, GENE_BINARY, 0, 1, 0.01, ""}, {7, GENE_BINARY, 0, 1, 0.01, ""},
    };
    Population *pop = ga_population_create(30, loci, 8);
    double div = ga_population_diversity(pop);
    assert(div >= 0.0);
    double ent = ga_gene_entropy(pop);
    assert(ent >= 0.0 && ent <= 1.0);
    ga_population_destroy(pop);
    printf("PASS\n");
}

static void test_convergence(void) {
    printf("  test_convergence... ");
    Locus loci[4] = {
        {0, GENE_BINARY, 0, 1, 0.01, ""}, {1, GENE_BINARY, 0, 1, 0.01, ""},
        {2, GENE_BINARY, 0, 1, 0.01, ""}, {3, GENE_BINARY, 0, 1, 0.01, ""},
    };
    GAConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.population_size = 10;
    cfg.num_generations = 5;
    cfg.crossover_rate = 0.8;
    cfg.mutation_rate = 0.01;
    cfg.select_method = SELECT_TOURNAMENT;
    cfg.crossover_method = CROSSOVER_SINGLE_POINT;
    cfg.mutation_method = MUTATE_BIT_FLIP;
    cfg.replace_method = REPLACE_ELITIST;
    cfg.elitism_count = 2;
    cfg.tournament_size = 3;
    cfg.stall_generations = 3;

    GAState *ga = ga_init(&cfg, onemax_func, NULL);
    assert(ga != NULL);
    assert(!ga->converged);

    /* Replace dummy population with real one */
    ga_population_destroy(ga->parent_pop);
    ga_population_destroy(ga->offspring_pop);
    ga->parent_pop = ga_population_create(cfg.population_size, loci, 4);
    ga->offspring_pop = ga_population_create(cfg.population_size, loci, 4);

    /* Initial eval */
    ga_population_evaluate(ga->parent_pop, onemax_func, NULL);
    assert(ga->parent_pop->max_fitness >= 0.0);

    /* Run for a few generations */
    for (int g = 0; g < 5; g++) {
        ga_generation(ga);
    }
    assert(ga->current_gen >= 1);
    assert(ga->history_len >= 1);
    assert(ga->parent_pop->max_fitness >= 0.0);

    ga_destroy(ga);
    printf("PASS\n");
}

static void test_effective_population(void) {
    printf("  test_effective_population... ");
    Locus loci[4] = {
        {0, GENE_BINARY, 0, 1, 0.01, ""}, {1, GENE_BINARY, 0, 1, 0.01, ""},
        {2, GENE_BINARY, 0, 1, 0.01, ""}, {3, GENE_BINARY, 0, 1, 0.01, ""},
    };
    Population *pop = ga_population_create(16, loci, 4);
    ga_population_evaluate(pop, onemax_func, NULL);

    double probs[16];
    for (int i = 0; i < 16; i++) probs[i] = 1.0 / 16.0;
    double n_eff = ga_effective_population_size(pop, probs);
    ASSERT_NEAR(n_eff, 16.0, 0.1);

    ga_population_destroy(pop);
    printf("PASS\n");
}

int main(void) {
    printf("=== test_core ===\n");
    test_chromosome_init();
    test_population_evaluate();
    test_population_sort();
    test_chromosome_copy();
    test_hamming_distance();
    test_diversity();
    test_convergence();
    test_effective_population();
    printf("=== All core tests passed ===\n");
    return 0;
}
