/**
 * ga_core.c — Core Genetic Algorithm Implementation
 *
 * Implements the fundamental GA loop: initialize population, then iterate
 * through generations applying selection → crossover → mutation →
 * replacement → evaluation. Includes convergence detection, diversity
 * monitoring, and restart mechanisms.
 *
 * Reference: Holland "Adaptation in Natural and Artificial Systems" (1975,1992)
 *            Goldberg "Genetic Algorithms in Search, Optimization & ML" (1989)
 *            Mitchell "An Introduction to Genetic Algorithms" (1998)
 *            Eiben & Smith "Introduction to Evolutionary Computing" (2015)
 *
 * Knowledge: L1 Definitions, L2 Core Concepts, L3 Math Structures, L5 Algorithms
 */

#include "ga_core.h"
#include "ga_operators.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ====================================================================
 * L2: Utility — Pseudorandom number generation
 * ==================================================================== */

static uint64_t _ga_rand_state = 123456789ULL;

static void _ga_srand(uint64_t seed) {
    _ga_rand_state = seed;
}

/* xorshift64* PRNG — fast, high-quality for GA */
static uint64_t _ga_rand64(void) {
    _ga_rand_state ^= _ga_rand_state >> 12;
    _ga_rand_state ^= _ga_rand_state << 25;
    _ga_rand_state ^= _ga_rand_state >> 27;
    return _ga_rand_state * 0x2545F4914F6CDD1DULL;
}

static double _ga_rand_double(void) {
    return (double)(_ga_rand64() >> 11) / (double)(1ULL << 53);
}

static int _ga_rand_int(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (int)(_ga_rand64() % (uint64_t)(hi - lo));
}

/* Box-Muller transform for Gaussian random numbers (used by operators via ga_operators.h) */
__attribute__((unused))
static double _ga_rand_gaussian(double mean, double stddev) {
    double u1 = _ga_rand_double();
    double u2 = _ga_rand_double();
    if (u1 < 1e-12) u1 = 1e-12;
    double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return mean + stddev * z;
}

/* ====================================================================
 * L2: Chromosome Operations
 * ==================================================================== */

void ga_chromosome_init_random(Chromosome *c) {
    if (!c) return;
    for (int i = 0; i < c->num_genes && i < GA_MAX_GENES; i++) {
        Locus *loc = &c->loci[i];
        switch (loc->type) {
            case GENE_BINARY:
                c->alleles[i].binary_val = (_ga_rand_double() < 0.5);
                break;
            case GENE_INTEGER: {
                int lo = (int)loc->lower_bound;
                int hi = (int)loc->upper_bound;
                c->alleles[i].integer_val = _ga_rand_int(lo, hi + 1);
                break;
            }
            case GENE_REAL: {
                double lo = loc->lower_bound;
                double hi = loc->upper_bound;
                c->alleles[i].real_val = lo + _ga_rand_double() * (hi - lo);
                break;
            }
            case GENE_PERMUTATION:
                c->alleles[i].integer_val = i; /* will be shuffled later */
                break;
            case GENE_SYMBOLIC:
                c->alleles[i].symbol_val = (char)_ga_rand_int(0, 256);
                break;
        }
    }
    c->fitness = 0.0;
    c->scaled_fitness = 0.0;
    c->evaluated = false;
    c->parent_ids[0] = -1;
    c->parent_ids[1] = -1;
}

/* Fisher-Yates shuffle for permutation chromosomes */
static void _fisher_yates_shuffle(Allele *alleles, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = _ga_rand_int(0, i + 1);
        Allele tmp = alleles[i];
        alleles[i] = alleles[j];
        alleles[j] = tmp;
    }
}

Chromosome ga_chromosome_copy(const Chromosome *src) {
    Chromosome c;
    memset(&c, 0, sizeof(c));
    if (!src) return c;
    c.num_genes = src->num_genes;
    c.fitness = src->fitness;
    c.scaled_fitness = src->scaled_fitness;
    c.evaluated = src->evaluated;
    c.id = src->id;
    c.birth_generation = src->birth_generation;
    c.parent_ids[0] = src->parent_ids[0];
    c.parent_ids[1] = src->parent_ids[1];
    memcpy(c.loci, src->loci, sizeof(Locus) * src->num_genes);
    memcpy(c.alleles, src->alleles, sizeof(Allele) * src->num_genes);
    return c;
}

bool ga_is_valid(const Chromosome *c) {
    if (!c || c->num_genes <= 0 || c->num_genes > GA_MAX_GENES) return false;
    for (int i = 0; i < c->num_genes; i++) {
        const Locus *loc = &c->loci[i];
        switch (loc->type) {
            case GENE_REAL:
                if (c->alleles[i].real_val < loc->lower_bound ||
                    c->alleles[i].real_val > loc->upper_bound)
                    return false;
                break;
            case GENE_INTEGER:
                if (c->alleles[i].integer_val < (int)loc->lower_bound ||
                    c->alleles[i].integer_val > (int)loc->upper_bound)
                    return false;
                break;
            case GENE_BINARY:
                break; /* always valid */
            case GENE_PERMUTATION:
                break; /* checked separately */
            case GENE_SYMBOLIC:
                break;
        }
    }
    return true;
}

/* ====================================================================
 * L2: Population Lifecycle
 * ==================================================================== */

Population* ga_population_create(int size, const Locus *loci_def, int num_genes) {
    if (size <= 0 || size > GA_MAX_POPULATION || num_genes <= 0
        || num_genes > GA_MAX_GENES || !loci_def)
        return NULL;

    Population *pop = (Population*)calloc(1, sizeof(Population));
    if (!pop) return NULL;

    pop->size = size;
    pop->num_genes = num_genes;
    pop->generation = 0;
    pop->next_id = 0;

    /* Allocate and initialize individuals */
    pop->individuals = (Chromosome*)calloc((size_t)size, sizeof(Chromosome));
    if (!pop->individuals) {
        free(pop);
        return NULL;
    }

    /* Copy locus definitions */
    pop->loci_def = (Locus*)malloc(sizeof(Locus) * (size_t)num_genes);
    if (!pop->loci_def) {
        free(pop->individuals);
        free(pop);
        return NULL;
    }
    memcpy(pop->loci_def, loci_def, sizeof(Locus) * (size_t)num_genes);

    /* Initialize each chromosome */
    for (int i = 0; i < size; i++) {
        Chromosome *c = &pop->individuals[i];
        c->num_genes = num_genes;
        memcpy(c->loci, loci_def, sizeof(Locus) * (size_t)num_genes);
        ga_chromosome_init_random(c);
        c->id = pop->next_id++;
        c->birth_generation = 0;
    }

    /* Shuffle permutation chromosomes */
    for (int i = 0; i < size; i++) {
        Chromosome *c = &pop->individuals[i];
        bool is_perm = false;
        for (int g = 0; g < num_genes; g++) {
            if (c->loci[g].type == GENE_PERMUTATION) {
                is_perm = true;
                break;
            }
        }
        if (is_perm) {
            _fisher_yates_shuffle(c->alleles, num_genes);
            /* Reassign integer values to be 0..n-1 after shuffle */
            for (int g = 0; g < num_genes; g++) {
                c->alleles[g].integer_val = g;
            }
            _fisher_yates_shuffle(c->alleles, num_genes);
        }
    }

    return pop;
}

void ga_population_destroy(Population *pop) {
    if (!pop) return;
    free(pop->individuals);
    free(pop->loci_def);
    free(pop);
}

void ga_population_evaluate(Population *pop,
                             double (*fitness_func)(const Chromosome*, void*),
                             void *user_data) {
    if (!pop || !fitness_func) return;

    pop->total_fitness = 0.0;
    pop->max_fitness = -DBL_MAX;
    pop->min_fitness = DBL_MAX;

    for (int i = 0; i < pop->size; i++) {
        Chromosome *c = &pop->individuals[i];
        double fit = fitness_func(c, user_data);
        c->fitness = fit;
        c->scaled_fitness = fit;
        c->evaluated = true;
        pop->total_fitness += fit;
        if (fit > pop->max_fitness) {
            pop->max_fitness = fit;
            pop->best_index = i;
        }
        if (fit < pop->min_fitness) pop->min_fitness = fit;
    }

    pop->avg_fitness = pop->total_fitness / (double)pop->size;

    /* Compute standard deviation */
    double sum_sq = 0.0;
    for (int i = 0; i < pop->size; i++) {
        double diff = pop->individuals[i].fitness - pop->avg_fitness;
        sum_sq += diff * diff;
    }
    pop->std_fitness = sqrt(sum_sq / (double)pop->size);
}

/* Comparison function for qsort (descending by fitness) */
static int _cmp_fitness_desc(const void *a, const void *b) {
    const Chromosome *ca = (const Chromosome*)a;
    const Chromosome *cb = (const Chromosome*)b;
    if (ca->fitness > cb->fitness) return -1;
    if (ca->fitness < cb->fitness) return 1;
    return 0;
}

void ga_population_sort(Population *pop) {
    if (!pop || pop->size <= 1) return;
    /* Default: sort descending (maximization) */
    qsort(pop->individuals, (size_t)pop->size, sizeof(Chromosome),
          _cmp_fitness_desc);
    pop->best_index = 0;
    pop->max_fitness = pop->individuals[0].fitness;
    pop->min_fitness = pop->individuals[pop->size - 1].fitness;
}

void ga_population_stats(Population *pop) {
    ga_population_evaluate(pop, NULL, NULL);
}

Chromosome* ga_population_best(Population *pop) {
    if (!pop || pop->size <= 0) return NULL;
    return &pop->individuals[pop->best_index];
}

void ga_population_clone(Population *dest, const Population *src) {
    if (!dest || !src) return;
    for (int i = 0; i < src->size && i < dest->size; i++) {
        dest->individuals[i] = ga_chromosome_copy(&src->individuals[i]);
    }
    dest->size = src->size;
    dest->num_genes = src->num_genes;
    dest->generation = src->generation;
    dest->total_fitness = src->total_fitness;
    dest->avg_fitness = src->avg_fitness;
    dest->max_fitness = src->max_fitness;
    dest->min_fitness = src->min_fitness;
    dest->std_fitness = src->std_fitness;
    dest->best_index = src->best_index;
}

/* ====================================================================
 * L5: GA State Management and Main Loop
 * ==================================================================== */

GAState* ga_init(const GAConfig *config,
                  double (*fitness_func)(const Chromosome*, void*),
                  void *user_data) {
    if (!config || !fitness_func) return NULL;

    GAState *ga = (GAState*)calloc(1, sizeof(GAState));
    if (!ga) return NULL;

    memcpy(&ga->config, config, sizeof(GAConfig));
    ga->fitness_func = fitness_func;
    ga->user_data = user_data;
    ga->current_gen = 0;
    ga->history_len = 0;
    ga->converged = false;
    ga->stall_count = 0;
    ga->total_evaluations = 0.0;
    ga->start_time_ms = (uint64_t)clock() * 1000 / CLOCKS_PER_SEC;

    if (config->random_seed != 0) {
        _ga_srand(config->random_seed);
    } else {
        _ga_srand((uint64_t)time(NULL));
    }

    /* Dummy loci for initialization — caller overrides with real definition */
    Locus dummy_loci[1];
    memset(dummy_loci, 0, sizeof(dummy_loci));
    dummy_loci[0].type = GENE_BINARY;
    dummy_loci[0].lower_bound = 0.0;
    dummy_loci[0].upper_bound = 1.0;
    dummy_loci[0].mutation_rate = config->mutation_rate;
    dummy_loci[0].position = 0;

    ga->parent_pop = ga_population_create(config->population_size,
                                           dummy_loci, 1);
    ga->offspring_pop = ga_population_create(config->population_size,
                                              dummy_loci, 1);
    ga->archive = ga_population_create(GA_MAX_ARCHIVE, dummy_loci, 1);

    if (!ga->parent_pop || !ga->offspring_pop || !ga->archive) {
        ga_destroy(ga);
        return NULL;
    }

    return ga;
}

void ga_destroy(GAState *ga) {
    if (!ga) return;
    ga_population_destroy(ga->parent_pop);
    ga_population_destroy(ga->offspring_pop);
    ga_population_destroy(ga->archive);
    free(ga);
}

int ga_generation(GAState *ga) {
    if (!ga || !ga->parent_pop || !ga->offspring_pop) return 0;
    Population *pop = ga->parent_pop;
    Population *off = ga->offspring_pop;
    GAConfig *cfg = &ga->config;

    int evals = 0;

    /* Phase 1: Selection & Reproduction */
    int n_offspring = 0;
    int pairs_needed = pop->size / 2;

    for (int pair = 0; pair < pairs_needed; pair++) {
        int p1_idx, p2_idx;

        switch (cfg->select_method) {
            case SELECT_ROULETTE:
                p1_idx = ga_select_roulette(pop);
                p2_idx = ga_select_roulette(pop);
                break;
            case SELECT_TOURNAMENT:
                p1_idx = ga_select_tournament(pop, cfg->tournament_size);
                p2_idx = ga_select_tournament(pop, cfg->tournament_size);
                break;
            case SELECT_RANK:
                p1_idx = ga_select_rank(pop, 1.5);
                p2_idx = ga_select_rank(pop, 1.5);
                break;
            case SELECT_SUS: {
                int sel[2];
                ga_select_sus(pop, sel, 2);
                p1_idx = sel[0];
                p2_idx = sel[1];
                break;
            }
            case SELECT_TRUNCATION:
                p1_idx = ga_select_truncation(pop, 0.5);
                p2_idx = ga_select_truncation(pop, 0.5);
                break;
            default:
                p1_idx = _ga_rand_int(0, pop->size);
                p2_idx = _ga_rand_int(0, pop->size);
        }

        Chromosome *p1 = &pop->individuals[p1_idx];
        Chromosome *p2 = &pop->individuals[p2_idx];

        /* Phase 2: Crossover */
        Chromosome c1 = ga_chromosome_copy(p1);
        Chromosome c2 = ga_chromosome_copy(p2);
        c1.id = pop->next_id++;
        c2.id = pop->next_id++;
        c1.birth_generation = (uint64_t)(ga->current_gen + 1);
        c2.birth_generation = (uint64_t)(ga->current_gen + 1);
        c1.parent_ids[0] = (int)p1->id;
        c1.parent_ids[1] = (int)p2->id;
        c2.parent_ids[0] = (int)p1->id;
        c2.parent_ids[1] = (int)p2->id;

        if (_ga_rand_double() < cfg->crossover_rate) {
            switch (cfg->crossover_method) {
                case CROSSOVER_SINGLE_POINT:
                    ga_crossover_one_point(p1, p2, &c1, &c2);
                    break;
                case CROSSOVER_TWO_POINT:
                    ga_crossover_two_point(p1, p2, &c1, &c2);
                    break;
                case CROSSOVER_UNIFORM:
                    ga_crossover_uniform(p1, p2, &c1, &c2, 0.5);
                    break;
                case CROSSOVER_ARITHMETIC:
                    ga_crossover_arithmetic(p1, p2, &c1, &c2, 0.25);
                    break;
                case CROSSOVER_OX:
                    ga_crossover_ox(p1, p2, &c1, &c2);
                    break;
                case CROSSOVER_PMX:
                    ga_crossover_pmx(p1, p2, &c1, &c2);
                    break;
                default:
                    break;
            }
        }

        /* Phase 3: Mutation */
        switch (cfg->mutation_method) {
            case MUTATE_BIT_FLIP:
                ga_mutate_bit_flip(&c1, cfg->mutation_rate);
                ga_mutate_bit_flip(&c2, cfg->mutation_rate);
                break;
            case MUTATE_GAUSSIAN:
                ga_mutate_gaussian(&c1, cfg->mutation_rate, 0.1);
                ga_mutate_gaussian(&c2, cfg->mutation_rate, 0.1);
                break;
            case MUTATE_UNIFORM:
                ga_mutate_uniform(&c1, cfg->mutation_rate);
                ga_mutate_uniform(&c2, cfg->mutation_rate);
                break;
            case MUTATE_SWAP:
                ga_mutate_swap(&c1);
                ga_mutate_swap(&c2);
                break;
            case MUTATE_INVERSION:
                ga_mutate_inversion(&c1);
                ga_mutate_inversion(&c2);
                break;
            default:
                break;
        }

        if (n_offspring < off->size) {
            off->individuals[n_offspring++] = c1;
        }
        if (n_offspring < off->size) {
            off->individuals[n_offspring++] = c2;
        }
    }

    /* Phase 4: Evaluate offspring */
    off->size = n_offspring;
    ga_population_evaluate(off, ga->fitness_func, ga->user_data);
    evals += off->size;

    /* Phase 5: Replacement */
    switch (cfg->replace_method) {
        case REPLACE_GENERATIONAL:
            ga_replace_generational(pop, off);
            break;
        case REPLACE_STEADY_STATE:
            ga_replace_steady_state(pop, off);
            break;
        case REPLACE_ELITIST:
            ga_replace_elitist(pop, off, cfg->elitism_count);
            break;
        case REPLACE_MU_PLUS_LAMBDA:
            ga_replace_mu_plus_lambda(pop, off);
            break;
        case REPLACE_MU_COMMA_LAMBDA:
            ga_replace_mu_comma_lambda(pop, off);
            break;
        default:
            ga_replace_generational(pop, off);
    }

    /* Re-evaluate population after replacement */
    ga_population_evaluate(pop, ga->fitness_func, ga->user_data);
    evals += pop->size;
    ga->total_evaluations += (double)evals;
    ga->current_gen++;
    pop->generation = ga->current_gen;

    /* Record history */
    if (ga->history_len < GA_MAX_POPULATION) {
        ga->best_fitness_history[ga->history_len] = pop->max_fitness;
        ga->avg_fitness_history[ga->history_len] = pop->avg_fitness;
        ga->diversity_history[ga->history_len] = ga_population_diversity(pop);
        ga->history_len++;
    }

    /* Convergence detection */
    if (ga->history_len >= cfg->stall_generations && cfg->stall_generations > 0) {
        double prev_best = ga->best_fitness_history[ga->history_len
                            - cfg->stall_generations];
        if (fabs(pop->max_fitness - prev_best) < 1e-10) {
            ga->stall_count++;
        } else {
            ga->stall_count = 0;
        }
        if (ga->stall_count >= cfg->stall_generations) {
            ga->converged = true;
        }
    }

    /* Target fitness check */
    if (cfg->target_fitness > 0) {
        if ((cfg->minimize && pop->min_fitness <= cfg->target_fitness) ||
            (!cfg->minimize && pop->max_fitness >= cfg->target_fitness)) {
            ga->converged = true;
        }
    }

    return evals;
}

int ga_run(GAState *ga) {
    if (!ga) return 0;
    int total_evals = 0;

    /* Initial evaluation */
    ga_population_evaluate(ga->parent_pop, ga->fitness_func, ga->user_data);
    total_evals += ga->parent_pop->size;
    ga->total_evaluations += (double)total_evals;

    /* Record initial population */
    ga->best_fitness_history[0] = ga->parent_pop->max_fitness;
    ga->avg_fitness_history[0] = ga->parent_pop->avg_fitness;
    ga->diversity_history[0] = ga_population_diversity(ga->parent_pop);
    ga->history_len = 1;

    for (int gen = 0; gen < ga->config.num_generations; gen++) {
        if (ga->converged) break;
        total_evals += ga_generation(ga);
    }

    return total_evals;
}

int ga_run_with_callback(GAState *ga,
                          bool (*callback)(int gen, double best, double avg,
                                           void *data),
                          void *cb_data) {
    if (!ga) return 0;
    int total_evals = 0;

    ga_population_evaluate(ga->parent_pop, ga->fitness_func, ga->user_data);
    total_evals += ga->parent_pop->size;
    ga->total_evaluations += (double)total_evals;

    ga->best_fitness_history[0] = ga->parent_pop->max_fitness;
    ga->avg_fitness_history[0] = ga->parent_pop->avg_fitness;
    ga->diversity_history[0] = ga_population_diversity(ga->parent_pop);
    ga->history_len = 1;

    if (callback) {
        bool stop = callback(0, ga->parent_pop->max_fitness,
                             ga->parent_pop->avg_fitness, cb_data);
        if (stop) return total_evals;
    }

    for (int gen = 0; gen < ga->config.num_generations; gen++) {
        if (ga->converged) break;
        total_evals += ga_generation(ga);
        if (callback) {
            bool stop = callback(ga->current_gen,
                                 ga->parent_pop->max_fitness,
                                 ga->parent_pop->avg_fitness, cb_data);
            if (stop) break;
        }
    }

    return total_evals;
}

/* ====================================================================
 * L2: Convergence and Diversity
 * ==================================================================== */

bool ga_has_converged(const GAState *ga, double tolerance) {
    if (!ga) return true;
    if (ga->converged) return true;
    if (ga->history_len < 2) return false;

    /* Check if best fitness has stagnated */
    double best = ga->best_fitness_history[ga->history_len - 1];
    double prev_best = ga->best_fitness_history[ga->history_len - 2];
    if (fabs(best - prev_best) < tolerance) {
        /* Also check population variance */
        if (ga->parent_pop && ga->parent_pop->std_fitness < tolerance) {
            return true;
        }
    }
    return false;
}

double ga_population_diversity(const Population *pop) {
    if (!pop || pop->size <= 1) return 0.0;

    double total_dist = 0.0;
    int count = 0;

    /* Sample-based pairwise diversity (avoid O(N²) for large populations) */
    int sample_size = (pop->size > 200) ? 50 : pop->size;
    int step = pop->size / sample_size;
    if (step < 1) step = 1;

    for (int i = 0; i < pop->size; i += step) {
        for (int j = i + step; j < pop->size; j += step) {
            total_dist += ga_hamming_distance(&pop->individuals[i],
                                               &pop->individuals[j]);
            count++;
        }
    }

    if (count == 0) return 0.0;
    return total_dist / (double)count;
}

double ga_gene_entropy(const Population *pop) {
    if (!pop || pop->num_genes <= 0 || pop->size <= 0) return 0.0;

    double total_entropy = 0.0;

    for (int g = 0; g < pop->num_genes; g++) {
        if (pop->loci_def[g].type != GENE_BINARY) continue;
        /* Count ones at this locus */
        int ones = 0;
        for (int i = 0; i < pop->size; i++) {
            if (pop->individuals[i].alleles[g].binary_val) ones++;
        }
        double p1 = (double)ones / (double)pop->size;
        double p0 = 1.0 - p1;
        double entropy = 0.0;
        if (p0 > 1e-12) entropy -= p0 * log2(p0);
        if (p1 > 1e-12) entropy -= p1 * log2(p1);
        total_entropy += entropy;
    }

    return total_entropy / (double)pop->num_genes;
}

double ga_effective_population_size(const Population *pop, double *sel_probs) {
    if (!pop || !sel_probs || pop->size <= 0) return 0.0;

    /* Effective population size: 1 / Σ(p_i²) */
    double sum_sq = 0.0;
    double sum = 0.0;
    for (int i = 0; i < pop->size; i++) {
        sum_sq += sel_probs[i] * sel_probs[i];
        sum += sel_probs[i];
    }
    if (sum < 1e-12) return 0.0;
    /* Normalize probabilities */
    sum_sq /= (sum * sum);
    if (sum_sq < 1e-12) return (double)pop->size;
    return 1.0 / sum_sq;
}

double ga_selection_pressure(const Population *pop,
                              double *sel_probs, int n) {
    if (!pop || !sel_probs || n <= 0) return 1.0;

    /* Find best individual's selection probability */
    double max_prob = sel_probs[0];
    double avg_prob = 0.0;
    for (int i = 0; i < n; i++) {
        if (sel_probs[i] > max_prob) max_prob = sel_probs[i];
        avg_prob += sel_probs[i];
    }
    avg_prob /= (double)n;
    if (avg_prob < 1e-12) return 1.0;
    return max_prob / avg_prob;
}

void ga_restart(GAState *ga, int keep_best_n) {
    if (!ga || !ga->parent_pop) return;

    /* Save best individuals */
    ga_population_sort(ga->parent_pop);
    Chromosome *best_saved = (Chromosome*)malloc(
        sizeof(Chromosome) * (size_t)keep_best_n);
    if (!best_saved) return;

    int save_count = (keep_best_n < ga->parent_pop->size)
                     ? keep_best_n : ga->parent_pop->size;
    for (int i = 0; i < save_count; i++) {
        best_saved[i] = ga_chromosome_copy(&ga->parent_pop->individuals[i]);
    }

    /* Re-randomize rest of population */
    for (int i = save_count; i < ga->parent_pop->size; i++) {
        ga_chromosome_init_random(&ga->parent_pop->individuals[i]);
        ga->parent_pop->individuals[i].id = ga->parent_pop->next_id++;
    }

    /* Restore best individuals */
    for (int i = 0; i < save_count; i++) {
        ga->parent_pop->individuals[i] = best_saved[i];
    }

    free(best_saved);
    ga->converged = false;
    ga->stall_count = 0;
    ga_population_evaluate(ga->parent_pop, ga->fitness_func, ga->user_data);
}

/* ====================================================================
 * L3: Mathematical Structures — Distance Metrics
 * ==================================================================== */

int ga_hamming_distance(const Chromosome *a, const Chromosome *b) {
    if (!a || !b) return 0;
    int min_len = (a->num_genes < b->num_genes) ? a->num_genes : b->num_genes;
    if (min_len <= 0) return 0;

    int dist = 0;
    for (int i = 0; i < min_len; i++) {
        if (a->loci[i].type != b->loci[i].type) {
            dist++;
            continue;
        }
        switch (a->loci[i].type) {
            case GENE_BINARY:
                if (a->alleles[i].binary_val != b->alleles[i].binary_val)
                    dist++;
                break;
            case GENE_INTEGER:
                if (a->alleles[i].integer_val != b->alleles[i].integer_val)
                    dist++;
                break;
            case GENE_PERMUTATION:
                if (a->alleles[i].integer_val != b->alleles[i].integer_val)
                    dist++;
                break;
            case GENE_REAL:
                if (fabs(a->alleles[i].real_val - b->alleles[i].real_val) > 1e-10)
                    dist++;
                break;
            case GENE_SYMBOLIC:
                if (a->alleles[i].symbol_val != b->alleles[i].symbol_val)
                    dist++;
                break;
        }
    }
    return dist;
}

double ga_euclidean_distance(const Chromosome *a, const Chromosome *b) {
    if (!a || !b) return 0.0;
    int min_len = (a->num_genes < b->num_genes) ? a->num_genes : b->num_genes;
    if (min_len <= 0) return 0.0;

    double sum_sq = 0.0;
    for (int i = 0; i < min_len; i++) {
        double va, vb;
        switch (a->loci[i].type) {
            case GENE_REAL:
                va = a->alleles[i].real_val;
                vb = b->alleles[i].real_val;
                break;
            case GENE_INTEGER:
                va = (double)a->alleles[i].integer_val;
                vb = (double)b->alleles[i].integer_val;
                break;
            case GENE_BINARY:
                va = a->alleles[i].binary_val ? 1.0 : 0.0;
                vb = b->alleles[i].binary_val ? 1.0 : 0.0;
                break;
            default:
                va = 0.0; vb = 0.0;
        }
        double diff = va - vb;
        sum_sq += diff * diff;
    }
    return sqrt(sum_sq);
}

void ga_per_locus_entropy(const Population *pop, double *entropy_out) {
    if (!pop || !entropy_out || pop->num_genes <= 0) return;

    for (int g = 0; g < pop->num_genes; g++) {
        double entropy = 0.0;
        Locus *loc = &pop->loci_def[g];

        if (loc->type == GENE_BINARY) {
            int ones = 0;
            for (int i = 0; i < pop->size; i++) {
                if (pop->individuals[i].alleles[g].binary_val) ones++;
            }
            double p1 = (double)ones / (double)pop->size;
            double p0 = 1.0 - p1;
            if (p0 > 1e-12) entropy -= p0 * log2(p0);
            if (p1 > 1e-12) entropy -= p1 * log2(p1);
        } else if (loc->type == GENE_REAL || loc->type == GENE_INTEGER) {
            /* Discretize into bins for entropy calculation */
            int bins = 10;
            int counts[10] = {0};
            for (int i = 0; i < pop->size; i++) {
                double val = (loc->type == GENE_REAL)
                    ? pop->individuals[i].alleles[g].real_val
                    : (double)pop->individuals[i].alleles[g].integer_val;
                double lo = loc->lower_bound;
                double hi = loc->upper_bound;
                if (hi <= lo) hi = lo + 1.0;
                int bin = (int)((val - lo) / (hi - lo) * (double)bins);
                if (bin < 0) bin = 0;
                if (bin >= bins) bin = bins - 1;
                counts[bin]++;
            }
            for (int b = 0; b < bins; b++) {
                double p = (double)counts[b] / (double)pop->size;
                if (p > 1e-12) entropy -= p * log2(p);
            }
        }
        entropy_out[g] = entropy;
    }
}
