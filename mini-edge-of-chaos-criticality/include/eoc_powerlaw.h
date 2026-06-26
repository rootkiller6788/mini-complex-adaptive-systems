#ifndef EOC_POWERLAW_H
#define EOC_POWERLAW_H

#include "eoc_core.h"

/* ============================================================================
 * Power-Law Distributions ? Detection, Fitting, and Testing
 *
 * Power-law distributions P(x) ~ x^{-alpha} are the hallmark of critical
 * phenomena and self-organized criticality. This module implements the
 * statistical methods described in:
 *
 *   Clauset, A., Shalizi, C.R., & Newman, M.E.J. (2009).
 *   "Power-law distributions in empirical data." SIAM Review, 51(4), 661-703.
 *
 * Key results:
 *   - MLE estimator for continuous power-law (Pareto): alpha = 1 + n / sum(ln(x_i/x_min))
 *   - MLE estimator for discrete power-law (Zipf): alpha via zeta function
 *   - Kolmogorov-Smirnov for x_min selection
 *   - Bootstrap goodness-of-fit test
 *   - Likelihood ratio test for power-law vs. alternative distributions
 *
 * References:
 *   Clauset, Shalizi, Newman, SIAM Review 51(4), 2009
 *   Newman, M.E.J. (2005). Contemporary Physics, 46(5), 323-351.
 *   Bauke, H. (2007). Eur. Phys. J. B, 58(2), 167-173.
 * ============================================================================ */

/** Maximum Likelihood Estimation for continuous power-law (Pareto).
 *  Given data x_i and lower bound x_min, estimates: alpha = 1 + n / sum(ln(x_i/x_min)).
 *  Standard error: sigma = (alpha - 1) / sqrt(n).
 *  Theorem (Clauset et al. 2009): This estimator is consistent and
 *  asymptotically normal for alpha > 1.
 *  Complexity: O(n). */
EOCPowerLawFit eoc_powerlaw_fit_continuous(double* data, int n, double xmin);

/** MLE for discrete power-law (Zipf distribution): P(k) ~ k^{-alpha} / zeta(alpha).
 *  Uses approximate zeta function for efficiency.
 *  Complexity: O(n + max(x)) to evaluate zeta. */
EOCPowerLawFit eoc_powerlaw_fit_discrete(int* data, int n, int xmin);

/** Find optimal x_min by minimizing Kolmogorov-Smirnov distance D.
 *  D = max_x |CDF_empirical(x) - CDF_powerlaw(x)|.
 *  Evaluates all candidate x_min values in data.
 *  Complexity: O(n^2) naive, O(n log n) with sorting. */
EOCPowerLawFit eoc_powerlaw_fit_auto(double* data, int n);

/** Kolmogorov-Smirnov statistic between empirical data and power-law model.
 *  Complexity: O(n log n). */
double eoc_powerlaw_ks_statistic(double* data, int n,
                                  double xmin, double alpha);

/** Bootstrap goodness-of-fit test (Clauset et al. 2009):
 *  1. Fit power-law to data, compute KS statistic D_obs.
 *  2. Generate n_bootstrap synthetic datasets from fitted power-law.
 *  3. For each, fit power-law and compute KS statistic D_sim.
 *  4. p = fraction of D_sim > D_obs.
 *  If p < 0.1, reject power-law hypothesis.
 *  Complexity: O(n_bootstrap * n log n). */
double eoc_powerlaw_bootstrap_pvalue(double* data, int n,
                                      double xmin, double alpha,
                                      int n_bootstrap);

/** Likelihood ratio test: power-law vs. exponential (null).
 *  R = L_powerlaw / L_exponential. log(R) > 0 favors power-law.
 *  p-value via Vuong's test.
 *  Complexity: O(n). */
double eoc_powerlaw_vs_exponential(double* data, int n, double xmin,
                                    double* p_value);

/** Likelihood ratio test: power-law vs. log-normal.
 *  Complexity: O(n). */
double eoc_powerlaw_vs_lognormal(double* data, int n, double xmin,
                                  double* p_value);

/** Likelihood ratio test: power-law vs. stretched exponential (Weibull).
 *  Complexity: O(n). */
double eoc_powerlaw_vs_stretched_exponential(double* data, int n,
                                              double xmin, double* p_value);

/** Likelihood ratio test: power-law vs. power-law with exponential cutoff.
 *  P(x) ~ x^{-alpha} * exp(-lambda*x).
 *  Complexity: O(n). */
double eoc_powerlaw_vs_truncated_powerlaw(double* data, int n,
                                            double xmin, double* p_value);

/** Generate synthetic data from continuous power-law using inverse transform
 *  sampling: x = x_min * (1 - u)^{-1/(alpha-1)} for u ~ Uniform(0,1).
 *  Complexity: O(n_samples). */
double* eoc_powerlaw_generate_continuous(int n_samples, double xmin,
                                          double alpha);

/** Generate synthetic data from discrete power-law (Zipf).
 *  Uses rejection sampling with zeta normalization.
 *  Complexity: O(n_samples * expected(iterations)). */
int* eoc_powerlaw_generate_discrete(int n_samples, int xmin, double alpha);

/** Hurwitz zeta function: zeta(s, q) = sum_{k=0}^{inf} (q+k)^{-s}.
 *  Used for discrete power-law normalization.
 *  Implements Euler-Maclaurin approximation for efficiency.
 *  Complexity: O(1) approximation. */
double eoc_hurwitz_zeta(double s, double q);

/** Compute cumulative distribution function for continuous power-law:
 *  P(X <= x) = 1 - (x / x_min)^{-(alpha - 1)} for x >= x_min.
 *  Complexity: O(1). */
double eoc_powerlaw_cdf_continuous(double x, double xmin, double alpha);

/** Compute probability density for continuous power-law.
 *  Complexity: O(1). */
double eoc_powerlaw_pdf_continuous(double x, double xmin, double alpha);

/** Create log-binned histogram suitable for power-law visualization.
 *  Bin edges: b_i = b_0 * ratio^i (logarithmic spacing).
 *  Complexity: O(n log n). */
void eoc_log_binned_histogram(double* data, int n, double b0, double ratio,
                               int n_bins, double* bin_edges, double* counts);

#endif /* EOC_POWERLAW_H */
