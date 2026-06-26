/**
 * example_ga_function_opt.c — GA for continuous function optimization
 *
 * Uses real-coded GA with arithmetic crossover and Gaussian mutation
 * to optimize the Rastrigin benchmark function, a highly multimodal
 * test function widely used to benchmark evolutionary algorithms.
 *
 * Rastrigin: f(x) = 10n + Σ[x_i² - 10 cos(2π x_i)]
 * Global minimum: x_i = 0, f(x) = 0
 * Search space: [-5.12, 5.12]^n
 *
 * Compares GA against this classical benchmark and reports convergence
 * behavior, schema analysis, and diversity metrics.
 *
 * Reference: Rastrigin (1974), Bäck & Schwefel (1993)
 */

#include "ga_core.h"
#include "ga_operators.h"
#include "ga_fitness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double rastrigin_fitness(const Chromosome *c, void *ud) {
    (void)ud;
    if (!c || c->num_genes <= 0) return 0.0;

    int n = c->num_genes;
    double sum = 10.0 * (double)n;
    for (int i = 0; i < n; i++) {
        double x = (c->loci[i].type == GENE_REAL)
                   ? c->alleles[i].real_val
                   : (double)c->alleles[i].integer_val;
        sum += x * x - 10.0 * cos(2.0 * M_PI * x);
    }
    return -sum; /* GA maximizes → minimize Rastrigin by negating */
}

int main(void) {
    printf("===== Real-Coded GA: Rastrigin Function Optimization =====\n\n");

    int n_dimensions = 10;   /* 10-dimensional Rastrigin */
    int pop_size = 80;
    int generations = 300;

    /* Define real-valued loci in [-5.12, 5.12] */
    Locus *loci = (Locus*)malloc(sizeof(Locus) * (size_t)n_dimensions);
    for (int i = 0; i < n_dimensions; i++) {
        loci[i].position = i;
        loci[i].type = GENE_REAL;
        loci[i].lower_bound = -5.12;
        loci[i].upper_bound = 5.12;
        loci[i].mutation_rate = 1.0 / (double)n_dimensions;
        snprintf(loci[i].name, 63, "x_%d", i);
    }

    Population *pop = ga_population_create(pop_size, loci, n_dimensions);
    if (!pop) { printf("Failed to create population\n"); free(loci); return 1; }

    /* Evaluate initial population */
    ga_population_evaluate(pop, rastrigin_fitness, NULL);

    double true_fitness = -pop->max_fitness;
    printf("Generation %3d: best=%.4f (actual Rastrigin value), avg=%.4f\n",
           0, true_fitness, -pop->avg_fitness);

    /* Evolution loop */
    double best_so_far = -pop->max_fitness;
    int stall_count = 0;

    for (int gen = 1; gen <= generations; gen++) {
        Population *offspring = ga_population_create(pop_size, loci,
                                                      n_dimensions);
        if (!offspring) break;

        int o_idx = 0;
        /* Elitism: copy best 2 */
        ga_population_sort(pop);
        offspring->individuals[o_idx++] = ga_chromosome_copy(&pop->individuals[0]);
        offspring->individuals[o_idx++] = ga_chromosome_copy(&pop->individuals[1]);

        while (o_idx < pop_size - 1) {
            int p1 = ga_select_tournament(pop, 3);
            int p2 = ga_select_tournament(pop, 3);

            Chromosome c1 = ga_chromosome_copy(&pop->individuals[p1]);
            Chromosome c2 = ga_chromosome_copy(&pop->individuals[p2]);

            /* Arithmetic crossover for real values */
            if ((double)rand() / RAND_MAX < 0.75) {
                ga_crossover_arithmetic(&pop->individuals[p1],
                                         &pop->individuals[p2],
                                         &c1, &c2, 0.25);
            }

            /* Gaussian mutation */
            ga_mutate_gaussian(&c1, 1.0 / (double)n_dimensions, 0.1);
            ga_mutate_gaussian(&c2, 1.0 / (double)n_dimensions, 0.1);

            if (o_idx < pop_size) offspring->individuals[o_idx++] = c1;
            if (o_idx < pop_size) offspring->individuals[o_idx++] = c2;
        }
        offspring->size = o_idx;

        ga_population_evaluate(offspring, rastrigin_fitness, NULL);
        ga_replace_elitist(pop, offspring, 2);
        ga_population_evaluate(pop, rastrigin_fitness, NULL);
        ga_population_destroy(offspring);

        double current_best = -pop->max_fitness;
        if (current_best < best_so_far) {
            best_so_far = current_best;
            stall_count = 0;
        } else {
            stall_count++;
        }

        /* Adaptive mutation: increase when stalled */
        if (stall_count > 30 && stall_count % 10 == 0) {
            /* Re-randomize worst half of population to increase diversity */
            ga_population_sort(pop);
            for (int i = pop->size / 2; i < pop->size; i++) {
                for (int g = 0; g < n_dimensions; g++) {
                    pop->individuals[i].alleles[g].real_val =
                        loci[g].lower_bound +
                        (double)rand() / RAND_MAX *
                        (loci[g].upper_bound - loci[g].lower_bound);
                }
            }
            stall_count = 0;
        }

        if (gen % 30 == 0 || gen == 1) {
            double div = ga_population_diversity(pop);
            double ent = ga_gene_entropy(pop);
            printf("Generation %3d: best=%.6f, avg=%.4f, div=%.2f, ent=%.4f\n",
                   gen, current_best, -pop->avg_fitness, div, ent);
        }

        /* Convergence check */
        if (current_best < 0.01) {
            printf("\n>>> Converged to near-global optimum at generation %d! <<<\n",
                   gen);
            break;
        }
    }

    /* Print final solution */
    ga_population_sort(pop);
    printf("\nFinal best solution (Rastrigin value = %.6f):\n",
           -pop->max_fitness);
    printf("Variables: ");
    for (int i = 0; i < n_dimensions; i++) {
        printf("%.6f", pop->individuals[0].alleles[i].real_val);
        if (i < n_dimensions - 1) printf(", ");
    }
    printf("\n");

    /* Landscape analysis on final population */
    printf("\n--- Fitness Landscape Analysis ---\n");
    double fdc = ga_fdc(pop, &pop->individuals[0]);
    printf("Fitness-Distance Correlation: %.4f\n", fdc);
    double epistasis = ga_epistasis_measure(pop);
    printf("Epistasis measure: %.4f\n", epistasis);
    int local_opts = ga_count_local_optima(pop, 0.5);
    printf("Approximate local optima count: %d\n", local_opts);

    ga_population_destroy(pop);
    free(loci);
    printf("\n===== Done =====\n");
    return 0;
}
