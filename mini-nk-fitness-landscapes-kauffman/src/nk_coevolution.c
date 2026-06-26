#include "nk_core.h"
#include "nk_coevolution.h"
#include "nk_landscape.h"
#include "nk_walk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * NKCS Coevolution Model (L7: Application / L8: Advanced)
 *
 * Implements Kauffman & Johnsen (1991) NKCS (NK Coupled Systems) model.
 * Two species coevolve on coupled fitness landscapes.
 * Each species' fitness depends on its own N loci and C loci of the
 * other species, creating coevolutionary dynamics.
 *
 * This is Kauffman's extension of the NK model to ecological communities.
 * The coupling parameter C controls the intensity of cross-species
 * epistasis, analogous to K controlling within-species epistasis.
 *
 * Key phenomena:
 *   - Red Queen dynamics: species must constantly adapt just to stay in place
 *   - Coevolutionary avalanches: cascades of mutual adaptation
 *   - Nash equilibria: stable coevolutionary states
 *   - Coupling catastrophe: excessive coupling reduces equilibrium fitness
 *
 * References:
 *   Kauffman & Johnsen (1991). J. Theor. Biol., 149(4), 467-505.
 *   Van Valen (1973). "A new evolutionary law." Evol. Theory, 1, 1-30.
 * ============================================================================ */

/* Forward declaration of helper from nk_walk.c */
extern bool walk_append_state(NKWalkResult *wr, const NKGenotype *g,
                               double fitness, int step);

/* ============================================================================
 * NKCS Landscape Lifecycle
 * ============================================================================ */

NKCSLandscape* nkcs_create(int N, int K, int C,
                            double coupling_strength,
                            uint64_t seed) {
    if (N <= 0 || K < 0 || K >= N || C < 0) return NULL;

    NKCSLandscape *nkcs = (NKCSLandscape*)calloc(1, sizeof(NKCSLandscape));
    if (!nkcs) return NULL;

    nkcs->C = C;
    nkcs->coupling_strength = coupling_strength;
    if (nkcs->coupling_strength < 0.0) nkcs->coupling_strength = 0.0;
    if (nkcs->coupling_strength > 1.0) nkcs->coupling_strength = 1.0;

    /* Create two independent base landscapes */
    nkcs->species_a = nk_landscape_create(N, K, NK_INTERACTION_RANDOM,
                                           NK_CONTRIB_UNIFORM, seed);
    nkcs->species_b = nk_landscape_create(N, K, NK_INTERACTION_RANDOM,
                                           NK_CONTRIB_UNIFORM, seed + 1000000);
    if (!nkcs->species_a || !nkcs->species_b) {
        nkcs_free(nkcs);
        return NULL;
    }

    /* Build cross-species coupling matrices */
    uint64_t rng = seed + 5000000;
    if (C > 0) {
        nkcs->coupling_a_to_b = (int**)malloc(N * sizeof(int*));
        nkcs->coupling_b_to_a = (int**)malloc(N * sizeof(int*));
        if (!nkcs->coupling_a_to_b || !nkcs->coupling_b_to_a) {
            nkcs_free(nkcs); return NULL;
        }
        for (int i = 0; i < N; i++) {
            nkcs->coupling_a_to_b[i] = (int*)malloc(C * sizeof(int));
            nkcs->coupling_b_to_a[i] = (int*)malloc(C * sizeof(int));
            if (!nkcs->coupling_a_to_b[i] || !nkcs->coupling_b_to_a[i]) {
                nkcs_free(nkcs); return NULL;
            }
            /* Random coupling: each locus i in A connects to C random loci in B */
            for (int c = 0; c < C; c++) {
                nkcs->coupling_a_to_b[i][c] = (int)(nk_random_uniform(&rng) * N);
                if (nkcs->coupling_a_to_b[i][c] >= N)
                    nkcs->coupling_a_to_b[i][c] = N - 1;
                nkcs->coupling_b_to_a[i][c] = (int)(nk_random_uniform(&rng) * N);
                if (nkcs->coupling_b_to_a[i][c] >= N)
                    nkcs->coupling_b_to_a[i][c] = N - 1;
            }
        }

        /* Create coupled fitness tables with extended configurations.
         * Configuration bits: [0] = self allele, [1..K] = K internal neighbors,
         * [K+1..K+C] = C cross-species coupling loci. */
        int total_K = K + C;
        int n_configs = 1 << (total_K + 1);

        nkcs->fitness_a = (NKFitnessTable*)calloc(1, sizeof(NKFitnessTable));
        nkcs->fitness_b = (NKFitnessTable*)calloc(1, sizeof(NKFitnessTable));
        if (!nkcs->fitness_a || !nkcs->fitness_b) { nkcs_free(nkcs); return NULL; }

        nkcs->fitness_a->N = N; nkcs->fitness_a->K = total_K;
        nkcs->fitness_b->N = N; nkcs->fitness_b->K = total_K;

        nkcs->fitness_a->f = (double**)malloc(N * sizeof(double*));
        nkcs->fitness_b->f = (double**)malloc(N * sizeof(double*));

        for (int i = 0; i < N; i++) {
            nkcs->fitness_a->f[i] = (double*)malloc(n_configs * sizeof(double));
            nkcs->fitness_b->f[i] = (double*)malloc(n_configs * sizeof(double));
            for (int cfg = 0; cfg < n_configs; cfg++) {
                /* Coupled fitness = (1-alpha)*uncoupled + alpha*coupled */
                double base_a = nk_fitness_table_get(nkcs->species_a->fitness, i, cfg & ((1 << (K+1))-1));
                double base_b = nk_fitness_table_get(nkcs->species_b->fitness, i, cfg & ((1 << (K+1))-1));
                double coupled_a = nk_random_uniform(&rng);
                double coupled_b = nk_random_uniform(&rng);
                nkcs->fitness_a->f[i][cfg] = (1.0 - coupling_strength) * base_a + coupling_strength * coupled_a;
                nkcs->fitness_b->f[i][cfg] = (1.0 - coupling_strength) * base_b + coupling_strength * coupled_b;
            }
        }
    }

    return nkcs;
}

void nkcs_free(NKCSLandscape *nkcs) {
    if (!nkcs) return;

    /* Save N before freeing landscapes (avoid use-after-free) */
    int Na = nkcs->species_a ? nkcs->species_a->N : 0;
    int Nb = nkcs->species_b ? nkcs->species_b->N : 0;

    nk_landscape_free(nkcs->species_a);
    nk_landscape_free(nkcs->species_b);

    if (nkcs->coupling_a_to_b) {
        for (int i = 0; i < Na; i++)
            free(nkcs->coupling_a_to_b[i]);
        free(nkcs->coupling_a_to_b);
    }
    if (nkcs->coupling_b_to_a) {
        for (int i = 0; i < Nb; i++)
            free(nkcs->coupling_b_to_a[i]);
        free(nkcs->coupling_b_to_a);
    }

    if (nkcs->fitness_a) {
        if (nkcs->fitness_a->f) {
            for (int i = 0; i < nkcs->fitness_a->N; i++)
                free(nkcs->fitness_a->f[i]);
            free(nkcs->fitness_a->f);
        }
        free(nkcs->fitness_a);
    }
    if (nkcs->fitness_b) {
        if (nkcs->fitness_b->f) {
            for (int i = 0; i < nkcs->fitness_b->N; i++)
                free(nkcs->fitness_b->f[i]);
            free(nkcs->fitness_b->f);
        }
        free(nkcs->fitness_b);
    }

    free(nkcs);
}

/* ============================================================================
 * Coupled Fitness Evaluation
 *
 * Builds the extended configuration for locus i including:
 *   bit 0: allele of locus i in self
 *   bits 1..K: alleles of K internal neighbors in self
 *   bits K+1..K+C: alleles of C cross-species coupling partners
 * ============================================================================ */

static int nkcs_config_a(const NKCSLandscape *nkcs, int i,
                          const NKGenotype *ga, const NKGenotype *gb) {
    int cfg = nk_genotype_get(ga, i);  /* bit 0 */
    /* Internal neighbors (bits 1..K) */
    NKEpistaticMatrix *em = nkcs->species_a->epistasis;
    for (int k = 0; k < em->K; k++) {
        int nb = em->neighbors[i][k];
        cfg |= (nk_genotype_get(ga, nb) << (k + 1));
    }
    /* Cross-species coupling (bits K+1..K+C) */
    for (int c = 0; c < nkcs->C; c++) {
        int nb = nkcs->coupling_a_to_b[i][c];
        cfg |= (nk_genotype_get(gb, nb) << (em->K + c + 1));
    }
    return cfg;
}

static int nkcs_config_b(const NKCSLandscape *nkcs, int i,
                          const NKGenotype *ga, const NKGenotype *gb) {
    int cfg = nk_genotype_get(gb, i);
    NKEpistaticMatrix *em = nkcs->species_b->epistasis;
    for (int k = 0; k < em->K; k++) {
        int nb = em->neighbors[i][k];
        cfg |= (nk_genotype_get(gb, nb) << (k + 1));
    }
    for (int c = 0; c < nkcs->C; c++) {
        int nb = nkcs->coupling_b_to_a[i][c];
        cfg |= (nk_genotype_get(ga, nb) << (em->K + c + 1));
    }
    return cfg;
}

double nkcs_fitness_a(const NKCSLandscape *nkcs,
                       const NKGenotype *ga,
                       const NKGenotype *gb) {
    if (!nkcs || !ga || !gb) return 0.0;
    if (nkcs->C == 0) {
        return nk_fitness(nkcs->species_a, ga);
    }
    double sum = 0.0;
    for (int i = 0; i < nkcs->fitness_a->N; i++) {
        int cfg = nkcs_config_a(nkcs, i, ga, gb);
        sum += nk_fitness_table_get(nkcs->fitness_a, i, cfg);
    }
    return sum / nkcs->fitness_a->N;
}

double nkcs_fitness_b(const NKCSLandscape *nkcs,
                       const NKGenotype *ga,
                       const NKGenotype *gb) {
    if (!nkcs || !ga || !gb) return 0.0;
    if (nkcs->C == 0) {
        return nk_fitness(nkcs->species_b, gb);
    }
    double sum = 0.0;
    for (int i = 0; i < nkcs->fitness_b->N; i++) {
        int cfg = nkcs_config_b(nkcs, i, ga, gb);
        sum += nk_fitness_table_get(nkcs->fitness_b, i, cfg);
    }
    return sum / nkcs->fitness_b->N;
}

/* ============================================================================
 * Coupled Adaptive Walk (L8: Advanced)
 *
 * Implements alternating-species adaptive walk:
 * Species A makes one greedy step while B is fixed,
 * then B makes one greedy step while A is fixed.
 * This iterates until both are at mutual local optima
 * (Nash equilibrium) or max_steps is reached.
 * ============================================================================ */

NKWalkResult* nkcs_coupled_walk_a(const NKCSLandscape *nkcs,
                                   const NKGenotype *start_a,
                                   const NKGenotype *start_b,
                                   int max_steps) {
    if (!nkcs || !start_a || !start_b || max_steps <= 0) return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_GREEDY;

    NKGenotype *ga = nk_genotype_clone(start_a);
    NKGenotype *gb = nk_genotype_clone(start_b);
    double fa = nkcs_fitness_a(nkcs, ga, gb);
    walk_append_state(wr, ga, fa, 0);

    for (int step = 1; step <= max_steps; step++) {
        /* Species A moves (greedy, B fixed) */
        int found_i = -1;
        double best_fa = fa;
        int N = nkcs->species_a ? nkcs->species_a->N : 0;

        for (int i = 0; i < N; i++) {
            NKGenotype *cand = nk_genotype_clone(ga);
            nk_genotype_flip(cand, i);
            double f_cand = nkcs_fitness_a(nkcs, cand, gb);
            if (f_cand > best_fa) {
                best_fa = f_cand;
                found_i = i;
            }
            nk_genotype_free(cand);
        }

        if (found_i >= 0) {
            nk_genotype_flip(ga, found_i);
            fa = best_fa;
        }

        walk_append_state(wr, ga, fa, step);

        /* Check if A is at a local optimum (given fixed B) */
        if (found_i < 0) {
            wr->reached_optimum = true;
            break;
        }
    }

    nk_genotype_free(ga);
    nk_genotype_free(gb);
    return wr;
}

NKWalkResult* nkcs_coupled_walk_b(const NKCSLandscape *nkcs,
                                   const NKGenotype *start_a,
                                   const NKGenotype *start_b,
                                   int max_steps) {
    if (!nkcs || !start_a || !start_b || max_steps <= 0) return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_GREEDY;

    NKGenotype *ga = nk_genotype_clone(start_a);
    NKGenotype *gb = nk_genotype_clone(start_b);
    double fb = nkcs_fitness_b(nkcs, ga, gb);
    walk_append_state(wr, gb, fb, 0);

    for (int step = 1; step <= max_steps; step++) {
        int found_j = -1;
        double best_fb = fb;
        int N = nkcs->species_b ? nkcs->species_b->N : 0;

        for (int j = 0; j < N; j++) {
            NKGenotype *cand = nk_genotype_clone(gb);
            nk_genotype_flip(cand, j);
            double f_cand = nkcs_fitness_b(nkcs, ga, cand);
            if (f_cand > best_fb) {
                best_fb = f_cand;
                found_j = j;
            }
            nk_genotype_free(cand);
        }

        if (found_j >= 0) {
            nk_genotype_flip(gb, found_j);
            fb = best_fb;
        }

        walk_append_state(wr, gb, fb, step);

        if (found_j < 0) {
            wr->reached_optimum = true;
            break;
        }
    }

    nk_genotype_free(ga);
    nk_genotype_free(gb);
    return wr;
}

/* ============================================================================
 * Red Queen Dynamics (L7: Application)
 *
 * Van Valen's (1973) Red Queen hypothesis: species must constantly
 * adapt merely to survive in a coevolving ecosystem.
 *
 * Red Queen measure: the fraction of adaptive steps that merely
 * compensate for the partner's adaptation rather than achieving
 * absolute fitness gains. In a coevolutionary arms race, each
 * improvement by one species effectively degrades the other's
 * fitness, creating a "running to stay in place" dynamic.
 * ============================================================================ */

double nkcs_red_queen_measure(const NKCSLandscape *nkcs,
                               int n_steps,
                               uint64_t *rng) {
    if (!nkcs || n_steps <= 0 || !rng) return 0.0;

    int N = nkcs->species_a->N;
    NKGenotype *ga = nk_genotype_create(N);
    NKGenotype *gb = nk_genotype_create(N);
    nk_genotype_randomize(ga, rng);
    nk_genotype_randomize(gb, rng);

    double fa_initial = nkcs_fitness_a(nkcs, ga, gb);
    double fb_initial = nkcs_fitness_b(nkcs, ga, gb);
    int compensatory = 0;
    int total_steps = 0;

    for (int t = 0; t < n_steps; t++) {
        double fa_before = nkcs_fitness_a(nkcs, ga, gb);

        /* Species B takes a greedy step */
        int best_j = -1;
        double best_fb = nkcs_fitness_b(nkcs, ga, gb);
        for (int j = 0; j < N; j++) {
            NKGenotype *cand = nk_genotype_clone(gb);
            nk_genotype_flip(cand, j);
            double fb_cand = nkcs_fitness_b(nkcs, ga, cand);
            if (fb_cand > best_fb) { best_fb = fb_cand; best_j = j; }
            nk_genotype_free(cand);
        }
        if (best_j >= 0) {
            nk_genotype_flip(gb, best_j);
            total_steps++;

            double fa_after = nkcs_fitness_a(nkcs, ga, gb);
            /* If A's fitness decreased, B's step was "Red Queen" for A */
            if (fa_after < fa_before) compensatory++;
        }
    }

    nk_genotype_free(ga);
    nk_genotype_free(gb);

    /* Silence unused variable warning */
    (void)fa_initial; (void)fb_initial;

    if (total_steps == 0) return 0.0;
    return (double)compensatory / total_steps;
}

void nkcs_fitness_trajectory(const NKCSLandscape *nkcs,
                              int n_steps,
                              double *fitness_a,
                              double *fitness_b,
                              uint64_t *rng) {
    if (!nkcs || !fitness_a || !fitness_b || n_steps <= 0 || !rng) return;

    int N = nkcs->species_a->N;
    NKGenotype *ga = nk_genotype_create(N);
    NKGenotype *gb = nk_genotype_create(N);
    nk_genotype_randomize(ga, rng);
    nk_genotype_randomize(gb, rng);

    fitness_a[0] = nkcs_fitness_a(nkcs, ga, gb);
    fitness_b[0] = nkcs_fitness_b(nkcs, ga, gb);

    for (int t = 1; t <= n_steps; t++) {
        /* Alternate: even steps = A moves, odd steps = B moves */
        if (t % 2 == 0) {
            /* Species A moves */
            int best_i = -1;
            double best_fa = nkcs_fitness_a(nkcs, ga, gb);
            for (int i = 0; i < N; i++) {
                NKGenotype *cand = nk_genotype_clone(ga);
                nk_genotype_flip(cand, i);
                double f_cand = nkcs_fitness_a(nkcs, cand, gb);
                if (f_cand > best_fa) { best_fa = f_cand; best_i = i; }
                nk_genotype_free(cand);
            }
            if (best_i >= 0) nk_genotype_flip(ga, best_i);
        } else {
            /* Species B moves */
            int best_j = -1;
            double best_fb = nkcs_fitness_b(nkcs, ga, gb);
            for (int j = 0; j < N; j++) {
                NKGenotype *cand = nk_genotype_clone(gb);
                nk_genotype_flip(cand, j);
                double f_cand = nkcs_fitness_b(nkcs, ga, cand);
                if (f_cand > best_fb) { best_fb = f_cand; best_j = j; }
                nk_genotype_free(cand);
            }
            if (best_j >= 0) nk_genotype_flip(gb, best_j);
        }
        fitness_a[t] = nkcs_fitness_a(nkcs, ga, gb);
        fitness_b[t] = nkcs_fitness_b(nkcs, ga, gb);
    }

    nk_genotype_free(ga);
    nk_genotype_free(gb);
}

/* ============================================================================
 * Nash Equilibrium Analysis (L8: Advanced)
 * ============================================================================ */

bool nkcs_is_nash_equilibrium(const NKCSLandscape *nkcs,
                               const NKGenotype *ga,
                               const NKGenotype *gb) {
    if (!nkcs || !ga || !gb) return false;

    double fa = nkcs_fitness_a(nkcs, ga, gb);
    double fb = nkcs_fitness_b(nkcs, ga, gb);
    int N = nkcs->species_a->N;

    /* Check if A can improve unilaterally */
    for (int i = 0; i < N; i++) {
        NKGenotype *cand = nk_genotype_clone(ga);
        nk_genotype_flip(cand, i);
        double f_cand = nkcs_fitness_a(nkcs, cand, gb);
        nk_genotype_free(cand);
        if (f_cand > fa) return false;
    }

    /* Check if B can improve unilaterally */
    for (int j = 0; j < N; j++) {
        NKGenotype *cand = nk_genotype_clone(gb);
        nk_genotype_flip(cand, j);
        double f_cand = nkcs_fitness_b(nkcs, ga, cand);
        nk_genotype_free(cand);
        if (f_cand > fb) return false;
    }

    return true;
}

int nkcs_find_nash_equilibrium(const NKCSLandscape *nkcs,
                                NKGenotype *ga_out,
                                NKGenotype *gb_out,
                                int max_iters,
                                uint64_t *rng) {
    if (!nkcs || !ga_out || !gb_out || max_iters <= 0 || !rng) return -1;

    int N = nkcs->species_a->N;
    nk_genotype_randomize(ga_out, rng);
    nk_genotype_randomize(gb_out, rng);

    for (int iter = 1; iter <= max_iters; iter++) {
        /* A's best response to B */
        double best_fa = nkcs_fitness_a(nkcs, ga_out, gb_out);
        int best_i = -1;
        for (int i = 0; i < N; i++) {
            NKGenotype *cand = nk_genotype_clone(ga_out);
            nk_genotype_flip(cand, i);
            double f_cand = nkcs_fitness_a(nkcs, cand, gb_out);
            if (f_cand > best_fa) { best_fa = f_cand; best_i = i; }
            nk_genotype_free(cand);
        }
        if (best_i >= 0) nk_genotype_flip(ga_out, best_i);

        /* B's best response to A */
        double best_fb = nkcs_fitness_b(nkcs, ga_out, gb_out);
        int best_j = -1;
        for (int j = 0; j < N; j++) {
            NKGenotype *cand = nk_genotype_clone(gb_out);
            nk_genotype_flip(cand, j);
            double f_cand = nkcs_fitness_b(nkcs, ga_out, cand);
            if (f_cand > best_fb) { best_fb = f_cand; best_j = j; }
            nk_genotype_free(cand);
        }
        if (best_j >= 0) nk_genotype_flip(gb_out, best_j);

        /* If neither improved, we've converged */
        if (best_i < 0 && best_j < 0) return iter;
    }

    return max_iters;  /* Did not converge within limit */
}

/* ============================================================================
 * Coupling Effects Analysis (L8: Advanced)
 * ============================================================================ */

double nkcs_mutual_information(const NKCSLandscape *nkcs,
                                int n_samples,
                                uint64_t *rng) {
    if (!nkcs || n_samples <= 0 || !rng) return 0.0;

    /* Estimate MI by computing correlation between fitness changes */
    int N = nkcs->species_a->N;
    double sum_fa = 0.0, sum_fb = 0.0;
    double sum_fafb = 0.0, sum_fa2 = 0.0, sum_fb2 = 0.0;

    for (int s = 0; s < n_samples; s++) {
        NKGenotype *ga = nk_genotype_create(N);
        NKGenotype *gb = nk_genotype_create(N);
        nk_genotype_randomize(ga, rng);
        nk_genotype_randomize(gb, rng);

        double fa = nkcs_fitness_a(nkcs, ga, gb);
        double fb = nkcs_fitness_b(nkcs, ga, gb);

        sum_fa += fa;
        sum_fb += fb;
        sum_fafb += fa * fb;
        sum_fa2 += fa * fa;
        sum_fb2 += fb * fb;

        nk_genotype_free(ga);
        nk_genotype_free(gb);
    }

    double n = (double)n_samples;
    double cov = (sum_fafb / n) - (sum_fa / n) * (sum_fb / n);
    double var_a = (sum_fa2 / n) - (sum_fa / n) * (sum_fa / n);
    double var_b = (sum_fb2 / n) - (sum_fb / n) * (sum_fb / n);

    if (var_a <= 0.0 || var_b <= 0.0) return 0.0;
    double corr = cov / sqrt(var_a * var_b);
    if (corr < 0.0) corr = 0.0;
    if (corr > 1.0) corr = 1.0;

    /* Approximate MI from correlation: MI = -0.5 * log(1 - rho^2) */
    double r2 = corr * corr;
    if (r2 >= 1.0) r2 = 0.999999;
    return -0.5 * log(1.0 - r2);
}

void nkcs_avalanche_distribution(const NKCSLandscape *nkcs,
                                  int n_events,
                                  int *sizes,
                                  uint64_t *rng) {
    if (!nkcs || !sizes || n_events <= 0 || !rng) return;

    int N = nkcs->species_a->N;
    for (int e = 0; e < n_events; e++) {
        NKGenotype *ga = nk_genotype_create(N);
        NKGenotype *gb = nk_genotype_create(N);
        nk_genotype_randomize(ga, rng);
        nk_genotype_randomize(gb, rng);

        int avalanche = 0;
        bool changed = true;
        while (changed && avalanche < 100) {
            changed = false;
            /* A moves */
            for (int i = 0; i < N; i++) {
                NKGenotype *cand = nk_genotype_clone(ga);
                nk_genotype_flip(cand, i);
                double f_cand = nkcs_fitness_a(nkcs, cand, gb);
                double f_cur = nkcs_fitness_a(nkcs, ga, gb);
                if (f_cand > f_cur) {
                    nk_genotype_free(ga);
                    ga = cand;
                    avalanche++;
                    changed = true;
                } else {
                    nk_genotype_free(cand);
                }
            }
            /* B moves */
            for (int j = 0; j < N; j++) {
                NKGenotype *cand = nk_genotype_clone(gb);
                nk_genotype_flip(cand, j);
                double f_cand = nkcs_fitness_b(nkcs, ga, cand);
                double f_cur = nkcs_fitness_b(nkcs, ga, gb);
                if (f_cand > f_cur) {
                    nk_genotype_free(gb);
                    gb = cand;
                    avalanche++;
                    changed = true;
                } else {
                    nk_genotype_free(cand);
                }
            }
        }
        sizes[e] = avalanche;
        nk_genotype_free(ga);
        nk_genotype_free(gb);
    }
}

void nkcs_coupling_catastrophe(int N, int K, int max_C,
                                int n_trials,
                                double *coupling,
                                double *fit,
                                uint64_t seed) {
    if (max_C < 0 || N <= 0 || !coupling || !fit) return;

    for (int c = 0; c <= max_C; c++) {
        coupling[c] = (double)c / (double)N;

        double sum_fit = 0.0;
        for (int t = 0; t < n_trials; t++) {
            NKCSLandscape *nkcs = nkcs_create(N, K, c, 0.5, seed + t * 100000);
            if (!nkcs) continue;

            uint64_t rng = seed + t;
            NKGenotype *ga = nk_genotype_create(N);
            NKGenotype *gb = nk_genotype_create(N);
            nk_genotype_randomize(ga, &rng);
            nk_genotype_randomize(gb, &rng);

            nkcs_find_nash_equilibrium(nkcs, ga, gb, 100, &rng);
            double fa = nkcs_fitness_a(nkcs, ga, gb);
            double fb = nkcs_fitness_b(nkcs, ga, gb);
            sum_fit += (fa + fb) / 2.0;

            nk_genotype_free(ga);
            nk_genotype_free(gb);
            nkcs_free(nkcs);
        }
        fit[c] = (n_trials > 0) ? (sum_fit / n_trials) : 0.0;
    }
}
