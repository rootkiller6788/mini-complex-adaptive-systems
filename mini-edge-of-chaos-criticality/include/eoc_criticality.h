#ifndef EOC_CRITICALITY_H
#define EOC_CRITICALITY_H

#include "eoc_core.h"

/* ============================================================================
 * Critical Phenomena ? Scaling, Universality, Renormalization
 *
 * The mathematical framework for understanding criticality:
 *   - Scaling relations between critical exponents
 *   - Universality: exponents depend only on symmetry, dimension, range
 *   - Renormalization group (RG): coarse-graining and fixed points
 *   - Finite-size scaling: how observables scale with system size L
 *
 * References:
 *   Stanley, H.E. (1987). Introduction to Phase Transitions and Critical
 *     Phenomena. Oxford University Press.
 *   Cardy, J. (1996). Scaling and Renormalization in Statistical Physics.
 *     Cambridge University Press.
 *   Goldenfeld, N. (1992). Lectures on Phase Transitions and the
 *     Renormalization Group. Addison-Wesley.
 *   Wilson, K.G. (1971). Phys. Rev. B, 4(9), 3174-3205.
 *   Lubeck, S. (2000). Phys. Rev. E, 62(5), 6149-6154.
 * ============================================================================ */

/* --- Scaling Relations --- */

/** Verify Rushbrooke scaling relation: alpha + 2*beta + gamma >= 2.
 *  Returns the discrepancy |alpha + 2*beta + gamma - 2|.
 *  Complexity: O(1). */
double eoc_scaling_rushbrooke(EOCCriticalExponents* ce);

/** Verify Widom scaling relation: gamma = beta * (delta - 1).
 *  Complexity: O(1). */
double eoc_scaling_widom(EOCCriticalExponents* ce);

/** Verify Fisher scaling relation: nu * (2 - eta) = gamma.
 *  Complexity: O(1). */
double eoc_scaling_fisher(EOCCriticalExponents* ce);

/** Verify hyperscaling: d * nu = 2 - alpha (valid for d <= d_c = 4).
 *  Complexity: O(1). */
double eoc_hyperscaling(EOCCriticalExponents* ce, int d);

/** Check all four scaling relations and return the maximum discrepancy.
 *  If all <= tolerance, the exponents are self-consistent.
 *  Complexity: O(1). */
bool eoc_exponents_consistent(EOCCriticalExponents* ce, double tol);

/* --- Finite-Size Scaling --- */

/** Finite-size scaling of the order parameter m(L, t):
 *  m(L, t) = L^{-beta/nu} * M(t * L^{1/nu}) where t = (T - T_c)/T_c.
 *  Data collapse method (Bhattacharjee & Seno 2001).
 *  Complexity: O(n_L * n_T). */
double eoc_fss_data_collapse(double** m_data, double** t_data,
                              int n_L, int* L_values, int n_T,
                              double* beta_over_nu, double* nu_out);

/** Finite-size scaling of susceptibility chi(L, t):
 *  chi_max(L) ~ L^{gamma/nu}, T_chi_max(L) - T_c ~ L^{-1/nu}.
 *  Complexity: O(n_L). */
void eoc_fss_chi_max(double* chi_max, int* L_values, int n_L,
                     double* gamma_over_nu, double* nu_out);

/** Binder cumulant crossing method for locating T_c:
 *  U_L = 1 - <m^4> / (3 * <m^2>^2)
 *  U_L curves for different L intersect at T_c (universal fixed point).
 *  Complexity: O(n_L * n_T). */
double eoc_binder_cumulant(double* m_data, int n_samples);

/** Autocorrelation time for critical slowing down:
 *  tau_corr(L) ~ L^z where z is the dynamical exponent.
 *  Complexity: O(n_L * n_tau). */
void eoc_critical_slowing_down(double** autocorr_data, int* L_values,
                                int n_L, int n_tau, double* z_out);

/* --- Renormalization Group (RG) --- */

/** Real-space RG transformation for 1D Ising model (decimation):
 *  K' = (1/2) * ln(cosh(2K)) where K = J/(k_B T).
 *  Fixed points: K=0 (T=inf), K=inf (T=0).
 *  No phase transition at finite T in 1D (Peierls argument).
 *  Complexity: O(n_iter). */
double eoc_rg_decimation_1d_ising(double K, int n_iter);

/** Real-space RG for 2D Ising model (majority rule block spin):
 *  Coarse-grains 3x3 blocks, computes renormalized coupling.
 *  Includes fixed point at K_c ~ 0.4407 (exact K_c = ln(1+sqrt(2))/2 ~ 0.4407).
 *  Complexity: O(n_iter * L^2). */
double eoc_rg_block_spin_2d_ising(int* spins, int L, int n_iter,
                                   double* K_values);

/** RG eigenvalue analysis: compute y_t = ln(lambda_t) / ln(b)
 *  where lambda_t is the thermal eigenvalue and b is the scaling factor.
 *  Then nu = 1/y_t.
 *  Complexity: O(1). */
double eoc_rg_thermal_exponent(double K_fixed, double K_perturbed,
                                double K_prime_fixed, double K_prime_perturbed,
                                double b);

/** Compute the RG flow diagram (K -> K') for the 1D Ising model.
 *  Returns arrays of K and K' values for visualization.
 *  Complexity: O(n_points). */
void eoc_rg_flow_diagram(double K_min, double K_max, int n_points,
                          double** K_in, double** K_out);

/* --- Universality Class Properties --- */

/** Get known critical exponents for a given universality class.
 *  Returns pre-populated EOCCriticalExponents from literature.
 *  Complexity: O(1). */
EOCCriticalExponents eoc_universality_exponents(UniversalityClass uc);

/** Get the name string for a universality class. */
const char* eoc_universality_name(UniversalityClass uc);

/** Get the upper critical dimension d_c for a universality class.
 *  Above d_c, mean-field exponents apply. */
int eoc_universality_upper_critical_dim(UniversalityClass uc);

/** Get the lower critical dimension d_l below which no phase transition. */
int eoc_universality_lower_critical_dim(UniversalityClass uc);

/* --- Order Parameter and Susceptibility --- */

/** Compute the order parameter (magnetization) from spin configuration.
 *  m = (1/N) * |sum_i s_i|.
 *  Complexity: O(N). */
double eoc_magnetization(int* spins, int N);

/** Compute the magnetic susceptibility via fluctuation-dissipation theorem:
 *  chi = (beta/N) * (<m^2> - <m>^2).
 *  Complexity: O(N). */
double eoc_susceptibility(int* spins, int N, double beta);

/** Compute Binder's fourth-order cumulant U.
 *  Complexity: O(N). */
double eoc_binder_cumulant_from_spins(int* spins, int N);

/** Compute the two-point correlation function G(r):
 *  G(r) = <s_i * s_{i+r}> - <s_i> * <s_{i+r}>
 *  Returns G(r) values for r = 1..max_r.
 *  Complexity: O(max_r * N). */
double* eoc_correlation_function(int* spins, int N, int max_r);

/** Fit correlation length xi from G(r) ~ exp(-r/xi) for large r.
 *  Complexity: O(max_r). */
double eoc_correlation_length(double* G_r, int max_r);

/** Compute the structure factor S(k) = (1/N) * |sum_j s_j * exp(i*k*j)|^2.
 *  Used for identifying periodic order.
 *  Complexity: O(N * n_k). */
void eoc_structure_factor(int* spins, int N,
                          double* k_values, int n_k, double* S_k);

/* --- Phase Transition Detection --- */

/** Detect phase transition from order parameter vs. temperature data.
 *  Uses maximum of chi(T) or d<m>/dT to locate T_c.
 *  Complexity: O(n_T). */
double eoc_detect_transition_temperature(double* T, double* m, int n_T);

/** Compute heat capacity C_v = (<E^2> - <E>^2) / (k_B * T^2).
 *  At critical point: C ~ |t|^{-alpha}.
 *  Complexity: O(n_samples). */
double eoc_heat_capacity(double* energies, int n, double T);

/** Analyze avalanche size distribution for evidence of criticality.
 *  Returns the critical exponent tau from P(s) ~ s^{-tau}.
 *  Complexity: O(n_avalanches log n_avalanches). */
EOCPowerLawFit eoc_critical_avalanche_analysis(double* avalanche_sizes,
                                                 int n);

#endif /* EOC_CRITICALITY_H */
