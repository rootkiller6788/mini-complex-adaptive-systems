#include "eoc_criticality.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Critical Phenomena ? Scaling, Universality, Renormalization Group
 *
 * L2 Core Concepts:
 *   - Universality: critical exponents depend only on spatial dimension,
 *     symmetry group, and interaction range ? not microscopic details.
 *   - Scaling: near T_c, thermodynamic quantities scale as power laws
 *     with universal exponents.
 *
 * L4 Fundamental Laws:
 *   - Scaling relations: Rushbrooke (alpha + 2*beta + gamma = 2),
 *     Widom (gamma = beta*(delta-1)), Fisher (nu*(2-eta) = gamma),
 *     Hyperscaling (d*nu = 2 - alpha for d <= 4).
 *   - Renormalization group flow to fixed points (Wilson 1971).
 *   - Finite-size scaling: L replaces correlation length xi as the
 *     diverging scale when L < xi (Fisher & Barber 1972).
 *
 * L5 Algorithms:
 *   - Data collapse for estimating critical exponents
 *   - Binder cumulant crossing for locating T_c
 *   - RG decimation and block-spin transformations
 *   - Structure factor computation for identifying order
 * ============================================================================ */

/* ============================================================================
 * Scaling Relations
 * ============================================================================ */

double eoc_scaling_rushbrooke(EOCCriticalExponents* ce) {
    if (!ce) return 1e10;
    return fabs(ce->alpha + 2.0 * ce->beta_b + ce->gamma - 2.0);
}

double eoc_scaling_widom(EOCCriticalExponents* ce) {
    if (!ce) return 1e10;
    return fabs(ce->gamma - ce->beta_b * (ce->delta - 1.0));
}

double eoc_scaling_fisher(EOCCriticalExponents* ce) {
    if (!ce) return 1e10;
    return fabs(ce->nu * (2.0 - ce->eta) - ce->gamma);
}

double eoc_hyperscaling(EOCCriticalExponents* ce, int d) {
    if (!ce) return 1e10;
    if (d > 4) d = 4; /* Upper critical dimension */
    return fabs((double)d * ce->nu - (2.0 - ce->alpha));
}

bool eoc_exponents_consistent(EOCCriticalExponents* ce, double tol) {
    if (!ce) return false;
    double d1 = eoc_scaling_rushbrooke(ce);
    double d2 = eoc_scaling_widom(ce);
    double d3 = eoc_scaling_fisher(ce);
    double d4 = eoc_hyperscaling(ce, ce->spatial_dimension);
    double max_err = d1;
    if (d2 > max_err) max_err = d2;
    if (d3 > max_err) max_err = d3;
    if (d4 > max_err) max_err = d4;
    return max_err <= tol;
}

/* ============================================================================
 * Finite-Size Scaling ? Data Collapse
 *
 * The order parameter for a finite system of size L at reduced temperature t:
 *   m_L(t) = L^{-beta/nu} * M~(t * L^{1/nu})
 *
 * If we plot m_L(t) * L^{beta/nu} vs t * L^{1/nu} for different L,
 * the curves should collapse onto a single scaling function M~.
 *
 * Data collapse quality is measured by minimizing the spread of the
 * scaled data (Bhattacharjee & Seno 2001).
 * ============================================================================ */

static double collapse_spread(double** m_data, double** t_data,
                               int n_L, int* L_values, int n_T,
                               double beta_over_nu, double nu) {
    /* Scale all data and compute variance across L for each scaled t bin */
    double one_over_nu = 1.0 / nu;

    /* Collect all scaled (x, y) points */
    int total_points = n_L * n_T;
    double* x_scaled = (double*)malloc(total_points * sizeof(double));
    double* y_scaled = (double*)malloc(total_points * sizeof(double));
    int idx = 0;

    for (int l = 0; l < n_L; l++) {
        double L_scale = pow((double)L_values[l], beta_over_nu);
        for (int t = 0; t < n_T; t++) {
            x_scaled[idx] = t_data[l][t] * pow((double)L_values[l], one_over_nu);
            y_scaled[idx] = m_data[l][t] * L_scale;
            idx++;
        }
    }

    /* Sort by x for binning */
    /* Simple bubble sort (small datasets) */
    for (int i = 0; i < total_points - 1; i++) {
        for (int j = i + 1; j < total_points; j++) {
            if (x_scaled[j] < x_scaled[i]) {
                double tmp = x_scaled[i]; x_scaled[i] = x_scaled[j]; x_scaled[j] = tmp;
                tmp = y_scaled[i]; y_scaled[i] = y_scaled[j]; y_scaled[j] = tmp;
            }
        }
    }

    /* Bin and compute variance within bins */
    int n_bins = total_points / n_L;
    if (n_bins < 2) n_bins = 2;

    double total_spread = 0.0;
    int n_used_bins = 0;

    for (int b = 0; b < n_bins; b++) {
        int start = b * total_points / n_bins;
        int end = (b + 1) * total_points / n_bins;
        if (end - start < 2) continue;

        /* Mean within bin */
        double y_mean = 0.0;
        for (int i = start; i < end; i++) y_mean += y_scaled[i];
        y_mean /= (end - start);

        /* Variance within bin */
        double var = 0.0;
        for (int i = start; i < end; i++) {
            double diff = y_scaled[i] - y_mean;
            var += diff * diff;
        }
        var /= (end - start);

        /* Weight by bin size, penalize spread */
        total_spread += var;
        n_used_bins++;
    }

    free(x_scaled);
    free(y_scaled);

    return (n_used_bins > 0) ? total_spread / n_used_bins : 1e100;
}

double eoc_fss_data_collapse(double** m_data, double** t_data,
                              int n_L, int* L_values, int n_T,
                              double* beta_over_nu, double* nu_out) {
    if (!m_data || !t_data || n_L < 2 || n_T < 2) return 1e100;

    /* Grid search for beta/nu and nu */
    double best_spread = 1e100;
    double best_beta_nu = 0.125; /* 2D Ising: beta=1/8, nu=1 -> beta/nu=1/8 */
    double best_nu = 1.0;

    for (int i = 0; i < 10; i++) {
        double beta_nu = 0.02 + 0.02 * i; /* 0.02 to 0.20 */
        for (int j = 0; j < 10; j++) {
            double nu = 0.5 + 0.1 * j; /* 0.5 to 1.4 */
            double spread = collapse_spread(m_data, t_data, n_L, L_values, n_T, beta_nu, nu);
            if (spread < best_spread) {
                best_spread = spread;
                best_beta_nu = beta_nu;
                best_nu = nu;
            }
        }
    }

    *beta_over_nu = best_beta_nu;
    *nu_out = best_nu;
    return best_spread;
}

/* ============================================================================
 * Finite-Size Scaling ? chi_max Analysis
 *
 * For susceptibility chi(L, T):
 *   chi_max(L) ~ L^{gamma/nu}
 *   T_chi_max(L) - T_c ~ L^{-1/nu}
 * ============================================================================ */

void eoc_fss_chi_max(double* chi_max, int* L_values, int n_L,
                     double* gamma_over_nu, double* nu_out) {
    if (!chi_max || !L_values || n_L < 2) {
        *gamma_over_nu = 0.0;
        *nu_out = 1.0;
        return;
    }

    /* log chi_max vs log L: slope = gamma/nu */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (int i = 0; i < n_L; i++) {
        if (chi_max[i] <= 0.0 || L_values[i] <= 0) continue;
        double lx = log((double)L_values[i]);
        double ly = log(chi_max[i]);
        sum_x += lx;
        sum_y += ly;
        sum_xy += lx * ly;
        sum_x2 += lx * lx;
    }

    *gamma_over_nu = (n_L * sum_xy - sum_x * sum_y) / (n_L * sum_x2 - sum_x * sum_x);
    *nu_out = 1.0; /* Needs T_chi_max data for nu separately */
}

/* ============================================================================
 * Binder Cumulant
 *
 * U_L = 1 - <m^4> / (3 * <m^2>^2)
 *
 * At T = T_c, U_L is independent of L (universal fixed point).
 * This allows precise location of T_c by finding where U_L curves
 * for different L intersect.
 * ============================================================================ */

double eoc_binder_cumulant(double* m_data, int n_samples) {
    if (!m_data || n_samples < 10) return 0.0;

    double sum_m2 = 0.0, sum_m4 = 0.0;
    for (int i = 0; i < n_samples; i++) {
        double m2 = m_data[i] * m_data[i];
        sum_m2 += m2;
        sum_m4 += m2 * m2;
    }
    double m2_avg = sum_m2 / n_samples;
    double m4_avg = sum_m4 / n_samples;

    if (m2_avg < 1e-30) return 0.0;
    return 1.0 - m4_avg / (3.0 * m2_avg * m2_avg);
}

/* Note: double definition, we define the actual implementation */
double eoc_binder_cumulant_from_spins(int* spins, int N) {
    if (!spins || N < 1) return 0.0;

    double m = 0.0;
    for (int i = 0; i < N; i++) m += spins[i];
    m /= N;

    /* Binder cumulant from spin configuration */
    double m2 = m * m;
    double m4 = m2 * m2;

    if (fabs(m2) < 1e-30) return 0.0;
    return 1.0 - m4 / (3.0 * m2 * m2);
}

/* ============================================================================
 * Critical Slowing Down
 *
 * Near T_c, the autocorrelation time diverges as tau ~ L^z.
 * The dynamical exponent z controls how long simulations must run.
 *
 * For the 2D Ising model (local dynamics): z ~ 2.166 (Metropolis),
 * z ~ 2.0 (cluster algorithms like Wolff).
 * ============================================================================ */

void eoc_critical_slowing_down(double** autocorr_data, int* L_values,
                                int n_L, int n_tau, double* z_out) {
    if (!autocorr_data || !L_values || n_L < 2) {
        *z_out = 2.0;
        return;
    }

    /* Extract tau_corr for each L: where autocorr drops to 1/e */
    double* tau_L = (double*)malloc(n_L * sizeof(double));
    for (int l = 0; l < n_L; l++) {
        tau_L[l] = (double)n_tau; /* Default */
        for (int t = 1; t < n_tau; t++) {
            if (autocorr_data[l][t] < 1.0 / 2.718281828459045) {
                tau_L[l] = (double)t;
                break;
            }
        }
    }

    /* log tau vs log L: slope = z */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (int l = 0; l < n_L; l++) {
        double lx = log((double)L_values[l]);
        double ly = log(tau_L[l]);
        sum_x += lx;
        sum_y += ly;
        sum_xy += lx * ly;
        sum_x2 += lx * lx;
    }

    *z_out = (n_L * sum_xy - sum_x * sum_y) / (n_L * sum_x2 - sum_x * sum_x);
    free(tau_L);
}

/* ============================================================================
 * Renormalization Group (RG)
 * ============================================================================ */

double eoc_rg_decimation_1d_ising(double K, int n_iter) {
    /* 1D Ising decimation: K' = (1/2) * ln(cosh(2K))
     *
     * Fixed points: K* = 0 (infinite T ? disordered)
     *               K* = infinity (T=0 ? ordered, but no phase transition in 1D)
     *
     * RG flow: K flows to 0 for all finite K, confirming no phase transition.
     */
    double K_current = K;
    for (int i = 0; i < n_iter; i++) {
        double cosh_2K = cosh(2.0 * K_current);
        if (cosh_2K <= 0.0) break;
        K_current = 0.5 * log(cosh_2K);
    }
    return K_current;
}

double eoc_rg_block_spin_2d_ising(int* spins, int L, int n_iter,
                                   double* K_values) {
    if (!spins || L < 3 || !K_values) return 0.0;

    int* current_spins = (int*)malloc(L * L * sizeof(int));
    memcpy(current_spins, spins, L * L * sizeof(int));

    int current_L = L;

    for (int iter = 0; iter < n_iter; iter++) {
        int new_L = current_L / 3;
        if (new_L < 2) break;

        int* new_spins = (int*)calloc(new_L * new_L, sizeof(int));

        /* Majority-rule block-spin transformation on 3x3 blocks */
        for (int by = 0; by < new_L; by++) {
            for (int bx = 0; bx < new_L; bx++) {
                int sum = 0;
                for (int dy = 0; dy < 3; dy++) {
                    for (int dx = 0; dx < 3; dx++) {
                        int px = bx * 3 + dx;
                        int py = by * 3 + dy;
                        if (px < current_L && py < current_L) {
                            sum += current_spins[py * current_L + px];
                        }
                    }
                }
                new_spins[by * new_L + bx] = (sum >= 0) ? 1 : -1;
            }
        }

        /* Estimate effective coupling K from magnetization */
        double m = 0.0;
        for (int i = 0; i < new_L * new_L; i++) m += new_spins[i];
        m /= (new_L * new_L);

        /* For 2D Ising at small m: K_eff ~ atanh(m) */
        double m_abs = fabs(m);
        if (m_abs < 1.0) {
            K_values[iter] = 0.5 * log((1.0 + m_abs) / (1.0 - m_abs));
        } else {
            K_values[iter] = 10.0;
        }

        free(current_spins);
        current_spins = new_spins;
        current_L = new_L;
    }

    free(current_spins);
    return K_values[0]; /* Return initial effective coupling */
}

double eoc_rg_thermal_exponent(double K_fixed, double K_perturbed,
                                double K_prime_fixed, double K_prime_perturbed,
                                double b) {
    /* Linearize RG transformation near fixed point:
     * K' - K* = lambda_t * (K - K*) where lambda_t = b^{y_t}
     * y_t = ln(lambda_t) / ln(b), nu = 1/y_t
     */
    double delta_K = K_perturbed - K_fixed;
    double delta_K_prime = K_prime_perturbed - K_prime_fixed;

    if (fabs(delta_K) < 1e-15 || fabs(b - 1.0) < 1e-10) return 1.0;

    double lambda_t = delta_K_prime / delta_K;
    double y_t = log(lambda_t) / log(b);

    return y_t;
}

void eoc_rg_flow_diagram(double K_min, double K_max, int n_points,
                          double** K_in, double** K_out) {
    *K_in = (double*)malloc(n_points * sizeof(double));
    *K_out = (double*)malloc(n_points * sizeof(double));

    for (int i = 0; i < n_points; i++) {
        double K = K_min + (K_max - K_min) * i / (n_points - 1);
        (*K_in)[i] = K;
        (*K_out)[i] = 0.5 * log(cosh(2.0 * K));
    }
}

/* ============================================================================
 * Universality Class Properties
 * ============================================================================ */

EOCCriticalExponents eoc_universality_exponents(UniversalityClass uc) {
    EOCCriticalExponents ce;
    memset(&ce, 0, sizeof(ce));

    switch (uc) {
    case UNIV_ISING_2D:
        /* 2D Ising (exact Onsager 1944 solution) */
        ce.alpha = 0.0;        /* Logarithmic divergence */
        ce.beta_b = 0.125;     /* = 1/8 */
        ce.gamma = 1.75;       /* = 7/4 */
        ce.delta = 15.0;
        ce.nu = 1.0;
        ce.eta = 0.25;         /* = 1/4 */
        ce.z = 2.166;          /* Metropolis dynamics */
        ce.spatial_dimension = 2;
        break;

    case UNIV_ISING_3D:
        ce.alpha = 0.110;
        ce.beta_b = 0.326;
        ce.gamma = 1.237;
        ce.delta = 4.789;
        ce.nu = 0.630;
        ce.eta = 0.036;
        ce.z = 2.024;
        ce.spatial_dimension = 3;
        break;

    case UNIV_MEAN_FIELD:
        ce.alpha = 0.0;        /* Jump discontinuity */
        ce.beta_b = 0.5;       /* = 1/2 */
        ce.gamma = 1.0;
        ce.delta = 3.0;
        ce.nu = 0.5;           /* = 1/2 */
        ce.eta = 0.0;
        ce.z = 2.0;
        ce.spatial_dimension = 4; /* Upper critical dimension */
        break;

    case UNIV_XY:
        /* 2D XY model ? BKT transition, no true long-range order */
        ce.alpha = -0.01;      /* Negative, non-diverging */
        ce.beta_b = 0.33;
        ce.gamma = 1.32;
        ce.delta = 5.0;        /* Below BKT, diverging */
        ce.nu = 0.67;
        ce.eta = 0.25;         /* At BKT transition */
        ce.z = 2.0;
        ce.spatial_dimension = 2;
        break;

    case UNIV_DIRECTED_PERCOLATION:
        /* DP exponents in 2D (Jensen 1999) */
        ce.beta_b = 0.584;
        ce.nu = 0.733;         /* nu_parallel (correlation time) ~ xi_{||} */
        ce.z = 1.581;          /* nu_parallel / nu_perp */
        ce.spatial_dimension = 2;
        break;

    case UNIV_BTW:
        /* BTW sandpile (2D) ? controversial, exact values debated */
        ce.tau_avalanche = 1.1;
        ce.D_avalanche = 2.7;
        ce.alpha_flicker = 1.0; /* 1/f noise */
        ce.spatial_dimension = 2;
        break;

    case UNIV_MANNA:
        /* Manna sandpile (2D) ? consistent with DP */
        ce.tau_avalanche = 1.28;
        ce.D_avalanche = 2.75;
        ce.spatial_dimension = 2;
        break;

    case UNIV_OSLO:
        /* Oslo ricepile (1D) */
        ce.tau_avalanche = 1.55;
        ce.D_avalanche = 2.25;
        ce.spatial_dimension = 1;
        break;

    case UNIV_PERCOLATION_2D:
        /* 2D random percolation */
        ce.beta_b = 0.139;     /* = 5/36 */
        ce.gamma = 2.389;      /* = 43/18 */
        ce.nu = 1.333;         /* = 4/3 */
        ce.eta = 0.208;        /* = 5/24 */
        ce.spatial_dimension = 2;
        break;

    case UNIV_BRANCHING_PROCESS:
        /* Mean-field branching process (Galton-Watson) */
        ce.tau_avalanche = 1.5; /* = 3/2 */
        ce.D_avalanche = 2.0;
        ce.spatial_dimension = 4; /* Upper critical dimension */
        break;

    default:
        ce.spatial_dimension = 2;
        break;
    }

    return ce;
}

const char* eoc_universality_name(UniversalityClass uc) {
    static const char* names[] = {
        "Directed Percolation",
        "2D Ising",
        "3D Ising",
        "Mean Field",
        "XY (Kosterlitz-Thouless)",
        "q=3 Potts",
        "Manna Sandpile",
        "BTW Sandpile",
        "Oslo Ricepile",
        "2D Percolation",
        "Branching Process"
    };
    if (uc < 0 || uc > UNIV_BRANCHING_PROCESS) return "Unknown";
    return names[uc];
}

int eoc_universality_upper_critical_dim(UniversalityClass uc) {
    switch (uc) {
    case UNIV_ISING_2D:
    case UNIV_ISING_3D:
    case UNIV_XY:
    case UNIV_POTTS_Q3:          return 4;
    case UNIV_DIRECTED_PERCOLATION: return 4;
    case UNIV_BTW:
    case UNIV_MANNA:             return 4;
    case UNIV_OSLO:              return 3;
    case UNIV_PERCOLATION_2D:    return 6;
    case UNIV_MEAN_FIELD:
    case UNIV_BRANCHING_PROCESS: return 4;
    default:                     return 4;
    }
}

int eoc_universality_lower_critical_dim(UniversalityClass uc) {
    switch (uc) {
    case UNIV_ISING_2D:          return 1; /* No transition in 1D */
    case UNIV_ISING_3D:          return 1;
    case UNIV_XY:                return 2; /* No true LRO in 2D */
    case UNIV_MEAN_FIELD:        return 1;
    default:                     return 1;
    }
}

/* ============================================================================
 * Order Parameter and Susceptibility
 * ============================================================================ */

double eoc_magnetization(int* spins, int N) {
    if (!spins || N < 1) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += spins[i];
    return fabs(sum) / N;
}

double eoc_susceptibility(int* spins, int N, double beta) {
    if (!spins || N < 1) return 0.0;
    double sum = 0.0, sum2 = 0.0;
    for (int i = 0; i < N; i++) {
        double s = (double)spins[i];
        sum += s;
        sum2 += s * s;
    }
    double m = sum / N;
    double m2_avg = sum2 / N;
    return beta * N * (m2_avg - m * m);
}

/* ============================================================================
 * Correlation Function
 *
 * G(r) = <s_i * s_{i+r}> - <s_i>^2
 *
 * For 1D spin chain with periodic boundaries.
 * At criticality: G(r) ~ r^{-(d-2+eta)} (power-law decay).
 * Away from criticality: G(r) ~ exp(-r/xi) (exponential decay).
 * ============================================================================ */

double* eoc_correlation_function(int* spins, int N, int max_r) {
    if (!spins || N < 2 || max_r < 1) return NULL;

    double* G = (double*)malloc(max_r * sizeof(double));

    /* Mean magnetization */
    double m = 0.0;
    for (int i = 0; i < N; i++) m += spins[i];
    m /= N;

    for (int r = 0; r < max_r; r++) {
        double corr = 0.0;
        for (int i = 0; i < N; i++) {
            int j = (i + r + 1) % N;
            corr += spins[i] * spins[j];
        }
        corr /= N;
        G[r] = corr - m * m;
    }

    return G;
}

double eoc_correlation_length(double* G_r, int max_r) {
    if (!G_r || max_r < 2) return 0.0;

    /* Fit log |G(r)| = -r/xi + const for r where G(r) > 0 */
    double sum_r = 0.0, sum_logG = 0.0, sum_r_logG = 0.0, sum_r2 = 0.0;
    int n_used = 0;

    for (int r = 0; r < max_r; r++) {
        if (G_r[r] > 1e-30) {
            double lG = log(G_r[r]);
            sum_r += (r + 1);
            sum_logG += lG;
            sum_r_logG += (r + 1) * lG;
            sum_r2 += (r + 1.0) * (r + 1.0);
            n_used++;
        }
    }

    if (n_used < 2) return 1.0;

    /* Slope = -1/xi */
    double slope = (n_used * sum_r_logG - sum_r * sum_logG) /
                   (n_used * sum_r2 - sum_r * sum_r);

    if (slope >= 0.0) return 1e10; /* No exponential decay */
    return -1.0 / slope;
}

/* ============================================================================
 * Structure Factor
 *
 * S(k) = (1/N) * |sum_j s_j * exp(i*k*j)|^2
 *
 * Peaks in S(k) indicate periodic order.
 * For ferromagnet: peak at k=0.
 * For antiferromagnet: peak at k=pi.
 * For spin density wave: peak at incommensurate k.
 * ============================================================================ */

void eoc_structure_factor(int* spins, int N,
                          double* k_values, int n_k, double* S_k) {
    if (!spins || !S_k || N < 2) return;

    for (int ik = 0; ik < n_k; ik++) {
        double k = k_values[ik];
        double sum_re = 0.0, sum_im = 0.0;
        for (int j = 0; j < N; j++) {
            double phase = k * j;
            sum_re += spins[j] * cos(phase);
            sum_im += spins[j] * sin(phase);
        }
        S_k[ik] = (sum_re * sum_re + sum_im * sum_im) / N;
    }
}

/* ============================================================================
 * Phase Transition Detection
 *
 * T_c is located by finding the maximum of chi(T) or the maximum of d<m>/dT.
 * At T_c: chi diverges as L^{gamma/nu} for infinite system.
 * ============================================================================ */

double eoc_detect_transition_temperature(double* T, double* m, int n_T) {
    if (!T || !m || n_T < 5) return 0.0;

    /* Compute numerical derivative d<m>/dT */
    double* dm_dT = (double*)malloc(n_T * sizeof(double));

    for (int i = 1; i < n_T - 1; i++) {
        dm_dT[i] = (m[i + 1] - m[i - 1]) / (T[i + 1] - T[i - 1]);
    }
    dm_dT[0] = (m[1] - m[0]) / (T[1] - T[0]);
    dm_dT[n_T - 1] = (m[n_T - 1] - m[n_T - 2]) / (T[n_T - 1] - T[n_T - 2]);

    /* Find temperature of maximum absolute derivative */
    double max_deriv = 0.0;
    int max_idx = n_T / 2;

    for (int i = 1; i < n_T - 1; i++) {
        double abs_d = fabs(dm_dT[i]);
        if (abs_d > max_deriv) {
            max_deriv = abs_d;
            max_idx = i;
        }
    }

    free(dm_dT);
    return T[max_idx];
}

/* ============================================================================
 * Heat Capacity
 *
 * C_V = (<E^2> - <E>^2) / (k_B T^2)
 *
 * From fluctuation-dissipation theorem.
 * At critical point: C ~ |t|^{-alpha} (diverges for alpha > 0).
 * ============================================================================ */

double eoc_heat_capacity(double* energies, int n, double T) {
    if (!energies || n < 2 || T < 1e-15) return 0.0;

    double sum_E = 0.0, sum_E2 = 0.0;
    for (int i = 0; i < n; i++) {
        double E = energies[i];
        sum_E += E;
        sum_E2 += E * E;
    }
    double E_avg = sum_E / n;
    double E2_avg = sum_E2 / n;

    return (E2_avg - E_avg * E_avg) / (T * T);
}

/* ============================================================================
 * Critical Avalanche Analysis
 *
 * Given a list of avalanche sizes, fit power-law: P(s) ~ s^{-tau}.
 * Returns the fitted exponent tau.
 *
 * This is the primary diagnostic for self-organized criticality:
 * if tau is approximately constant across different observation windows
 * and lies in the range 1.0-2.0, the system is likely in a SOC state.
 * ============================================================================ */

EOCPowerLawFit eoc_critical_avalanche_analysis(double* avalanche_sizes,
                                                 int n) {
    EOCPowerLawFit result;
    memset(&result, 0, sizeof(result));

    if (!avalanche_sizes || n < 20) return result;

    /* Extract non-zero avalanches */
    double* valid = (double*)malloc(n * sizeof(double));
    int n_valid = 0;
    for (int i = 0; i < n; i++) {
        if (avalanche_sizes[i] > 0.0) {
            valid[n_valid++] = avalanche_sizes[i];
        }
    }

    if (n_valid < 10) { free(valid); return result; }

    /* Simple MLE for discrete-like data (treat as continuous) */
    /* Find minimum */
    double s_min = valid[0];
    for (int i = 1; i < n_valid; i++) {
        if (valid[i] < s_min) s_min = valid[i];
    }
    if (s_min < 1.0) s_min = 1.0;

    double sum_log = 0.0;
    int n_above = 0;
    for (int i = 0; i < n_valid; i++) {
        if (valid[i] >= s_min) {
            sum_log += log(valid[i] / s_min);
            n_above++;
        }
    }

    if (n_above >= 5 && sum_log > 1e-10) {
        result.xmin = s_min;
        result.alpha = 1.0 + n_above / sum_log;
        result.alpha_std_error = (result.alpha - 1.0) / sqrt(n_above);
        result.n_samples = n_above;
        result.n_total = n_valid;
        result.is_power_law = (result.alpha > 1.0 && result.alpha < 4.0);
    }

    free(valid);
    return result;
}
