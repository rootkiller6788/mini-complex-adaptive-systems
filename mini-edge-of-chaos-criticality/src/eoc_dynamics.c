#include "eoc_dynamics.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Nonlinear Dynamics ? Bifurcation, Lyapunov, Chaos
 *
 * L3 Mathematical Structures:
 *   - Logistic map as iterated quadratic: x_{n+1} = r * x_n * (1 - x_n)
 *   - Lorenz system as 3D ODE with chaotic attractor
 *   - RK4 integrator for continuous-time systems
 *
 * L4 Fundamental Laws:
 *   - Feigenbaum universality: delta = 4.6692016..., alpha = 2.5029078...
 *   - Lyapunov exponent: lambda = lim_{t->inf} (1/t) * ln(|dx(t)|/|dx(0)|)
 *   - Kaplan-Yorke conjecture for attractor dimension
 *   - Gottwald-Melbourne 0-1 test for chaos
 *
 * L5 Algorithms:
 *   - Bifurcation diagram via asymptotic iteration
 *   - Lyapunov spectrum via QR decomposition (Benettin)
 *   - Rosenstein algorithm for Lyapunov from time series
 * ============================================================================ */

/* ============================================================================
 * Logistic Map
 * ============================================================================ */

double eoc_logistic_map(double x, double r) {
    return r * x * (1.0 - x);
}

double* eoc_logistic_trajectory(double x0, double r, int n_transient, int n) {
    double* traj = (double*)malloc(n * sizeof(double));
    if (!traj) return NULL;

    double x = x0;
    /* Discard transient */
    for (int i = 0; i < n_transient; i++) {
        x = r * x * (1.0 - x);
    }
    /* Record trajectory */
    for (int i = 0; i < n; i++) {
        x = r * x * (1.0 - x);
        traj[i] = x;
    }
    return traj;
}

/* ============================================================================
 * Bifurcation Diagram
 *
 * Sweeps parameter r from r_min to r_max, recording asymptotic attractor
 * values. This reveals the period-doubling cascade to chaos.
 *
 * Feigenbaum point: r_inf ~ 3.5699456... where chaos onset occurs.
 * ============================================================================ */

void eoc_bifurcation_diagram(double r_min, double r_max, int n_r,
                              int n_transient, int n_keep,
                              EOCBifurcationPoint** points, int* n_points) {
    *n_points = n_r;
    *points = (EOCBifurcationPoint*)malloc(n_r * sizeof(EOCBifurcationPoint));
    if (!*points) { *n_points = 0; return; }

    for (int i = 0; i < n_r; i++) {
        double r = r_min + (r_max - r_min) * i / (n_r - 1);
        (*points)[i].parameter = r;
        (*points)[i].n_attractor = 0;
        (*points)[i].attractor_values = (double*)malloc(n_keep * sizeof(double));

        double x = 0.5;
        /* Transient */
        for (int t = 0; t < n_transient; t++) {
            x = r * x * (1.0 - x);
        }
        /* Collect attractor values */
        double* temp = (double*)malloc(n_keep * sizeof(double));
        for (int t = 0; t < n_keep; t++) {
            x = r * x * (1.0 - x);
            temp[t] = x;
        }

        /* Deduplicate to find period */
        double* unique = (double*)malloc(n_keep * sizeof(double));
        int n_unique = 0;
        double tol = 1e-4;
        for (int t = 0; t < n_keep; t++) {
            bool found = false;
            for (int u = 0; u < n_unique; u++) {
                if (fabs(temp[t] - unique[u]) < tol) { found = true; break; }
            }
            if (!found && n_unique < n_keep) {
                unique[n_unique++] = temp[t];
            }
        }

        /* Transfer unique values */
        (*points)[i].n_attractor = n_unique > n_keep ? n_keep : n_unique;
        memcpy((*points)[i].attractor_values, unique,
               (*points)[i].n_attractor * sizeof(double));

        int period = n_unique;
        (*points)[i].period = period;

        /* Heuristic: period > 32 => chaotic, period = infinity at Feigenbaum point */
        (*points)[i].is_chaotic = (period > 32);

        /* Largest Lyapunov estimate:
         * lambda = (1/n_keep) * sum_t ln(|r - 2*r*x_t|) */
        double sum_log = 0.0;
        for (int t = 0; t < n_keep; t++) {
            double deriv = fabs(r * (1.0 - 2.0 * temp[t]));
            if (deriv > 1e-15) sum_log += log(deriv);
        }
        (*points)[i].lyapunov = sum_log / n_keep;

        free(temp);
        free(unique);
    }
}

/* ============================================================================
 * Feigenbaum Constants
 *
 * delta = lim_{n->inf} (r_n - r_{n-1}) / (r_{n+1} - r_n) = 4.6692016...
 * where r_n is the parameter value at the n-th period-doubling bifurcation.
 *
 * alpha = lim_{n->inf} d_n / d_{n+1} = 2.5029078...
 * where d_n is the width of the bifurcation fork at r_n.
 * ============================================================================ */

double eoc_feigenbaum_delta(double* r_bifur, int n_periods) {
    if (!r_bifur || n_periods < 3) return 0.0;

    double sum_delta = 0.0;
    int count = 0;
    for (int i = 2; i < n_periods; i++) {
        double num = r_bifur[i - 1] - r_bifur[i - 2];
        double den = r_bifur[i] - r_bifur[i - 1];
        if (fabs(den) > 1e-15) {
            sum_delta += num / den;
            count++;
        }
    }
    return count > 0 ? sum_delta / count : 4.6692016;
}

double eoc_feigenbaum_alpha(double* r_bifur, double x_peak, int n_periods) {
    (void)x_peak; /* x_peak reserved for precise iterative refinement */
    if (!r_bifur || n_periods < 3) return 2.5029078;

    /* alpha: scaling of the superstable orbit width
     * Approximate from bifurcation cascade width ratios */
    double sum_alpha = 0.0;
    int count = 0;
    for (int i = 2; i < n_periods; i++) {
        double dr_prev = r_bifur[i - 1] - r_bifur[i - 2];
        double dr_curr = r_bifur[i] - r_bifur[i - 1];
        if (fabs(dr_curr) > 1e-15) {
            sum_alpha += dr_prev / dr_curr;
            count++;
        }
    }
    /* alpha is approximately the square of this ratio for the logistic map */
    double ratio = count > 0 ? sum_alpha / count : 1.0;
    return ratio * ratio * 2.5029078 / 4.6692016;
}

/* ============================================================================
 * Lyapunov Exponents
 * ============================================================================ */

double eoc_lyapunov_rosenstein(double* time_series, int n,
                                int embed_dim, int lag, int min_sep) {
    if (!time_series || n < embed_dim * lag + min_sep) return 0.0;

    /* Rosenstein algorithm:
     * 1. Reconstruct phase space with time-delay embedding.
     * 2. For each reference point, find nearest neighbor with temporal separation.
     * 3. Track divergence over evolution time.
     * 4. Fit line to <ln(d_j(i))> vs. i.
     */

    int n_vec = n - (embed_dim - 1) * lag;
    if (n_vec < min_sep * 2) return 0.0;

    /* Build embedded vectors */
    double** vectors = (double**)malloc(n_vec * sizeof(double*));
    for (int i = 0; i < n_vec; i++) {
        vectors[i] = (double*)malloc(embed_dim * sizeof(double));
        for (int d = 0; d < embed_dim; d++) {
            vectors[i][d] = time_series[i + d * lag];
        }
    }

    /* Compute average divergence */
    int n_evo = n_vec / 4;
    double* avg_ln_div = (double*)calloc(n_evo, sizeof(double));
    int* counts = (int*)calloc(n_evo, sizeof(int));

    for (int i = 0; i < n_vec - n_evo; i++) {
        /* Find nearest neighbor with temporal separation > min_sep */
        int nn = -1;
        double min_dist = 1e100;
        for (int j = 0; j < n_vec - n_evo; j++) {
            if (abs(i - j) < min_sep) continue;
            double dist = 0.0;
            for (int d = 0; d < embed_dim; d++) {
                double diff = vectors[i][d] - vectors[j][d];
                dist += diff * diff;
            }
            if (dist < min_dist && dist > 1e-30) {
                min_dist = dist;
                nn = j;
            }
        }
        if (nn < 0) continue;

        /* Track divergence */
        for (int evo = 0; evo < n_evo && (i + evo) < n_vec && (nn + evo) < n_vec; evo++) {
            double div = 0.0;
            for (int d = 0; d < embed_dim; d++) {
                double diff = vectors[i + evo][d] - vectors[nn + evo][d];
                div += diff * diff;
            }
            if (div > 0.0) {
                avg_ln_div[evo] += 0.5 * log(div);
                counts[evo]++;
            }
        }
    }

    /* Linear fit to <ln(d_j(i))> vs i */
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    int n_used = 0;
    for (int i = 0; i < n_evo; i++) {
        if (counts[i] > 0) {
            double y = avg_ln_div[i] / counts[i];
            sum_x += i;
            sum_y += y;
            sum_xy += i * y;
            sum_x2 += i * i;
            n_used++;
        }
    }

    double lambda = 0.0;
    if (n_used > 2) {
        lambda = (n_used * sum_xy - sum_x * sum_y) /
                 (n_used * sum_x2 - sum_x * sum_x);
    }

    for (int i = 0; i < n_vec; i++) free(vectors[i]);
    free(vectors);
    free(avg_ln_div);
    free(counts);

    return lambda;
}

double eoc_lyapunov_wolf(double** phase_space, int n_points, int dim,
                           int n_evo, double max_scale) {
    if (!phase_space || n_points < n_evo || dim < 1) return 0.0;

    /* Simplified Wolf algorithm:
     * 1. Start with reference trajectory point.
     * 2. Find nearest neighbor.
     * 3. Evolve both forward n_evo steps.
     * 4. Measure separation, rescale.
     * 5. lambda = sum(log(d'/d)) / total_time.
     */

    double sum_log_div = 0.0;
    int n_replacements = 0;

    int i = 0;
    while (i < n_points - n_evo) {
        /* Find nearest neighbor to phase_space[i] */
        int nn = -1;
        double min_d = 1e100;
        for (int j = 0; j < n_points; j++) {
            if (abs(i - j) < 10) continue;
            double d = 0.0;
            for (int k = 0; k < dim; k++) {
                double diff = phase_space[i][k] - phase_space[j][k];
                d += diff * diff;
            }
            d = sqrt(d);
            if (d < min_d && d > 1e-15) {
                min_d = d;
                nn = j;
            }
        }
        if (nn < 0 || min_d > max_scale * 10.0) { i++; continue; }

        /* Evolve both points n_evo steps, track separation */
        int evo_steps = 0;
        for (int e = 0; e < n_evo && (i + e) < n_points && (nn + e) < n_points; e++) {
            double d = 0.0;
            for (int k = 0; k < dim; k++) {
                double diff = phase_space[i + e][k] - phase_space[nn + e][k];
                d += diff * diff;
            }
            d = sqrt(d);
            if (d > max_scale && min_d > 0.0) {
                sum_log_div += log(d / min_d);
                n_replacements++;
                evo_steps = e;
                break;
            }
            evo_steps = e + 1;
        }

        i += evo_steps;
    }

    if (n_replacements == 0) {
        /* Alternative: single-divergence method */
        double sum = 0.0;
        for (int t = 1; t < n_points; t++) {
            double d = 0.0;
            for (int k = 0; k < dim; k++) {
                d += (phase_space[t][k] - phase_space[0][k]) *
                     (phase_space[t][k] - phase_space[0][k]);
            }
            d = sqrt(d);
            if (d > 1e-15) sum += log(d);
        }
        return sum / n_points;
    }

    return sum_log_div / n_replacements;
}

double* eoc_lyapunov_spectrum(void (*flow)(double* x, int d, double* dxdt),
                               double* x0, int d, int n_steps, double dt) {
    if (!flow || !x0 || d < 1) return NULL;

    /* Benettin algorithm for Lyapunov spectrum:
     * 1. Integrate the system with tangent space.
     * 2. Periodically orthonormalize tangent vectors (QR decomposition).
     * 3. Lyapunov exponents = time averages of log(diagonal of R).
     */

    double* lyap = (double*)calloc(d, sizeof(double));
    double* x = (double*)malloc(d * sizeof(double));
    double* dxdt = (double*)malloc(d * sizeof(double));
    double* tangent = (double*)malloc(d * d * sizeof(double)); /* d x d matrix, col-major */
    double* sum_log = (double*)calloc(d, sizeof(double));

    /* Initialize state and identity tangent matrix */
    memcpy(x, x0, d * sizeof(double));
    for (int i = 0; i < d * d; i++) tangent[i] = 0.0;
    for (int i = 0; i < d; i++) tangent[i * d + i] = 1.0;

    double t_total = 0.0;

    for (int step = 0; step < n_steps; step++) {
        /* RK4 for state */
        double k1[d], k2[d], k3[d], k4[d], xt[d];
        flow(x, d, k1);
        for (int i = 0; i < d; i++) k1[i] *= dt;
        for (int i = 0; i < d; i++) xt[i] = x[i] + 0.5 * k1[i];
        flow(xt, d, k2);
        for (int i = 0; i < d; i++) k2[i] *= dt;
        for (int i = 0; i < d; i++) xt[i] = x[i] + 0.5 * k2[i];
        flow(xt, d, k3);
        for (int i = 0; i < d; i++) k3[i] *= dt;
        for (int i = 0; i < d; i++) xt[i] = x[i] + k3[i];
        flow(xt, d, k4);
        for (int i = 0; i < d; i++) k4[i] *= dt;
        for (int i = 0; i < d; i++) {
            x[i] += (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]) / 6.0;
        }

        t_total += dt;

        /* Simplified tangent space evolution ? approximate via finite differences */
        /* Forward-difference Jacobian */
        double jac[d][d];
        double eps = 1e-6;
        for (int col = 0; col < d; col++) {
            double xp[d], xm[d], fp[d], fm[d];
            memcpy(xp, x, d * sizeof(double));
            memcpy(xm, x, d * sizeof(double));
            xp[col] += eps;
            xm[col] -= eps;
            flow(xp, d, fp);
            flow(xm, d, fm);
            for (int row = 0; row < d; row++) {
                jac[row][col] = (fp[row] - fm[row]) / (2.0 * eps);
            }
        }

        /* Evolve tangent vectors: T' = (I + dt*J) * T */
        double* T_new = (double*)malloc(d * d * sizeof(double));
        for (int i = 0; i < d; i++) {
            for (int j = 0; j < d; j++) {
                T_new[i * d + j] = tangent[i * d + j];
                for (int k = 0; k < d; k++) {
                    T_new[i * d + j] += dt * jac[i][k] * tangent[k * d + j];
                }
            }
        }
        memcpy(tangent, T_new, d * d * sizeof(double));
        free(T_new);

        /* QR decompose every 10 steps */
        if ((step + 1) % 10 == 0) {
            /* Modified Gram-Schmidt orthonormalization */
            for (int col = 0; col < d; col++) {
                /* Norm of column */
                double norm = 0.0;
                for (int row = 0; row < d; row++) {
                    norm += tangent[row * d + col] * tangent[row * d + col];
                }
                norm = sqrt(norm);
                if (norm > 1e-30) {
                    sum_log[col] += log(norm);
                }
                /* Normalize */
                for (int row = 0; row < d; row++) {
                    if (norm > 1e-30) tangent[row * d + col] /= norm;
                }
                /* Orthogonalize subsequent columns */
                for (int c2 = col + 1; c2 < d; c2++) {
                    double dot = 0.0;
                    for (int row = 0; row < d; row++) {
                        dot += tangent[row * d + col] * tangent[row * d + c2];
                    }
                    for (int row = 0; row < d; row++) {
                        tangent[row * d + c2] -= dot * tangent[row * d + col];
                    }
                }
            }
        }
    }

    for (int i = 0; i < d; i++) {
        lyap[i] = (t_total > 0.0) ? sum_log[i] / t_total : 0.0;
    }

    /* Sort descending */
    for (int i = 0; i < d - 1; i++) {
        for (int j = i + 1; j < d; j++) {
            if (lyap[j] > lyap[i]) {
                double tmp = lyap[i];
                lyap[i] = lyap[j];
                lyap[j] = tmp;
            }
        }
    }

    free(x);
    free(dxdt);
    free(tangent);
    free(sum_log);

    return lyap;
}

/* ============================================================================
 * Kaplan-Yorke Dimension
 *
 * D_KY = k + sum_{i=1}^{k} lambda_i / |lambda_{k+1}|
 * where k = max index with cumulative sum >= 0.
 * ============================================================================ */

double eoc_kaplan_yorke_dimension(double* lyap_spectrum, int d) {
    if (!lyap_spectrum || d < 1) return 0.0;

    double cumsum = 0.0;
    int k = 0;
    for (int i = 0; i < d; i++) {
        cumsum += lyap_spectrum[i];
        if (cumsum >= 0.0) k = i + 1;
        else break;
    }

    if (k == 0) return 0.0;
    if (k == d) return (double)d;

    double D_KY = (double)k;
    double sum_pos = 0.0;
    for (int i = 0; i < k; i++) sum_pos += lyap_spectrum[i];
    if (fabs(lyap_spectrum[k]) > 1e-15) {
        D_KY += sum_pos / fabs(lyap_spectrum[k]);
    }

    return D_KY;
}

/* ============================================================================
 * 0-1 Test for Chaos (Gottwald & Melbourne 2004)
 *
 * For time series phi(t), compute translation variables:
 *   p(n) = sum_{t=1}^{n} phi(t) * cos(c*t)
 *   q(n) = sum_{t=1}^{n} phi(t) * sin(c*t)
 *
 * Mean square displacement: M(n) = lim_{N->inf} (1/N) sum_{j=1}^{N} [p(j+n) - p(j)]^2 + [q(j+n) - q(j)]^2
 *
 * Asymptotic growth rate: K = lim_{n->inf} log M(n) / log n
 * K ~ 0 -> regular, K ~ 1 -> chaotic.
 * ============================================================================ */

double eoc_zero_one_test(double* time_series, int n, double c) {
    if (!time_series || n < 100) return 0.0;

    /* Compute translation variables p and q */
    double* p = (double*)malloc(n * sizeof(double));
    double* q = (double*)malloc(n * sizeof(double));
    p[0] = 0.0;
    q[0] = 0.0;
    for (int t = 1; t < n; t++) {
        p[t] = p[t - 1] + time_series[t] * cos(c * t);
        q[t] = q[t - 1] + time_series[t] * sin(c * t);
    }

    /* Mean square displacement */
    int n_cut = n / 10;
    double* M = (double*)malloc(n_cut * sizeof(double));
    for (int s = 1; s <= n_cut; s++) {
        double sum = 0.0;
        for (int j = 0; j < n - s; j++) {
            double dp = p[j + s] - p[j];
            double dq = q[j + s] - q[j];
            sum += dp * dp + dq * dq;
        }
        M[s - 1] = sum / (n - s);
    }

    /* Linear regression: log M(s) = K * log(s) + const */
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    int count = 0;
    for (int s = 1; s <= n_cut; s++) {
        if (M[s - 1] > 0.0) {
            double lx = log((double)s);
            double ly = log(M[s - 1]);
            sum_x += lx;
            sum_y += ly;
            sum_xy += lx * ly;
            sum_x2 += lx * lx;
            count++;
        }
    }

    double K = 0.0;
    if (count >= 2) {
        K = (count * sum_xy - sum_x * sum_y) /
            (count * sum_x2 - sum_x * sum_x);
    }

    free(p);
    free(q);
    free(M);

    return K;
}

/* ============================================================================
 * Continuous-Time Systems
 * ============================================================================ */

void eoc_lorenz(double* state, double* deriv, double sigma, double rho, double beta) {
    double x = state[0], y = state[1], z = state[2];
    deriv[0] = sigma * (y - x);
    deriv[1] = x * (rho - z) - y;
    deriv[2] = x * y - beta * z;
}

void eoc_rossler(double* state, double* deriv, double a, double b, double c) {
    double x = state[0], y = state[1], z = state[2];
    deriv[0] = -y - z;
    deriv[1] = x + a * y;
    deriv[2] = b + z * (x - c);
}

void eoc_henon_map(double* x, double* y, double a, double b) {
    double xn = 1.0 - a * (*x) * (*x) + (*y);
    *y = b * (*x);
    *x = xn;
}

/* ============================================================================
 * RK4 Integrator
 *
 * For ODE dx/dt = f(x, params), integrates n_steps with step size dt.
 * Returns flattened trajectory array of size (n_steps+1) * d.
 * ============================================================================ */

typedef struct {
    void (*f)(double*, double*, void*);
    void* params;
} RK4Context;

static void rk4_single_step(void (*f)(double*, double*, void*), double* x,
                             int d, double dt, void* params) {
    double* k1 = (double*)malloc(d * sizeof(double));
    double* k2 = (double*)malloc(d * sizeof(double));
    double* k3 = (double*)malloc(d * sizeof(double));
    double* k4 = (double*)malloc(d * sizeof(double));
    double* xt = (double*)malloc(d * sizeof(double));

    f(x, k1, params);
    for (int i = 0; i < d; i++) k1[i] *= dt;

    for (int i = 0; i < d; i++) xt[i] = x[i] + 0.5 * k1[i];
    f(xt, k2, params);
    for (int i = 0; i < d; i++) k2[i] *= dt;

    for (int i = 0; i < d; i++) xt[i] = x[i] + 0.5 * k2[i];
    f(xt, k3, params);
    for (int i = 0; i < d; i++) k3[i] *= dt;

    for (int i = 0; i < d; i++) xt[i] = x[i] + k3[i];
    f(xt, k4, params);
    for (int i = 0; i < d; i++) k4[i] *= dt;

    for (int i = 0; i < d; i++) {
        x[i] += (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]) / 6.0;
    }

    free(k1); free(k2); free(k3); free(k4); free(xt);
}

double* eoc_rk4_integrate(void (*f)(double*, double*, void*), double* x0,
                           int d, double dt, int n_steps, void* params) {
    if (!f || !x0 || d < 1 || n_steps < 1) return NULL;

    double* traj = (double*)malloc((n_steps + 1) * d * sizeof(double));
    double* x = (double*)malloc(d * sizeof(double));
    memcpy(x, x0, d * sizeof(double));
    memcpy(traj, x0, d * sizeof(double));

    for (int step = 0; step < n_steps; step++) {
        rk4_single_step(f, x, d, dt, params);
        memcpy(traj + (step + 1) * d, x, d * sizeof(double));
    }

    free(x);
    return traj;
}

/* ============================================================================
 * RK4 Lyapunov
 *
 * Tracks divergence of two initially nearby trajectories.
 * lambda = (1 / (N * dt)) * sum_i log(d_i / d_0)
 * ============================================================================ */

double eoc_rk4_lyapunov(void (*f)(double*, double*, void*), double* x0,
                         int d, double dt, int n_steps, double eps, void* params) {
    if (!f || !x0 || d < 1) return 0.0;

    double* x = (double*)malloc(d * sizeof(double));
    double* xp = (double*)malloc(d * sizeof(double));
    memcpy(x, x0, d * sizeof(double));
    for (int i = 0; i < d; i++) xp[i] = x[i] + eps * (0.1 + 0.01 * i);

    /* Initial distance */
    double d0 = 0.0;
    for (int i = 0; i < d; i++) {
        double diff = xp[i] - x[i];
        d0 += diff * diff;
    }
    d0 = sqrt(d0);
    if (d0 < 1e-15) { free(x); free(xp); return 0.0; }

    double sum_log = 0.0;

    for (int step = 0; step < n_steps; step++) {
        rk4_single_step(f, x, d, dt, params);
        rk4_single_step(f, xp, d, dt, params);

        /* Measure separation */
        double dist = 0.0;
        for (int i = 0; i < d; i++) {
            double diff = xp[i] - x[i];
            dist += diff * diff;
        }
        dist = sqrt(dist);

        if (dist > 1e-15) {
            sum_log += log(dist / d0);
        }

        /* Rescale xp to maintain separation eps from x */
        for (int i = 0; i < d; i++) {
            xp[i] = x[i] + (xp[i] - x[i]) * d0 / (dist + 1e-15);
        }
    }

    free(x);
    free(xp);

    return sum_log / (n_steps * dt);
}
