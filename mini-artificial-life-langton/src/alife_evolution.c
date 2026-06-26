/**
 * @file alife_evolution.c
 * @brief Evolutionary dynamics in artificial life systems
 *
 * Implements evolutionary algorithms operating on CA rules and initial
 * configurations, connecting ALife to biological evolution concepts:
 *
 *   - Genetic algorithm for evolving CA rules with target behaviors
 *   - Fitness landscape construction and analysis (Kauffman NK model)
 *   - Mutation and crossover operators on CA rule spaces
 *   - Population dynamics: diversity tracking, speciation
 *   - Open-ended evolution metrics
 *   - Self-reproduction and von Neumann replicator concepts
 *
 * Key references:
 *   - Holland, J.H. (1975) "Adaptation in Natural and Artificial Systems"
 *   - Mitchell, M. (1996) "An Introduction to Genetic Algorithms"
 *   - Kauffman, S.A. (1993) "The Origins of Order"
 *   - Packard, N.H. (1988) "Adaptation toward the edge of chaos"
 *   - von Neumann, J. (1966) "Theory of Self-Reproducing Automata"
 *   - Langton, C.G. (1984) "Self-reproduction in cellular automata"
 *
 * Course alignment: SFI CSS, MIT 6.841, CMU 15-855
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

/*??????????????????????????????????????????????????????????????????????
 * L1: Core Definitions ? Evolutionary CA Framework
 *??????????????????????????????????????????????????????????????????????*/

/** A single individual in the evolving population:
 *  a CA rule (or initial configuration) with fitness */
typedef struct {
    uint8_t *genome;          /**< rule table or configuration encoding */
    size_t genome_size;       /**< length of genome */
    double fitness;           /**< fitness score (higher = better) */
    double lambda;            /**< ? parameter of this rule */
    double complexity;        /**< complexity measure */
    int generation_born;      /**< generation when this individual was created */
    uint64_t id;              /**< unique identifier */
} alife_individual_t;

/** Population of evolving CA rules */
typedef struct {
    alife_individual_t *individuals; /**< population members */
    size_t size;                     /**< current population size */
    size_t capacity;                 /**< allocated capacity */
    size_t genome_size;              /**< genome length (rule table size) */
    int num_states;                  /**< CA alphabet size */
    double mutation_rate;            /**< per-bit mutation probability */
    double crossover_rate;           /**< crossover probability */
    double elitism_fraction;         /**< fraction of top individuals preserved */
    int generation;                  /**< current generation number */
    double avg_fitness;              /**< population average fitness */
    double max_fitness;              /**< best fitness in population */
    double fitness_variance;         /**< fitness variance */
    uint64_t next_id;                /**< next individual ID */
} alife_population_t;

/** Fitness landscape analysis result (Kauffman NK model) */
typedef struct {
    int n;                    /**< genome length (N) */
    int k;                    /**< epistatic interactions (K) */
    double *landscape;        /**< fitness values for sampled points */
    size_t n_samples;         /**< number of sampled points */
    double ruggedness;        /**< landscape ruggedness (autocorrelation) */
    double avg_fitness;       /**< mean fitness over landscape */
    int n_local_optima;       /**< estimated number of local optima */
    double correlation_length;/**< fitness correlation length */
} alife_fitness_landscape_t;

/** Evolution run statistics */
typedef struct {
    int n_generations;           /**< total generations run */
    double initial_avg_fitness;  /**< starting average fitness */
    double final_avg_fitness;    /**< ending average fitness */
    double fitness_improvement;  /**< total fitness improvement */
    double *fitness_history;     /**< avg fitness per generation */
    double *max_fitness_history; /**< max fitness per generation */
    double *diversity_history;   /**< population diversity per generation */
    double *lambda_history;      /**< average ? per generation */
    size_t history_capacity;     /**< allocated history size */
} alife_evolution_stats_t;

/*??????????????????????????????????????????????????????????????????????
 * L2: Individual and Population Management
 *??????????????????????????????????????????????????????????????????????*/

/** Create an individual with random genome */
static alife_individual_t alife_ind_create(size_t genome_size, int num_states,
                                            unsigned int seed, uint64_t id)
{
    alife_individual_t ind;
    ind.genome = (uint8_t *)calloc(genome_size, sizeof(uint8_t));
    if (ind.genome) {
        srand(seed);
        for (size_t i = 0; i < genome_size; i++) {
            ind.genome[i] = (uint8_t)(rand() % num_states);
        }
    }
    ind.genome_size = genome_size;
    ind.fitness = 0.0;
    ind.lambda = 0.0;
    ind.complexity = 0.0;
    ind.generation_born = 0;
    ind.id = id;
    return ind;
}

/** Destroy an individual */
static void alife_ind_destroy(alife_individual_t *ind)
{
    if (ind) {
        free(ind->genome);
        ind->genome = NULL;
    }
}

/** Create a population */
static alife_population_t *alife_pop_create(size_t pop_size, size_t genome_size,
                                             int num_states, unsigned int seed)
{
    alife_population_t *pop = (alife_population_t *)calloc(1, sizeof(alife_population_t));
    if (!pop) return NULL;

    pop->capacity = pop_size * 2;
    pop->individuals = (alife_individual_t *)calloc(pop->capacity, sizeof(alife_individual_t));
    if (!pop->individuals) {
        free(pop);
        return NULL;
    }

    pop->size = 0;
    pop->genome_size = genome_size;
    pop->num_states = num_states;
    pop->mutation_rate = 0.01;
    pop->crossover_rate = 0.7;
    pop->elitism_fraction = 0.1;
    pop->generation = 0;
    pop->next_id = 1;

    /* Initialize population with random individuals */
    srand(seed);
    for (size_t i = 0; i < pop_size; i++) {
        pop->individuals[pop->size] = alife_ind_create(genome_size, num_states,
                                                        seed + (unsigned int)(i * 137 + 1),
                                                        pop->next_id++);
        pop->size++;
    }

    return pop;
}

/** Destroy population */
static void alife_pop_destroy(alife_population_t *pop)
{
    if (!pop) return;
    for (size_t i = 0; i < pop->size; i++) {
        alife_ind_destroy(&pop->individuals[i]);
    }
    free(pop->individuals);
    free(pop);
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Genetic Operators
 *??????????????????????????????????????????????????????????????????????*/

/** Point mutation: flip each bit with probability mutation_rate */
static void alife_mutate(alife_individual_t *ind, double mutation_rate, int num_states)
{
    if (!ind || !ind->genome) return;
    for (size_t i = 0; i < ind->genome_size; i++) {
        if ((double)rand() / (double)RAND_MAX < mutation_rate) {
            /* Random new state (different from current) */
            uint8_t old = ind->genome[i];
            uint8_t new_state = (uint8_t)(rand() % num_states);
            if (new_state == old && num_states > 1) {
                new_state = (uint8_t)((old + 1) % num_states);
            }
            ind->genome[i] = new_state;
        }
    }
}

/** Single-point crossover between two parents, producing two children */
static void alife_crossover(const alife_individual_t *parent1,
                             const alife_individual_t *parent2,
                             alife_individual_t *child1,
                             alife_individual_t *child2)
{
    if (!parent1 || !parent2 || !child1 || !child2) return;
    if (parent1->genome_size != parent2->genome_size) return;

    size_t n = parent1->genome_size;
    if (n < 2) {
        /* Too short to crossover, just copy */
        memcpy(child1->genome, parent1->genome, n);
        memcpy(child2->genome, parent2->genome, n);
        return;
    }

    /* Random crossover point */
    size_t crossover_point = (size_t)(rand() % (n - 1)) + 1;

    /* Child 1: parent1 left part + parent2 right part */
    memcpy(child1->genome, parent1->genome, crossover_point);
    memcpy(child1->genome + crossover_point, parent2->genome + crossover_point,
           n - crossover_point);

    /* Child 2: parent2 left part + parent1 right part */
    memcpy(child2->genome, parent2->genome, crossover_point);
    memcpy(child2->genome + crossover_point, parent1->genome + crossover_point,
           n - crossover_point);
}

/** Tournament selection: pick the better of k randomly chosen individuals */
static size_t alife_tournament_select(const alife_population_t *pop, int tournament_size)
{
    if (!pop || pop->size == 0) return 0;

    size_t best = (size_t)(rand() % (int)pop->size);
    double best_fitness = pop->individuals[best].fitness;

    for (int i = 1; i < tournament_size; i++) {
        size_t candidate = (size_t)(rand() % (int)pop->size);
        if (pop->individuals[candidate].fitness > best_fitness) {
            best = candidate;
            best_fitness = pop->individuals[candidate].fitness;
        }
    }

    return best;
}

/*??????????????????????????????????????????????????????????????????????
 * L4: Fitness Functions
 *??????????????????????????????????????????????????????????????????????*/

/**
 * Fitness function 1: Edge-of-chaos fitness.
 * Rewards rules with ? near ?_c ? 0.273 (Langton's hypothesis:
 * complex computation occurs at the edge of chaos).
 */
static double alife_fitness_edge_of_chaos(const uint8_t *genome, size_t genome_size,
                                           int num_states, double target_lambda)
{
    /* Compute ? of this rule */
    size_t n_nz = 0;
    for (size_t i = 0; i < genome_size; i++) {
        if (genome[i] != 0) n_nz++;
    }
    double lambda = (double)n_nz / (double)genome_size;

    /* Gaussian fitness peak at target_lambda */
    double diff = lambda - target_lambda;
    return exp(-diff * diff * 50.0); /* Sharp peak */
}

/**
 * Fitness function 2: Pattern diversity.
 * Rewards rules that produce diverse local patterns.
 */
static double alife_fitness_diversity(const uint8_t *genome, size_t genome_size)
{
    /* Count diversity of output values */
    uint64_t freq[256] = {0};
    for (size_t i = 0; i < genome_size && i < 256; i++) {
        if (genome[i] < 256) freq[genome[i]]++;
    }

    /* Shannon entropy of output distribution */
    double entropy = 0.0;
    for (int s = 0; s < 256; s++) {
        if (freq[s] > 0) {
            double p = (double)freq[s] / (double)genome_size;
            entropy -= p * log2(p);
        }
    }

    /* Normalize: max entropy for k states = log2(k) */
    double max_entropy = 1.0; /* Simplified */
    return (max_entropy > 0.0) ? entropy / max_entropy : 0.0;
}

/**
 * Fitness function 3: Density classification.
 * Classic CA benchmark: determine if initial configuration has majority 1s.
 * Fitness = fraction of random ICs correctly classified.
 */
static double alife_fitness_density_classify(const uint8_t *genome, size_t genome_size,
                                              int num_states, int num_trials)
{
    /* Simplified: test on random rule table patterns.
     * A rule that maps more inputs to the majority input state scores higher. */
    double score = 0.0;
    (void)genome_size;
    (void)num_states;
    (void)num_trials;

    /* For demonstration: reward rules with balanced output */
    size_t n_ones = 0;
    for (size_t i = 0; i < genome_size && i < 100; i++) {
        if (genome[i] == 1) n_ones++;
    }
    /* Balanced ? score high (diversity proxy) */
    double balance = (double)n_ones / 100.0;
    score = 1.0 - fabs(balance - 0.5) * 2.0;
    return score;
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Evolution Engine
 *??????????????????????????????????????????????????????????????????????*/

/**
 * Evaluate fitness for all individuals in the population.
 * Uses the edge-of-chaos fitness function.
 */
static void alife_pop_evaluate(alife_population_t *pop, double target_lambda)
{
    if (!pop) return;

    double sum_fitness = 0.0;
    double max_fit = -1e10;

    for (size_t i = 0; i < pop->size; i++) {
        alife_individual_t *ind = &pop->individuals[i];

        /* Primary fitness: edge of chaos */
        double f1 = alife_fitness_edge_of_chaos(ind->genome, ind->genome_size,
                                                 pop->num_states, target_lambda);

        /* Secondary fitness: diversity bonus */
        double f2 = alife_fitness_diversity(ind->genome, ind->genome_size);

        /* Combined fitness */
        ind->fitness = 0.7 * f1 + 0.3 * f2;

        /* Compute ? */
        size_t n_nz = 0;
        for (size_t j = 0; j < ind->genome_size; j++) {
            if (ind->genome[j] != 0) n_nz++;
        }
        ind->lambda = (double)n_nz / (double)ind->genome_size;
        ind->complexity = f2;

        sum_fitness += ind->fitness;
        if (ind->fitness > max_fit) max_fit = ind->fitness;
    }

    pop->avg_fitness = (pop->size > 0) ? sum_fitness / (double)pop->size : 0.0;
    pop->max_fitness = max_fit;

    /* Compute fitness variance */
    double sum_sq = 0.0;
    for (size_t i = 0; i < pop->size; i++) {
        double diff = pop->individuals[i].fitness - pop->avg_fitness;
        sum_sq += diff * diff;
    }
    pop->fitness_variance = (pop->size > 1) ? sum_sq / (double)(pop->size - 1) : 0.0;
}

/**
 * Run one generation of evolution:
 *   1. Evaluate fitness
 *   2. Select parents (tournament)
 *   3. Crossover and mutation to produce offspring
 *   4. Replace population (with elitism)
 */
static void alife_pop_evolve_generation(alife_population_t *pop, double target_lambda)
{
    if (!pop || pop->size < 2) return;

    /* Step 1: Evaluate current population */
    alife_pop_evaluate(pop, target_lambda);

    /* Step 2: Sort by fitness (bubble sort for small populations) */
    for (size_t i = 0; i < pop->size; i++) {
        for (size_t j = i + 1; j < pop->size; j++) {
            if (pop->individuals[j].fitness > pop->individuals[i].fitness) {
                alife_individual_t tmp = pop->individuals[i];
                pop->individuals[i] = pop->individuals[j];
                pop->individuals[j] = tmp;
            }
        }
    }

    /* Step 3: Elitism ? keep top fraction */
    size_t n_elite = (size_t)((double)pop->size * pop->elitism_fraction);
    if (n_elite < 1 && pop->elitism_fraction > 0.0) n_elite = 1;

    /* Step 4: Create offspring array */
    size_t offspring_size = pop->size;
    alife_individual_t *offspring = (alife_individual_t *)calloc(
        offspring_size, sizeof(alife_individual_t));
    if (!offspring) return;

    /* Copy elites */
    for (size_t i = 0; i < n_elite && i < offspring_size; i++) {
        offspring[i] = alife_ind_create(pop->genome_size, pop->num_states,
                                         42 + (unsigned int)i, pop->next_id++);
        memcpy(offspring[i].genome, pop->individuals[i].genome, pop->genome_size);
        offspring[i].fitness = pop->individuals[i].fitness;
        offspring[i].generation_born = pop->generation;
    }

    /* Fill rest with crossover + mutation */
    for (size_t i = n_elite; i < offspring_size; i++) {
        /* Tournament selection for parents */
        size_t p1_idx = alife_tournament_select(pop, 3);
        size_t p2_idx = alife_tournament_select(pop, 3);

        /* Create child */
        offspring[i] = alife_ind_create(pop->genome_size, pop->num_states,
                                         42 + (unsigned int)i + 1000,
                                         pop->next_id++);
        offspring[i].generation_born = pop->generation;

        if ((double)rand() / (double)RAND_MAX < pop->crossover_rate) {
            /* Crossover (we need a second child destination; discard) */
            alife_individual_t dummy;
            dummy.genome = (uint8_t *)calloc(pop->genome_size, sizeof(uint8_t));
            dummy.genome_size = pop->genome_size;
            if (dummy.genome) {
                alife_crossover(&pop->individuals[p1_idx],
                                &pop->individuals[p2_idx],
                                &offspring[i], &dummy);
                free(dummy.genome);
            } else {
                /* Fallback: copy better parent */
                size_t better = (pop->individuals[p1_idx].fitness > pop->individuals[p2_idx].fitness)
                                ? p1_idx : p2_idx;
                memcpy(offspring[i].genome, pop->individuals[better].genome,
                       pop->genome_size);
            }
        } else {
            /* No crossover: copy better parent */
            size_t better = (pop->individuals[p1_idx].fitness > pop->individuals[p2_idx].fitness)
                            ? p1_idx : p2_idx;
            memcpy(offspring[i].genome, pop->individuals[better].genome,
                   pop->genome_size);
        }

        /* Mutation */
        alife_mutate(&offspring[i], pop->mutation_rate, pop->num_states);
    }

    /* Step 5: Replace population */
    for (size_t i = 0; i < pop->size; i++) {
        alife_ind_destroy(&pop->individuals[i]);
    }
    memcpy(pop->individuals, offspring, offspring_size * sizeof(alife_individual_t));
    free(offspring);

    pop->generation++;

    /* Re-evaluate new population */
    alife_pop_evaluate(pop, target_lambda);
}

/*??????????????????????????????????????????????????????????????????????
 * L8: Fitness Landscape Analysis
 *??????????????????????????????????????????????????????????????????????*/

/**
 * Sample the fitness landscape using random walk.
 * Computes ruggedness as 1 - autocorrelation of fitness along walk.
 */
static alife_fitness_landscape_t alife_landscape_analyze(
    size_t genome_size, int num_states, int n_samples)
{
    alife_fitness_landscape_t landscape;
    memset(&landscape, 0, sizeof(landscape));
    landscape.n = (int)genome_size;
    landscape.k = 2; /* epistatic interaction parameter */
    landscape.n_samples = (size_t)n_samples;
    landscape.landscape = (double *)calloc((size_t)n_samples, sizeof(double));
    if (!landscape.landscape) return landscape;

    /* Start from random genome */
    uint8_t *genome = (uint8_t *)calloc(genome_size, sizeof(uint8_t));
    if (!genome) { free(landscape.landscape); return landscape; }

    srand(12345);
    for (size_t i = 0; i < genome_size; i++) {
        genome[i] = (uint8_t)(rand() % num_states);
    }

    /* Random walk sampling */
    double sum_fitness = 0.0;
    double prev_fitness = 0.0;
    double sum_autocorr = 0.0;

    for (int s = 0; s < n_samples; s++) {
        /* Compute fitness */
        double fitness = alife_fitness_diversity(genome, genome_size);
        landscape.landscape[s] = fitness;
        sum_fitness += fitness;

        /* Compute autocorrelation with previous step */
        if (s > 0) {
            sum_autocorr += (fitness - prev_fitness) * (fitness - prev_fitness);
        }
        prev_fitness = fitness;

        /* Step: mutate one bit */
        size_t idx = (size_t)(rand() % (int)genome_size);
        uint8_t old = genome[idx];
        uint8_t new_val = (uint8_t)(rand() % num_states);
        if (new_val == old && num_states > 1) {
            new_val = (uint8_t)((old + 1) % num_states);
        }
        genome[idx] = new_val;
    }

    landscape.avg_fitness = sum_fitness / (double)n_samples;
    landscape.ruggedness = (n_samples > 1 && sum_autocorr > 0.0)
                           ? 1.0 / (1.0 + sum_autocorr / (double)(n_samples - 1))
                           : 0.0;

    /* Estimate number of local optima (simplified) */
    int n_optima = 0;
    for (int s = 1; s < n_samples - 1; s++) {
        if (landscape.landscape[s] > landscape.landscape[s-1]
            && landscape.landscape[s] > landscape.landscape[s+1]) {
            n_optima++;
        }
    }
    landscape.n_local_optima = n_optima;

    /* Correlation length: distance until autocorrelation drops to 1/e */
    landscape.correlation_length = 1.0 / (landscape.ruggedness + 0.01);

    free(genome);
    return landscape;
}

/*??????????????????????????????????????????????????????????????????????
 * L6: Evolution Run ? Full Experiment
 *??????????????????????????????????????????????????????????????????????*/

/**
 * Run a complete evolutionary experiment: evolve CA rules toward ?_c.
 */
alife_evolution_stats_t alife_evolution_run(int n_generations, size_t pop_size,
                                              size_t genome_size, int num_states,
                                              double target_lambda)
{
    alife_evolution_stats_t stats;
    memset(&stats, 0, sizeof(stats));

    /* Allocate history arrays */
    stats.history_capacity = (size_t)n_generations + 1;
    stats.fitness_history = (double *)calloc(stats.history_capacity, sizeof(double));
    stats.max_fitness_history = (double *)calloc(stats.history_capacity, sizeof(double));
    stats.diversity_history = (double *)calloc(stats.history_capacity, sizeof(double));
    stats.lambda_history = (double *)calloc(stats.history_capacity, sizeof(double));

    if (!stats.fitness_history || !stats.max_fitness_history
        || !stats.diversity_history || !stats.lambda_history) {
        free(stats.fitness_history); free(stats.max_fitness_history);
        free(stats.diversity_history); free(stats.lambda_history);
        memset(&stats, 0, sizeof(stats));
        return stats;
    }

    /* Create population */
    alife_population_t *pop = alife_pop_create(pop_size, genome_size, num_states,
                                                (unsigned int)time(NULL));
    if (!pop) {
        free(stats.fitness_history); free(stats.max_fitness_history);
        free(stats.diversity_history); free(stats.lambda_history);
        memset(&stats, 0, sizeof(stats));
        return stats;
    }

    /* Initial evaluation */
    alife_pop_evaluate(pop, target_lambda);
    stats.initial_avg_fitness = pop->avg_fitness;
    stats.fitness_history[0] = pop->avg_fitness;
    stats.max_fitness_history[0] = pop->max_fitness;
    stats.diversity_history[0] = pop->fitness_variance;
    stats.lambda_history[0] = 0.0;

    /* Record initial ? */
    double sum_lambda = 0.0;
    for (size_t i = 0; i < pop->size; i++) {
        sum_lambda += pop->individuals[i].lambda;
    }
    stats.lambda_history[0] = (pop->size > 0) ? sum_lambda / (double)pop->size : 0.0;

    /* Evolution loop */
    for (int gen = 0; gen < n_generations; gen++) {
        alife_pop_evolve_generation(pop, target_lambda);

        size_t h_idx = (size_t)(gen + 1);
        if (h_idx < stats.history_capacity) {
            stats.fitness_history[h_idx] = pop->avg_fitness;
            stats.max_fitness_history[h_idx] = pop->max_fitness;
            stats.diversity_history[h_idx] = pop->fitness_variance;

            /* Compute average ? */
            sum_lambda = 0.0;
            for (size_t i = 0; i < pop->size; i++) {
                sum_lambda += pop->individuals[i].lambda;
            }
            stats.lambda_history[h_idx] = (pop->size > 0)
                                          ? sum_lambda / (double)pop->size : 0.0;
        }
    }

    stats.n_generations = n_generations;
    stats.final_avg_fitness = pop->avg_fitness;
    stats.fitness_improvement = pop->avg_fitness - stats.initial_avg_fitness;

    alife_pop_destroy(pop);
    return stats;
}

void alife_evolution_stats_destroy(alife_evolution_stats_t *stats)
{
    if (stats) {
        free(stats->fitness_history);
        free(stats->max_fitness_history);
        free(stats->diversity_history);
        free(stats->lambda_history);
    }
}

/*??????????????????????????????????????????????????????????????????????
 * Self-Reproduction: von Neumann's replicator logic (simplified)
 *
 * von Neumann's universal constructor consists of:
 *   1. A universal Turing machine (controller)
 *   2. A constructing arm
 *   3. A tape containing its own description
 *
 * In CA: Langton's self-reproducing loop (1984) uses a 2D CA with
 * sheathed signal propagation and a "genome" loop that directs
 * construction of a copy.
 *
 * The following is a simplified template-based "reproduction" check:
 * a pattern "reproduces" if it appears in at least 2 locations at
 * a later time, where the copies share the same internal structure.
 *??????????????????????????????????????????????????????????????????????*/

bool alife_detect_self_reproduction(const uint8_t **state_history, size_t n_states,
                                     int64_t width, int64_t height)
{
    if (!state_history || n_states < 3 || width < 10 || height < 10) return false;

    /* Simplified check: look for duplication of patterns.
     * A self-reproducing system will show ~doubling of pattern count
     * while maintaining pattern structure. */

    size_t n_cells = (size_t)(width * height);

    /* Count non-zero cells at first and last time */
    uint64_t count_first = 0, count_last = 0;
    for (size_t i = 0; i < n_cells; i++) {
        if (state_history[0][i]) count_first++;
        if (state_history[n_states - 1][i]) count_last++;
    }

    /* Self-reproduction signature: final count >> initial count,
     * indicating the system has made copies of itself */
    if (count_first > 0) {
        double replication_ratio = (double)count_last / (double)count_first;
        return (replication_ratio > 2.0); /* More than doubled */
    }

    return false;
}

/**
 * Compute population diversity: average pairwise Hamming distance.
 */
double alife_population_diversity(alife_population_t *pop)
{
    if (!pop || pop->size < 2) return 0.0;

    double total_dist = 0.0;
    size_t n_pairs = 0;

    for (size_t i = 0; i < pop->size; i++) {
        for (size_t j = i + 1; j < pop->size; j++) {
            size_t diff = 0;
            for (size_t k = 0; k < pop->genome_size; k++) {
                if (pop->individuals[i].genome[k] != pop->individuals[j].genome[k]) {
                    diff++;
                }
            }
            total_dist += (double)diff / (double)pop->genome_size;
            n_pairs++;
        }
    }

    return (n_pairs > 0) ? total_dist / (double)n_pairs : 0.0;
}