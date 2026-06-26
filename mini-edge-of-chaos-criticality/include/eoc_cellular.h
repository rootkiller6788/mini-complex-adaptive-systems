#ifndef EOC_CELLULAR_H
#define EOC_CELLULAR_H

#include "eoc_core.h"

/* ============================================================================
 * Cellular Automata ? Wolfram Classes and Edge of Chaos
 *
 * Cellular automata (CA) provide the simplest mathematical models of spatially
 * extended dynamical systems. The relationship between CA rule space, dynamical
 * behavior, and computational capability was first systematically explored by:
 *
 *   Wolfram, S. (1984). "Universality and complexity in cellular automata."
 *     Physica D, 10(1-2), 1-35.
 *
 *   Langton, C.G. (1990). "Computation at the edge of chaos."
 *     Physica D, 42(1-3), 12-37.
 *
 * Langton's lambda parameter:
 *   lambda = (n - n_q) / n  where n = total states, n_q = quiescent transitions
 *   lambda ~ 0: ordered (Class I,II)
 *   lambda ~ lambda_c ~ 0.273: edge of chaos (Class IV) ? maximal computation
 *   lambda ~ 0.5: chaotic (Class III)
 *
 * Key concepts:
 *   - Rule space topology and dynamical classification (Wuensche 1992)
 *   - Mutual information as order parameter (Langton 1990)
 *   - Computation universality: Rule 110 (Cook 2004), Game of Life (Conway 1970)
 * ============================================================================ */

/* --- 1D Elementary Cellular Automata --- */

/** Evaluate Wolfram 1D rule (rule number 0-255) with radius 1 (k=2, r=1).
 *  8-bit rule encodes next state for each 3-bit neighborhood (111..000).
 *  Returns the next state of cell given left, center, right values.
 *  Complexity: O(1). */
int eoc_ca_rule_eval(int rule_number, int left, int center, int right);

/** Evolve 1D elementary CA for n_steps.
 *  lattice: array of length width (wrap-around or fixed boundaries).
 *  Writes history into state_history[row][col] (caller provides buffer).
 *  Complexity: O(n_steps * width). */
void eoc_ca_evolve_1d(int rule_number, int* lattice, int width,
                       int n_steps, int** state_history, bool periodic);

/** Compute Langton's lambda for a given 1D rule.
 *  lambda = fraction of non-quiescent output states in rule table.
 *  For elementary CA (k=2, r=1): lambda in {0, 1/8, 2/8, ..., 7/8, 1}.
 *  Complexity: O(1). */
double eoc_ca_lambda_1d(int rule_number, int quiescent_state);

/** Classify 1D CA rule into Wolfram classes I-IV.
 *  Based on: entropy, density, mutual information, transient length.
 *  Complexity: O(n_test * width * n_steps). */
WolframClass eoc_ca_classify_1d(int rule_number, int width, int n_steps);

/* --- 2D Cellular Automata --- */

/** Conway's Game of Life (B3/S23).
 *  Neighborhood: Moore (8 neighbors).
 *  Birth: dead cell with exactly 3 live neighbors -> alive.
 *  Survival: live cell with 2 or 3 live neighbors -> survives.
 *  Complexity: O(width * height) per step. */
void eoc_game_of_life_step(int* grid, int* next_grid, int width, int height);

/** Evolve Game of Life for n_steps tracking density.
 *  Complexity: O(n_steps * width * height). */
void eoc_game_of_life_evolve(int* grid, int width, int height,
                              int n_steps, double* density_history);

/** Classify a 2D CA state based on density, entropy, and clustering.
 *  Complexity: O(width * height). */
WolframClass eoc_ca_classify_2d(int* grid, int width, int height);

/** Compute the spatial entropy (Shannon) of a CA grid.
 *  Uses block-entropy method with block_size x block_size blocks.
 *  Complexity: O(width * height). */
double eoc_ca_spatial_entropy(int* grid, int width, int height, int block_size);

/** Compute mutual information between two CA configurations.
 *  I(X;Y) = H(X) + H(Y) - H(X,Y).
 *  Used as order parameter in Langton's phase transition analysis.
 *  Complexity: O(n). */
double eoc_ca_mutual_information(int* config1, int* config2, int n);

/** Langton's ant: a 2D Turing machine on a grid.
 *  Rules: on white (0) turn right, on black (1) turn left. Flip color.
 *  Emerges highway pattern after ~10,000 steps (emergent order).
 *  Complexity: O(n_steps). */
void eoc_langtons_ant(int* grid, int width, int height,
                       int* ant_x, int* ant_y, int* ant_dir, int n_steps);

/** Brian's Brain CA (/2/3): three states (0=dead, 1=dying, 2=alive).
 *  Demonstrates complex wave patterns with gliders at edge of chaos.
 *  Complexity: O(width * height) per step. */
void eoc_brians_brain_step(int* grid, int* next_grid, int width, int height);

/** Wireworld CA: 4 states (empty, conductor, electron-head, electron-tail).
 *  Turing-complete, supports logic gate construction.
 *  Complexity: O(width * height) per step. */
void eoc_wireworld_step(int* grid, int* next_grid, int width, int height);

/** Compute the Langton lambda phase transition curve for 1D CA rules.
 *  For each lambda, sample many rules, measure average mutual information.
 *  Returns arrays of lambda values and average mutual information.
 *  Complexity: O(n_lambda * n_rules * n_steps * width). */
void eoc_langton_phase_transition(int n_lambda, int n_rules_per_bin,
                                   int width, int n_steps,
                                   double** lambdas, double** mi_avg,
                                   int* n_points);

/** Wuensche's basin of attraction analysis for a 1D CA rule.
 *  Enumerate all 2^width states, evolve each to attractor, classify.
 *  Returns: n_fixed_points, n_cycles, average_transient, basin_entropy.
 *  Complexity: O(2^width * max_steps). */
void eoc_ca_basin_analysis(int rule_number, int width, int max_steps,
                            int* n_fixed, int* n_cycles,
                            double* avg_transient, double* basin_entropy);

#endif /* EOC_CELLULAR_H */
