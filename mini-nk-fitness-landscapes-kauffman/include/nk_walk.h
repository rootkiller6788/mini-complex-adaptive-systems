#ifndef NK_WALK_H
#define NK_WALK_H

#include "nk_core.h"

/* ============================================================================
 * Adaptive Walks on NK Fitness Landscapes (L5: Algorithms)
 *
 * Implements six adaptive walk strategies on rugged fitness landscapes.
 * Each strategy represents a different model of evolutionary dynamics.
 *
 * Key references:
 *   Kauffman & Levin (1987): Greedy and steepest-ascent walks
 *   Gillespie (1984): Molecular evolution on fitness landscapes
 *   Orr (2002): Adaptation on rugged landscapes, extreme value theory
 *   Metropolis et al. (1953): Monte Carlo sampling
 *
 * Theorems:
 *   Thm 1: Greedy walk length ~ ln(N) for K >= 2 (Kauffman 1993)
 *   Thm 2: Expected fitness reached increases with population size
 *   Thm 3: Steepest ascent reaches higher fitness per step at O(N) cost
 *   Thm 4: Number of accessible local optima grows exponentially with N
 * ============================================================================ */

/* === Walk Result Lifecycle === */

/** Allocate a walk result with capacity for max_steps states.
 *  Complexity: O(max_steps). */
NKWalkResult* nk_walk_result_create(int max_steps);

/** Free a walk result and all contained genotypes. */
void nk_walk_result_free(NKWalkResult *wr);

/* === Core Walk Algorithms === */

/** Greedy adaptive walk: at each step, scan loci systematically (0..N-1)
 *  and move to the FIRST improving neighbor found.
 *  Properties: O(N*K) per step average, walk length O(log N).
 *  This is the original adaptive walk studied by Kauffman & Levin (1987).
 *
 *  @param land       The NK fitness landscape
 *  @param start      Starting genotype (copied internally)
 *  @param max_steps  Maximum steps before forced termination
 *  @return           Walk result with full trajectory */
NKWalkResult* nk_walk_greedy(const NKLandscape *land,
                              const NKGenotype *start,
                              int max_steps);

/** Steepest-ascent walk: at each step, evaluate ALL N neighbors
 *  and move to the one with highest fitness (if improving).
 *  Properties: reaches higher local optima than greedy,
 *  but costs O(N^2 * K) per step. Walk length typically shorter.
 *  Reference: Wright (1932) shifting balance theory. */
NKWalkResult* nk_walk_steepest_ascent(const NKLandscape *land,
                                       const NKGenotype *start,
                                       int max_steps);

/** Next-ascent walk: systematic scan with wraparound.
 *  At step t, continue scanning from the locus after the one
 *  that was just flipped, taking the first improvement found.
 *  This avoids the bias of always starting scans from locus 0.
 *  Complexity: similar to greedy, O(N*K) per step average. */
NKWalkResult* nk_walk_next_ascent(const NKLandscape *land,
                                   const NKGenotype *start,
                                   int max_steps);

/** Random-better walk: evaluate all N neighbors, identify the set
 *  of improving moves, and pick one uniformly at random.
 *  Models neutral molecular evolution with drift among
 *  beneficial mutations (Kimura 1983, Orr 2002).
 *  Complexity: O(N^2 * K) per step. */
NKWalkResult* nk_walk_random_better(const NKLandscape *land,
                                     const NKGenotype *start,
                                     int max_steps);

/** Metropolis walk: at each step, pick a random neighbor and
 *  accept with probability min(1, exp(-(F_old - F_new)/T)).
 *  Can move downhill, escaping local optima.
 *  Temperature T controls exploration vs exploitation.
 *  Complexity: O(N*K) per step.
 *
 *  @param T  Temperature parameter (T > 0) */
NKWalkResult* nk_walk_metropolis(const NKLandscape *land,
                                  const NKGenotype *start,
                                  int max_steps, double T);

/** Simulated annealing walk: Metropolis dynamics with a cooling schedule.
 *  Temperature decreases as T(t) = T0 / (1 + cooling_rate * t).
 *  Aims to find the global optimum by gradually reducing
 *  the probability of accepting worse moves.
 *  Complexity: O(N*K * max_steps).
 *
 *  @param T0            Initial temperature
 *  @param cooling_rate  Cooling rate (positive) */
NKWalkResult* nk_walk_simulated_annealing(const NKLandscape *land,
                                           const NKGenotype *start,
                                           int max_steps,
                                           double T0,
                                           double cooling_rate);

/* === Walk Analysis === */

/** Compute the basin of attraction size for a given local optimum.
 *  Starts adaptive walks from n_starts random genotypes;
 *  count how many converge to (or past) the target optimum.
 *  Returns the fraction [0,1] of starts that reach the target.
 *  Complexity: O(n_starts * N * K * avg_walk_length). */
double nk_walk_basin_size(const NKLandscape *land,
                           const NKGenotype *target_optimum,
                           int n_starts,
                           NKWalkStrategy strategy,
                           uint64_t *rng_state);

/** Print a walk trajectory to stdout in a human-readable format.
 *  Each line: step, genotype, fitness, delta from previous. */
void nk_walk_print_trajectory(const NKWalkResult *wr);

/** Find the highest-fitness step in a walk trajectory.
 *  Returns the index of the step with maximum fitness. */
int nk_walk_best_step(const NKWalkResult *wr);

/** Count how many distinct local optima are reachable from
 *  random starting points using a given walk strategy.
 *  Useful for estimating landscape ruggedness empirically.
 *  Complexity: O(n_walks * N * K * avg_walk_length). */
int nk_walk_count_distinct_optima(const NKLandscape *land,
                                   int n_walks,
                                   NKWalkStrategy strategy,
                                   uint64_t *rng_state);

#endif /* NK_WALK_H */
