/**
 * @file    sfi_adaptation.c
 * @brief   SFI Methodology: Adaptation on NK fitness landscapes,
 *          replicator dynamics, and classifier systems
 *
 * Implements Kauffman's NK rugged fitness landscape model
 * with adaptive walks, replicator dynamics from evolutionary
 * game theory, and Holland's classifier system bucket brigade.
 *
 * Knowledge: L1 (NK landscape, replicator, classifier types),
 * L2 (adaptive landscape concept), L3 (fitness functions),
 * L4 (NK complexity catastrophe, Schema Theorem), L5 (all algorithms)
 *
 * Course: CMU 15-855 ? Graduate Complexity (NK model);
 *         SFI CSSS ? EGT module; Oxford C20 ? Adaptive Control
 */

#include "sfi_adaptation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ---- Internal PRNG (same xoshiro256** as macrosystems) ---- */
static uint64_t prng_state2[4] = {0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL,
                                  0xA4093822299F31D0ULL, 0x082EFA98EC4E6C89ULL};

static inline uint64_t prng_rotl2(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t prng_next2(void) {
    uint64_t result = prng_rotl2(prng_state2[1] * 5, 7) * 9;
    uint64_t t = prng_state2[1] << 17;
    prng_state2[2] ^= prng_state2[0];
    prng_state2[3] ^= prng_state2[1];
    prng_state2[1] ^= prng_state2[2];
    prng_state2[0] ^= prng_state2[3];
    prng_state2[2] ^= t;
    prng_state2[3] = prng_rotl2(prng_state2[3], 45);
    return result;
}

static void prng_seed2(uint64_t s) {
    prng_state2[0] = s ^ 0x9E3779B97F4A7C15ULL;
    prng_state2[1] = (s >> 32) ^ 0x6A09E667F3BCC908ULL;
    prng_state2[2] = s * 6364136223846793005ULL;
    prng_state2[3] = (s << 17) ^ 0x3C6EF372FE94F82BULL;
    for (int i = 0; i < 4; i++) prng_next2();
}

static double prng_uniform2(void) {
    return (prng_next2() >> 11) * 0x1.0p-53;
}

/* ================================================================
 * L5: NK Fitness Landscape ? Generation (Kauffman 1993)
 * ================================================================ */

int sfi_nk_landscape_generate(sfi_nk_landscape_t *land,
                              uint32_t N, uint32_t K, uint64_t seed) {
    if (!land || N == 0 || K >= N) return -1;
    if (N > 24) return -2;  /* 2^24 = 16M entries, too large */
    memset(land, 0, sizeof(*land));
    land->N = N;
    land->K = (K < N) ? K : N - 1;
    prng_seed2(seed);

    uint64_t n_genotypes = 1ULL << (uint64_t)N;
    uint32_t n_contributors = land->K + 1;

    /* Interaction matrix: which loci affect each locus i */
    land->interaction_matrix = (uint32_t *)calloc(
        (size_t)N * (size_t)n_contributors, sizeof(uint32_t));
    if (!land->interaction_matrix) return -3;

    for (uint32_t i = 0; i < N; i++) {
        uint32_t *row = &land->interaction_matrix[i * n_contributors];
        row[0] = i;  /* Always include self */
        for (uint32_t k = 1; k < n_contributors; k++) {
            uint32_t candidate;
            int unique = 0;
            int tries = 0;
            while (!unique && tries < 100) {
                candidate = (uint32_t)(prng_uniform2() * (double)N);
                if (candidate >= N) candidate = N - 1;
                unique = 1;
                for (uint32_t pk = 0; pk < k; pk++) {
                    if (row[pk] == candidate) { unique = 0; break; }
                }
                tries++;
            }
            if (!unique) candidate = (i + k + 1) % N;
            row[k] = candidate;
        }
    }

    /* Fitness table: random contribution for each sub-configuration */
    uint64_t table_size = n_genotypes * (uint64_t)n_contributors;
    land->fitness_table = (double *)malloc(
        (size_t)table_size * sizeof(double));
    if (!land->fitness_table) {
        free(land->interaction_matrix);
        land->interaction_matrix = NULL;
        return -4;
    }

    for (uint64_t j = 0; j < table_size; j++) {
        land->fitness_table[j] = prng_uniform2();
    }

    /* Pre-compute statistics */
    double sum = 0.0, sum_sq = 0.0;
    for (uint64_t g = 0; g < n_genotypes; g++) {
        double fit = sfi_nk_evaluate_fitness(land, g);
        sum += fit;
        sum_sq += fit * fit;
    }
    land->mean_fitness = sum / (double)n_genotypes;
    double var = (sum_sq / (double)n_genotypes) - land->mean_fitness * land->mean_fitness;
    land->std_fitness = (var > 0.0) ? sqrt(var) : 0.0;

    /* Autocorrelation at d=1 */
    land->autocorr_1step = sfi_nk_autocorrelation(land);

    /* Count local optima */
    land->local_optima_count = sfi_nk_count_local_optima(land,
        (n_genotypes > 100000) ? 10000 : (uint32_t)n_genotypes);

    return 0;
}

void sfi_nk_landscape_destroy(sfi_nk_landscape_t *land) {
    if (!land) return;
    free(land->fitness_table);
    free(land->interaction_matrix);
    memset(land, 0, sizeof(*land));
}

/**
 * L5: Evaluate fitness of genotype on NK landscape.
 *
 * Fitness F(g) = (1/N) ?_i f_i(g_i, g_{j1}, ..., g_{jK})
 * where each f_i is a lookup from the random table and
 * j1..jK are the K loci interacting with locus i.
 */
double sfi_nk_evaluate_fitness(const sfi_nk_landscape_t *land,
                               uint64_t genotype) {
    if (!land || !land->fitness_table || !land->interaction_matrix) return 0.0;
    uint32_t N = land->N;
    uint32_t n_contributors = land->K + 1;

    double sum = 0.0;
    for (uint32_t i = 0; i < N; i++) {
        /* Build sub-genotype index for the interacting loci */
        uint64_t sub_index = 0;
        const uint32_t *row = &land->interaction_matrix[i * n_contributors];
        for (uint32_t k = 0; k < n_contributors; k++) {
            uint32_t locus = row[k];
            uint64_t bit = (genotype >> (uint64_t)(N - 1 - locus)) & 1ULL;
            if (bit) sub_index |= (1ULL << (uint64_t)(n_contributors - 1 - k));
        }
        /* Lookup contribution: each locus i has 2^{K+1} contributions
           indexed by the sub-genotype */
        uint64_t table_idx = (uint64_t)i * (1ULL << (uint64_t)n_contributors) + sub_index;
        sum += land->fitness_table[table_idx];
    }
    return sum / (double)N;
}

/**
 * L5: Greedy adaptive walk ? Kauffman's canonical algorithm
 *
 * Starting from a genotype, repeatedly flip one bit if the
 * resulting mutant has higher fitness.  Stops at a local
 * peak.  Average walk length ~ ln(N) for K=0, ~ N for K=N-1.
 */
uint64_t sfi_nk_greedy_walk(const sfi_nk_landscape_t *land,
                            uint64_t start_genotype,
                            double *final_fitness,
                            uint32_t *steps_taken) {
    if (!land) {
        if (final_fitness) *final_fitness = 0.0;
        if (steps_taken) *steps_taken = 0;
        return 0;
    }

    uint64_t current = start_genotype;
    uint64_t n_genotypes = 1ULL << (uint64_t)land->N;
    if (current >= n_genotypes) current = n_genotypes - 1;

    double current_fit = sfi_nk_evaluate_fitness(land, current);
    uint32_t steps = 0;
    uint32_t max_steps = land->N * 100;  /* Avoid infinite loops */

    int improved = 1;
    while (improved && steps < max_steps) {
        improved = 0;
        /* Try all 1-bit mutants in random order */
        uint32_t *order = (uint32_t *)malloc((size_t)land->N * sizeof(uint32_t));
        if (!order) break;
        for (uint32_t i = 0; i < land->N; i++) order[i] = i;
        for (uint32_t i = land->N - 1; i > 0; i--) {
            uint32_t j = (uint32_t)(prng_uniform2() * (double)(i + 1));
            if (j > i) j = i;
            uint32_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }
        for (uint32_t ni = 0; ni < land->N; ni++) {
            uint32_t bit = order[ni];
            uint64_t mutant = current ^ (1ULL << (uint64_t)(land->N - 1 - bit));
            double mut_fit = sfi_nk_evaluate_fitness(land, mutant);
            if (mut_fit > current_fit) {
                current = mutant;
                current_fit = mut_fit;
                improved = 1;
                steps++;
                break;
            }
        }
        free(order);
        if (!improved) break;
    }

    if (final_fitness) *final_fitness = current_fit;
    if (steps_taken) *steps_taken = steps;
    return current;
}

/**
 * L5: Count local optima ? exhaustive or sampled
 */
uint32_t sfi_nk_count_local_optima(const sfi_nk_landscape_t *land,
                                   uint32_t sample_limit) {
    if (!land || land->N == 0) return 0;
    uint64_t total = 1ULL << (uint64_t)land->N;

    if (total <= (uint64_t)sample_limit) {
        /* Exhaustive */
        uint32_t count = 0;
        for (uint64_t g = 0; g < total; g++) {
            double fit = sfi_nk_evaluate_fitness(land, g);
            int is_local_opt = 1;
            for (uint32_t b = 0; b < land->N && is_local_opt; b++) {
                uint64_t mut = g ^ (1ULL << (uint64_t)(land->N - 1 - b));
                if (sfi_nk_evaluate_fitness(land, mut) > fit)
                    is_local_opt = 0;
            }
            if (is_local_opt) count++;
        }
        return count;
    } else {
        /* Sampled */
        uint32_t count = 0;
        for (uint32_t s = 0; s < sample_limit; s++) {
            uint64_t g = (uint64_t)(prng_uniform2() * (double)total);
            if (g >= total) g = total - 1;
            double fit = sfi_nk_evaluate_fitness(land, g);
            int is_local_opt = 1;
            for (uint32_t b = 0; b < land->N && is_local_opt; b++) {
                uint64_t mut = g ^ (1ULL << (uint64_t)(land->N - 1 - b));
                if (sfi_nk_evaluate_fitness(land, mut) > fit)
                    is_local_opt = 0;
            }
            if (is_local_opt) count++;
        }
        /* Extrapolate to full landscape */
        return (uint32_t)((double)count * (double)total / (double)sample_limit);
    }
}

/**
 * L5: Compute 1-step fitness autocorrelation rho(d=1)
 *
 * rho(1) = Corr[ F(g), F(g') ] for random 1-mutant pairs.
 * High rho ? smooth landscape; low rho ? rugged.
 */
double sfi_nk_autocorrelation(const sfi_nk_landscape_t *land) {
    if (!land) return 0.0;
    uint64_t total = 1ULL << (uint64_t)land->N;
    if (total < 2) return 1.0;

    double sum_f = 0.0, sum_f2 = 0.0, sum_f_fp = 0.0;
    uint64_t pair_count = 0;

    /* Compute mean first */
    for (uint64_t g = 0; g < total; g++) {
        double f = sfi_nk_evaluate_fitness(land, g);
        sum_f += f;
    }
    double mean = sum_f / (double)total;

    /* Covariance and variance */
    for (uint64_t g = 0; g < total; g++) {
        double f = sfi_nk_evaluate_fitness(land, g);
        double df = f - mean;
        sum_f2 += df * df;
    }

    for (uint64_t g = 0; g < total; g++) {
        double f = sfi_nk_evaluate_fitness(land, g);
        double df = f - mean;
        /* Pick one random 1-mutant neighbour */
        uint32_t bit = (uint32_t)(prng_uniform2() * (double)land->N);
        if (bit >= land->N) bit = land->N - 1;
        uint64_t mutant = g ^ (1ULL << (uint64_t)(land->N - 1 - bit));
        double f_mut = sfi_nk_evaluate_fitness(land, mutant);
        sum_f_fp += df * (f_mut - mean);
        pair_count++;
    }

    if (pair_count == 0 || sum_f2 < 1e-15) return 0.0;
    return sum_f_fp / sum_f2;
}

/* ================================================================
 * L5: Replicator Dynamics
 * ================================================================ */

int sfi_replicator_init(sfi_replicator_state_t *rep, uint32_t n) {
    if (!rep || n == 0) return -1;
    memset(rep, 0, sizeof(*rep));
    rep->n_strategies = n;
    rep->frequencies = (double *)calloc((size_t)n, sizeof(double));
    rep->payoffs = (double *)calloc((size_t)n, sizeof(double));
    rep->payoff_matrix = (double *)calloc((size_t)n * (size_t)n, sizeof(double));
    if (!rep->frequencies || !rep->payoffs || !rep->payoff_matrix) {
        sfi_replicator_destroy(rep);
        return -2;
    }
    /* Uniform initial distribution */
    double init = 1.0 / (double)n;
    for (uint32_t i = 0; i < n; i++) rep->frequencies[i] = init;
    return 0;
}

void sfi_replicator_destroy(sfi_replicator_state_t *rep) {
    if (!rep) return;
    free(rep->frequencies);
    free(rep->payoffs);
    free(rep->payoff_matrix);
    memset(rep, 0, sizeof(*rep));
}

void sfi_replicator_step(sfi_replicator_state_t *rep, double dt) {
    if (!rep || dt <= 0.0) return;
    uint32_t n = rep->n_strategies;
    if (n == 0) return;

    /* Compute current payoffs: payoff_i = ?_j A_ij * x_j */
    for (uint32_t i = 0; i < n; i++) {
        rep->payoffs[i] = 0.0;
        for (uint32_t j = 0; j < n; j++) {
            rep->payoffs[i] += rep->payoff_matrix[i * n + j] * rep->frequencies[j];
        }
    }

    /* Mean population payoff */
    rep->mean_population_payoff = 0.0;
    for (uint32_t i = 0; i < n; i++) {
        rep->mean_population_payoff += rep->frequencies[i] * rep->payoffs[i];
    }

    /* Euler step: x_i(t+dt) = x_i(t) + dt * x_i * (payoff_i - mean_payoff) */
    double *new_x = (double *)malloc((size_t)n * sizeof(double));
    if (!new_x) return;
    double sum = 0.0;
    for (uint32_t i = 0; i < n; i++) {
        double dx = rep->frequencies[i] * (rep->payoffs[i] - rep->mean_population_payoff);
        new_x[i] = rep->frequencies[i] + dt * dx;
        if (new_x[i] < 0.0) new_x[i] = 0.0;
        sum += new_x[i];
    }
    /* Renormalise */
    if (sum > 1e-15) {
        for (uint32_t i = 0; i < n; i++)
            rep->frequencies[i] = new_x[i] / sum;
    }
    free(new_x);
}

int sfi_replicator_is_fixed_point(const sfi_replicator_state_t *rep,
                                  double tolerance) {
    if (!rep) return 1;
    uint32_t n = rep->n_strategies;
    double max_dx = 0.0;
    for (uint32_t i = 0; i < n; i++) {
        double dx = fabs(rep->frequencies[i]
            * (rep->payoffs[i] - rep->mean_population_payoff));
        if (dx > max_dx) max_dx = dx;
    }
    return (max_dx < tolerance) ? 1 : 0;
}

/* ================================================================
 * L4: Holland's Schema Theorem ? Estimated schema growth
 * ================================================================ */

double sfi_schema_theorem_expected_copies(
    const uint64_t *population, const double *fitness,
    uint64_t pop_size, uint32_t genotype_len,
    uint32_t schema_mask, uint32_t schema_value,
    double crossover_rate, double mutation_rate) {
    if (!population || !fitness || pop_size == 0 || genotype_len == 0) return 0.0;

    /* Count schema instances and their fitness */
    uint64_t schema_count = 0;
    double schema_fitness_sum = 0.0;
    double total_fitness = 0.0;

    for (uint64_t i = 0; i < pop_size; i++) {
        uint64_t g = population[i];
        /* Check if genotype matches schema: (g & mask) == (value & mask) */
        if ((g & (uint64_t)schema_mask) == ((uint64_t)schema_value & (uint64_t)schema_mask)) {
            schema_count++;
            schema_fitness_sum += fitness[i];
        }
        total_fitness += fitness[i];
    }

    if (schema_count == 0) return 0.0;

    double f_schema = schema_fitness_sum / (double)schema_count;
    double f_avg = total_fitness / (double)pop_size;

    /* Defining length: distance between first and last fixed bit in schema */
    uint32_t first = 0, last = 0;
    int found = 0;
    for (uint32_t b = 0; b < genotype_len; b++) {
        uint32_t mb = (schema_mask >> (genotype_len - 1 - b)) & 1U;
        if (mb) {
            if (!found) { first = b; found = 1; }
            last = b;
        }
    }
    uint32_t defining_length = found ? (last - first) : 0;

    /* Order of schema: number of fixed bits */
    uint32_t order = 0;
    for (uint32_t b = 0; b < genotype_len; b++) {
        if ((schema_mask >> (genotype_len - 1 - b)) & 1U) order++;
    }

    /* Schema Theorem estimate:
       E[m(H,t+1)] >= m(H,t) * (f(H)/f_avg) * [1 - p_c * delta(H)/(L-1) * (1 - p(H,t))]
                     * (1 - p_m)^o(H)
       (Simplified: assume p(H,t) ? m(H,t)/pop_size) */
    double p_H = (double)schema_count / (double)pop_size;
    double delta_over_L = (genotype_len > 1)
        ? (double)defining_length / (double)(genotype_len - 1) : 0.0;
    double disruption_crossover = 1.0 - crossover_rate * delta_over_L * (1.0 - p_H);
    if (disruption_crossover < 0.0) disruption_crossover = 0.0;
    double survival_mutation = pow(1.0 - mutation_rate, (double)order);

    double expected = (double)schema_count * (f_schema / f_avg)
                      * disruption_crossover * survival_mutation;
    return expected;
}

/* ================================================================
 * L5: Classifier System ? Bucket Brigade
 * ================================================================ */

sfi_classifier_t sfi_classifier_create(uint32_t id,
    const char *condition, const char *action) {
    sfi_classifier_t cl;
    memset(&cl, 0, sizeof(cl));
    cl.id = id;
    if (condition) {
        strncpy(cl.condition, condition, 63);
        cl.condition[63] = '\0';
    }
    if (action) {
        strncpy(cl.action, action, 63);
        cl.action[63] = '\0';
    }
    cl.strength = 100.0;  /* Initial strength */
    /* Compute specificity: fraction of non-# bits */
    uint32_t total = 0, specific = 0;
    for (uint32_t i = 0; cl.condition[i] && i < 64; i++) {
        if (cl.condition[i] == '0' || cl.condition[i] == '1') specific++;
        total++;
    }
    cl.specificity = (total > 0) ? (double)specific / (double)total : 0.0;
    cl.fire_count = 0;
    cl.match_count = 0;
    cl.generation_born = 0;
    return cl;
}

int sfi_classifier_matches(const sfi_classifier_t *cl,
                           const char *percept) {
    if (!cl || !percept) return 0;
    for (uint32_t i = 0; cl->condition[i] && percept[i] && i < 64; i++) {
        if (cl->condition[i] != '#' && cl->condition[i] != percept[i])
            return 0;
    }
    return 1;
}

void sfi_classifier_bucket_brigade(sfi_classifier_t *winner,
                                   sfi_classifier_t *prev_winner,
                                   double bid_fraction, double reward) {
    if (!winner) return;
    /* Winner bids a fraction of its strength */
    double bid = winner->strength * bid_fraction;
    /* Winner pays bid to previous winner (if any) */
    if (prev_winner) {
        prev_winner->strength += bid;
        winner->strength -= bid;
    }
    /* Winner receives external reward */
    winner->strength += reward;
    winner->fire_count++;
}
