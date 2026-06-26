#include "nk_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * NK Fitness Landscapes -- Core Implementation (L1-L3)
 *
 * L1 Definitions: NKGenotype, NKEpistaticMatrix, NKFitnessTable
 * L2 Core Concepts: NKLandscape lifecycle, phase classification
 * L3 Mathematical Structures: bit-packed genotype, epistatic adjacency
 *
 * Reference: Kauffman (1993) The Origins of Order, Ch. 3.
 * ============================================================================ */

/* Forward declarations */
static uint64_t xorshift64_star(uint64_t *state);
void nk_landscape_free(NKLandscape *land);
void nk_epistasis_free(NKEpistaticMatrix *em);
void nk_fitness_table_free(NKFitnessTable *ft);

/* ============================================================================
 * Internal: Xorshift64* pseudo-random number generator
 * Vigna, S. (2016). "An experimental exploration of Marsaglia's
 *   xorshift generators, scrambled." ACM Trans. Math. Softw.
 * Returns double in [0, 1). Independent knowledge point: PRNG design.
 * ============================================================================ */
static uint64_t xorshift64_star(uint64_t *state) {
    uint64_t x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

double nk_random_uniform(uint64_t *rng) {
    return (double)(xorshift64_star(rng) >> 11) * 0x1.0p-53;
}

/* Box-Muller transform: generate N(0,1) from two uniforms.
 * Independent knowledge point: normal distribution sampling. */
double nk_random_normal(uint64_t *rng) {
    double u1 = nk_random_uniform(rng);
    double u2 = nk_random_uniform(rng);
    if (u1 < 1e-15) u1 = 1e-15;
    double r = sqrt(-2.0 * log(u1));
    double theta = 2.0 * 3.14159265358979323846 * u2;
    return r * cos(theta);
}

/* ============================================================================
 * Genotype Lifecycle (L1: Core Definition)
 * ============================================================================ */

NKGenotype* nk_genotype_create(int N) {
    if (N <= 0) return NULL;
    NKGenotype *g = (NKGenotype*)calloc(1, sizeof(NKGenotype));
    if (!g) return NULL;
    g->N = N;
    int bytes = (N + 7) / 8;
    g->alleles = (uint8_t*)calloc(bytes, 1);
    if (!g->alleles) {
        free(g);
        return NULL;
    }
    return g;
}

NKGenotype* nk_genotype_clone(const NKGenotype *src) {
    if (!src) return NULL;
    NKGenotype *g = nk_genotype_create(src->N);
    if (!g) return NULL;
    int bytes = (src->N + 7) / 8;
    memcpy(g->alleles, src->alleles, bytes);
    return g;
}

void nk_genotype_free(NKGenotype *g) {
    if (!g) return;
    free(g->alleles);
    free(g);
}

void nk_genotype_set(NKGenotype *g, int i, int a) {
    if (!g || i < 0 || i >= g->N) return;
    int byte_idx = i / 8;
    int bit_idx = i % 8;
    if (a) {
        g->alleles[byte_idx] |= (1u << bit_idx);
    } else {
        g->alleles[byte_idx] &= ~(1u << bit_idx);
    }
}

int nk_genotype_get(const NKGenotype *g, int i) {
    if (!g || i < 0 || i >= g->N) return 0;
    int byte_idx = i / 8;
    int bit_idx = i % 8;
    return (g->alleles[byte_idx] >> bit_idx) & 1u;
}

void nk_genotype_flip(NKGenotype *g, int i) {
    if (!g || i < 0 || i >= g->N) return;
    int byte_idx = i / 8;
    int bit_idx = i % 8;
    g->alleles[byte_idx] ^= (1u << bit_idx);
}

bool nk_genotype_equal(const NKGenotype *a, const NKGenotype *b) {
    if (!a || !b) return (a == b);
    if (a->N != b->N) return false;
    int bytes = (a->N + 7) / 8;
    for (int i = 0; i < bytes; i++) {
        if (a->alleles[i] != b->alleles[i]) return false;
    }
    int leftover = a->N % 8;
    if (leftover > 0) {
        uint8_t mask = (1u << leftover) - 1;
        if ((a->alleles[bytes-1] & mask) != (b->alleles[bytes-1] & mask))
            return false;
    }
    return true;
}

int nk_genotype_hamming(const NKGenotype *a, const NKGenotype *b) {
    if (!a || !b) return -1;
    int dist = 0;
    int limit = (a->N < b->N) ? a->N : b->N;
    for (int i = 0; i < limit; i++) {
        if (nk_genotype_get(a, i) != nk_genotype_get(b, i)) dist++;
    }
    return dist;
}

void nk_genotype_randomize(NKGenotype *g, uint64_t *rng) {
    if (!g) return;
    for (int i = 0; i < g->N; i++) {
        nk_genotype_set(g, i, (nk_random_uniform(rng) < 0.5) ? 1 : 0);
    }
}

char* nk_genotype_to_string(const NKGenotype *g) {
    if (!g) return NULL;
    char *s = (char*)malloc(g->N + 1);
    if (!s) return NULL;
    for (int i = 0; i < g->N; i++) {
        s[i] = nk_genotype_get(g, i) ? '1' : '0';
    }
    s[g->N] = '\0';
    return s;
}

NKGenotype* nk_genotype_from_string(const char *s) {
    if (!s) return NULL;
    int N = (int)strlen(s);
    if (N == 0) return NULL;
    for (int i = 0; i < N; i++) {
        if (s[i] != '0' && s[i] != '1') return NULL;
    }
    NKGenotype *g = nk_genotype_create(N);
    if (!g) return NULL;
    for (int i = 0; i < N; i++) {
        nk_genotype_set(g, i, (s[i] == '1') ? 1 : 0);
    }
    return g;
}

/* ============================================================================
 * Epistatic Matrix Construction (L3: Mathematical Structure)
 *
 * Three topology types implement distinct biological interaction models:
 * - RANDOM: Kauffman original, each gene regulated by K random others
 * - ADJACENT: linear chromosome with local epistasis
 * - BLOCK: modular pleiotropy with independent functional modules
 * ============================================================================ */

NKEpistaticMatrix* nk_epistasis_create(int N, int K, NKInteractionType itype,
                                        uint64_t *rng) {
    if (N <= 0 || K < 0 || K >= N) return NULL;
    NKEpistaticMatrix *em = (NKEpistaticMatrix*)calloc(1, sizeof(NKEpistaticMatrix));
    if (!em) return NULL;
    em->N = N;
    em->K = K;
    em->itype = itype;
    em->neighbors = (int**)malloc(N * sizeof(int*));
    if (!em->neighbors) { free(em); return NULL; }
    for (int i = 0; i < N; i++) {
        em->neighbors[i] = (int*)malloc(K * sizeof(int));
        if (!em->neighbors[i]) {
            for (int j = 0; j < i; j++) free(em->neighbors[j]);
            free(em->neighbors); free(em); return NULL;
        }
    }

    switch (itype) {
    case NK_INTERACTION_RANDOM: {
        for (int i = 0; i < N; i++) {
            int count = 0;
            while (count < K) {
                int cand = (int)(nk_random_uniform(rng) * N);
                if (cand >= N) cand = N - 1;
                if (cand == i) continue;
                int dup = 0;
                for (int k = 0; k < count; k++) {
                    if (em->neighbors[i][k] == cand) { dup = 1; break; }
                }
                if (!dup) {
                    em->neighbors[i][count++] = cand;
                }
            }
        }
        break;
    }
    case NK_INTERACTION_ADJACENT: {
        for (int i = 0; i < N; i++) {
            for (int k = 0; k < K; k++) {
                em->neighbors[i][k] = (i + 1 + k) % N;
            }
        }
        break;
    }
    case NK_INTERACTION_BLOCK: {
        int block_size = K + 1;
        for (int i = 0; i < N; i++) {
            int block_start = (i / block_size) * block_size;
            int count = 0;
            for (int j = block_start; j < block_start + block_size && j < N; j++) {
                if (j != i && count < K) {
                    em->neighbors[i][count++] = j;
                }
            }
            while (count < K) {
                int wrap_j = (block_start + block_size + (count - (block_size - 1))) % N;
                if (wrap_j == i) wrap_j = (wrap_j + 1) % N;
                em->neighbors[i][count++] = wrap_j;
            }
        }
        break;
    }
    default:
        for (int i = 0; i < N; i++) {
            for (int k = 0; k < K; k++) {
                em->neighbors[i][k] = (i + k + 1) % N;
            }
        }
        break;
    }
    return em;
}

void nk_epistasis_free(NKEpistaticMatrix *em) {
    if (!em) return;
    if (em->neighbors) {
        for (int i = 0; i < em->N; i++) {
            free(em->neighbors[i]);
        }
        free(em->neighbors);
    }
    free(em);
}

int nk_epistasis_config(const NKEpistaticMatrix *em, int i,
                         const NKGenotype *g) {
    if (!em || !g || i < 0 || i >= em->N) return 0;
    int config = nk_genotype_get(g, i);
    for (int k = 0; k < em->K; k++) {
        int neighbor = em->neighbors[i][k];
        if (neighbor >= 0 && neighbor < g->N) {
            config |= (nk_genotype_get(g, neighbor) << (k + 1));
        }
    }
    return config;
}

/* ============================================================================
 * Fitness Table Construction (L3: Mathematical Structure)
 *
 * Each type of fitness distribution models different evolutionary regimes:
 * - UNIFORM: maximum entropy, Kauffman original specification
 * - NORMAL: additive genetic variance with epistatic perturbations
 * - EXPONENTIAL: rare high-fitness peaks (innovation landscapes)
 * - BIMODAL: two phenotype clusters (adaptive speciation)
 * - LOGNORMAL: multiplicative fitness effects
 * ============================================================================ */

NKFitnessTable* nk_fitness_table_create(int N, int K,
                                         NKContribDistribution dist,
                                         uint64_t *rng) {
    if (N <= 0 || K < 0 || K >= N) return NULL;
    NKFitnessTable *ft = (NKFitnessTable*)calloc(1, sizeof(NKFitnessTable));
    if (!ft) return NULL;
    ft->N = N;
    ft->K = K;
    ft->dist_type = dist;

    int n_configs = 1 << (K + 1);
    ft->f = (double**)malloc(N * sizeof(double*));
    if (!ft->f) { free(ft); return NULL; }
    for (int i = 0; i < N; i++) {
        ft->f[i] = (double*)malloc(n_configs * sizeof(double));
        if (!ft->f[i]) {
            for (int j = 0; j < i; j++) free(ft->f[j]);
            free(ft->f); free(ft); return NULL;
        }
    }

    for (int i = 0; i < N; i++) {
        for (int c = 0; c < n_configs; c++) {
            double val;
            switch (dist) {
            case NK_CONTRIB_UNIFORM:
                val = nk_random_uniform(rng);
                break;
            case NK_CONTRIB_NORMAL:
                val = 0.5 + 0.15 * nk_random_normal(rng);
                if (val < 0.0) val = 0.0;
                if (val > 1.0) val = 1.0;
                break;
            case NK_CONTRIB_EXPONENTIAL: {
                double u = nk_random_uniform(rng);
                if (u < 1e-15) u = 1e-15;
                val = -0.2 * log(u);
                if (val > 1.0) val = 1.0;
                break;
            }
            case NK_CONTRIB_BIMODAL:
                val = (nk_random_uniform(rng) < 0.5)
                      ? 0.2 + 0.1 * nk_random_normal(rng)
                      : 0.8 + 0.1 * nk_random_normal(rng);
                if (val < 0.0) val = 0.0;
                if (val > 1.0) val = 1.0;
                break;
            case NK_CONTRIB_LOGNORMAL: {
                double z = nk_random_normal(rng);
                val = exp(0.5 * z) / 7.389;
                if (val > 1.0) val = 1.0;
                break;
            }
            default:
                val = nk_random_uniform(rng);
                break;
            }
            ft->f[i][c] = val;
        }
    }
    return ft;
}

void nk_fitness_table_free(NKFitnessTable *ft) {
    if (!ft) return;
    if (ft->f) {
        for (int i = 0; i < ft->N; i++) {
            free(ft->f[i]);
        }
        free(ft->f);
    }
    free(ft);
}

double nk_fitness_table_get(const NKFitnessTable *ft, int i, int cfg) {
    if (!ft || i < 0 || i >= ft->N) return 0.0;
    int n_configs = 1 << (ft->K + 1);
    if (cfg < 0 || cfg >= n_configs) return 0.0;
    return ft->f[i][cfg];
}

/* ============================================================================
 * Landscape Lifecycle (L2: Core Concept)
 * ============================================================================ */

NKLandscape* nk_landscape_create(int N, int K,
                                  NKInteractionType itype,
                                  NKContribDistribution dist,
                                  uint64_t seed) {
    if (N <= 0 || K < 0 || K >= N) return NULL;
    NKLandscape *land = (NKLandscape*)calloc(1, sizeof(NKLandscape));
    if (!land) return NULL;

    land->N = N;
    land->K = K;
    land->seed = seed;

    uint64_t rng = seed;
    if (rng == 0) rng = 123456789ULL;

    land->epistasis = nk_epistasis_create(N, K, itype, &rng);
    if (!land->epistasis) { nk_landscape_free(land); return NULL; }

    land->fitness = nk_fitness_table_create(N, K, dist, &rng);
    if (!land->fitness) { nk_landscape_free(land); return NULL; }

    char buf[64];
    snprintf(buf, sizeof(buf), "NK_N%d_K%d", N, K);
    land->name = strdup(buf);

    return land;
}

void nk_landscape_free(NKLandscape *land) {
    if (!land) return;
    nk_epistasis_free(land->epistasis);
    nk_fitness_table_free(land->fitness);
    free(land->name);
    free(land);
}

/* ============================================================================
 * Fitness Evaluation (L5: Algorithm)
 *
 * F(g) = (1/N) * sum_{i=0}^{N-1} f_i(config_i(g))
 * where config_i(g) encodes the alleles of locus i and its K neighbors.
 *
 * Complexity: O(N*K). Linear in N and K.
 * Theorem (Kauffman 1993): For fixed K, fitness is computable in O(N).
 * ============================================================================ */

double nk_fitness(const NKLandscape *land, const NKGenotype *g) {
    if (!land || !g) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < land->N; i++) {
        int cfg = nk_epistasis_config(land->epistasis, i, g);
        sum += nk_fitness_table_get(land->fitness, i, cfg);
    }
    return sum / (double)land->N;
}

double nk_locus_contribution(const NKLandscape *land, int i,
                              const NKGenotype *g) {
    if (!land || !g || i < 0 || i >= land->N) return 0.0;
    int cfg = nk_epistasis_config(land->epistasis, i, g);
    return nk_fitness_table_get(land->fitness, i, cfg);
}

void nk_neighbor_fitnesses(const NKLandscape *land, const NKGenotype *g,
                            double *fitness_out) {
    if (!land || !g || !fitness_out) return;
    for (int i = 0; i < land->N; i++) {
        NKGenotype *ng = nk_genotype_clone(g);
        nk_genotype_flip(ng, i);
        fitness_out[i] = nk_fitness(land, ng);
        nk_genotype_free(ng);
    }
}

/* ============================================================================
 * Local Optimum Detection (L4: Fundamental Law)
 *
 * A genotype g is a local optimum iff for all i in [0,N-1]:
 *   fitness(flip(g, i)) <= fitness(g)
 * This defines the adaptive walk termination condition.
 *
 * Theorem (Kauffman & Levin 1987): For K=N-1, expected number of
 * local optima = 2^N / (N+1). For K=0, exactly 1 global optimum.
 * ============================================================================ */

bool nk_is_local_optimum(const NKLandscape *land, const NKGenotype *g) {
    if (!land || !g) return false;
    double f0 = nk_fitness(land, g);
    for (int i = 0; i < land->N; i++) {
        NKGenotype *ng = nk_genotype_clone(g);
        nk_genotype_flip(ng, i);
        double fi = nk_fitness(land, ng);
        nk_genotype_free(ng);
        if (fi > f0) return false;
    }
    return true;
}

void nk_best_neighbor(const NKLandscape *land, const NKGenotype *g,
                       int *best_i, double *best_fitness) {
    if (!land || !g || !best_i || !best_fitness) return;
    *best_i = -1;
    *best_fitness = nk_fitness(land, g);
    for (int i = 0; i < land->N; i++) {
        NKGenotype *ng = nk_genotype_clone(g);
        nk_genotype_flip(ng, i);
        double fi = nk_fitness(land, ng);
        nk_genotype_free(ng);
        if (fi > *best_fitness) {
            *best_fitness = fi;
            *best_i = i;
        }
    }
}

void nk_first_improving_neighbor(const NKLandscape *land,
                                  const NKGenotype *g,
                                  int *first_i, double *new_fitness) {
    if (!land || !g || !first_i || !new_fitness) return;
    *first_i = -1;
    double f0 = nk_fitness(land, g);
    *new_fitness = f0;
    for (int i = 0; i < land->N; i++) {
        NKGenotype *ng = nk_genotype_clone(g);
        nk_genotype_flip(ng, i);
        double fi = nk_fitness(land, ng);
        nk_genotype_free(ng);
        if (fi > f0) {
            *first_i = i;
            *new_fitness = fi;
            return;
        }
    }
}

/* ============================================================================
 * Landscape Phase Classification (L2: Core Concept)
 *
 * Kauffman phase diagram for NK landscapes:
 *   Ordered phase (K/N < 0.2): highly correlated, single peak dominates
 *   Critical phase (0.2 <= K/N <= 0.5): edge of chaos, optimal evolvability
 *   Chaotic phase (K/N > 0.5): rugged, uncorrelated, many local optima
 *
 * The critical regime at K approx 2-3 (for moderate N) is where
 * complex adaptations are most likely to emerge (Kauffman 1993, Ch. 5).
 * ============================================================================ */

NKPhaseClassification nk_landscape_phase(const NKLandscape *land) {
    if (!land || land->N == 0) return NK_PHASE_ORDERED;
    double ratio = (double)land->K / (double)land->N;
    if (ratio < 0.2) return NK_PHASE_ORDERED;
    if (ratio <= 0.5) return NK_PHASE_CRITICAL;
    return NK_PHASE_CHAOTIC;
}

/* ============================================================================
 * Ruggedness Estimation (L4: Fundamental Law)
 *
 * Ruggedness = 1 - rho(1), where rho(1) is the fitness autocorrelation
 * at Hamming distance 1. Measures how rapidly fitness decorrelates
 * under single-locus mutations.
 *
 * Weinberger (1990) derived analytically: rho(d) approx (1 - d/N)^K.
 * So rho(1) approx (1 - 1/N)^K. As K increases, rho(1) decreases,
 * confirming that ruggedness increases with epistasis.
 * ============================================================================ */

double nk_ruggedness_estimate(const NKLandscape *land, int samples) {
    if (!land || samples <= 0) return 0.0;

    double sum_f = 0.0, sum_f2 = 0.0, sum_ff = 0.0;
    uint64_t rng = land->seed + 9999;

    for (int s = 0; s < samples; s++) {
        NKGenotype *g = nk_genotype_create(land->N);
        nk_genotype_randomize(g, &rng);

        double f = nk_fitness(land, g);

        int flip_i = (int)(nk_random_uniform(&rng) * land->N);
        if (flip_i >= land->N) flip_i = land->N - 1;
        nk_genotype_flip(g, flip_i);
        double fn = nk_fitness(land, g);

        sum_f += f;
        sum_f2 += f * f;
        sum_ff += f * fn;

        nk_genotype_free(g);
    }

    double mean_f = sum_f / samples;
    double var_f = (sum_f2 / samples) - (mean_f * mean_f);
    if (var_f <= 0.0) return 1.0;

    double cov = (sum_ff / samples) - (mean_f * mean_f);
    double rho = cov / var_f;

    double ruggedness = 1.0 - rho;
    if (ruggedness < 0.0) ruggedness = 0.0;
    if (ruggedness > 1.0) ruggedness = 1.0;
    return ruggedness;
}
