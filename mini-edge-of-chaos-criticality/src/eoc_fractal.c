#include "eoc_fractal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Fractal Geometry ? Dimension, Scaling, Multifractals
 *
 * L3 Mathematical Structures:
 *   - Box-counting dimension D_0 (capacity dimension)
 *   - Correlation dimension D_2 (Grassberger-Procaccia)
 *   - Generalized Renyi dimensions D_q
 *   - Multifractal spectrum f(alpha)
 *
 * L4 Fundamental Laws:
 *   - Fractal dimension as measure of space-filling capacity
 *   - Multifractal formalism: tau(q) = (q-1)*D_q
 *   - Legendre transform: f(alpha) = q*alpha - tau(q)
 *
 * L5 Algorithms:
 *   - Box-counting via hierarchical grid
 *   - Correlation integral via pairwise distances
 *   - Sandbox method for large datasets
 *   - DFA for time series scaling exponents
 * ============================================================================ */

/* ============================================================================
 * Box-Counting Dimension (Capacity Dimension D_0)
 *
 * N(eps) = number of boxes of side eps needed to cover the set.
 * D_0 = - lim_{eps->0} log N(eps) / log eps
 *
 * For d-dimensional points, we use a grid-based approach with spatial hashing.
 * Complexity: O(n_eps * n_points) with grid bucketing.
 * ============================================================================ */

double eoc_box_counting_dimension(double** points, int n_points, int dim,
                                   double eps_min, double eps_max, int n_eps) {
    if (!points || n_points < 2 || dim < 1 || n_eps < 2) return 0.0;

    double* log_eps = (double*)malloc(n_eps * sizeof(double));
    double* log_N   = (double*)malloc(n_eps * sizeof(double));

    /* Find bounding box */
    double* mins = (double*)malloc(dim * sizeof(double));
    double* maxs = (double*)malloc(dim * sizeof(double));
    for (int d = 0; d < dim; d++) {
        mins[d] = points[0][d];
        maxs[d] = points[0][d];
    }
    for (int i = 1; i < n_points; i++) {
        for (int d = 0; d < dim; d++) {
            if (points[i][d] < mins[d]) mins[d] = points[i][d];
            if (points[i][d] > maxs[d]) maxs[d] = points[i][d];
        }
    }

    for (int e = 0; e < n_eps; e++) {
        double eps = eps_min * pow(eps_max / eps_min, (double)e / (n_eps - 1));

        /* Number of grid cells per dimension */
        int* grid_dims = (int*)malloc(dim * sizeof(int));
        int total_cells = 1;
        for (int d = 0; d < dim; d++) {
            double extent = maxs[d] - mins[d];
            grid_dims[d] = (int)(extent / eps) + 1;
            if (grid_dims[d] < 1) grid_dims[d] = 1;
            total_cells *= grid_dims[d];
        }

        /* Track which cells are occupied using a simple hash set */
        int* occupied = (int*)calloc(total_cells + 1, sizeof(int));
        int n_occupied = 0;

        for (int i = 0; i < n_points; i++) {
            int cell_idx = 0;
            int mult = 1;
            for (int d = 0; d < dim; d++) {
                int c = (int)((points[i][d] - mins[d]) / eps);
                if (c < 0) c = 0;
                if (c >= grid_dims[d]) c = grid_dims[d] - 1;
                cell_idx += c * mult;
                mult *= grid_dims[d];
            }
            if (!occupied[cell_idx]) {
                occupied[cell_idx] = 1;
                n_occupied++;
            }
        }

        log_eps[e] = log(eps);
        log_N[e] = log((double)n_occupied);

        free(grid_dims);
        free(occupied);
    }

    /* Linear regression: log N = -D_0 * log eps + const */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (int e = 0; e < n_eps; e++) {
        sum_x += log_eps[e];
        sum_y += log_N[e];
        sum_xy += log_eps[e] * log_N[e];
        sum_x2 += log_eps[e] * log_eps[e];
    }

    double D_0 = 0.0;
    if (n_eps > 1) {
        D_0 = - (n_eps * sum_xy - sum_x * sum_y) / (n_eps * sum_x2 - sum_x * sum_x);
    }

    free(log_eps);
    free(log_N);
    free(mins);
    free(maxs);

    return D_0;
}

/* ============================================================================
 * Box-Counting for 2D Binary Images
 *
 * Simpler implementation for grid data (binary images).
 * N(L) = number of boxes of size L with at least one occupied pixel.
 * D_0 = -d log N / d log L
 * ============================================================================ */

double eoc_box_counting_2d(int* grid, int width, int height) {
    if (!grid || width < 2 || height < 2) return 0.0;

    int max_L = (width < height ? width : height) / 2;
    if (max_L < 2) max_L = 2;

    int n_scales = 0;
    for (int L = 1; L <= max_L; L *= 2) n_scales++;

    double* log_L = (double*)malloc(n_scales * sizeof(double));
    double* log_N = (double*)malloc(n_scales * sizeof(double));
    int scale_idx = 0;

    for (int L = 1; L <= max_L; L *= 2) {
        int n_boxes_x = width / L;
        int n_boxes_y = height / L;
        int n_occupied = 0;

        for (int by = 0; by < n_boxes_y; by++) {
            for (int bx = 0; bx < n_boxes_x; bx++) {
                bool occupied = false;
                for (int dy = 0; dy < L && !occupied; dy++) {
                    for (int dx = 0; dx < L && !occupied; dx++) {
                        int px = bx * L + dx;
                        int py = by * L + dy;
                        if (py < height && px < width && grid[py * width + px]) {
                            occupied = true;
                        }
                    }
                }
                if (occupied) n_occupied++;
            }
        }

        log_L[scale_idx] = log((double)L);
        log_N[scale_idx] = log((double)n_occupied);
        scale_idx++;
    }

    /* Linear regression */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    int used = 0;
    for (int i = 0; i < n_scales; i++) {
        if (log_N[i] > 0) {
            sum_x += log_L[i];
            sum_y += log_N[i];
            sum_xy += log_L[i] * log_N[i];
            sum_x2 += log_L[i] * log_L[i];
            used++;
        }
    }

    double D_0 = 0.0;
    if (used > 1) {
        D_0 = - (used * sum_xy - sum_x * sum_y) / (used * sum_x2 - sum_x * sum_x);
    }

    free(log_L);
    free(log_N);

    return D_0;
}

/* ============================================================================
 * Correlation Dimension D_2 (Grassberger-Procaccia 1983)
 *
 * C(eps) = (2 / (N*(N-1))) * sum_{i<j} Theta(eps - |x_i - x_j|)
 * where Theta is the Heaviside step function.
 *
 * D_2 = lim_{eps->0} log C(eps) / log eps
 *
 * For chaotic attractors, D_2 is a non-integer (fractal) dimension.
 * Complexity: O(N^2) pairwise distances.
 * ============================================================================ */

static double euclidean_dist_sq(double* a, double* b, int dim) {
    double sum = 0.0;
    for (int d = 0; d < dim; d++) {
        double diff = a[d] - b[d];
        sum += diff * diff;
    }
    return sum;
}

EOCCorrelationIntegral eoc_correlation_dimension(double** points, int n_points,
                                                   int dim, double eps_min,
                                                   double eps_max, int n_eps) {
    EOCCorrelationIntegral result;
    memset(&result, 0, sizeof(result));

    if (!points || n_points < 10 || dim < 1 || n_eps < 2) return result;

    /* Find bounding box for eps range */
    double max_dist = 0.0;
    for (int i = 0; i < n_points; i++) {
        for (int j = i + 1; j < n_points; j++) {
            double d = euclidean_dist_sq(points[i], points[j], dim);
            if (d > max_dist) max_dist = d;
        }
    }
    max_dist = sqrt(max_dist);
    if (eps_max > max_dist) eps_max = max_dist * 0.5;

    result.n_radii = n_eps;
    result.radii = (double*)malloc(n_eps * sizeof(double));
    result.c_epsilon = (double*)malloc(n_eps * sizeof(double));

    /* Pre-compute all pairwise distances (for smaller N) or sample */
    int n_pairs = n_points * (n_points - 1) / 2;
    int max_pairs = 100000;
    double* distances = NULL;
    int n_dists = 0;

    if (n_pairs <= max_pairs) {
        distances = (double*)malloc(n_pairs * sizeof(double));
        for (int i = 0; i < n_points; i++) {
            for (int j = i + 1; j < n_points; j++) {
                distances[n_dists++] = euclidean_dist_sq(points[i], points[j], dim);
            }
        }
    } else {
        /* Random sample of pairs */
        n_dists = max_pairs;
        distances = (double*)malloc(n_dists * sizeof(double));
        for (int k = 0; k < n_dists; k++) {
            int i = rand() % n_points;
            int j = rand() % n_points;
            if (i == j) { k--; continue; }
            distances[k] = euclidean_dist_sq(points[i], points[j], dim);
        }
    }

    double* log_eps_array = (double*)malloc(n_eps * sizeof(double));
    double* log_C_array = (double*)malloc(n_eps * sizeof(double));

    for (int e = 0; e < n_eps; e++) {
        double eps = eps_min * pow(eps_max / eps_min, (double)e / (n_eps - 1));
        double eps_sq = eps * eps;

        int count = 0;
        for (int k = 0; k < n_dists; k++) {
            if (distances[k] <= eps_sq) count++;
        }

        double C = (double)count / n_dists;
        result.radii[e] = eps;
        result.c_epsilon[e] = C;

        if (C > 0.0) {
            log_eps_array[e] = log(eps);
            log_C_array[e] = log(C);
        } else {
            log_eps_array[e] = log(eps);
            log_C_array[e] = -30.0; /* Close to -inf */
        }
    }

    /* Linear regression: log C = D_2 * log eps + const */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    int n_used = 0;
    for (int e = 0; e < n_eps; e++) {
        if (result.c_epsilon[e] > 0.0 && result.c_epsilon[e] < 1.0) {
            sum_x += log_eps_array[e];
            sum_y += log_C_array[e];
            sum_xy += log_eps_array[e] * log_C_array[e];
            sum_x2 += log_eps_array[e] * log_eps_array[e];
            n_used++;
        }
    }

    if (n_used > 1) {
        result.dimension = (n_used * sum_xy - sum_x * sum_y) /
                           (n_used * sum_x2 - sum_x * sum_x);

        /* R-squared */
        double y_mean = sum_y / n_used;
        double ss_res = 0, ss_tot = 0;
        for (int e = 0; e < n_eps; e++) {
            if (result.c_epsilon[e] > 0.0 && result.c_epsilon[e] < 1.0) {
                double y_pred = result.dimension * log_eps_array[e] + (sum_y - result.dimension * sum_x) / n_used;
                ss_res += (log_C_array[e] - y_pred) * (log_C_array[e] - y_pred);
                ss_tot += (log_C_array[e] - y_mean) * (log_C_array[e] - y_mean);
            }
        }
        result.r_squared = (ss_tot > 0.0) ? 1.0 - ss_res / ss_tot : 0.0;
    }

    free(distances);
    free(log_eps_array);
    free(log_C_array);

    return result;
}

/* ============================================================================
 * Generalized Dimension D_q
 *
 * C_q(eps) = [ 1/N * sum_i (n_i(eps)/N)^{q-1} ]^{1/(q-1)}
 * D_q = lim_{eps->0} log C_q(eps) / log eps
 *
 * Special cases:
 *   D_0: capacity (box-counting) dimension
 *   D_1: information dimension
 *   D_2: correlation dimension
 * ============================================================================ */

double eoc_generalized_dimension(double** points, int n_points, int dim,
                                  double epsilon, double q) {
    if (!points || n_points < 2) return 0.0;

    double* n_i = (double*)malloc(n_points * sizeof(double));

    /* For each point, count neighbors within epsilon */
    double eps_sq = epsilon * epsilon;
    for (int i = 0; i < n_points; i++) {
        n_i[i] = 0.0;
        for (int j = 0; j < n_points; j++) {
            double dist_sq = 0.0;
            for (int d = 0; d < dim; d++) {
                double diff = points[i][d] - points[j][d];
                dist_sq += diff * diff;
            }
            if (dist_sq <= eps_sq) n_i[i] += 1.0;
        }
    }

    double sum = 0.0;
    if (fabs(q - 1.0) < 1e-10) {
        /* D_1: information dimension */
        for (int i = 0; i < n_points; i++) {
            double p = n_i[i] / n_points;
            if (p > 0.0) sum += p * log(p);
        }
        sum = sum / log(epsilon);
    } else {
        /* D_q for q != 1 */
        double C_q = 0.0;
        for (int i = 0; i < n_points; i++) {
            double p = n_i[i] / n_points;
            if (p > 0.0) C_q += pow(p, q - 1.0);
        }
        C_q /= n_points;
        if (C_q > 0.0) sum = log(C_q) / ((q - 1.0) * log(epsilon));
    }

    free(n_i);
    return sum;
}

/* ============================================================================
 * Multifractal Spectrum f(alpha)
 *
 * tau(q) = lim_{eps->0} log(sum_i p_i^q) / log eps  (mass exponent)
 * alpha(q) = d tau / d q (Holder exponent)
 * f(alpha) = q * alpha - tau(q) (multifractal spectrum)
 *
 * The width of f(alpha) measures the degree of multifractality.
 * ============================================================================ */

EOCMultifractalSpectrum* eoc_multifractal_spectrum(double** points,
                                                     int n_points, int dim,
                                                     double* q_values,
                                                     int n_q) {
    EOCMultifractalSpectrum* ms = eoc_multifractal_alloc(n_q);
    if (!ms) return NULL;

    /* Fixed epsilon for simplicity ? in practice use multiple epsilons and extrapolate */
    /* Find characteristic scale from data span */
    double max_span = 0.0;
    for (int d = 0; d < dim; d++) {
        double dmin = points[0][d], dmax = points[0][d];
        for (int i = 0; i < n_points; i++) {
            if (points[i][d] < dmin) dmin = points[i][d];
            if (points[i][d] > dmax) dmax = points[i][d];
        }
        double span = dmax - dmin;
        if (span > max_span) max_span = span;
    }
    double epsilon = max_span / sqrt((double)n_points);

    /* Compute p_i(box) for each point (local density) */
    double* p_i = (double*)malloc(n_points * sizeof(double));
    double eps_sq = epsilon * epsilon;
    for (int i = 0; i < n_points; i++) {
        p_i[i] = 0.0;
        for (int j = 0; j < n_points; j++) {
            double dist_sq = 0.0;
            for (int d = 0; d < dim; d++) {
                double diff = points[i][d] - points[j][d];
                dist_sq += diff * diff;
            }
            if (dist_sq <= eps_sq) p_i[i] += 1.0;
        }
        p_i[i] /= n_points;
    }

    double log_eps = log(epsilon);

    for (int qi = 0; qi < n_q; qi++) {
        double q = q_values[qi];
        ms->q_values[qi] = q;

        /* tau(q) via partition function */
        double sum_pq = 0.0;
        for (int i = 0; i < n_points; i++) {
            if (p_i[i] > 0.0) sum_pq += pow(p_i[i], q);
        }
        if (sum_pq > 0.0 && fabs(log_eps) > 1e-15) {
            ms->tau_q[qi] = log(sum_pq) / log_eps;
        } else {
            ms->tau_q[qi] = 0.0;
        }
    }

    /* alpha(q) = d(tau)/dq via finite differences */
    for (int qi = 1; qi < n_q - 1; qi++) {
        ms->alpha_q[qi] = (ms->tau_q[qi + 1] - ms->tau_q[qi - 1]) /
                          (q_values[qi + 1] - q_values[qi - 1]);
    }
    if (n_q > 1) {
        ms->alpha_q[0] = (ms->tau_q[1] - ms->tau_q[0]) /
                         (q_values[1] - q_values[0]);
        ms->alpha_q[n_q - 1] = (ms->tau_q[n_q - 1] - ms->tau_q[n_q - 2]) /
                               (q_values[n_q - 1] - q_values[n_q - 2]);
    }

    /* f(alpha) = q * alpha - tau(q) */
    for (int qi = 0; qi < n_q; qi++) {
        ms->f_alpha[qi] = q_values[qi] * ms->alpha_q[qi] - ms->tau_q[qi];
        if (ms->f_alpha[qi] < 0.0) ms->f_alpha[qi] = 0.0;
    }

    /* D_0 = f(alpha_max), D_1 = alpha at q=1, D_2 from specific q=2 */
    double f_max = 0.0;
    for (int qi = 0; qi < n_q; qi++) {
        if (ms->f_alpha[qi] > f_max) f_max = ms->f_alpha[qi];
    }
    ms->D_0 = f_max;

    /* D_1: interpolate alpha at q=1 */
    ms->D_1 = 0.0;
    for (int qi = 0; qi < n_q - 1; qi++) {
        if (q_values[qi] <= 1.0 && q_values[qi + 1] >= 1.0) {
            double frac = (1.0 - q_values[qi]) / (q_values[qi + 1] - q_values[qi]);
            ms->D_1 = ms->alpha_q[qi] + frac * (ms->alpha_q[qi + 1] - ms->alpha_q[qi]);
            break;
        }
    }
    ms->D_2 = eoc_generalized_dimension(points, n_points, dim, epsilon, 2.0);

    free(p_i);
    return ms;
}

/* ============================================================================
 * Information Dimension D_1
 *
 * D_1 = lim_{eps->0} sum_i p_i(eps) * log p_i(eps) / log eps
 * ============================================================================ */

double eoc_information_dimension(double** points, int n_points, int dim,
                                  double eps_min, double eps_max, int n_eps) {
    if (!points || n_points < 2) return 0.0;

    double* log_eps_arr = (double*)malloc(n_eps * sizeof(double));
    double* info_arr = (double*)malloc(n_eps * sizeof(double));

    for (int e = 0; e < n_eps; e++) {
        double eps = eps_min * pow(eps_max / eps_min, (double)e / (n_eps - 1));

        /* Count neighbors for each point */
        double H = 0.0;
        double eps_sq = eps * eps;
        for (int i = 0; i < n_points; i++) {
            double n_i = 0.0;
            for (int j = 0; j < n_points; j++) {
                double dist_sq = 0.0;
                for (int d = 0; d < dim; d++) {
                    double diff = points[i][d] - points[j][d];
                    dist_sq += diff * diff;
                }
                if (dist_sq <= eps_sq) n_i += 1.0;
            }
            double p = n_i / n_points;
            if (p > 0.0) H -= p * log(p);
        }
        H /= n_points;

        log_eps_arr[e] = log(eps);
        info_arr[e] = H;
    }

    /* Linear regression */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (int e = 0; e < n_eps; e++) {
        sum_x += log_eps_arr[e];
        sum_y += info_arr[e];
        sum_xy += log_eps_arr[e] * info_arr[e];
        sum_x2 += log_eps_arr[e] * log_eps_arr[e];
    }

    double D_1 = 0.0;
    if (n_eps > 1) {
        D_1 = -(n_eps * sum_xy - sum_x * sum_y) / (n_eps * sum_x2 - sum_x * sum_x);
    }

    free(log_eps_arr);
    free(info_arr);
    return D_1;
}

/* ============================================================================
 * Higuchi Fractal Dimension (Higuchi 1988)
 *
 * Computes fractal dimension of time series from curve length at different
 * time scales k. Suitable for 1D signals.
 *
 * L(k) ~ k^{-D} => D from slope of log L(k) vs log k.
 * ============================================================================ */

double eoc_higuchi_fractal_dimension(double* time_series, int n, int k_max) {
    if (!time_series || n < 10 || k_max < 2 || k_max > n / 2) return 0.0;

    double* log_k = (double*)malloc(k_max * sizeof(double));
    double* log_L = (double*)malloc(k_max * sizeof(double));

    for (int k = 1; k <= k_max; k++) {
        double L_k = 0.0;

        for (int m = 0; m < k; m++) {
            /* Curve length for this m */
            double L_m = 0.0;
            int count = 0;
            for (int i = m; i + k < n; i += k) {
                L_m += fabs(time_series[i + k] - time_series[i]);
                count++;
            }
            if (count > 0) {
                L_m *= (double)(n - 1) / (count * k); /* Normalization */
                L_k += L_m;
            }
        }

        L_k /= k;
        log_k[k - 1] = log((double)k);
        log_L[k - 1] = log(L_k + 1e-30);
    }

    /* Linear regression */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (int k = 0; k < k_max; k++) {
        sum_x += log_k[k];
        sum_y += log_L[k];
        sum_xy += log_k[k] * log_L[k];
        sum_x2 += log_k[k] * log_k[k];
    }

    double D = 0.0;
    if (k_max > 1) {
        D = -(k_max * sum_xy - sum_x * sum_y) / (k_max * sum_x2 - sum_x * sum_x);
    }

    free(log_k);
    free(log_L);
    return D;
}

/* ============================================================================
 * Detrended Fluctuation Analysis (DFA) ? Peng et al. 1994
 *
 * DFA exponent alpha:
 *   alpha ~ 0.5: uncorrelated white noise
 *   alpha ~ 1.0: 1/f noise (pink noise ? hallmark of criticality)
 *   alpha ~ 1.5: Brownian noise
 *   alpha > 1.0: long-range correlations (persistent)
 *
 * Method:
 * 1. Integrate the signal: y(k) = sum_i [x_i - <x>]
 * 2. For each scale n, divide y(k) into windows of size n
 * 3. In each window, fit a polynomial (typically linear, order 1)
 * 4. Compute fluctuation F(n) = sqrt(mean of squared residuals)
 * 5. F(n) ~ n^alpha => alpha = d log F / d log n
 * ============================================================================ */

double eoc_dfa_exponent(double* time_series, int n, int min_scale, int max_scale) {
    if (!time_series || n < 100) return 0.0;

    /* Compute mean */
    double mean_x = 0.0;
    for (int i = 0; i < n; i++) mean_x += time_series[i];
    mean_x /= n;

    /* Integrated signal */
    double* y = (double*)malloc(n * sizeof(double));
    y[0] = time_series[0] - mean_x;
    for (int i = 1; i < n; i++) {
        y[i] = y[i - 1] + time_series[i] - mean_x;
    }

    /* DFA at multiple scales */
    int n_scales = 0;
    for (int scale = min_scale; scale <= max_scale; scale *= 2) n_scales++;

    double* log_scales = (double*)malloc(n_scales * sizeof(double));
    double* log_F = (double*)malloc(n_scales * sizeof(double));
    int s_idx = 0;

    for (int scale = min_scale; scale <= max_scale; scale *= 2) {
        int n_windows = n / scale;
        if (n_windows < 2) continue;

        double F2_sum = 0.0;
        for (int w = 0; w < n_windows; w++) {
            int start = w * scale;
            (void)(start + scale); /* end = start + scale — used implicitly via scale */

            /* Linear detrending: fit y = a*t + b in window */
            double sum_t = 0, sum_ty = 0, sum_t2 = 0, sum_yw = 0;
            for (int t = 0; t < scale; t++) {
                int idx = start + t;
                double y_val = y[idx];
                sum_t += t;
                sum_ty += t * y_val;
                sum_t2 += t * t;
                sum_yw += y_val;
            }

            double a, b;
            double denom = scale * sum_t2 - sum_t * sum_t;
            if (fabs(denom) > 1e-15) {
                a = (scale * sum_ty - sum_t * sum_yw) / denom;
                b = (sum_yw * sum_t2 - sum_t * sum_ty) / denom;
            } else {
                a = 0.0;
                b = sum_yw / scale;
            }

            /* Variance of residuals */
            double F2_w = 0.0;
            for (int t = 0; t < scale; t++) {
                double trend = a * t + b;
                double res = y[start + t] - trend;
                F2_w += res * res;
            }
            F2_w /= scale;
            F2_sum += F2_w;
        }

        double F = sqrt(F2_sum / n_windows);
        log_scales[s_idx] = log((double)scale);
        log_F[s_idx] = log(F);
        s_idx++;
    }

    /* Linear regression */
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (int i = 0; i < s_idx; i++) {
        sum_x += log_scales[i];
        sum_y += log_F[i];
        sum_xy += log_scales[i] * log_F[i];
        sum_x2 += log_scales[i] * log_scales[i];
    }

    double alpha = 0.5; /* Default: white noise */
    if (s_idx > 1) {
        alpha = (s_idx * sum_xy - sum_x * sum_y) / (s_idx * sum_x2 - sum_x * sum_x);
    }

    free(y);
    free(log_scales);
    free(log_F);

    return alpha;
}
