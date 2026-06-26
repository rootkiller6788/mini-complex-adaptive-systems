#ifndef ACE_DYNAMICS_H
#define ACE_DYNAMICS_H

#include "ace_core.h"

/* ============================================================================
 * Complex Adaptive Systems in Economics ? Dynamics Models (L4-L5)
 *
 * Implements the core dynamic models from W. Brian Arthur's work:
 *   - Polya urn process with increasing returns (Arthur, Ermoliev, Kaniovski 1987)
 *   - Generalized urn with arbitrary reinforcement function
 *   - Adoption dynamics under network externalities
 *   - Path-dependent stochastic processes
 *   - Lock-in detection and characterization
 *
 * Reference: Arthur (1994), "Increasing Returns and Path Dependence in the Economy"
 * Reference: Arthur, Ermoliev, Kaniovski (1987), "Path-dependent processes and the
 *            emergence of macro-structure", European Journal of Operational Research
 * ============================================================================ */

/* ============================================================================
 * L4: Fundamental Theorems ? Urn Model Functions
 * ============================================================================ */

/*
 * Arthur's Polya Urn Theorem (strong law of large numbers for generalized urn):
 * For a generalized Polya process with continuous urn function f: [0,1] -> R+,
 * the proportion X_n of red balls converges almost surely to a fixed point x*
 * where f(x*) = x*, provided f is continuous and f(0) > 0, f(1) < 1.
 * 
 * When f(x) > x for all x, increasing returns dominate and the process
 * converges to one of the boundary points (lock-in).
 */

/* Initialize urn with a specific addition function */
void ace_polya_urn_init_function(ACEPolyaUrn* urn,
                                  double (*addition_func)(double proportion));

/* Standard Arthur urn function: f(p) = p^gamma / (p^gamma + (1-p)^gamma) */
double ace_arthur_urn_function(double proportion, double gamma);

/* Linear reinforcement: f(p) = a*p + b, with a > 0 */
double ace_linear_urn_function(double proportion, double a, double b);

/* S-shaped adoption function: f(p) = 1/(1+exp(-k*(p - p0))) */
double ace_sigmoid_urn_function(double proportion, double k, double p0);

/* Perform one draw from the Polya urn, updating the state */
int ace_polya_urn_draw(ACEPolyaUrn* urn, unsigned int* seed);

/* Run N draws of the Polya urn, recording the path */
void ace_polya_urn_run(ACEPolyaUrn* urn, int n_draws, ACEPathHistory* hist,
                       unsigned int* seed);

/* Compute the current proportion of each color in the urn */
void ace_polya_urn_proportions(const ACEPolyaUrn* urn, double* proportions);

/* ============================================================================
 * L5: Adoption Dynamics and Lock-in
 * ============================================================================ */

/*
 * Arthur's Adoption Dynamics Model (1989):
 * Two technologies A and B compete. Each adopter chooses the technology
 * that gives higher payoff, where payoff depends on:
 *   - Inherent returns r_A, r_B
 *   - Network effects: n_A^gamma, n_B^gamma
 *   - Adoption costs: c_A, c_B
 *
 * Under increasing returns (gamma > 1), the market tips to a single technology.
 * The tipping point depends on the stochastic order of early adopters.
 */

/* Perform one step of Arthur's binary adoption model */
void ace_adoption_step_binary(ACEAdoptionState* state, double r_a, double r_b,
                               double gamma_a, double gamma_b,
                               unsigned int* seed);

/* Multi-technology adoption step with network effects */
void ace_adoption_step_multi(ACEAdoptionState* state,
                              const double* inherent_returns,
                              const double* network_exponents,
                              int n_techs, unsigned int* seed);

/* Compute adoption probability for each technology under increasing returns */
void ace_adoption_probabilities(const ACEAdoptionState* state,
                                 const double* returns, double gamma,
                                 double* probs);

/* Detect lock-in from a path history: a technology has >threshold share
   and has maintained it for >persistence periods */
ACELockState ace_detect_lockin(const ACEPathHistory* hist,
                                double threshold, int persistence);

/* Compute the critical mass (tipping point) for a technology under network effects */
double ace_critical_mass(double gamma, double n_total);

/* ============================================================================
 * L5: Stochastic Process Simulation
 * ============================================================================ */

/* Initialize a stochastic process as a Markov process on a finite state space */
void ace_markov_process_init(ACEStochasticProcess* sp,
                              const double* transition_matrix_flat);

/* Step a general stochastic process forward (Euler-Maruyama scheme) */
void ace_stochastic_step(ACEStochasticProcess* sp, unsigned int* seed);

/* Run a stochastic process for n_steps, recording the path */
void ace_stochastic_run(ACEStochasticProcess* sp, int n_steps,
                         double** path_out, unsigned int* seed);

/* Estimate the drift and diffusion coefficients from a time series */
void ace_estimate_drift_diffusion(const ACETimeSeries* ts,
                                   double* drift_est, double* diffusion_est);

/* Test whether a process is path-dependent by comparing outcomes from
   different initial conditions after long runs */
bool ace_test_path_dependence(ACEStochasticProcess* sp, int n_trials,
                               int steps_per_trial, unsigned int* seed);

/* ============================================================================
 * L5: Network Effects and Tipping
 * ============================================================================ */

/* Schelling's segregation model: compute fraction of neighbors of same type.
   Returns the utility for agent at position idx based on neighborhood composition. */
double ace_schelling_utility(const ACENetwork* net, int idx,
                              const int* agent_types, double threshold);

/* Compute the tipping threshold: the minimum initial adoption share
   that leads to full adoption under network externalities */
double ace_tipping_threshold(const ACENetwork* net, double adoption_benefit,
                              double baseline_utility, int max_iterations,
                              unsigned int* seed);

/* Simulate a cascade on a network: start with initial adopters,
   each agent adopts when fraction of adopting neighbors > threshold */
void ace_network_cascade(ACENetwork* net, bool* adopted,
                          const double* thresholds, unsigned int* seed);

/* Compute the final adoption share after a network cascade */
double ace_cascade_final_share(const ACENetwork* net,
                                const bool* initial_adopters,
                                const double* thresholds,
                                unsigned int* seed);

/* ============================================================================
 * L5: Time Series Analysis
 * ============================================================================ */

/* Append a data point to the time series, updating statistics online */
void ace_timeseries_append(ACETimeSeries* ts, double value);

/* Compute sample mean online (Welford's method) ? updates internal state */
void ace_timeseries_update_mean(ACETimeSeries* ts);

/* Compute sample variance online (Welford's method) ? updates internal state */
void ace_timeseries_update_variance(ACETimeSeries* ts);

/* Compute skewness and kurtosis from current data */
void ace_timeseries_compute_moments(ACETimeSeries* ts);

/* Estimate Hurst exponent using rescaled range (R/S) analysis */
double ace_estimate_hurst(const double* data, int n);

/* Test for stationarity using a simple runs test */
bool ace_test_stationarity(const double* data, int n, double significance);

/* ============================================================================
 * L5: Distribution Analysis
 * ============================================================================ */

/* Compute a histogram from raw data */
void ace_distribution_from_data(ACEDistribution* dist, const double* data, int n);

/* Compute the Gini coefficient from a sorted array of values */
double ace_compute_gini(const double* data, int n);

/* Test for heavy tails: fit a Pareto distribution to the upper tail */
void ace_test_heavy_tail(ACEDistribution* dist, const double* data, int n,
                          double tail_fraction);

/* Compute the Lorenz curve ordinates for inequality analysis */
void ace_lorenz_curve(const double* sorted_data, int n,
                       double* cumulative_pop, double* cumulative_income);

/* ============================================================================
 * L5: Macro State Aggregation
 * ============================================================================ */

/* Compute macro state from agent population */
void ace_compute_macro_state(const ACEAgent* agents, int n_agents,
                              ACEMacroState* ms);

/* Detect whether the system is in a crisis state (Herfindahl > 0.5 & vol > 2*sigma) */
bool ace_detect_crisis(const ACEMacroState* ms, double vol_threshold,
                        double concentration_threshold);

/* Compute the market concentration dynamics from agent wealth distribution */
void ace_market_concentration_dynamics(const ACEAgent* agents, int n_agents,
                                        int n_periods, double* herfindahl_series);

#endif /* ACE_DYNAMICS_H */
