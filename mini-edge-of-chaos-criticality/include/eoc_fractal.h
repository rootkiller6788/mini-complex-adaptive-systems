#ifndef EOC_FRACTAL_H
#define EOC_FRACTAL_H

#include "eoc_core.h"

/* ============================================================================
 * Fractal Geometry ? Dimension, Scaling, and Multifractals
 *
 * Fractal structures are ubiquitous at the edge of chaos and in SOC.
 * This module implements the standard methods for characterizing
 * fractal and multifractal properties.
 *
 * References:
 *   Mandelbrot, B.B. (1982). The Fractal Geometry of Nature. W.H. Freeman.
 *   Feder, J. (1988). Fractals. Plenum Press.
 *   Hentschel, H.G.E. & Procaccia, I. (1983). Physica D, 8(3), 435-444.
 *   Grassberger, P. & Procaccia, I. (1983). Physica D, 9(1-2), 189-208.
 *   Halsey, T.C. et al. (1986). Phys. Rev. A, 33(2), 1141-1151.
 * ============================================================================ */

/** Box-counting dimension (capacity dimension D_0):
 *  N(epsilon) ~ epsilon^{-D_0} as epsilon -> 0.
 *  For a set of points in d-dimensional space.
 *  Complexity: O(n_eps * n_points * log n_points) with spatial hashing. */
double eoc_box_counting_dimension(double** points, int n_points, int dim,
                                   double eps_min, double eps_max, int n_eps);

/** Box-counting for 2D binary images (grid data).
 *  N(box_size) = number of boxes of size L that contain at least one
 *  occupied site. D_0 = -d log N / d log L.
 *  Complexity: O(n_scales * width * height). */
double eoc_box_counting_2d(int* grid, int width, int height);

/** Correlation dimension D_2 (Grassberger-Procaccia 1983):
 *  C(epsilon) = (2/N(N-1)) * sum_{i<j} Theta(epsilon - |x_i - x_j|)
 *  C(epsilon) ~ epsilon^{D_2}.
 *  Complexity: O(N^2) pairwise distances. */
EOCCorrelationIntegral eoc_correlation_dimension(double** points, int n_points,
                                                   int dim, double eps_min,
                                                   double eps_max, int n_eps);

/** Generalized correlation integral for D_q dimensions.
 *  C_q(epsilon) = [1/N * sum_i (n_i(epsilon)/N)^{q-1}]^{1/(q-1)}
 *  where n_i(epsilon) = number of points within epsilon of point i.
 *  Complexity: O(N^2) pairwise distances. */
double eoc_generalized_dimension(double** points, int n_points, int dim,
                                  double epsilon, double q);

/** Compute full multifractal spectrum f(alpha) via the method of moments.
 *  For each moment order q, computes tau(q), alpha(q), f(alpha(q)).
 *  Complexity: O(n_q * N^2). */
EOCMultifractalSpectrum* eoc_multifractal_spectrum(double** points,
                                                     int n_points, int dim,
                                                     double* q_values,
                                                     int n_q);

/** Information dimension D_1 = lim_{q->1} D_q.
 *  D_1 = lim_{eps->0} sum_i p_i(eps) * log(p_i(eps)) / log(eps).
 *  Complexity: O(N^2). */
double eoc_information_dimension(double** points, int n_points, int dim,
                                  double eps_min, double eps_max, int n_eps);

/** Higuchi's fractal dimension for time series (Higuchi 1988).
 *  Computes fractal dimension from curve length at different scales.
 *  Suitable for 1D time series analysis.
 *  Complexity: O(n * k_max). */
double eoc_higuchi_fractal_dimension(double* time_series, int n, int k_max);

/** Detrended Fluctuation Analysis (DFA) ? Peng et al. 1994.
 *  Computes the scaling exponent alpha for time series:
 *  alpha ~ 0.5: white noise (uncorrelated)
 *  alpha ~ 1.0: 1/f noise (pink noise ? critical)
 *  alpha ~ 1.5: Brownian noise
 *  Complexity: O(n log n). */
double eoc_dfa_exponent(double* time_series, int n, int min_scale, int max_scale);

/** Sandbox method for generalized dimensions.
 *  Place boxes at random positions, count points per box, average.
 *  More efficient than pairwise method for large N.
 *  Complexity: O(n_boxes * N). */
double eoc_sandbox_dimension(double** points, int n_points, int dim,
                              double box_size, int n_samples);

/** Cluster fractal dimension via the radius of gyration method.
 *  For a connected cluster: R_g^2 = (1/N) * sum_i (r_i - r_cm)^2
 *  Fits: N ~ R_g^{D_f}.
 *  Complexity: O(N). */
double eoc_cluster_fractal_dimension(int* cluster_sites, int n_sites,
                                      double** positions);
#endif /* EOC_FRACTAL_H */
