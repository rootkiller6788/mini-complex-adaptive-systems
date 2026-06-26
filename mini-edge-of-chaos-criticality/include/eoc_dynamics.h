#ifndef EOC_DYNAMICS_H
#define EOC_DYNAMICS_H

#include "eoc_core.h"

/* ============================================================================
 * Nonlinear Dynamics ? Bifurcation, Lyapunov, Chaos
 *
 * Core topics in dynamical systems theory at the edge of chaos:
 *   - Logistic map as canonical period-doubling route to chaos
 *   - Lyapunov exponents for chaos detection
 *   - Bifurcation diagram construction
 *   - Feigenbaum universal constants (delta ~ 4.669, alpha ~ 2.502)
 *   - Strange attractors (Lorenz, Rossler, Henon)
 *
 * References:
 *   Strogatz, S.H. (2018). Nonlinear Dynamics and Chaos. 2nd ed. Westview.
 *   Feigenbaum, M.J. (1978). J. Stat. Phys. 19(1), 25-52.
 *   Lorenz, E.N. (1963). J. Atmos. Sci. 20(2), 130-141.
 *   Eckmann, J.P. & Ruelle, D. (1985). Rev. Mod. Phys. 57(3), 617.
 * ============================================================================ */

/* ============================================================================
 * Logistic Map: x_{n+1} = r * x_n * (1 - x_n)
 * Canonical route to chaos via period-doubling.
 * ============================================================================ */

/** Single iteration of the logistic map. Complexity: O(1). */
double eoc_logistic_map(double x, double r);

/** Iterate logistic map n times starting from x0 with parameter r.
 *  Returns array of trajectory values (caller must free). */
double* eoc_logistic_trajectory(double x0, double r, int n_transient, int n);

/** Bifurcation diagram: for each r in [r_min, r_max], iterate and
 *  collect asymptotic values. Complexity: O(n_r * n * n_keep). */
void eoc_bifurcation_diagram(double r_min, double r_max, int n_r,
                              int n_transient, int n_keep,
                              EOCBifurcationPoint** points, int* n_points);

/** Compute Feigenbaum's delta constant from period-doubling cascade.
 *  delta = lim_{n->inf} (r_n - r_{n-1}) / (r_{n+1} - r_n) ~ 4.6692016...
 *  Complexity: O(2^max_period * max_period). */
double eoc_feigenbaum_delta(double* r_bifur, int n_periods);

/** Compute Feigenbaum's alpha constant (scaling factor).
 *  alpha = lim_{n->inf} d_n / d_{n+1} ~ 2.5029078... */
double eoc_feigenbaum_alpha(double* r_bifur, double x_peak, int n_periods);

/* ============================================================================
 * Lyapunov Exponents
 * ============================================================================ */

/** Largest Lyapunov exponent via direct divergence method (Rosenstein 1993).
 *  lambda = (1/t) * ln(|dx(t)| / |dx(0)|)
 *  lambda > 0 -> chaotic, lambda = 0 -> marginal, lambda < 0 -> stable.
 *  Complexity: O(n * d^2) where d is embedding dimension. */
double eoc_lyapunov_rosenstein(double* time_series, int n,
                                int embed_dim, int lag, int min_sep);

/** Largest Lyapunov exponent via Wolf algorithm (Wolf et al. 1985).
 *  Tracks divergence of nearby trajectories in phase space.
 *  Complexity: O(n * d^2 * n_evo). */
double eoc_lyapunov_wolf(double** phase_space, int n_points, int dim,
                           int n_evo, double max_scale);

/** Full Lyapunov spectrum via QR decomposition (Benettin et al. 1980).
 *  Returns array of d exponents sorted descending (caller must free).
 *  Complexity: O(n * d^3). */
double* eoc_lyapunov_spectrum(void (*flow)(double* x, int d, double* dxdt),
                               double* x0, int d, int n_steps, double dt);

/** Kaplan-Yorke (Lyapunov) dimension:
 *  D_KY = k + sum_{i=1}^{k} lambda_i / |lambda_{k+1}|
 *  where k is the largest index such that sum_{i=1}^{k} lambda_i >= 0. */
double eoc_kaplan_yorke_dimension(double* lyap_spectrum, int d);

/** 0-1 Test for Chaos (Gottwald & Melbourne 2004):
 *  K ~ 0 -> regular, K ~ 1 -> chaotic.
 *  Complexity: O(n). */
double eoc_zero_one_test(double* time_series, int n, double c);

/* ============================================================================
 * Continuous-Time Systems
 * ============================================================================ */

/** Lorenz system: dx/dt = sigma*(y-x), dy/dt = x*(rho-z)-y,
 *  dz/dt = x*y - beta*z.
 *  Classic parameters: sigma=10, rho=28, beta=8/3 -> strange attractor. */
void eoc_lorenz(double* state, double* deriv, double sigma, double rho, double beta);

/** Rossler system: dx/dt = -y - z, dy/dt = x + a*y, dz/dt = b + z*(x - c).
 *  Classic: a=0.2, b=0.2, c=5.7 -> chaotic. */
void eoc_rossler(double* state, double* deriv, double a, double b, double c);

/** Henon map (discrete 2D): x_{n+1} = 1 - a*x_n^2 + y_n,
 *  y_{n+1} = b*x_n. Classic: a=1.4, b=0.3 -> strange attractor. */
void eoc_henon_map(double* x, double* y, double a, double b);

/** Runge-Kutta 4th order integrator for ODE systems.
 *  Complexity: O(n_steps * d * 4). */
double* eoc_rk4_integrate(void (*f)(double*, double*, void*), double* x0,
                           int d, double dt, int n_steps, void* params);

/** Estimate largest Lyapunov exponent from two nearby RK4 trajectories. */
double eoc_rk4_lyapunov(void (*f)(double*, double*, void*), double* x0,
                         int d, double dt, int n_steps, double eps, void* params);

#endif /* EOC_DYNAMICS_H */
