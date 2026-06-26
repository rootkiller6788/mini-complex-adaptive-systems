/**
 * ga_operators.c — Selection, Crossover, Mutation, Replacement Operators
 *
 * Implements all canonical GA variation and selection operators.
 * Each function implements a distinct evolutionary mechanism grounded
 * in both biological analogy and mathematical optimization theory.
 *
 * Reference: Goldberg "Genetic Algorithms" (1989) Ch3
 *            Bäck "Evolutionary Algorithms in Theory and Practice" (1996)
 *            Eiben & Smith "Introduction to Evolutionary Computing" (2015)
 *            Deb "Multi-Objective Optimization using EAs" (2001)
 *
 * Knowledge: L5 Algorithms/Methods
 */

#include "ga_operators.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Forward declarations of random utilities from ga_core */
extern uint64_t _ga_rand_state; /* not needed, we use our own local versions */

/* Local PRNG — xorshift64* */
static uint64_t _op_rand_state = 987654321ULL;

static uint64_t _op_rand64(void) {
    _op_rand_state ^= _op_rand_state >> 12;
    _op_rand_state ^= _op_rand_state << 25;
    _op_rand_state ^= _op_rand_state >> 27;
    return _op_rand_state * 0x2545F4914F6CDD1DULL;
}

static double _op_rand_double(void) {
    return (double)(_op_rand64() >> 11) / (double)(1ULL << 53);
}

static int _op_rand_int(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (int)(_op_rand64() % (uint64_t)(hi - lo));
}

static double _op_gaussian(double mean, double stddev) {
    double u1 = _op_rand_double();
    double u2 = _op_rand_double();
    if (u1 < 1e-12) u1 = 1e-12;
    return mean + stddev * sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

/* ====================================================================
 * L5: Selection Operators
 * ==================================================================== */

int ga_select_roulette(const Population *pop) {
    if (!pop || pop->size <= 0) return -1;
    if (pop->total_fitness <= 0.0) return _op_rand_int(0, pop->size);

    double target = _op_rand_double() * pop->total_fitness;
    double cumulative = 0.0;

    for (int i = 0; i < pop->size; i++) {
        cumulative += pop->individuals[i].fitness;
        if (cumulative >= target) return i;
    }
    return pop->size - 1;
}

int ga_select_tournament(const Population *pop, int tournament_size) {
    if (!pop || pop->size <= 0) return -1;

    int best_idx = _op_rand_int(0, pop->size);
    double best_fit = pop->individuals[best_idx].fitness;

    for (int k = 1; k < tournament_size && k < pop->size; k++) {
        int idx = _op_rand_int(0, pop->size);
        if (pop->individuals[idx].fitness > best_fit) {
            best_fit = pop->individuals[idx].fitness;
            best_idx = idx;
        }
    }
    return best_idx;
}

int ga_select_rank(const Population *pop, double selection_pressure) {
    if (!pop || pop->size <= 0) return -1;

    /* Create sorted indices by fitness */
    int *ranks = (int*)malloc(sizeof(int) * (size_t)pop->size);
    if (!ranks) return _op_rand_int(0, pop->size);

    /* Simple O(N²) ranking (sufficient for moderate populations) */
    for (int i = 0; i < pop->size; i++) ranks[i] = i;
    for (int i = 0; i < pop->size; i++) {
        for (int j = i + 1; j < pop->size; j++) {
            if (pop->individuals[ranks[j]].fitness >
                pop->individuals[ranks[i]].fitness) {
                int tmp = ranks[i];
                ranks[i] = ranks[j];
                ranks[j] = tmp;
            }
        }
    }

    /* Linear ranking: P(rank_r) = [2-sp + 2r(sp-1)/(N-1)] / N */
    double sp = (selection_pressure < 1.0) ? 1.0 :
                (selection_pressure > 2.0) ? 2.0 : selection_pressure;
    double *probs = (double*)malloc(sizeof(double) * (size_t)pop->size);
    if (!probs) { free(ranks); return _op_rand_int(0, pop->size); }

    double N = (double)pop->size;
    double sum_p = 0.0;
    for (int r = 0; r < pop->size; r++) {
        double p = (2.0 - sp) / N + 2.0 * (double)r * (sp - 1.0) / (N * (N - 1.0));
        probs[r] = p;
        sum_p += p;
    }

    /* Sample from distribution */
    double target = _op_rand_double() * sum_p;
    double cum = 0.0;
    int selected = ranks[pop->size - 1];
    for (int r = 0; r < pop->size; r++) {
        cum += probs[r];
        if (cum >= target) {
            selected = ranks[r];
            break;
        }
    }

    free(ranks);
    free(probs);
    return selected;
}

void ga_select_sus(const Population *pop, int *selected_indices, int n_select) {
    if (!pop || !selected_indices || n_select <= 0) return;
    if (pop->total_fitness <= 0.0) {
        for (int i = 0; i < n_select; i++)
            selected_indices[i] = _op_rand_int(0, pop->size);
        return;
    }

    /* SUS: single random start + equally spaced pointers */
    double spacing = pop->total_fitness / (double)n_select;
    double start = _op_rand_double() * spacing;

    for (int s = 0; s < n_select; s++) {
        double target = start + (double)s * spacing;
        double cum = 0.0;
        for (int i = 0; i < pop->size; i++) {
            cum += pop->individuals[i].fitness;
            if (cum >= target) {
                selected_indices[s] = i;
                break;
            }
        }
    }
}

int ga_select_truncation(const Population *pop, double truncation_ratio) {
    if (!pop || pop->size <= 0) return -1;

    int cutoff = (int)(truncation_ratio * (double)pop->size);
    if (cutoff <= 0) cutoff = 1;
    if (cutoff > pop->size) cutoff = pop->size;

    /* Find top individuals efficiently: copy and partial sort */
    /* For simplicity: assume population is already sorted or find elite set */
    /* Select uniformly from top cutoff% */
    return _op_rand_int(0, cutoff);
}

void ga_selection_probabilities(const Population *pop,
                                 SelectionMethod method,
                                 double *probs, double param) {
    if (!pop || !probs || pop->size <= 0) return;

    switch (method) {
        case SELECT_ROULETTE: {
            if (pop->total_fitness <= 0.0) {
                double uniform = 1.0 / (double)pop->size;
                for (int i = 0; i < pop->size; i++) probs[i] = uniform;
            } else {
                for (int i = 0; i < pop->size; i++)
                    probs[i] = pop->individuals[i].fitness / pop->total_fitness;
            }
            break;
        }
        case SELECT_RANK: {
            /* Compute ranks */
            int *order = (int*)malloc(sizeof(int) * (size_t)pop->size);
            if (!order) return;
            for (int i = 0; i < pop->size; i++) order[i] = i;
            for (int i = 0; i < pop->size; i++) {
                for (int j = i + 1; j < pop->size; j++) {
                    if (pop->individuals[order[j]].fitness >
                        pop->individuals[order[i]].fitness) {
                        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
                    }
                }
            }
            double sp = (param < 1.0) ? 1.0 : (param > 2.0) ? 2.0 : param;
            double N = (double)pop->size;
            double sum = 0.0;
            for (int r = 0; r < pop->size; r++) {
                probs[order[r]] = (2.0 - sp) / N +
                    2.0 * (double)r * (sp - 1.0) / (N * (N - 1.0));
                sum += probs[order[r]];
            }
            for (int i = 0; i < pop->size; i++) probs[i] /= sum;
            free(order);
            break;
        }
        case SELECT_TOURNAMENT: {
            /* Approximate: probability proportional to (rank-based) */
            int *order = (int*)malloc(sizeof(int) * (size_t)pop->size);
            if (!order) return;
            for (int i = 0; i < pop->size; i++) order[i] = i;
            for (int i = 0; i < pop->size; i++) {
                for (int j = i + 1; j < pop->size; j++) {
                    if (pop->individuals[order[j]].fitness >
                        pop->individuals[order[i]].fitness) {
                        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
                    }
                }
            }
            double sum = 0.0;
            for (int r = 0; r < pop->size; r++) {
                probs[order[r]] = pow((double)(r + 1) / (double)pop->size,
                                       (double)(param > 0 ? param : 2));
                sum += probs[order[r]];
            }
            for (int i = 0; i < pop->size; i++) probs[i] /= sum;
            free(order);
            break;
        }
        default:
            for (int i = 0; i < pop->size; i++)
                probs[i] = 1.0 / (double)pop->size;
    }
}

/* ====================================================================
 * L5: Crossover Operators
 * ==================================================================== */

void ga_crossover_one_point(const Chromosome *p1, const Chromosome *p2,
                             Chromosome *c1, Chromosome *c2) {
    if (!p1 || !p2 || !c1 || !c2) return;
    int len = p1->num_genes;
    if (len <= 1) return;

    int point = _op_rand_int(1, len);

    /* child1: p1[0..point) + p2[point..len) */
    for (int i = 0; i < point; i++)
        c1->alleles[i] = p1->alleles[i];
    for (int i = point; i < len; i++)
        c1->alleles[i] = p2->alleles[i];

    /* child2: p2[0..point) + p1[point..len) */
    for (int i = 0; i < point; i++)
        c2->alleles[i] = p2->alleles[i];
    for (int i = point; i < len; i++)
        c2->alleles[i] = p1->alleles[i];
}

void ga_crossover_two_point(const Chromosome *p1, const Chromosome *p2,
                             Chromosome *c1, Chromosome *c2) {
    if (!p1 || !p2 || !c1 || !c2) return;
    int len = p1->num_genes;
    if (len <= 2) { ga_crossover_one_point(p1, p2, c1, c2); return; }

    int k1 = _op_rand_int(1, len - 1);
    int k2 = _op_rand_int(1, len - 1);
    if (k1 > k2) { int tmp = k1; k1 = k2; k2 = tmp; }
    if (k1 == k2) k2 = k1 + 1;

    /* child1: p1[0,k1) + p2[k1,k2) + p1[k2,len) */
    for (int i = 0; i < k1; i++) c1->alleles[i] = p1->alleles[i];
    for (int i = k1; i < k2; i++) c1->alleles[i] = p2->alleles[i];
    for (int i = k2; i < len; i++) c1->alleles[i] = p1->alleles[i];

    /* child2: p2[0,k1) + p1[k1,k2) + p2[k2,len) */
    for (int i = 0; i < k1; i++) c2->alleles[i] = p2->alleles[i];
    for (int i = k1; i < k2; i++) c2->alleles[i] = p1->alleles[i];
    for (int i = k2; i < len; i++) c2->alleles[i] = p2->alleles[i];
}

void ga_crossover_uniform(const Chromosome *p1, const Chromosome *p2,
                           Chromosome *c1, Chromosome *c2,
                           double bias) {
    if (!p1 || !p2 || !c1 || !c2) return;
    int len = p1->num_genes;

    for (int i = 0; i < len; i++) {
        if (_op_rand_double() < bias) {
            c1->alleles[i] = p1->alleles[i];
            c2->alleles[i] = p2->alleles[i];
        } else {
            c1->alleles[i] = p2->alleles[i];
            c2->alleles[i] = p1->alleles[i];
        }
    }
}

void ga_crossover_arithmetic(const Chromosome *p1, const Chromosome *p2,
                              Chromosome *c1, Chromosome *c2,
                              double alpha_ext) {
    if (!p1 || !p2 || !c1 || !c2) return;
    int len = p1->num_genes;

    for (int i = 0; i < len; i++) {
        if (p1->loci[i].type != GENE_REAL && p1->loci[i].type != GENE_INTEGER) {
            /* Binary/symbolic: uniform crossover fallback */
            if (_op_rand_double() < 0.5) {
                c1->alleles[i] = p1->alleles[i];
                c2->alleles[i] = p2->alleles[i];
            } else {
                c1->alleles[i] = p2->alleles[i];
                c2->alleles[i] = p1->alleles[i];
            }
            continue;
        }

        double v1 = (p1->loci[i].type == GENE_REAL)
                     ? p1->alleles[i].real_val
                     : (double)p1->alleles[i].integer_val;
        double v2 = (p2->loci[i].type == GENE_REAL)
                     ? p2->alleles[i].real_val
                     : (double)p2->alleles[i].integer_val;

        /* α ∈ [-alpha_ext, 1 + alpha_ext] */
        double alpha = -alpha_ext + _op_rand_double() * (1.0 + 2.0 * alpha_ext);
        double child1_val = alpha * v1 + (1.0 - alpha) * v2;
        double child2_val = alpha * v2 + (1.0 - alpha) * v1;

        /* Clamp to bounds */
        double lo = p1->loci[i].lower_bound;
        double hi = p1->loci[i].upper_bound;
        if (child1_val < lo) child1_val = lo;
        if (child1_val > hi) child1_val = hi;
        if (child2_val < lo) child2_val = lo;
        if (child2_val > hi) child2_val = hi;

        if (p1->loci[i].type == GENE_REAL) {
            c1->alleles[i].real_val = child1_val;
            c2->alleles[i].real_val = child2_val;
        } else {
            c1->alleles[i].integer_val = (int)round(child1_val);
            c2->alleles[i].integer_val = (int)round(child2_val);
        }
    }
}

void ga_crossover_sbx(const Chromosome *p1, const Chromosome *p2,
                       Chromosome *c1, Chromosome *c2, double eta) {
    if (!p1 || !p2 || !c1 || !c2) return;
    int len = p1->num_genes;
    if (eta <= 0.0) eta = 1.0;

    for (int i = 0; i < len; i++) {
        if (p1->loci[i].type != GENE_REAL) {
            c1->alleles[i] = p1->alleles[i];
            c2->alleles[i] = p2->alleles[i];
            continue;
        }

        double v1 = p1->alleles[i].real_val;
        double v2 = p2->alleles[i].real_val;
        double lo = p1->loci[i].lower_bound;
        double hi = p1->loci[i].upper_bound;

        if (fabs(v1 - v2) < 1e-12) {
            c1->alleles[i].real_val = v1;
            c2->alleles[i].real_val = v2;
            continue;
        }

        /* Ensure v1 < v2 */
        if (v1 > v2) { double tmp = v1; v1 = v2; v2 = tmp; }

        double u = _op_rand_double();
        double beta;
        if (u <= 0.5) {
            beta = pow(2.0 * u, 1.0 / (eta + 1.0));
        } else {
            beta = pow(1.0 / (2.0 * (1.0 - u)), 1.0 / (eta + 1.0));
        }

        double child1 = 0.5 * ((v1 + v2) - beta * (v2 - v1));
        double child2 = 0.5 * ((v1 + v2) + beta * (v2 - v1));

        if (child1 < lo) child1 = lo;
        if (child1 > hi) child1 = hi;
        if (child2 < lo) child2 = lo;
        if (child2 > hi) child2 = hi;

        c1->alleles[i].real_val = child1;
        c2->alleles[i].real_val = child2;
    }
}

void ga_crossover_ox(const Chromosome *p1, const Chromosome *p2,
                      Chromosome *c1, Chromosome *c2) {
    if (!p1 || !p2 || !c1 || !c2) return;
    int len = p1->num_genes;
    if (len <= 1) return;

    /* Order Crossover (OX) for permutation chromosomes:
     * 1. Select two cut points
     * 2. Copy middle segment from parent 1 to child 1
     * 3. Fill remaining positions with genes from parent 2 in order,
     *    skipping genes already placed.
     */
    int k1 = _op_rand_int(0, len);
    int k2 = _op_rand_int(0, len);
    if (k1 > k2) { int tmp = k1; k1 = k2; k2 = tmp; }

    /* Mark which genes are in the middle segment */
    bool used[GA_MAX_GENES] = {false};

    /* Child 1 */
    for (int i = k1; i < k2; i++) {
        c1->alleles[i] = p1->alleles[i];
        used[p1->alleles[i].integer_val] = true;
    }
    /* Fill remaining from p2 in order, skipping used genes */
    int fill_pos = k2 % len;
    for (int src = k2; src < len + k2; src++) {
        int si = src % len;
        int gene_val = p2->alleles[si].integer_val;
        if (!used[gene_val]) {
            c1->alleles[fill_pos] = p2->alleles[si];
            used[gene_val] = true;
            fill_pos = (fill_pos + 1) % len;
            if (fill_pos == k1) fill_pos = k2; /* skip middle segment */
        }
    }

    /* Child 2 — symmetric */
    bool used2[GA_MAX_GENES] = {false};
    for (int i = k1; i < k2; i++) {
        c2->alleles[i] = p2->alleles[i];
        used2[p2->alleles[i].integer_val] = true;
    }
    fill_pos = k2 % len;
    for (int src = k2; src < len + k2; src++) {
        int si = src % len;
        int gene_val = p1->alleles[si].integer_val;
        if (!used2[gene_val]) {
            c2->alleles[fill_pos] = p1->alleles[si];
            used2[gene_val] = true;
            fill_pos = (fill_pos + 1) % len;
            if (fill_pos == k1) fill_pos = k2;
        }
    }
}

void ga_crossover_pmx(const Chromosome *p1, const Chromosome *p2,
                       Chromosome *c1, Chromosome *c2) {
    if (!p1 || !p2 || !c1 || !c2) return;
    int len = p1->num_genes;
    if (len <= 1) return;

    int k1 = _op_rand_int(0, len);
    int k2 = _op_rand_int(0, len);
    if (k1 > k2) { int tmp = k1; k1 = k2; k2 = tmp; }

    /* Initialize children as copies */
    for (int i = 0; i < len; i++) {
        c1->alleles[i] = p1->alleles[i];
        c2->alleles[i] = p2->alleles[i];
    }

    /* Swap middle segments */
    for (int i = k1; i < k2; i++) {
        /* Find positions of the swapped alleles and resolve conflicts */
        int a1 = p1->alleles[i].integer_val;
        int a2 = p2->alleles[i].integer_val;

        if (a1 == a2) continue;

        /* Find a2 in child1 and replace with a1 */
        for (int j = 0; j < len; j++) {
            if (j >= k1 && j < k2) continue;
            if (c1->alleles[j].integer_val == a2) {
                c1->alleles[j].integer_val = a1;
                break;
            }
        }
        /* Find a1 in child2 and replace with a2 */
        for (int j = 0; j < len; j++) {
            if (j >= k1 && j < k2) continue;
            if (c2->alleles[j].integer_val == a1) {
                c2->alleles[j].integer_val = a2;
                break;
            }
        }
    }

    /* Copy the swapped segments */
    for (int i = k1; i < k2; i++) {
        c1->alleles[i] = p2->alleles[i];
        c2->alleles[i] = p1->alleles[i];
    }
}

/* ====================================================================
 * L5: Mutation Operators
 * ==================================================================== */

void ga_mutate_bit_flip(Chromosome *c, double mutation_rate) {
    if (!c) return;
    for (int i = 0; i < c->num_genes; i++) {
        if (c->loci[i].type == GENE_BINARY) {
            if (_op_rand_double() < mutation_rate) {
                c->alleles[i].binary_val = !c->alleles[i].binary_val;
            }
        }
    }
}

void ga_mutate_gaussian(Chromosome *c, double mutation_rate, double step_size) {
    if (!c) return;
    for (int i = 0; i < c->num_genes; i++) {
        if (c->loci[i].type != GENE_REAL && c->loci[i].type != GENE_INTEGER)
            continue;
        if (_op_rand_double() >= mutation_rate) continue;

        double range = c->loci[i].upper_bound - c->loci[i].lower_bound;
        double sigma = step_size * range;

        if (c->loci[i].type == GENE_REAL) {
            double val = c->alleles[i].real_val + _op_gaussian(0.0, sigma);
            if (val < c->loci[i].lower_bound) val = c->loci[i].lower_bound;
            if (val > c->loci[i].upper_bound) val = c->loci[i].upper_bound;
            c->alleles[i].real_val = val;
        } else {
            double val = (double)c->alleles[i].integer_val + _op_gaussian(0.0, sigma);
            c->alleles[i].integer_val = (int)round(val);
            if (c->alleles[i].integer_val < (int)c->loci[i].lower_bound)
                c->alleles[i].integer_val = (int)c->loci[i].lower_bound;
            if (c->alleles[i].integer_val > (int)c->loci[i].upper_bound)
                c->alleles[i].integer_val = (int)c->loci[i].upper_bound;
        }
    }
}

void ga_mutate_uniform(Chromosome *c, double mutation_rate) {
    if (!c) return;
    for (int i = 0; i < c->num_genes; i++) {
        if (_op_rand_double() >= mutation_rate) continue;

        switch (c->loci[i].type) {
            case GENE_BINARY:
                c->alleles[i].binary_val = (_op_rand_double() < 0.5);
                break;
            case GENE_REAL:
                c->alleles[i].real_val = c->loci[i].lower_bound +
                    _op_rand_double() * (c->loci[i].upper_bound -
                                          c->loci[i].lower_bound);
                break;
            case GENE_INTEGER: {
                int lo = (int)c->loci[i].lower_bound;
                int hi = (int)c->loci[i].upper_bound;
                c->alleles[i].integer_val = _op_rand_int(lo, hi + 1);
                break;
            }
            default:
                break;
        }
    }
}

void ga_mutate_polynomial(Chromosome *c, double mutation_rate, double eta) {
    if (!c) return;
    if (eta <= 0.0) eta = 1.0;

    for (int i = 0; i < c->num_genes; i++) {
        if (c->loci[i].type != GENE_REAL) continue;
        if (_op_rand_double() >= mutation_rate) continue;

        double val = c->alleles[i].real_val;
        double lo = c->loci[i].lower_bound;
        double hi = c->loci[i].upper_bound;

        double u = _op_rand_double();
        double delta;
        if (u < 0.5) {
            delta = pow(2.0 * u, 1.0 / (eta + 1.0)) - 1.0;
        } else {
            delta = 1.0 - pow(2.0 * (1.0 - u), 1.0 / (eta + 1.0));
        }

        val += delta * (hi - lo);
        if (val < lo) val = lo;
        if (val > hi) val = hi;
        c->alleles[i].real_val = val;
    }
}

void ga_mutate_swap(Chromosome *c) {
    if (!c || c->num_genes < 2) return;
    int i = _op_rand_int(0, c->num_genes);
    int j = _op_rand_int(0, c->num_genes);
    if (i == j) return;
    Allele tmp = c->alleles[i];
    c->alleles[i] = c->alleles[j];
    c->alleles[j] = tmp;
}

void ga_mutate_inversion(Chromosome *c) {
    if (!c || c->num_genes < 2) return;
    int i = _op_rand_int(0, c->num_genes);
    int j = _op_rand_int(0, c->num_genes);
    if (i > j) { int tmp = i; i = j; j = tmp; }
    if (i == j) return;

    /* Reverse subsequence [i, j] */
    while (i < j) {
        Allele tmp = c->alleles[i];
        c->alleles[i] = c->alleles[j];
        c->alleles[j] = tmp;
        i++;
        j--;
    }
}

void ga_mutate_scramble(Chromosome *c) {
    if (!c || c->num_genes < 2) return;
    int i = _op_rand_int(0, c->num_genes);
    int j = _op_rand_int(0, c->num_genes);
    if (i > j) { int tmp = i; i = j; j = tmp; }
    if (i == j) return;

    /* Fisher-Yates shuffle of subsequence */
    for (int k = j; k > i; k--) {
        int r = _op_rand_int(i, k + 1);
        Allele tmp = c->alleles[k];
        c->alleles[k] = c->alleles[r];
        c->alleles[r] = tmp;
    }
}

/* ====================================================================
 * L5: Replacement Strategies
 * ==================================================================== */

void ga_replace_generational(Population *parent, Population *offspring) {
    if (!parent || !offspring) return;
    int n = (offspring->size < parent->size) ? offspring->size : parent->size;
    for (int i = 0; i < n; i++) {
        parent->individuals[i] = ga_chromosome_copy(&offspring->individuals[i]);
    }
}

void ga_replace_steady_state(Population *parent,
                              const Population *offspring) {
    if (!parent || !offspring || offspring->size <= 0) return;

    /* Find worst individuals in parent to replace */
    for (int o = 0; o < offspring->size && o < parent->size; o++) {
        int worst_idx = 0;
        double worst_fit = parent->individuals[0].fitness;
        for (int i = 1; i < parent->size; i++) {
            if (parent->individuals[i].fitness < worst_fit) {
                worst_fit = parent->individuals[i].fitness;
                worst_idx = i;
            }
        }
        /* Replace only if offspring is better */
        if (offspring->individuals[o].fitness > worst_fit) {
            parent->individuals[worst_idx] =
                ga_chromosome_copy(&offspring->individuals[o]);
        }
    }
}

void ga_replace_elitist(Population *parent, Population *offspring,
                         int n_elite) {
    if (!parent || !offspring) return;

    /* Sort both populations */
    ga_population_sort(parent);
    ga_population_sort(offspring);

    /* Keep best n_elite from parent, fill rest from offspring */
    if (n_elite > parent->size) n_elite = parent->size;
    if (n_elite < 0) n_elite = 0;

    for (int i = n_elite; i < parent->size; i++) {
        int off_idx = i - n_elite;
        if (off_idx < offspring->size) {
            parent->individuals[i] =
                ga_chromosome_copy(&offspring->individuals[off_idx]);
        }
    }
    /* Elites already in place at indices 0..n_elite-1 */
}

void ga_replace_mu_plus_lambda(Population *parent,
                                Population *offspring) {
    if (!parent || !offspring) return;

    /* Combine parent and offspring into a temporary pool */
    int total = parent->size + offspring->size;
    Chromosome *pool = (Chromosome*)malloc(sizeof(Chromosome) * (size_t)total);
    if (!pool) return;

    for (int i = 0; i < parent->size; i++)
        pool[i] = ga_chromosome_copy(&parent->individuals[i]);
    for (int i = 0; i < offspring->size; i++)
        pool[parent->size + i] = ga_chromosome_copy(&offspring->individuals[i]);

    /* Sort by fitness descending */
    for (int i = 0; i < total; i++) {
        for (int j = i + 1; j < total; j++) {
            if (pool[j].fitness > pool[i].fitness) {
                Chromosome tmp = pool[i];
                pool[i] = pool[j];
                pool[j] = tmp;
            }
        }
    }

    /* Keep best μ (parent->size) individuals */
    for (int i = 0; i < parent->size; i++) {
        parent->individuals[i] = pool[i];
    }

    free(pool);
}

void ga_replace_mu_comma_lambda(Population *parent,
                                 Population *offspring) {
    if (!parent || !offspring) return;

    /* Select best μ from λ offspring only (parents discarded) */
    /* Requires λ > μ (offspring->size > parent->size) */
    if (offspring->size <= parent->size) {
        ga_replace_generational(parent, offspring);
        return;
    }

    /* Sort offspring */
    for (int i = 0; i < offspring->size; i++) {
        for (int j = i + 1; j < offspring->size; j++) {
            if (offspring->individuals[j].fitness >
                offspring->individuals[i].fitness) {
                Chromosome tmp;
                memcpy(&tmp, &offspring->individuals[i], sizeof(Chromosome));
                memcpy(&offspring->individuals[i],
                       &offspring->individuals[j], sizeof(Chromosome));
                memcpy(&offspring->individuals[j], &tmp, sizeof(Chromosome));
            }
        }
    }

    for (int i = 0; i < parent->size; i++) {
        parent->individuals[i] =
            ga_chromosome_copy(&offspring->individuals[i]);
    }
}

void ga_replace_crowding(Population *parent, Population *offspring) {
    if (!parent || !offspring) return;

    for (int o = 0; o < offspring->size && o < parent->size; o++) {
        /* Find most similar parent to each offspring */
        int nearest = 0;
        int min_dist = ga_hamming_distance(&offspring->individuals[o],
                                            &parent->individuals[0]);
        for (int p = 1; p < parent->size; p++) {
            int dist = ga_hamming_distance(&offspring->individuals[o],
                                            &parent->individuals[p]);
            if (dist < min_dist) {
                min_dist = dist;
                nearest = p;
            }
        }
        /* Replace if offspring is fitter */
        if (offspring->individuals[o].fitness >
            parent->individuals[nearest].fitness) {
            parent->individuals[nearest] =
                ga_chromosome_copy(&offspring->individuals[o]);
        }
    }
}

/* ====================================================================
 * L3: Operator Analysis
 * ==================================================================== */

double ga_crossover_disruption_rate(int chromosome_length,
                                     CrossoverMethod method) {
    if (chromosome_length <= 1) return 0.0;

    switch (method) {
        case CROSSOVER_SINGLE_POINT: {
            /* P(disrupt) = δ(H) / (L-1) for schema of defining length δ.
             * Average over all schemata: E[δ] = (L+1)/3.
             * So E[disrupt] = (L+1) / (3(L-1)). */
            double L = (double)chromosome_length;
            return (L + 1.0) / (3.0 * (L - 1.0));
        }
        case CROSSOVER_TWO_POINT: {
            /* Two-point has slightly lower disruption for schemata with
             * defining length > L/2. Approximation. */
            double L = (double)chromosome_length;
            return 2.0 * (L + 1.0) / (3.0 * (L - 1.0)) *
                   (1.0 - pow(0.5, L - 1));
        }
        case CROSSOVER_UNIFORM: {
            /* Each locus independently swapped with 50% probability.
             * For schema of order o, P(survive) = 0.5^o.
             * Average disruption for all positions. */
            return 0.5;
        }
        default:
            return 0.0;
    }
}

double ga_schema_survival_probability(int defining_length, int order,
                                       double mutation_rate, int chrom_len) {
    /* P(survival | crossover) ≥ 1 - p_c * δ(H) / (L-1) */
    double p_c = 1.0;  /* assume crossover always applied */

    /* For one-point crossover */
    double p_survive_cross = 1.0 - p_c * (double)defining_length /
                             (double)(chrom_len - 1);
    if (p_survive_cross < 0.0) p_survive_cross = 0.0;

    /* P(survival | mutation) = (1 - p_m)^o(H) */
    double p_survive_mut = pow(1.0 - mutation_rate, (double)order);

    return p_survive_cross * p_survive_mut;
}

double ga_positional_bias(CrossoverMethod method, int chrom_len) {
    /* Positional bias measures how strongly crossover favors
     * keeping genes that are physically close together.
     * Higher bias = adjacent genes more likely to stay together. */
    switch (method) {
        case CROSSOVER_SINGLE_POINT: {
            /* Two genes at positions i, j stay together if cut point is
             * outside [i,j]. P(together) = 1 - |j-i|/(L-1). */
            double avg_dist = (double)chrom_len / 3.0;
            return 1.0 - avg_dist / (double)(chrom_len - 1);
        }
        case CROSSOVER_TWO_POINT: {
            /* More complex: higher positional bias than one-point */
            double avg_dist = (double)chrom_len / 4.0;
            return 1.0 - avg_dist / (double)(chrom_len - 1);
        }
        case CROSSOVER_UNIFORM:
            return 0.0;  /* No positional bias */
        default:
            return 0.5;
    }
}
