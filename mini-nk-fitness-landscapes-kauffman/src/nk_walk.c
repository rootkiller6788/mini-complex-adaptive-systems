#include "nk_core.h"
#include "nk_walk.h"
#include "nk_landscape.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Adaptive Walks on NK Fitness Landscapes (L5: Algorithms)
 *
 * Implements six adaptive walk strategies on rugged fitness landscapes.
 * Each represents a distinct model of evolutionary dynamics drawn from
 * the theoretical biology literature.
 *
 * References:
 *   Kauffman & Levin (1987): greedy adaptive walk on NK landscapes
 *   Wright (1932): shifting balance and steepest ascent
 *   Gillespie (1984): molecular evolution by adaptive walks
 *   Metropolis et al. (1953): Monte Carlo importance sampling
 *   Kirkpatrick et al. (1983): simulated annealing optimization
 *   Orr (2002): adaptation theory, extreme value distributions
 * ============================================================================ */

/* RNG from nk_core.c, declared in nk_core.h */

/* ============================================================================
 * Walk Result Lifecycle
 * ============================================================================ */

NKWalkResult* nk_walk_result_create(int max_steps) {
    if (max_steps <= 0) return NULL;
    NKWalkResult *wr = (NKWalkResult*)calloc(1, sizeof(NKWalkResult));
    if (!wr) return NULL;
    wr->steps = 0;
    wr->reached_optimum = false;
    wr->path = (NKWalkState**)calloc(max_steps + 1, sizeof(NKWalkState*));
    if (!wr->path) {
        free(wr);
        return NULL;
    }
    return wr;
}

void nk_walk_result_free(NKWalkResult *wr) {
    if (!wr) return;
    if (wr->path) {
        for (int i = 0; i <= wr->steps && wr->path[i]; i++) {
            nk_genotype_free(wr->path[i]->genotype);
            free(wr->path[i]);
        }
        free(wr->path);
    }
    free(wr);
}

/* Helper: append a state to a walk result. Public for use by coevolution module. */
bool walk_append_state(NKWalkResult *wr, const NKGenotype *g,
                               double fitness, int step) {
    if (!wr || !g) return false;
    NKWalkState *state = (NKWalkState*)calloc(1, sizeof(NKWalkState));
    if (!state) return false;
    state->genotype = nk_genotype_clone(g);
    state->fitness = fitness;
    state->step = step;
    wr->path[step] = state;
    wr->steps = step;
    return true;
}

/* ============================================================================
 * Greedy Adaptive Walk (L5: Algorithm)
 *
 * The original adaptive walk of Kauffman & Levin (1987).
 * At each step, scan loci from 0 to N-1 and flip the first locus
 * that improves fitness. Terminates at a local optimum.
 *
 * Properties:
 *   - Average walk length approx ln(N) for K >= 2
 *   - O(N*K) per step in expectation
 *   - Discovers local optima efficiently but may miss higher peaks
 *
 * Independent knowledge point: first-improvement hill-climbing with
 * systematic scan order. Contrasts with stochastic local search.
 * ============================================================================ */

NKWalkResult* nk_walk_greedy(const NKLandscape *land,
                              const NKGenotype *start,
                              int max_steps) {
    if (!land || !start || max_steps <= 0) return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_GREEDY;

    NKGenotype *current = nk_genotype_clone(start);
    double current_f = nk_fitness(land, current);

    walk_append_state(wr, current, current_f, 0);

    for (int step = 1; step <= max_steps; step++) {
        int first_i;
        double new_f;
        nk_first_improving_neighbor(land, current, &first_i, &new_f);

        if (first_i < 0) {
            /* Local optimum reached */
            wr->reached_optimum = true;
            nk_genotype_free(current);
            return wr;
        }

        nk_genotype_flip(current, first_i);
        current_f = new_f;
        walk_append_state(wr, current, current_f, step);
    }

    nk_genotype_free(current);
    return wr;
}

/* ============================================================================
 * Steepest-Ascent Walk (L5: Algorithm)
 *
 * At each step, evaluate all N one-bit-flip neighbors and move to the
 * one with the highest fitness. This is Wright's (1932) model of
 * adaptation under strong selection.
 *
 * Properties:
 *   - Reaches higher fitness local optima than greedy walk
 *   - Cost: O(N^2 * K) per step (evaluates all N neighbors)
 *   - Walk length typically shorter than greedy
 *   - Equivalent to "best-improvement" local search
 *
 * Independent knowledge point: exhaustive neighborhood evaluation
 * vs first-improvement, a fundamental distinction in local search.
 * ============================================================================ */

NKWalkResult* nk_walk_steepest_ascent(const NKLandscape *land,
                                       const NKGenotype *start,
                                       int max_steps) {
    if (!land || !start || max_steps <= 0) return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_STEEPEST_ASCENT;

    NKGenotype *current = nk_genotype_clone(start);
    double current_f = nk_fitness(land, current);
    walk_append_state(wr, current, current_f, 0);

    for (int step = 1; step <= max_steps; step++) {
        int best_i;
        double best_f;
        nk_best_neighbor(land, current, &best_i, &best_f);

        if (best_i < 0 || best_f <= current_f) {
            wr->reached_optimum = true;
            nk_genotype_free(current);
            return wr;
        }

        nk_genotype_flip(current, best_i);
        current_f = best_f;
        walk_append_state(wr, current, current_f, step);
    }

    nk_genotype_free(current);
    return wr;
}

/* ============================================================================
 * Next-Ascent Walk (L5: Algorithm)
 *
 * Systematic scan with wraparound memory. At each step, continue
 * scanning from the locus after the one just flipped. This removes
 * the bias toward low-index loci inherent in the greedy walk.
 *
 * This corresponds to the "next-descent" (or "next-ascent") strategy
 * in combinatorial optimization, adapted to fitness landscapes.
 *
 * Independent knowledge point: stateful scan order in local search,
 * eliminating positional bias present in simple greedy search.
 * ============================================================================ */

NKWalkResult* nk_walk_next_ascent(const NKLandscape *land,
                                   const NKGenotype *start,
                                   int max_steps) {
    if (!land || !start || max_steps <= 0) return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_NEXT_ASCENT;

    NKGenotype *current = nk_genotype_clone(start);
    double current_f = nk_fitness(land, current);
    walk_append_state(wr, current, current_f, 0);

    int scan_start = 0;  /* resume scanning from here */

    for (int step = 1; step <= max_steps; step++) {
        int found_i = -1;
        double found_f = current_f;
        int N = land->N;

        /* Scan from scan_start, wrapping around */
        for (int offset = 0; offset < N; offset++) {
            int i = (scan_start + offset) % N;
            NKGenotype *ng = nk_genotype_clone(current);
            nk_genotype_flip(ng, i);
            double fi = nk_fitness(land, ng);
            nk_genotype_free(ng);

            if (fi > current_f) {
                found_i = i;
                found_f = fi;
                break;
            }
        }

        if (found_i < 0) {
            wr->reached_optimum = true;
            nk_genotype_free(current);
            return wr;
        }

        nk_genotype_flip(current, found_i);
        current_f = found_f;
        scan_start = (found_i + 1) % N;
        walk_append_state(wr, current, current_f, step);
    }

    nk_genotype_free(current);
    return wr;
}

/* ============================================================================
 * Random-Better Walk (L5: Algorithm)
 *
 * At each step, identify ALL improving one-bit-flip neighbors,
 * then select one uniformly at random. Models the "neutral theory"
 * of molecular evolution (Kimura 1983) where beneficial mutations
 * are rare and any one of them may fix.
 *
 * This is related to Gillespie's (1984) "mutational landscape" model
 * and Orr's (2002) extreme value theory of adaptation.
 *
 * Independent knowledge point: uniform random selection among
 * beneficial mutations, modeling genetic drift in adaptive evolution.
 * ============================================================================ */

NKWalkResult* nk_walk_random_better(const NKLandscape *land,
                                     const NKGenotype *start,
                                     int max_steps) {
    if (!land || !start || max_steps <= 0) return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_RANDOM_BETTER;

    NKGenotype *current = nk_genotype_clone(start);
    double current_f = nk_fitness(land, current);
    walk_append_state(wr, current, current_f, 0);

    uint64_t rng = land->seed + 7777;
    int *improving = (int*)malloc(land->N * sizeof(int));

    for (int step = 1; step <= max_steps; step++) {
        int n_improving = 0;

        for (int i = 0; i < land->N; i++) {
            NKGenotype *ng = nk_genotype_clone(current);
            nk_genotype_flip(ng, i);
            double fi = nk_fitness(land, ng);
            nk_genotype_free(ng);

            if (fi > current_f) {
                improving[n_improving++] = i;
            }
        }

        if (n_improving == 0) {
            wr->reached_optimum = true;
            free(improving);
            nk_genotype_free(current);
            return wr;
        }

        /* Pick random improving move */
        int pick = (int)(nk_random_uniform(&rng) * n_improving);
        if (pick >= n_improving) pick = n_improving - 1;
        int chosen_i = improving[pick];

        nk_genotype_flip(current, chosen_i);
        current_f = nk_fitness(land, current);
        walk_append_state(wr, current, current_f, step);
    }

    free(improving);
    nk_genotype_free(current);
    return wr;
}

/* ============================================================================
 * Metropolis Walk (L5: Algorithm)
 *
 * At each step, pick a random neighbor and accept the move with
 * probability min(1, exp(-(F_old - F_new)/T)). This allows
 * downhill moves that escape local optima.
 *
 * The Metropolis algorithm (Metropolis et al. 1953) is the foundation
 * of Monte Carlo methods in statistical physics. Applied here to
 * fitness landscapes, it enables exploration of genotype space
 * beyond local optima.
 *
 * Independent knowledge point: Metropolis criterion for accepting
 * deleterious mutations, connecting evolutionary dynamics to
 * equilibrium statistical mechanics.
 * ============================================================================ */

NKWalkResult* nk_walk_metropolis(const NKLandscape *land,
                                  const NKGenotype *start,
                                  int max_steps, double T) {
    if (!land || !start || max_steps <= 0 || T <= 0.0) return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_METROPOLIS;

    NKGenotype *current = nk_genotype_clone(start);
    double current_f = nk_fitness(land, current);
    walk_append_state(wr, current, current_f, 0);

    uint64_t rng = land->seed + 12345;

    for (int step = 1; step <= max_steps; step++) {
        /* Pick random locus to flip */
        int i = (int)(nk_random_uniform(&rng) * land->N);
        if (i >= land->N) i = land->N - 1;

        NKGenotype *candidate = nk_genotype_clone(current);
        nk_genotype_flip(candidate, i);
        double cand_f = nk_fitness(land, candidate);

        double delta = current_f - cand_f;  /* positive if candidate is worse */

        if (delta <= 0.0) {
            /* Uphill or neutral move: always accept */
            nk_genotype_free(current);
            current = candidate;
            current_f = cand_f;
        } else {
            /* Downhill move: accept with probability exp(-delta/T) */
            double prob = exp(-delta / T);
            if (nk_random_uniform(&rng) < prob) {
                nk_genotype_free(current);
                current = candidate;
                current_f = cand_f;
            } else {
                nk_genotype_free(candidate);
            }
        }

        walk_append_state(wr, current, current_f, step);
    }

    nk_genotype_free(current);
    return wr;
}

/* ============================================================================
 * Simulated Annealing Walk (L5: Algorithm)
 *
 * Metropolis dynamics with a cooling schedule:
 *   T(t) = T0 / (1 + cooling_rate * t)
 * As T decreases, downhill moves become progressively less likely,
 * guiding the walk toward a (hopefully global) optimum.
 *
 * Simulated annealing was introduced by Kirkpatrick, Gelatt & Vecchi
 * (1983) as an optimization method inspired by metallurgical annealing.
 *
 * Independent knowledge point: temperature scheduling in stochastic
 * optimization, the key insight that distinguishes simulated annealing
 * from random Metropolis walks.
 * ============================================================================ */

NKWalkResult* nk_walk_simulated_annealing(const NKLandscape *land,
                                           const NKGenotype *start,
                                           int max_steps,
                                           double T0,
                                           double cooling_rate) {
    if (!land || !start || max_steps <= 0 || T0 <= 0.0 || cooling_rate <= 0.0)
        return NULL;

    NKWalkResult *wr = nk_walk_result_create(max_steps);
    if (!wr) return NULL;
    wr->strategy = NK_WALK_SIMULATED_ANNEAL;

    NKGenotype *current = nk_genotype_clone(start);
    double current_f = nk_fitness(land, current);
    walk_append_state(wr, current, current_f, 0);

    uint64_t rng = land->seed + 54321;
    double T = T0;

    for (int step = 1; step <= max_steps; step++) {
        /* Update temperature via cooling schedule */
        T = T0 / (1.0 + cooling_rate * step);

        int i = (int)(nk_random_uniform(&rng) * land->N);
        if (i >= land->N) i = land->N - 1;

        NKGenotype *candidate = nk_genotype_clone(current);
        nk_genotype_flip(candidate, i);
        double cand_f = nk_fitness(land, candidate);

        double delta = current_f - cand_f;

        if (delta <= 0.0 || nk_random_uniform(&rng) < exp(-delta / T)) {
            nk_genotype_free(current);
            current = candidate;
            current_f = cand_f;
        } else {
            nk_genotype_free(candidate);
        }

        walk_append_state(wr, current, current_f, step);
    }

    /* Check final state */
    wr->reached_optimum = nk_is_local_optimum(land, current);

    nk_genotype_free(current);
    return wr;
}

/* ============================================================================
 * Walk Analysis Utilities
 * ============================================================================ */

double nk_walk_basin_size(const NKLandscape *land,
                           const NKGenotype *target_optimum,
                           int n_starts,
                           NKWalkStrategy strategy,
                           uint64_t *rng) {
    if (!land || !target_optimum || n_starts <= 0 || !rng) return 0.0;

    int hits = 0;
    for (int s = 0; s < n_starts; s++) {
        NKGenotype *start = nk_genotype_create(land->N);
        nk_genotype_randomize(start, rng);

        NKWalkResult *wr = NULL;
        switch (strategy) {
        case NK_WALK_GREEDY:
            wr = nk_walk_greedy(land, start, 10 * land->N);
            break;
        case NK_WALK_STEEPEST_ASCENT:
            wr = nk_walk_steepest_ascent(land, start, 10 * land->N);
            break;
        default:
            wr = nk_walk_greedy(land, start, 10 * land->N);
            break;
        }

        if (wr && wr->reached_optimum) {
            NKGenotype *final_geno = wr->path[wr->steps]->genotype;
            if (nk_genotype_equal(final_geno, target_optimum)) {
                hits++;
            }
        }

        nk_walk_result_free(wr);
        nk_genotype_free(start);
    }

    return (double)hits / (double)n_starts;
}

void nk_walk_print_trajectory(const NKWalkResult *wr) {
    if (!wr) {
        printf("(null walk result)\n");
        return;
    }
    printf("=== Walk Trajectory (strategy=%d, steps=%d, optimum=%s) ===\n",
           wr->strategy, wr->steps, wr->reached_optimum ? "yes" : "no");
    printf("%-6s %-20s %12s %12s\n", "Step", "Genotype", "Fitness", "Delta");
    double prev_f = 0.0;
    for (int i = 0; i <= wr->steps; i++) {
        if (!wr->path[i]) break;
        char *s = nk_genotype_to_string(wr->path[i]->genotype);
        double f = wr->path[i]->fitness;
        double delta = (i > 0) ? f - prev_f : 0.0;
        printf("%-6d %-20s %12.6f %+12.6f\n", i, s ? s : "?", f, delta);
        prev_f = f;
        free(s);
    }
}

int nk_walk_best_step(const NKWalkResult *wr) {
    if (!wr || wr->steps < 0) return -1;
    int best = 0;
    double best_f = wr->path[0]->fitness;
    for (int i = 1; i <= wr->steps; i++) {
        if (wr->path[i] && wr->path[i]->fitness > best_f) {
            best_f = wr->path[i]->fitness;
            best = i;
        }
    }
    return best;
}

int nk_walk_count_distinct_optima(const NKLandscape *land,
                                   int n_walks,
                                   NKWalkStrategy strategy,
                                   uint64_t *rng) {
    if (!land || n_walks <= 0 || !rng) return 0;

    /* Store distinct optima found. Since we cannot enumerate all 2^N,
     * we use a simple list with linear search. For large N, this is
     * limited by memory but sufficient for analysis purposes. */
    int capacity = n_walks;
    NKGenotype **optima = (NKGenotype**)calloc(capacity, sizeof(NKGenotype*));
    int n_distinct = 0;

    for (int w = 0; w < n_walks; w++) {
        NKGenotype *start = nk_genotype_create(land->N);
        nk_genotype_randomize(start, rng);

        NKWalkResult *wr = NULL;
        switch (strategy) {
        case NK_WALK_GREEDY:
            wr = nk_walk_greedy(land, start, 10 * land->N);
            break;
        case NK_WALK_STEEPEST_ASCENT:
            wr = nk_walk_steepest_ascent(land, start, 10 * land->N);
            break;
        case NK_WALK_NEXT_ASCENT:
            wr = nk_walk_next_ascent(land, start, 10 * land->N);
            break;
        case NK_WALK_RANDOM_BETTER:
            wr = nk_walk_random_better(land, start, 10 * land->N);
            break;
        default:
            wr = nk_walk_greedy(land, start, 10 * land->N);
            break;
        }

        if (wr && wr->reached_optimum && n_distinct < capacity) {
            NKGenotype *final_geno = wr->path[wr->steps]->genotype;
            bool found = false;
            for (int d = 0; d < n_distinct; d++) {
                if (nk_genotype_equal(optima[d], final_geno)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                optima[n_distinct] = nk_genotype_clone(final_geno);
                n_distinct++;
            }
        }

        nk_walk_result_free(wr);
        nk_genotype_free(start);
    }

    for (int d = 0; d < n_distinct; d++) {
        nk_genotype_free(optima[d]);
    }
    free(optima);

    return n_distinct;
}
