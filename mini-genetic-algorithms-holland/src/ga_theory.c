/**
 * ga_theory.c — Schema Theory and Theoretical Foundations
 *
 * Implements John Holland's Schema Theorem, Building Block Hypothesis,
 * implicit parallelism analysis, deception detection, Price's Equation,
 * Fisher's Fundamental Theorem, Vose's infinite population model,
 * and No Free Lunch bounds.
 *
 * Reference: Holland "Adaptation in Natural and Artificial Systems" (1975,1992)
 *            Goldberg "Genetic Algorithms" (1989) Ch2, Ch4
 *            Vose "The Simple Genetic Algorithm" (1999)
 *            Wolpert & Macready "No Free Lunch Theorems" (IEEE TEC, 1997)
 *            Price "Selection and Covariance" (Nature, 1970)
 *
 * Knowledge: L1 Definitions, L3 Math Structures, L4 Fundamental Laws, L8 Advanced
 */

#include "ga_theory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ====================================================================
 * L1: Schema Creation and Matching
 * ==================================================================== */

Schema ga_schema_create(const char *template_str) {
    Schema s;
    memset(&s, 0, sizeof(s));
    if (!template_str) return s;

    s.length = (int)strlen(template_str);
    if (s.length > GA_SCHEMA_MAX_LENGTH) s.length = GA_SCHEMA_MAX_LENGTH;

    for (int i = 0; i < s.length; i++) {
        switch (template_str[i]) {
            case '0': s.alleles[i] = SCHEMA_ALLELE_0; break;
            case '1': s.alleles[i] = SCHEMA_ALLELE_1; break;
            case '*': s.alleles[i] = SCHEMA_ALLELE_STAR; break;
            default:  s.alleles[i] = SCHEMA_ALLELE_STAR; break;
        }
    }

    s.order = ga_schema_order(&s);
    s.defining_length = ga_schema_defining_length(&s);
    return s;
}

bool ga_schema_matches(const Schema *schema, const Chromosome *c) {
    if (!schema || !c) return false;
    int len = (schema->length < c->num_genes) ? schema->length : c->num_genes;

    for (int i = 0; i < len; i++) {
        if (schema->alleles[i] == SCHEMA_ALLELE_STAR) continue;
        bool expected = (schema->alleles[i] == SCHEMA_ALLELE_1);
        bool actual = false;
        if (i < c->num_genes) {
            if (c->loci[i].type == GENE_BINARY) {
                actual = c->alleles[i].binary_val;
            } else if (c->loci[i].type == GENE_INTEGER) {
                actual = (c->alleles[i].integer_val != 0);
            } else {
                actual = false; /* non-binary genes: only match star */
            }
        }
        if (expected != actual) return false;
    }
    return true;
}

int ga_schema_order(const Schema *schema) {
    if (!schema) return 0;
    int order = 0;
    for (int i = 0; i < schema->length; i++) {
        if (schema->alleles[i] != SCHEMA_ALLELE_STAR) order++;
    }
    return order;
}

int ga_schema_defining_length(const Schema *schema) {
    if (!schema) return 0;
    int first = -1, last = -1;
    for (int i = 0; i < schema->length; i++) {
        if (schema->alleles[i] != SCHEMA_ALLELE_STAR) {
            if (first < 0) first = i;
            last = i;
        }
    }
    if (first < 0) return 0;
    return last - first;
}

int ga_chromosome_schemata(const Chromosome *c, int order,
                            Schema *result, int max_results) {
    if (!c || !result || max_results <= 0) return 0;
    int count = 0;
    int L = c->num_genes;
    if (L > GA_SCHEMA_MAX_LENGTH) L = GA_SCHEMA_MAX_LENGTH;

    /* Generate all order-k schemata derivable from this chromosome.
     * For a chromosome of length L, there are C(L, order) schemata of exact order.
     * Since each fixed position MUST match the chromosome's allele,
     * there is exactly 1 schema per choice of fixed positions. */
    if (order > L || order < 0) return 0;

    /* Enumerate combinations: choose 'order' positions from L to fix */
    int *positions = (int*)malloc(sizeof(int) * (size_t)order);
    if (!positions) return 0;

    /* Start with first combination */
    for (int i = 0; i < order; i++) positions[i] = i;

    while (count < max_results) {
        /* Build schema from current combination */
        Schema s;
        memset(&s, 0, sizeof(s));
        s.length = L;
        for (int i = 0; i < L; i++) s.alleles[i] = SCHEMA_ALLELE_STAR;
        for (int k = 0; k < order; k++) {
            int pos = positions[k];
            s.alleles[pos] = c->alleles[pos].binary_val
                             ? SCHEMA_ALLELE_1 : SCHEMA_ALLELE_0;
        }
        s.order = order;
        s.defining_length = ga_schema_defining_length(&s);
        result[count++] = s;

        /* Generate next combination */
        int i;
        for (i = order - 1; i >= 0; i--) {
            if (positions[i] < L - order + i) break;
        }
        if (i < 0) break;
        positions[i]++;
        for (int j = i + 1; j < order; j++) {
            positions[j] = positions[j-1] + 1;
        }
    }

    free(positions);
    return count;
}

Schema ga_schema_from_two(const Chromosome *a, const Chromosome *b) {
    Schema s;
    memset(&s, 0, sizeof(s));
    if (!a || !b) return s;

    int len = (a->num_genes < b->num_genes) ? a->num_genes : b->num_genes;
    s.length = len;

    for (int i = 0; i < len; i++) {
        if (a->loci[i].type == GENE_BINARY && b->loci[i].type == GENE_BINARY) {
            if (a->alleles[i].binary_val == b->alleles[i].binary_val) {
                s.alleles[i] = a->alleles[i].binary_val
                               ? SCHEMA_ALLELE_1 : SCHEMA_ALLELE_0;
            } else {
                s.alleles[i] = SCHEMA_ALLELE_STAR;
            }
        } else {
            s.alleles[i] = SCHEMA_ALLELE_STAR;
        }
    }
    s.order = ga_schema_order(&s);
    s.defining_length = ga_schema_defining_length(&s);
    return s;
}

void ga_schema_print(const Schema *schema, char *buf, int buf_len) {
    if (!schema || !buf || buf_len <= 0) return;
    int len = (schema->length < buf_len - 1) ? schema->length : buf_len - 1;
    for (int i = 0; i < len; i++) {
        switch (schema->alleles[i]) {
            case SCHEMA_ALLELE_0:   buf[i] = '0'; break;
            case SCHEMA_ALLELE_1:   buf[i] = '1'; break;
            case SCHEMA_ALLELE_STAR: buf[i] = '*'; break;
            default:                 buf[i] = '?'; break;
        }
    }
    buf[len] = '\0';
}

/* ====================================================================
 * L4: Schema Theorem (Holland, 1975)
 * ==================================================================== */

double ga_schema_theorem_lower_bound(const Schema *schema,
                                      const Population *pop,
                                      double crossover_rate,
                                      double mutation_rate) {
    if (!schema || !pop || pop->size <= 0) return 0.0;

    /* m(H, t+1) ≥ m(H, t) * [f(H)/f̄] * [1 - p_c * δ(H)/(L-1) * (1 - P_survive)]
     *    * (1 - p_m)^o(H)
     *
     * Lower bound for one-point crossover:
     * P_survive_one_point = 1 - δ(H)/(L-1)
     * So term = 1 - p_c * δ(H)/(L-1)
     */

    int m_H = ga_schema_count(schema, pop);
    double f_H = ga_schema_avg_fitness(schema, pop);
    double f_avg = pop->avg_fitness;
    int L = pop->num_genes;

    if (f_avg <= 0.0 || L <= 1) return 0.0;

    double fitness_ratio = f_H / f_avg;
    double delta = (double)schema->defining_length;
    double crossover_loss = 1.0 - crossover_rate * delta / (double)(L - 1);
    if (crossover_loss < 0.0) crossover_loss = 0.0;
    double mutation_loss = pow(1.0 - mutation_rate, (double)schema->order);

    return (double)m_H * fitness_ratio * crossover_loss * mutation_loss;
}

double ga_schema_theorem_upper_bound(const Schema *schema,
                                      const Population *pop,
                                      double crossover_rate,
                                      double mutation_rate) {
    if (!schema || !pop || pop->size <= 0) return 0.0;

    /* Upper bound refinement (Stephens & Waelbroeck 1999):
     * Account for schema creation by crossover (not just disruption). */
    int m_H = ga_schema_count(schema, pop);
    double f_H = ga_schema_avg_fitness(schema, pop);
    double f_avg = pop->avg_fitness;
    int L = pop->num_genes;

    if (f_avg <= 0.0) return 0.0;

    double fitness_ratio = f_H / f_avg;
    double delta = (double)schema->defining_length;

    /* Upper bound: assume no disruption by crossover (best case) */
    double crossover_gain = 1.0 + crossover_rate *
                            (1.0 - delta / (double)(L - 1)) * (f_H / f_avg - 1.0);
    if (crossover_gain < 0.0) crossover_gain = 0.0;

    double mutation_loss = pow(1.0 - mutation_rate, (double)schema->order);

    return (double)m_H * fitness_ratio * crossover_gain * mutation_loss;
}

double ga_schema_avg_fitness(const Schema *schema, const Population *pop) {
    if (!schema || !pop || pop->size <= 0) return 0.0;

    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < pop->size; i++) {
        if (ga_schema_matches(schema, &pop->individuals[i])) {
            sum += pop->individuals[i].fitness;
            count++;
        }
    }
    if (count == 0) return 0.0;
    return sum / (double)count;
}

int ga_schema_count(const Schema *schema, const Population *pop) {
    if (!schema || !pop) return 0;
    int count = 0;
    for (int i = 0; i < pop->size; i++) {
        if (ga_schema_matches(schema, &pop->individuals[i])) count++;
    }
    return count;
}

/* ====================================================================
 * L4: Building Block Hypothesis
 * ==================================================================== */

int ga_identify_building_blocks(const Population *pop,
                                 int max_order,
                                 int max_delta,
                                 Schema *blocks,
                                 int max_blocks) {
    if (!pop || !blocks || max_blocks <= 0) return 0;
    int count = 0;
    int L = pop->num_genes;
    if (L <= 0 || L > GA_SCHEMA_MAX_LENGTH) return 0;

    /* Exhaustive enumeration of low-order schemata up to max_order.
     * For each chromosome in population, enumerate its order-k schemata
     * and check building block criteria. */
    for (int order = 1; order <= max_order && order <= L; order++) {
        /* Process each chromosome's order-k schemata */
        for (int ci = 0; ci < pop->size; ci++) {
            Schema temp_schemata[256];
            int n_schem = ga_chromosome_schemata(&pop->individuals[ci],
                                                  order, temp_schemata, 256);
            for (int s = 0; s < n_schem && count < max_blocks; s++) {
                if (ga_is_building_block(&temp_schemata[s], pop,
                                          max_order, max_delta)) {
                    /* Check if already in list (deduplication) */
                    bool dup = false;
                    for (int b = 0; b < count; b++) {
                        bool same = true;
                        for (int i = 0; i < L; i++) {
                            if (blocks[b].alleles[i] != temp_schemata[s].alleles[i]) {
                                same = false;
                                break;
                            }
                        }
                        if (same) { dup = true; break; }
                    }
                    if (!dup) {
                        blocks[count++] = temp_schemata[s];
                    }
                }
            }
        }
    }

    return count;
}

double ga_building_block_value(const Schema *schema, const Population *pop) {
    if (!schema || !pop) return 0.0;
    double f_H = ga_schema_avg_fitness(schema, pop);
    int o_H = schema->order;
    int d_H = schema->defining_length;
    if (o_H <= 0 || d_H <= 0) return 0.0;
    return f_H / ((double)o_H * (double)d_H);
}

bool ga_is_building_block(const Schema *schema, const Population *pop,
                           int max_order, int max_delta) {
    if (!schema || !pop) return false;
    if (schema->order > max_order) return false;
    if (schema->defining_length > max_delta) return false;
    double f_H = ga_schema_avg_fitness(schema, pop);
    return (f_H > pop->avg_fitness);
}

/* ====================================================================
 * L4: Implicit Parallelism
 * ==================================================================== */

double ga_implicit_parallelism(int population_size, int chromosome_length) {
    /* Holland's estimate: ~ N³ schemata processed per generation.
     *
     * Derivation (Goldberg 1989):
     * - Each chromosome belongs to 2^L schemata
     * - N chromosomes → N * 2^L schemata
     * - But many overlap; effective number ≈ N³
     *
     * More precisely: ~ c * N³ for some constant c.
     * The 1989 analysis gives k * N³ with k ≈ 1 / (2 * sqrt(pi) * σ_f)
     *
     * Simplified: O(N³) */
    (void)chromosome_length;
    double N = (double)population_size;
    return N * N * N;
}

/* ====================================================================
 * L4: Deception Analysis
 * ==================================================================== */

bool ga_is_deceptive(const Schema *opt_schema, const Schema *comp_schema,
                      const Population *pop) {
    if (!opt_schema || !comp_schema || !pop) return false;

    double f_opt = ga_schema_avg_fitness(opt_schema, pop);
    double f_comp = ga_schema_avg_fitness(comp_schema, pop);

    /* Deceptive if competitive schema has higher fitness */
    return (f_comp > f_opt);
}

double ga_deception_level(const Schema *true_schema,
                           const Schema **misleading_schemata,
                           int n_misleading, const Population *pop) {
    if (!true_schema || !misleading_schemata || n_misleading <= 0 || !pop)
        return 0.0;

    double f_true = ga_schema_avg_fitness(true_schema, pop);
    double max_misleading = 0.0;

    for (int i = 0; i < n_misleading; i++) {
        double f_mis = ga_schema_avg_fitness(misleading_schemata[i], pop);
        if (f_mis > max_misleading) max_misleading = f_mis;
    }

    /* Deception level = fitness gap between misleading and true schemata */
    return (max_misleading > f_true) ? (max_misleading - f_true) : 0.0;
}

/* ====================================================================
 * L4: Price's Equation and Fisher's Fundamental Theorem
 * ==================================================================== */

void ga_price_equation(const double *trait_values,
                        const double *fitness_values,
                        const double *trait_changes,
                        int n,
                        double *selection_effect,
                        double *transmission_effect) {
    if (!trait_values || !fitness_values || !trait_changes || n <= 0) {
        if (selection_effect) *selection_effect = 0.0;
        if (transmission_effect) *transmission_effect = 0.0;
        return;
    }

    /* Price Equation:
     *   Δz̄ = Cov(w_i, z_i) / w̄ + E(w_i * Δz_i) / w̄
     *
     * where:
     *   z_i = trait value of individual i
     *   w_i = fitness of individual i
     *   Δz_i = change in trait from parent to offspring
     *   Cov(w,z) / w̄ = selection effect
     *   E(w * Δz) / w̄ = transmission effect
     */

    double mean_w = 0.0, mean_z = 0.0;
    for (int i = 0; i < n; i++) {
        mean_w += fitness_values[i];
        mean_z += trait_values[i];
    }
    mean_w /= (double)n;
    mean_z /= (double)n;

    /* Covariance: E[w*z] - E[w]*E[z] */
    double cov = 0.0;
    for (int i = 0; i < n; i++) {
        cov += fitness_values[i] * trait_values[i];
    }
    cov = cov / (double)n - mean_w * mean_z;

    /* Expectation: E[w * Δz] */
    double expect = 0.0;
    for (int i = 0; i < n; i++) {
        expect += fitness_values[i] * trait_changes[i];
    }
    expect /= (double)n;

    if (selection_effect) *selection_effect = cov / mean_w;
    if (transmission_effect) *transmission_effect = expect / mean_w;
}

double ga_fisher_fundamental_theorem(const double *fitness_values, int n) {
    if (!fitness_values || n <= 0) return 0.0;

    /* Fisher's Fundamental Theorem:
     *   Δw̄ ≈ Var_g(w) / w̄
     *
     * The rate of increase in mean fitness equals the additive genetic
     * variance in fitness divided by mean fitness.
     */

    double mean = 0.0;
    for (int i = 0; i < n; i++) mean += fitness_values[i];
    mean /= (double)n;

    double var = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = fitness_values[i] - mean;
        var += diff * diff;
    }
    var /= (double)n;

    if (mean < 1e-12) return 0.0;
    return var / mean;
}

/* ====================================================================
 * L8: Vose's Infinite Population Model
 * ==================================================================== */

double ga_vose_mixing_entry(int i, int j, int k, int string_length) {
    /* Compute probability that crossover of strings j and k,
     * followed by mutation, produces string i.
     *
     * For one-point crossover at position c:
     *   child = j[0..c) + k[c..L)
     * with probability 1/(L-1) per cut point.
     *
     * M[i,{j,k}] = (1/2) * Σ_c P(crossover produces i | cut at c)
     */

    if (string_length <= 1) return 0.0;

    double prob = 0.0;
    for (int c = 1; c < string_length; c++) {
        /* Child from j-head + k-tail at cut c */
        int child1 = ((j >> c) << c) | (k & ((1 << c) - 1));
        /* Child from k-head + j-tail */
        int child2 = ((k >> c) << c) | (j & ((1 << c) - 1));

        if (child1 == i) prob += 0.5;
        if (child2 == i) prob += 0.5;
    }
    return prob / (double)(string_length - 1);
}

void ga_vose_heuristic_step(double *pop_distribution,
                             int num_strings,
                             double mutation_rate,
                             const double *fitness_values) {
    if (!pop_distribution || !fitness_values || num_strings <= 0) return;

    /* Heuristic Vose step: compute next-generation distribution.
     * p_i(t+1) = (1/σ̄) * Σ_j Σ_k p_j(t) p_k(t) * P(cross+mut j,k → i)
     *
     * Simplified: use selection + mutation approximation.
     */

    double total_fit = 0.0;
    for (int i = 0; i < num_strings; i++) {
        total_fit += pop_distribution[i] * fitness_values[i];
    }

    if (total_fit < 1e-12) return;

    double *new_dist = (double*)calloc((size_t)num_strings, sizeof(double));
    if (!new_dist) return;

    /* Selection: fitness-proportionate */
    for (int i = 0; i < num_strings; i++) {
        new_dist[i] = pop_distribution[i] * fitness_values[i] / total_fit;
    }

    /* Mutation: transition matrix approximation */
    for (int i = 0; i < num_strings; i++) {
        double stay_prob = pow(1.0 - mutation_rate, (double)num_strings);
        pop_distribution[i] = stay_prob * new_dist[i];
        /* Distribute mutated probability uniformly */
        double leak = (1.0 - stay_prob) * new_dist[i];
        for (int j = 0; j < num_strings; j++) {
            if (j != i) pop_distribution[j] += leak / (double)(num_strings - 1);
        }
    }

    free(new_dist);
}

/* ====================================================================
 * L4: No Free Lunch Theorem
 * ==================================================================== */

double ga_nfl_bound(int search_space_size, int sample_size) {
    /* No Free Lunch: All algorithms have the same expected performance
     * when averaged over all possible fitness functions.
     *
     * E[Φ] = Σ_{f} P(f) * Φ(A, f) = constant
     *
     * For optimization, the probability of finding a solution in m samples
     * from search space of size |X| is:
     *   P(found) = m / |X|
     *
     * This holds for ANY black-box algorithm.
     */
    if (search_space_size <= 0) return 0.0;
    return (double)sample_size / (double)search_space_size;
}
