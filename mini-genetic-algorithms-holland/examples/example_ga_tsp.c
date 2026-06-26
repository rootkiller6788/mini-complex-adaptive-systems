/**
 * example_ga_tsp.c — GA solving the Traveling Salesman Problem
 *
 * Uses permutation encoding with Order Crossover (OX) and swap mutation
 * to find near-optimal tours for a Euclidean TSP instance.
 *
 * TSP: Given N cities with coordinates, find the shortest Hamiltonian cycle
 * visiting each city exactly once and returning to the start.
 *
 * Reference: Goldberg & Lingle (1985) — PMX for TSP
 *            Larrañaga et al. (1999) — GA for TSP review
 */

#include "ga_core.h"
#include "ga_operators.h"
#include "ga_encoding.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N_CITIES 15

/* City coordinates (15-city problem) */
static double cities_x[N_CITIES] = {
    0.0, 3.0, 6.0, 8.0, 10.0, 4.0, 2.0, 7.0,
    9.0, 1.0, 5.0, 3.5, 7.5, 8.5, 6.5
};
static double cities_y[N_CITIES] = {
    0.0, 2.0, 1.0, 4.0, 3.0, 6.0, 7.0, 5.0,
    2.0, 4.0, 0.0, 8.0, 3.0, 6.0, 1.0
};

static double city_distance(int i, int j) {
    double dx = cities_x[i] - cities_x[j];
    double dy = cities_y[i] - cities_y[j];
    return sqrt(dx*dx + dy*dy);
}

static double tsp_fitness(const Chromosome *c, void *ud) {
    (void)ud;
    double total = 0.0;
    for (int i = 0; i < c->num_genes; i++) {
        int from = c->alleles[i].integer_val;
        int to = c->alleles[(i + 1) % c->num_genes].integer_val;
        total += city_distance(from, to);
    }
    return -total; /* maximize negative distance = minimize tour length */
}

int main(void) {
    printf("===== GA for Traveling Salesman Problem (%d cities) =====\n\n",
           N_CITIES);

    int pop_size = 100;
    int generations = 200;

    /* Define permutation loci */
    Locus *loci = (Locus*)malloc(sizeof(Locus) * (size_t)N_CITIES);
    for (int i = 0; i < N_CITIES; i++) {
        loci[i].position = i;
        loci[i].type = GENE_PERMUTATION;
        loci[i].lower_bound = 0.0;
        loci[i].upper_bound = (double)(N_CITIES - 1);
        loci[i].mutation_rate = 0.05;
        snprintf(loci[i].name, 63, "city_%d", i);
    }

    Population *pop = ga_population_create(pop_size, loci, N_CITIES);
    if (!pop) { printf("Failed to create population\n"); free(loci); return 1; }

    ga_population_evaluate(pop, tsp_fitness, NULL);
    printf("Generation %d: best tour length = %.2f, avg = %.2f\n",
           0, -pop->max_fitness, -pop->avg_fitness);

    double best_tour_length = -pop->max_fitness;

    for (int gen = 1; gen <= generations; gen++) {
        Population *offspring = ga_population_create(pop_size, loci, N_CITIES);
        if (!offspring) break;

        /* Elitism: keep 2 best parents */
        ga_population_sort(pop);
        offspring->individuals[0] = ga_chromosome_copy(&pop->individuals[0]);
        offspring->individuals[1] = ga_chromosome_copy(&pop->individuals[1]);
        int o_idx = 2;

        while (o_idx < pop_size - 1) {
            int p1 = ga_select_tournament(pop, 3);
            int p2 = ga_select_tournament(pop, 3);

            Chromosome c1 = ga_chromosome_copy(&pop->individuals[p1]);
            Chromosome c2 = ga_chromosome_copy(&pop->individuals[p2]);
            c1.id = pop->next_id++;
            c2.id = pop->next_id++;

            /* Order Crossover for permutation */
            if ((double)rand() / RAND_MAX < 0.9) {
                ga_crossover_ox(&pop->individuals[p1],
                                 &pop->individuals[p2], &c1, &c2);
            }

            /* Swap mutation */
            if ((double)rand() / RAND_MAX < 0.1) ga_mutate_swap(&c1);
            if ((double)rand() / RAND_MAX < 0.1) ga_mutate_swap(&c2);

            if (o_idx < pop_size) offspring->individuals[o_idx++] = c1;
            if (o_idx < pop_size) offspring->individuals[o_idx++] = c2;
        }
        offspring->size = o_idx;
        ga_population_evaluate(offspring, tsp_fitness, NULL);

        /* Replace parent with offspring (elitist) */
        ga_replace_elitist(pop, offspring, 2);
        ga_population_evaluate(pop, tsp_fitness, NULL);
        ga_population_destroy(offspring);

        if (-pop->max_fitness < best_tour_length) {
            best_tour_length = -pop->max_fitness;
            if (gen % 20 == 0) {
                printf("Generation %d: best tour length = %.2f (new best!)\n",
                       gen, best_tour_length);
            }
        } else if (gen % 50 == 0) {
            printf("Generation %d: best tour length = %.2f, avg = %.2f\n",
                   gen, -pop->max_fitness, -pop->avg_fitness);
        }
    }

    /* Print best tour found */
    ga_population_sort(pop);
    printf("\nBest tour (length = %.2f):\n", -pop->max_fitness);
    printf("Tour order:");
    for (int i = 0; i < N_CITIES; i++) {
        printf(" %d", pop->individuals[0].alleles[i].integer_val);
        if (i < N_CITIES - 1) printf(" →");
    }
    printf(" → %d\n", pop->individuals[0].alleles[0].integer_val);

    /* Verify it's a valid permutation */
    bool valid = ga_is_valid_permutation(&pop->individuals[0]);
    printf("Valid permutation: %s\n", valid ? "YES" : "NO");

    ga_population_destroy(pop);
    free(loci);
    printf("\n===== Done =====\n");
    return 0;
}
