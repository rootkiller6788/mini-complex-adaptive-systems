/**
 * example_ga_onemax.c — End-to-end GA solving OneMax problem
 *
 * Demonstrates the complete GA pipeline: initializes a population,
 * evolves it through selection→crossover→mutation→replacement,
 * and reports convergence on the canonical OneMax benchmark.
 *
 * OneMax: f(x) = Σ x_i → maximize number of 1s in a binary chromosome.
 * This is the simplest GA benchmark with no deception or local optima.
 *
 * Reference: Holland (1975), Goldberg (1989)
 */

#include "ga_core.h"
#include "ga_operators.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* OneMax fitness: count the number of 1-bits */
static double onemax(const Chromosome *c, void *ud) {
    (void)ud;
    double sum = 0.0;
    for (int i = 0; i < c->num_genes; i++) {
        if (c->alleles[i].binary_val) sum += 1.0;
    }
    return sum;
}

int main(void) {
    printf("===== GA on OneMax Problem =====\n\n");

    int chrom_len = 30;
    int pop_size = 50;
    int generations = 100;

    /* Define binary loci */
    Locus *loci = (Locus*)malloc(sizeof(Locus) * (size_t)chrom_len);
    for (int i = 0; i < chrom_len; i++) {
        loci[i].position = i;
        loci[i].type = GENE_BINARY;
        loci[i].lower_bound = 0.0;
        loci[i].upper_bound = 1.0;
        loci[i].mutation_rate = 0.01;
        snprintf(loci[i].name, 63, "bit_%d", i);
    }

    /* Create population */
    Population *pop = ga_population_create(pop_size, loci, chrom_len);
    if (!pop) { printf("Failed to create population\n"); return 1; }

    /* Evaluate initial population */
    ga_population_evaluate(pop, onemax, NULL);
    printf("Generation %d: best=%.0f, avg=%.2f, worst=%.0f\n",
           0, pop->max_fitness, pop->avg_fitness, pop->min_fitness);

    /* Evolution loop */
    for (int gen = 1; gen <= generations; gen++) {
        /* Create offspring population */
        Population *offspring = ga_population_create(pop_size, loci, chrom_len);
        if (!offspring) break;

        /* Generate offspring via selection + crossover + mutation */
        int o_idx = 0;
        while (o_idx < pop_size - 1) {
            /* Tournament selection: pick 2 parents */
            int p1 = ga_select_tournament(pop, 3);
            int p2 = ga_select_tournament(pop, 3);

            /* Crossover: create children */
            Chromosome c1 = ga_chromosome_copy(&pop->individuals[p1]);
            Chromosome c2 = ga_chromosome_copy(&pop->individuals[p2]);
            c1.id = pop->next_id++;
            c2.id = pop->next_id++;
            c1.birth_generation = (uint64_t)gen;
            c2.birth_generation = (uint64_t)gen;

            if ((double)rand() / RAND_MAX < 0.8) {
                ga_crossover_one_point(&pop->individuals[p1],
                                        &pop->individuals[p2], &c1, &c2);
            }

            /* Mutation: flip bits */
            ga_mutate_bit_flip(&c1, 1.0 / (double)chrom_len);
            ga_mutate_bit_flip(&c2, 1.0 / (double)chrom_len);

            if (o_idx < pop_size) offspring->individuals[o_idx++] = c1;
            if (o_idx < pop_size) offspring->individuals[o_idx++] = c2;
        }
        offspring->size = o_idx;

        /* Evaluate offspring */
        ga_population_evaluate(offspring, onemax, NULL);

        /* Elitist replacement: keep 2 best from parent + best from offspring */
        ga_population_sort(pop);
        ga_population_sort(offspring);

        /* Keep best 2 parents, fill rest with best offspring */
        Chromosome saved[2];
        saved[0] = ga_chromosome_copy(&pop->individuals[0]);
        saved[1] = ga_chromosome_copy(&pop->individuals[1]);

        for (int i = 0; i < pop_size && i < offspring->size; i++) {
            pop->individuals[i] = ga_chromosome_copy(&offspring->individuals[i]);
        }
        pop->individuals[0] = saved[0];
        pop->individuals[1] = saved[1];

        ga_population_evaluate(pop, onemax, NULL);
        ga_population_destroy(offspring);

        if (gen % 10 == 0 || gen == 1) {
            printf("Generation %d: best=%.0f, avg=%.2f, diversity=%.2f\n",
                   gen, pop->max_fitness, pop->avg_fitness,
                   ga_population_diversity(pop));
        }

        /* Early stop if we solved the problem */
        if (pop->max_fitness >= (double)chrom_len - 0.5) {
            printf("\n>>> Optimal solution found at generation %d! <<<\n", gen);
            break;
        }
    }

    /* Print best solution */
    printf("\nBest solution (fitness = %.0f / %d):\n",
           pop->max_fitness, chrom_len);
    ga_population_sort(pop);
    printf("Genes: ");
    for (int i = 0; i < chrom_len; i++) {
        printf("%c", pop->individuals[0].alleles[i].binary_val ? '1' : '0');
        if ((i + 1) % 10 == 0) printf(" ");
    }
    printf("\n");

    /* Diversity analysis */
    printf("\nFinal population diversity: %.2f\n",
           ga_population_diversity(pop));
    printf("Gene entropy: %.4f\n", ga_gene_entropy(pop));

    ga_population_destroy(pop);
    free(loci);
    printf("\n===== Done =====\n");
    return 0;
}
