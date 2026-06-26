#ifndef EOC_SANDPILE_H
#define EOC_SANDPILE_H

#include "eoc_core.h"

/* ============================================================================
 * Self-Organized Criticality ? Sandpile Models
 *
 * Implements the Bak-Tang-Wiesenfeld (BTW) sandpile model and the Manna
 * stochastic sandpile model ? the two canonical models of self-organized
 * criticality (SOC).
 *
 * BTW Model (Bak, Tang, Wiesenfeld 1987):
 *   - Deterministic toppling rule on a d-dimensional lattice
 *   - Open boundary conditions (grains lost at edges)
 *   - Produces power-law avalanche distributions with tau ~ 1.1 (2D)
 *   - Universality class: UNIV_BTW
 *
 * Manna Model (Manna 1991):
 *   - Stochastic toppling rule (grains redistributed randomly)
 *   - Same universality class as directed percolation (DP)
 *   - Universality class: UNIV_MANNA
 *
 * Oslo Ricepile (Christensen et al. 1996):
 *   - Slope-dependent thresholds, relevant for granular systems
 *   - Universality class: UNIV_OSLO
 *
 * References:
 *   Bak, Tang, Wiesenfeld, PRL 59(4), 381 (1987)
 *   Manna, J. Phys. A 24, L363 (1991)
 *   Christensen et al., PRL 77(1), 107 (1996)
 * ============================================================================ */

/* --- Sandpile Operations --- */

/**
 * BTW Sandpile: Add a grain at site (x,y), return avalanche size.
 * Theorem (Dhar 1990): The BTW model on any graph has a unique
 * recurrent configuration space forming an Abelian group under addition.
 * Complexity: O(L^2) worst-case per grain for an LxL lattice.
 */
int eoc_sandpile_add_grain_btw(EOCSandpileModel* sp, int x, int y);

/**
 * Manna stochastic sandpile: add grain, random redistribution on toppling.
 * Complexity: O(L^2) expected per grain.
 */
int eoc_sandpile_add_grain_manna(EOCSandpileModel* sp, int x, int y);

/**
 * Oslo ricepile model: slope-dependent thresholds.
 * Each site i has threshold z_i chosen randomly from {1,2} on reset.
 * Complexity: O(L^2) expected per grain.
 */
int eoc_sandpile_add_grain_oslo(EOCSandpileModel* sp, int x, int y);

/** Drive the system for n_grains additions, recording avalanche statistics. */
void eoc_sandpile_drive(EOCSandpileModel* sp, int n_grains,
                         int (*add_fn)(EOCSandpileModel*, int, int));

/**
 * Compute the avalanche size distribution from recorded history.
 * Returns probability P(s) for avalanche size s in log-spaced bins.
 * Complexity: O(N log N) where N = n_avalanches_recorded.
 */
void eoc_sandpile_distribution(EOCSandpileModel* sp,
                                double** sizes, double** probs, int* n_bins);

/** Fit power-law to avalanche distribution: P(s) ~ s^{-tau}. */
EOCPowerLawFit eoc_sandpile_fit_powerlaw(EOCSandpileModel* sp);

/**
 * Finite-size scaling analysis (Lubeck 2000):
 * For an LxL system at criticality: P(s, L) = L^{-beta} * G(s / L^{D})
 * where D is the avalanche fractal dimension.
 * Returns estimated tau and D.
 */
void eoc_sandpile_finite_size_scaling(EOCSandpileModel** models, int n_sizes,
                                       int* sizes_L, double* tau_out,
                                       double* D_out);

/**
 * Compute the average avalanche size as a function of system size L.
 * For BTW in 2D: <s> ~ L^{2} (diverges with system size ? hallmark of SOC).
 * Complexity: O(n_grains * L^2).
 */
double eoc_sandpile_average_size(EOCSandpileModel* sp, int n_grains);

/** Estimate branching ratio sigma = average number of topplings per grain. */
double eoc_sandpile_branching_ratio(EOCSandpileModel* sp, int n_grains);

/** Check if the system has reached the critical steady state. */
bool eoc_sandpile_is_critical(EOCSandpileModel* sp);

/** Reset sandpile to empty state (all heights = 0). */
void eoc_sandpile_reset(EOCSandpileModel* sp);

/** Calculate 1/f noise spectrum from avalanche time series.
 *  Uses the periodogram method (Welch).
 *  Complexity: O(N log N). */
void eoc_sandpile_flicker_noise(EOCSandpileModel* sp,
                                 double** freqs, double** power, int* n_freqs);

#endif /* EOC_SANDPILE_H */
