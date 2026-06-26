/**
 * example_schema_analysis.c — Schema Theorem validation on Royal Road
 *
 * Demonstrates schema processing: tracks schema instances across generations
 * to empirically validate Holland's Schema Theorem. Uses the Royal Road
 * fitness function to show building block assembly.
 *
 * Reference: Mitchell, Forrest & Holland (1992) — Royal Road
 *            Holland (1975) — Schema Theorem
 */

#include "ga_core.h"
#include "ga_operators.h"
#include "ga_theory.h"
#include "ga_fitness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(void) {
    printf("===== Schema Theorem Validation on Royal Road =====\n\n");

    int chrom_len = 32;  /* 4 blocks of 8 */
    int pop_size = 60;
    int generations = 50;

    /* Define binary loci */
    Locus *loci = (Locus*)malloc(sizeof(Locus) * (size_t)chrom_len);
    for (int i = 0; i < chrom_len; i++) {
        loci[i].position = i; loci[i].type = GENE_BINARY;
        loci[i].lower_bound = 0.0; loci[i].upper_bound = 1.0;
        loci[i].mutation_rate = 0.005;
        snprintf(loci[i].name, 63, "g%d", i);
    }

    Population *pop = ga_population_create(pop_size, loci, chrom_len);
    if (!pop) { free(loci); return 1; }

    ga_population_evaluate(pop, ga_fitness_royal_road, NULL);

    /* Define schema to track: H = 11111111********... (first block all-1) */
    Schema target_schema;
    memset(&target_schema, 0, sizeof(target_schema));
    target_schema.length = 8;
    for (int i = 0; i < 8; i++) target_schema.alleles[i] = SCHEMA_ALLELE_1;
    target_schema.order = 8;
    target_schema.defining_length = 7;

    printf("Tracking schema H = '11111111********...' (order=%d, δ=%d)\n",
           target_schema.order, target_schema.defining_length);
    printf("Generation | m(H,t) | f(H)    | f̄       | Schema LB | Schema UB\n");
    printf("-----------+--------+----------+----------+----------+----------\n");

    for (int gen = 0; gen <= generations; gen++) {
        int m_H = ga_schema_count(&target_schema, pop);
        double f_H = ga_schema_avg_fitness(&target_schema, pop);
        double f_avg = pop->avg_fitness;
        double lb = ga_schema_theorem_lower_bound(&target_schema, pop, 0.8, 0.005);
        double ub = ga_schema_theorem_upper_bound(&target_schema, pop, 0.8, 0.005);

        printf("  %5d    |  %4d   | %8.2f | %8.2f | %8.2f | %8.2f\n",
               gen, m_H, f_H, f_avg, lb, ub);

        if (gen >= generations) break;

        /* Evolve one generation */
        Population *offspring = ga_population_create(pop_size, loci, chrom_len);
        if (!offspring) break;

        for (int o = 0; o < pop_size; o += 2) {
            int p1 = ga_select_tournament(pop, 2);
            int p2 = ga_select_tournament(pop, 2);

            Chromosome c1 = ga_chromosome_copy(&pop->individuals[p1]);
            Chromosome c2 = ga_chromosome_copy(&pop->individuals[p2]);

            if ((double)rand() / RAND_MAX < 0.8 && chrom_len > 1) {
                ga_crossover_two_point(&pop->individuals[p1],
                                        &pop->individuals[p2], &c1, &c2);
            }
            ga_mutate_bit_flip(&c1, 0.005);
            ga_mutate_bit_flip(&c2, 0.005);

            if (o < pop_size) offspring->individuals[o] = c1;
            if (o + 1 < pop_size) offspring->individuals[o + 1] = c2;
        }
        offspring->size = pop_size;
        ga_population_evaluate(offspring, ga_fitness_royal_road, NULL);

        /* Replace with elitism */
        ga_population_sort(pop);
        ga_population_sort(offspring);
        for (int i = 2; i < pop_size; i++) {
            pop->individuals[i] = ga_chromosome_copy(&offspring->individuals[i]);
        }
        ga_population_evaluate(pop, ga_fitness_royal_road, NULL);
        ga_population_destroy(offspring);
    }

    /* Identify final building blocks */
    Schema blocks[30];
    int n_blocks = ga_identify_building_blocks(pop, 4, 8, blocks, 30);
    printf("\nIdentified %d building blocks (order≤4, δ≤8) in final population\n",
           n_blocks);
    if (n_blocks > 0) {
        char buf[128];
        for (int i = 0; i < (n_blocks < 3 ? n_blocks : 3); i++) {
            ga_schema_print(&blocks[i], buf, sizeof(buf));
            printf("  BB %d: %s (o=%d, δ=%d, bbv=%.2f)\n",
                   i+1, buf, blocks[i].order, blocks[i].defining_length,
                   ga_building_block_value(&blocks[i], pop));
        }
    }

    /* Implicit parallelism estimate */
    double ip = ga_implicit_parallelism(pop_size, chrom_len);
    printf("\nImplicit parallelism: ~%.0f schemata processed/generation\n", ip);

    ga_population_destroy(pop);
    free(loci);
    printf("\n===== Done =====\n");
    return 0;
}
