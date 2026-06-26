#ifndef NK_COEVOLUTION_H
#define NK_COEVOLUTION_H

#include "nk_core.h"

/* ============================================================================
 * NKCS Coevolution Model (L7: Applications / L8: Advanced Topics)
 *
 * Extends the NK model to two coupled species, implementing the
 * NKCS (NK Coupled Systems) framework of Kauffman & Johnsen (1991).
 *
 * In the NKCS model, two species coevolve on coupled fitness landscapes.
 * Each species' fitness depends on its own N loci and C loci of the
 * other species, creating a coevolutionary dynamic.
 *
 * Key references:
 *   Kauffman, S.A. & Johnsen, S. (1991). "Coevolution to the edge of
 *     chaos: Coupled fitness landscapes, poised states, and coevolutionary
 *     avalanches." J. Theor. Biol., 149(4), 467-505.
 *   Kauffman, S.A. (1995). At Home in the Universe. Oxford University Press.
 *   Van Valen, L. (1973). "A new evolutionary law." Evolutionary Theory, 1, 1-30.
 *     (Red Queen hypothesis)
 * ============================================================================ */

/* === NKCS Landscape Lifecycle === */

/** Create an NKCS coevolution landscape pair.
 *  N: loci per species. K: internal epistasis. C: cross-species epistasis.
 *  coupling_strength in [0,1]: 0 = independent, 1 = fully coupled.
 *  Complexity: O(N * (2^(K+1) + 2^(K+C+1))). */
NKCSLandscape* nkcs_create(int N, int K, int C,
                            double coupling_strength,
                            uint64_t seed);

/** Free NKCS landscape. */
void nkcs_free(NKCSLandscape *nkcs);

/* === Coupled Fitness Evaluation === */

/** Evaluate fitness of species A given the genotypes of both species.
 *  F_A(g_A, g_B) = (1/N) * sum_i f_i(config_i(g_A, g_B))
 *  where config_i includes alleles of i, its K internal neighbors,
 *  and its C cross-species coupling partners.
 *  Complexity: O(N * (K + C)). */
double nkcs_fitness_a(const NKCSLandscape *nkcs,
                       const NKGenotype *ga,
                       const NKGenotype *gb);

/** Evaluate fitness of species B given the genotypes of both species.
 *  Symmetric to fitness_a.
 *  Complexity: O(N * (K + C)). */
double nkcs_fitness_b(const NKCSLandscape *nkcs,
                       const NKGenotype *ga,
                       const NKGenotype *gb);

/* === Coupled Adaptive Walks === */

/** Perform a coupled adaptive walk: species A and B take turns
 *  making fitness-improving single-locus mutations.
 *  Models coevolutionary arms races.
 *  At each step, one species mutates while the other is fixed,
 *  alternating species.
 *  Complexity: O(max_steps * N^2 * (K + C)). */
NKWalkResult* nkcs_coupled_walk_a(const NKCSLandscape *nkcs,
                                   const NKGenotype *start_a,
                                   const NKGenotype *start_b,
                                   int max_steps);

/** Coupled walk tracking species B. Symmetric to walk_a. */
NKWalkResult* nkcs_coupled_walk_b(const NKCSLandscape *nkcs,
                                   const NKGenotype *start_a,
                                   const NKGenotype *start_b,
                                   int max_steps);

/* === Red Queen Dynamics === */

/** Measure Red Queen effect: the rate of fitness change required
 *  for a species to maintain its relative fitness against a
 *  coevolving partner.
 *  Returns the fraction of steps where a mutation was needed just
 *  to "stay in place" (Van Valen 1973).
 *  Complexity: O(n_steps * N^2 * (K + C)). */
double nkcs_red_queen_measure(const NKCSLandscape *nkcs,
                               int n_steps,
                               uint64_t *rng_state);

/** Compute the coevolutionary fitness trajectory for both species.
 *  Fills fitness_a[t] and fitness_b[t] with the fitness values at
 *  each coevolutionary time step.
 *  Arrays must be pre-allocated to size n_steps+1.
 *  Complexity: O(n_steps * N^2 * (K + C)). */
void nkcs_fitness_trajectory(const NKCSLandscape *nkcs,
                              int n_steps,
                              double *fitness_a,
                              double *fitness_b,
                              uint64_t *rng_state);

/* === Nash Equilibrium Analysis === */

/** Check if a pair of genotypes (ga, gb) forms a Nash equilibrium
 *  in the coevolutionary game. A Nash equilibrium means neither
 *  species can unilaterally improve its fitness by a single mutation.
 *  Complexity: O(N^2 * (K + C)). */
bool nkcs_is_nash_equilibrium(const NKCSLandscape *nkcs,
                               const NKGenotype *ga,
                               const NKGenotype *gb);

/** Find a Nash equilibrium by iterative best-response dynamics.
 *  Returns the number of iterations until convergence (or max_iters if not found).
 *  Complexity: O(max_iters * N^2 * (K + C)). */
int nkcs_find_nash_equilibrium(const NKCSLandscape *nkcs,
                                NKGenotype *ga_out,
                                NKGenotype *gb_out,
                                int max_iters,
                                uint64_t *rng_state);

/* === Coupling Effects Analysis === */

/** Compute the mutual information between the fitness landscapes
 *  of species A and B, measuring the degree of coupling.
 *  Higher MI = stronger coevolutionary interdependence.
 *  Complexity: O(n_samples * N * (K + C)). */
double nkcs_mutual_information(const NKCSLandscape *nkcs,
                                int n_samples,
                                uint64_t *rng_state);

/** Compute the coevolutionary avalanche size distribution.
 *  An avalanche is a cascade of mutations triggered by a single
 *  mutation in one species, causing alternating fitness changes.
 *  Fills sizes[0..n_events-1] with avalanche sizes.
 *  Complexity: O(n_events * N^2 * (K + C)). */
void nkcs_avalanche_distribution(const NKCSLandscape *nkcs,
                                  int n_events,
                                  int *sizes,
                                  uint64_t *rng_state);

/** Measure complexity catastrophe in coevolution.
 *  As coupling C increases, the mean fitness at Nash equilibrium
 *  decreases, analogous to the internal epistasis complexity
 *  catastrophe.
 *  Fills coupling[0..max_C] and fit[0..max_C] with results.
 *  Complexity: O(max_C * N^2 * (K + max_C) * n_trials). */
void nkcs_coupling_catastrophe(int N, int K, int max_C,
                                int n_trials,
                                double *coupling,
                                double *fit,
                                uint64_t seed);

#endif /* NK_COEVOLUTION_H */
