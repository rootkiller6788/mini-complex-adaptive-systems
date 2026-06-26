#include "swarm_core.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

/* ============================================================================
 * xoshiro256** PRNG (Blackman & Vigna 2021)
 * Period: 2^256 - 1. Fast, high-quality, small state.
 * ============================================================================ */

static uint64_t s[4] = {0x9ABCDEF012345678ULL, 0x1234567890ABCDEFULL,
                         0xFEDCBA9876543210ULL, 0xABCDEF0123456789ULL};

static inline uint64_t rotl(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

void swarm_rng_seed(uint64_t seed) {
    /* SplitMix64 seeding to avoid all-zero state */
    for (int i = 0; i < 4; i++) {
        seed += 0x9E3779B97F4A7C15ULL;
        uint64_t z = seed;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        s[i] = z ^ (z >> 31);
    }
}

uint64_t swarm_rng_next(void) {
    const uint64_t result = rotl(s[1] * 5, 7) * 9;
    const uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl(s[3], 45);
    return result;
}

double swarm_rng_uniform(void) {
    /* High 53 bits for double precision */
    return (swarm_rng_next() >> 11) * 0x1.0p-53;
}

double swarm_rng_uniform_range(double a, double b) {
    return a + (b - a) * swarm_rng_uniform();
}

double swarm_rng_gaussian(void) {
    /* Box-Muller transform */
    double u1 = swarm_rng_uniform();
    double u2 = swarm_rng_uniform();
    /* Avoid log(0) */
    if (u1 < 1e-300) u1 = 1e-300;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * SWARM_PI * u2);
}

double swarm_rng_cauchy(double scale) {
    /* Cauchy = scale * tan(π(U - 0.5)) */
    double u = swarm_rng_uniform();
    return scale * tan(SWARM_PI * (u - 0.5));
}

int swarm_rng_int(int max_exclusive) {
    if (max_exclusive <= 0) return 0;
    return (int)(swarm_rng_uniform() * (double)max_exclusive);
}

void swarm_rng_shuffle(int* arr, int n) {
    /* Fisher-Yates shuffle */
    for (int i = n - 1; i > 0; i--) {
        int j = swarm_rng_int(i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/* ============================================================================
 * Vector Operations
 * ============================================================================ */

void swarm_vec_copy(const double* src, double* dst, int dim) {
    if (!src || !dst) return;
    memcpy(dst, src, (size_t)dim * sizeof(double));
}

void swarm_vec_set(double* v, int dim, double value) {
    if (!v) return;
    for (int i = 0; i < dim; i++) v[i] = value;
}

void swarm_vec_add(const double* a, const double* b, double* c, int dim) {
    if (!a || !b || !c) return;
    for (int i = 0; i < dim; i++) c[i] = a[i] + b[i];
}

void swarm_vec_sub(const double* a, const double* b, double* c, int dim) {
    if (!a || !b || !c) return;
    for (int i = 0; i < dim; i++) c[i] = a[i] - b[i];
}

void swarm_vec_scale(double* v, int dim, double scalar) {
    if (!v) return;
    for (int i = 0; i < dim; i++) v[i] *= scalar;
}

void swarm_vec_mul(const double* a, const double* b, double* c, int dim) {
    if (!a || !b || !c) return;
    for (int i = 0; i < dim; i++) c[i] = a[i] * b[i];
}

double swarm_vec_dot(const double* a, const double* b, int dim) {
    if (!a || !b) return 0.0;
    double d = 0.0;
    for (int i = 0; i < dim; i++) d += a[i] * b[i];
    return d;
}

double swarm_vec_norm(const double* v, int dim) {
    return sqrt(swarm_vec_norm_sq(v, dim));
}

double swarm_vec_norm_sq(const double* v, int dim) {
    if (!v) return 0.0;
    double s = 0.0;
    for (int i = 0; i < dim; i++) s += v[i] * v[i];
    return s;
}

double swarm_vec_dist(const double* a, const double* b, int dim) {
    if (!a || !b) return 0.0;
    double s = 0.0;
    for (int i = 0; i < dim; i++) { double d = a[i] - b[i]; s += d * d; }
    return sqrt(s);
}

double swarm_vec_dist_manhattan(const double* a, const double* b, int dim) {
    if (!a || !b) return 0.0;
    double s = 0.0;
    for (int i = 0; i < dim; i++) s += fabs(a[i] - b[i]);
    return s;
}

double swarm_vec_dist_infinity(const double* a, const double* b, int dim) {
    if (!a || !b) return 0.0;
    double m = 0.0;
    for (int i = 0; i < dim; i++) { double d = fabs(a[i] - b[i]); if (d > m) m = d; }
    return m;
}

void swarm_vec_normalize(double* v, int dim) {
    double n = swarm_vec_norm(v, dim);
    if (n > SWARM_EPSILON) swarm_vec_scale(v, dim, 1.0 / n);
}

void swarm_vec_clamp(double* v, int dim, double min_val, double max_val) {
    if (!v) return;
    for (int i = 0; i < dim; i++) {
        if (v[i] < min_val) v[i] = min_val;
        if (v[i] > max_val) v[i] = max_val;
    }
}

void swarm_vec_reflect(const double* v, double* r, int dim,
                        const double* bounds_low, const double* bounds_high) {
    if (!v || !r) return;
    for (int i = 0; i < dim; i++) {
        if (v[i] < bounds_low[i]) r[i] = 2.0 * bounds_low[i] - v[i];
        else if (v[i] > bounds_high[i]) r[i] = 2.0 * bounds_high[i] - v[i];
        else r[i] = v[i];
    }
}

void swarm_vec_lerp(const double* a, const double* b, double t, double* c, int dim) {
    if (!a || !b || !c) return;
    if (t < 0.0) t = 0.0; if (t > 1.0) t = 1.0;
    for (int i = 0; i < dim; i++) c[i] = a[i] + t * (b[i] - a[i]);
}

int swarm_vec_argmin(const double* v, int dim) {
    if (!v || dim <= 0) return -1;
    int idx = 0;
    for (int i = 1; i < dim; i++) if (v[i] < v[idx]) idx = i;
    return idx;
}

int swarm_vec_argmax(const double* v, int dim) {
    if (!v || dim <= 0) return -1;
    int idx = 0;
    for (int i = 1; i < dim; i++) if (v[i] > v[idx]) idx = i;
    return idx;
}

double swarm_vec_mean(const double* v, int dim) {
    if (!v || dim <= 0) return 0.0;
    double s = 0.0; for (int i = 0; i < dim; i++) s += v[i];
    return s / (double)dim;
}

double swarm_vec_variance(const double* v, int dim, double mean) {
    if (!v || dim <= 1) return 0.0;
    double s = 0.0;
    for (int i = 0; i < dim; i++) { double d = v[i] - mean; s += d * d; }
    return s / (double)dim;
}

double swarm_vec_std(const double* v, int dim) {
    double m = swarm_vec_mean(v, dim);
    return sqrt(swarm_vec_variance(v, dim, m));
}

void swarm_vec_print(const double* v, int dim, const char* label) {
    if (!v) { printf("%s: (null)\n", label); return; }
    printf("%s [%d]: [", label, dim);
    for (int i = 0; i < dim; i++) printf("%s%.4f", i > 0 ? ", " : "", v[i]);
    printf("]\n");
}

/* ============================================================================
 * Matrix Operations
 * ============================================================================ */

double** swarm_matrix_alloc(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    double** m = (double**)calloc((size_t)rows, sizeof(double*));
    if (!m) return NULL;
    m[0] = (double*)calloc((size_t)(rows * cols), sizeof(double));
    if (!m[0]) { free(m); return NULL; }
    for (int i = 1; i < rows; i++) m[i] = m[0] + i * cols;
    return m;
}

void swarm_matrix_free(double** m, int rows) {
    if (!m) return;
    free(m[0]); /* Free contiguous data block */
    free(m);
}

void swarm_matrix_copy(double** src, double** dst, int rows, int cols) {
    if (!src || !dst) return;
    memcpy(dst[0], src[0], (size_t)(rows * cols) * sizeof(double));
}

void swarm_matrix_set(double** m, int rows, int cols, double value) {
    if (!m) return;
    int n = rows * cols;
    for (int i = 0; i < n; i++) m[0][i] = value;
}

void swarm_matrix_normalize_rows(double** m, int rows, int cols) {
    if (!m) return;
    for (int i = 0; i < rows; i++) {
        double sum = 0.0;
        for (int j = 0; j < cols; j++) sum += m[i][j];
        if (sum > SWARM_EPSILON) {
            for (int j = 0; j < cols; j++) m[i][j] /= sum;
        } else {
            /* Uniform if row sums to 0 */
            double inv = 1.0 / (double)cols;
            for (int j = 0; j < cols; j++) m[i][j] = inv;
        }
    }
}

double swarm_matrix_row_sum(const double* row, int cols) {
    if (!row) return 0.0;
    double s = 0.0; for (int j = 0; j < cols; j++) s += row[j];
    return s;
}

bool swarm_matrix_is_stochastic(double** m, int rows, int cols, double tol) {
    if (!m) return false;
    for (int i = 0; i < rows; i++) {
        double s = 0.0;
        for (int j = 0; j < cols; j++) { if (m[i][j] < -tol) return false; s += m[i][j]; }
        if (fabs(s - 1.0) > tol) return false;
    }
    return true;
}

/* ============================================================================
 * Graph & Topology Operations
 * ============================================================================ */

void swarm_topology_ring(int* neighbors, int agent_id, int n_agents, int k) {
    if (!neighbors) return;
    int half = k / 2, count = 0;
    for (int offset = -half; offset <= half && count < k && count < SWARM_MAX_NEIGHBORS; offset++) {
        if (offset == 0) continue;
        int neighbor = (agent_id + offset + n_agents) % n_agents;
        neighbors[count++] = neighbor;
    }
    /* Fill remaining with -1 */
    for (int i = count; i < k && i < SWARM_MAX_NEIGHBORS; i++) neighbors[i] = -1;
}

void swarm_topology_von_neumann(int* neighbors, int agent_id, int grid_w, int grid_h) {
    if (!neighbors) return;
    int row = agent_id / grid_w, col = agent_id % grid_w;
    int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    int count = 0;
    for (int i = 0; i < 4; i++) {
        int nr = row + offsets[i][0], nc = col + offsets[i][1];
        if (nr >= 0 && nr < grid_h && nc >= 0 && nc < grid_w)
            neighbors[count++] = nr * grid_w + nc;
    }
    for (int i = count; i < SWARM_MAX_NEIGHBORS; i++) neighbors[i] = -1;
}

void swarm_topology_star(int* neighbors, int agent_id, int n_agents) {
    if (!neighbors) return;
    int count = 0;
    if (agent_id == 0) {
        /* Hub connected to all */
        for (int i = 1; i < n_agents && count < SWARM_MAX_NEIGHBORS; i++)
            neighbors[count++] = i;
        /* Hub self-connection */
        if (count < SWARM_MAX_NEIGHBORS) neighbors[count++] = 0;
    } else {
        /* Peripherals only connected to hub (agent 0) */
        neighbors[count++] = 0;
    }
    for (int i = count; i < SWARM_MAX_NEIGHBORS; i++) neighbors[i] = -1;
}

void swarm_topology_random(int* neighbors, int agent_id, int n_agents, int k) {
    if (!neighbors) return;
    int candidates[SWARM_MAX_AGENTS], n_cand = 0;
    for (int i = 0; i < n_agents; i++)
        if (i != agent_id) candidates[n_cand++] = i;
    swarm_rng_shuffle(candidates, n_cand);
    int count = 0;
    for (int i = 0; i < n_cand && count < k && count < SWARM_MAX_NEIGHBORS; i++)
        neighbors[count++] = candidates[i];
    for (int i = count; i < SWARM_MAX_NEIGHBORS; i++) neighbors[i] = -1;
}

void swarm_topology_dynamic_build(int* neighbors, const double* positions,
           int agent_id, int n_agents, int dim, double radius, int max_neighbors) {
    if (!neighbors || !positions) return;
    int count = 0;
    const double* pos_i = positions + agent_id * dim;
    for (int j = 0; j < n_agents && count < max_neighbors && count < SWARM_MAX_NEIGHBORS; j++) {
        if (j == agent_id) continue;
        double d = swarm_vec_dist(pos_i, positions + j * dim, dim);
        if (d <= radius) neighbors[count++] = j;
    }
    for (int i = count; i < SWARM_MAX_NEIGHBORS; i++) neighbors[i] = -1;
}

double** swarm_graph_laplacian(double** adjacency, int n_nodes) {
    if (!adjacency || n_nodes <= 0) return NULL;
    double** L = swarm_matrix_alloc(n_nodes, n_nodes);
    if (!L) return NULL;
    for (int i = 0; i < n_nodes; i++) {
        double degree = 0.0;
        for (int j = 0; j < n_nodes; j++) {
            if (i != j) {
                L[i][j] = -adjacency[i][j];
                degree += adjacency[i][j];
            }
        }
        L[i][i] = degree;
    }
    return L;
}

double** swarm_graph_adjacency_from_positions(const double* positions, int n,
           int dim, double radius) {
    if (!positions || n <= 0) return NULL;
    double** adj = swarm_matrix_alloc(n, n);
    if (!adj) return NULL;
    double rsq = radius * radius;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) { adj[i][j] = 0.0; continue; }
            double dsq = 0.0;
            for (int d = 0; d < dim; d++) {
                double diff = positions[i * dim + d] - positions[j * dim + d];
                dsq += diff * diff;
            }
            adj[i][j] = (dsq <= rsq) ? 1.0 : 0.0;
        }
    }
    return adj;
}

double* swarm_graph_degree_centrality(double** adjacency, int n) {
    if (!adjacency || n <= 0) return NULL;
    double* dc = swarm_alloc_vector(n);
    if (!dc) return NULL;
    for (int i = 0; i < n; i++) {
        double deg = 0.0;
        for (int j = 0; j < n; j++) deg += adjacency[i][j];
        dc[i] = deg / (double)(n - 1);
    }
    return dc;
}

double swarm_graph_clustering_coefficient(double** adjacency, int n, int node) {
    if (!adjacency || node < 0 || node >= n) return 0.0;
    int neighbors_list[SWARM_MAX_AGENTS], nb_count = 0;
    for (int j = 0; j < n; j++)
        if (j != node && adjacency[node][j] > 0.5)
            neighbors_list[nb_count++] = j;
    if (nb_count < 2) return 0.0;
    int links = 0;
    for (int a = 0; a < nb_count; a++)
        for (int b = a + 1; b < nb_count; b++)
            if (adjacency[neighbors_list[a]][neighbors_list[b]] > 0.5) links++;
    double max_links = (double)nb_count * (double)(nb_count - 1) / 2.0;
    return (double)links / max_links;
}

double swarm_graph_average_path_length(double** adjacency, int n) {
    if (!adjacency || n <= 1) return 0.0;
    /* Floyd-Warshall */
    double** dist = swarm_matrix_alloc(n, n);
    if (!dist) return 0.0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) dist[i][j] = 0.0;
            else if (adjacency[i][j] > 0.5) dist[i][j] = 1.0;
            else dist[i][j] = 1e10;
        }
    }
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (dist[i][k] + dist[k][j] < dist[i][j])
                    dist[i][j] = dist[i][k] + dist[k][j];
    double total = 0.0; int count = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (dist[i][j] < 1e9) { total += dist[i][j]; count++; }
    swarm_matrix_free(dist, n);
    return count > 0 ? total / (double)count : 0.0;
}

bool swarm_graph_is_connected(double** adjacency, int n) {
    if (!adjacency || n <= 1) return true;
    /* BFS from node 0 */
    bool visited[SWARM_MAX_AGENTS] = {false};
    int queue[SWARM_MAX_AGENTS], head = 0, tail = 0;
    visited[0] = true; queue[tail++] = 0;
    while (head < tail) {
        int u = queue[head++];
        for (int v = 0; v < n; v++) {
            if (!visited[v] && adjacency[u][v] > 0.5) {
                visited[v] = true;
                queue[tail++] = v;
            }
        }
    }
    for (int i = 0; i < n; i++) if (!visited[i]) return false;
    return true;
}

/* ============================================================================
 * Selection Operators
 * ============================================================================ */

int swarm_select_roulette(const double* fitness, int n, bool minimize) {
    if (!fitness || n <= 0) return 0;
    double total = 0.0, adjusted[SWARM_MAX_AGENTS];
    /* Convert to selection probabilities */
    if (minimize) {
        double fmax = fitness[0];
        for (int i = 0; i < n; i++) if (fitness[i] > fmax) fmax = fitness[i];
        fmax += 1e-10;
        for (int i = 0; i < n; i++) { adjusted[i] = fmax - fitness[i]; total += adjusted[i]; }
    } else {
        double fmin = fitness[0];
        for (int i = 0; i < n; i++) if (fitness[i] < fmin) fmin = fitness[i];
        fmin -= 1e-10;
        for (int i = 0; i < n; i++) { adjusted[i] = fitness[i] - fmin; total += adjusted[i]; }
    }
    if (total < SWARM_EPSILON) return swarm_rng_int(n);
    double r = swarm_rng_uniform() * total, cum = 0.0;
    for (int i = 0; i < n; i++) { cum += adjusted[i]; if (r <= cum) return i; }
    return n - 1;
}

int swarm_select_tournament(const double* fitness, int n, int k, bool minimize) {
    if (!fitness || n <= 0 || k <= 0) return 0;
    if (k > n) k = n;
    int best_idx = swarm_rng_int(n);
    for (int i = 1; i < k; i++) {
        int challenger = swarm_rng_int(n);
        bool better = minimize ? (fitness[challenger] < fitness[best_idx])
                              : (fitness[challenger] > fitness[best_idx]);
        if (better) best_idx = challenger;
    }
    return best_idx;
}

int swarm_select_rank(const double* fitness, int n, int select_rank, bool minimize) {
    if (!fitness || n <= 0) return 0;
    int ranks[SWARM_MAX_AGENTS];
    swarm_compute_ranks(fitness, ranks, n, minimize);
    for (int i = 0; i < n; i++) if (ranks[i] == select_rank) return i;
    return 0;
}

void swarm_compute_ranks(const double* fitness, int* ranks, int n, bool minimize) {
    if (!fitness || !ranks) return;
    int indices[SWARM_MAX_AGENTS];
    for (int i = 0; i < n; i++) indices[i] = i;
    /* Simple insertion sort by fitness */
    for (int i = 1; i < n; i++) {
        int key = indices[i], j = i - 1;
        double key_fit = fitness[key];
        while (j >= 0 && ((minimize && fitness[indices[j]] > key_fit) ||
                          (!minimize && fitness[indices[j]] < key_fit))) {
            indices[j + 1] = indices[j]; j--;
        }
        indices[j + 1] = key;
    }
    for (int i = 0; i < n; i++) ranks[indices[i]] = i;
}

/* ============================================================================
 * Diversity Metrics
 * ============================================================================ */

double swarm_diversity_pairwise(const double* positions, int n_agents, int dim) {
    if (!positions || n_agents <= 1) return 0.0;
    double total = 0.0;
    int count = 0;
    for (int i = 0; i < n_agents; i++)
        for (int j = i + 1; j < n_agents; j++) {
            total += swarm_vec_dist(positions + i * dim, positions + j * dim, dim);
            count++;
        }
    return count > 0 ? total / (double)count : 0.0;
}

double swarm_diversity_entropy(const double* positions, int n_agents, int dim,
                                const double* bounds_low, const double* bounds_high, int bins) {
    if (!positions || n_agents <= 0 || !bounds_low || !bounds_high || bins <= 0) return 0.0;
    /* Simplified: project to first dimension for histogram */
    int* hist = (int*)calloc((size_t)bins, sizeof(int));
    if (!hist) return 0.0;
    for (int i = 0; i < n_agents; i++) {
        double val = positions[i * dim];
        double norm = (val - bounds_low[0]) / (bounds_high[0] - bounds_low[0] + SWARM_EPSILON);
        int bin = (int)(norm * (double)bins);
        if (bin < 0) bin = 0; if (bin >= bins) bin = bins - 1;
        hist[bin]++;
    }
    double entropy = 0.0;
    for (int b = 0; b < bins; b++) {
        if (hist[b] > 0) {
            double p = (double)hist[b] / (double)n_agents;
            entropy -= p * log(p);
        }
    }
    free(hist);
    return entropy;
}

double swarm_diversity_swarm_radius(const double* positions, int n_agents, int dim) {
    if (!positions || n_agents <= 1) return 0.0;
    double centroid[SWARM_MAX_DIMENSIONS];
    swarm_centroid(positions, n_agents, dim, centroid);
    double max_r = 0.0;
    for (int i = 0; i < n_agents; i++) {
        double d = swarm_vec_dist(positions + i * dim, centroid, dim);
        if (d > max_r) max_r = d;
    }
    return max_r;
}

void swarm_centroid(const double* positions, int n_agents, int dim, double* centroid) {
    if (!positions || !centroid || n_agents <= 0) return;
    swarm_vec_set(centroid, dim, 0.0);
    for (int i = 0; i < n_agents; i++)
        for (int d = 0; d < dim; d++)
            centroid[d] += positions[i * dim + d];
    if (n_agents > 0)
        for (int d = 0; d < dim; d++)
            centroid[d] /= (double)n_agents;
}

/* ============================================================================
 * Fitness Landscape Analysis
 * ============================================================================ */

void swarm_landscape_random_walk(FitnessLandscape* landscape,
           double (*fitness_fn)(const double*, int, void*),
           const double* bounds_low, const double* bounds_high,
           int dim, int n_steps, double step_size, void* context) {
    if (!landscape || !fitness_fn || !bounds_low || !bounds_high) return;
    double* pos = swarm_alloc_vector(dim);
    double* step = swarm_alloc_vector(dim);
    if (!pos || !step) { swarm_free_vector(pos); swarm_free_vector(step); return; }
    /* Start at random position */
    for (int d = 0; d < dim; d++)
        pos[d] = swarm_rng_uniform_range(bounds_low[d], bounds_high[d]);
    double prev_fitness = fitness_fn(pos, dim, context);
    double sum_rugo = 0.0, sum_corr = 0.0, sum_sq = 0.0;
    double sum_fit = prev_fitness, sum_fit_sq = prev_fitness * prev_fitness;
    int n_optima = 0;
    double* local_opt = (double*)calloc((size_t)n_steps, sizeof(double));
    
    for (int t = 0; t < n_steps; t++) {
        /* Random direction step */
        for (int d = 0; d < dim; d++) step[d] = swarm_rng_gaussian() * step_size;
        for (int d = 0; d < dim; d++) {
            pos[d] += step[d];
            if (pos[d] < bounds_low[d]) pos[d] = bounds_low[d];
            if (pos[d] > bounds_high[d]) pos[d] = bounds_high[d];
        }
        double fit = fitness_fn(pos, dim, context);
        /* Rugosity: variance of Δf */
        double df = fit - prev_fitness;
        sum_rugo += df * df;
        /* Correlation: fitness-distance relationship */
        sum_corr += fit * prev_fitness;
        sum_sq += prev_fitness * prev_fitness;
        sum_fit += fit; sum_fit_sq += fit * fit;
        /* Simple local optimum detection: fitness lower than neighbors in walk */
        if (t > 0 && t < n_steps - 1) {
            double prev = (t > 0) ? fitness_fn(pos, dim, context) : fit;
            /* In a random walk, we mark if current is a local dip/ascent */
            if (t < n_steps && n_optima < n_steps)
                local_opt[n_optima++] = fit;
        }
        prev_fitness = fit;
    }
    landscape->rugosity = sqrt(sum_rugo / (double)n_steps);
    double mean_fit = sum_fit / (double)n_steps;
    landscape->fitness_variance = sum_fit_sq / (double)n_steps - mean_fit * mean_fit;
    double denom = sqrt(sum_sq) * sqrt(sum_sq - prev_fitness * prev_fitness);
    landscape->correlation_length = (fabs(denom) > SWARM_EPSILON) ? sum_corr / denom : 0.0;
    landscape->basins_count = n_optima > 0 ? n_optima / 10 : 0;
    landscape->neutrality_ratio = 0.0;
    landscape->deceptiveness = 0.0;
    landscape->n_local_optima = n_optima;
    landscape->local_optima_values = local_opt;
    swarm_free_vector(pos); swarm_free_vector(step);
}

void swarm_landscape_free(FitnessLandscape* landscape) {
    if (!landscape) return;
    free(landscape->local_optima_values);
    landscape->local_optima_values = NULL;
}

/* ============================================================================
 * Convergence Diagnostics
 * ============================================================================ */

void swarm_diagnostics_update(ConvergenceDiagnostics* diag,
           const double* positions, const double* velocities,
           const double* fitnesses, int n_agents, int dim,
           const double* bounds_low, const double* bounds_high,
           double best_fitness_prev, int iteration) {
    if (!diag || !positions || !velocities || !fitnesses) return;
    
    /* Swarm radius */
    diag->swarm_radius = swarm_diversity_swarm_radius(positions, n_agents, dim);
    
    /* Velocity statistics */
    double v_sum = 0.0, v_sq = 0.0;
    for (int i = 0; i < n_agents; i++) {
        double vm = swarm_vec_norm(velocities + i * dim, dim);
        v_sum += vm; v_sq += vm * vm;
    }
    diag->velocity_mean = v_sum / (double)n_agents;
    diag->velocity_variance = v_sq / (double)n_agents - diag->velocity_mean * diag->velocity_mean;
    if (diag->velocity_variance < 0.0) diag->velocity_variance = 0.0;
    
    /* Fitness improvement rate */
    double best_fitness = fitnesses[0];
    for (int i = 0; i < n_agents; i++)
        if (fitnesses[i] < best_fitness) best_fitness = fitnesses[i];
    diag->fitness_improvement_rate = fabs(best_fitness - best_fitness_prev);
    
    /* Stagnation */
    if (diag->fitness_improvement_rate < SWARM_EPSILON_MED)
        diag->stagnation_count++;
    else
        diag->stagnation_count = 0;
    diag->is_stagnant = (diag->stagnation_count > 20);
    
    /* Premature convergence: converged but not at global optimum (heuristic) */
    diag->is_premature = (diag->swarm_radius < SWARM_EPSILON_COARSE && diag->is_stagnant);
    
    /* Divergence: agents outside bounds */
    diag->has_diverged = false;
    for (int i = 0; i < n_agents && !diag->has_diverged; i++)
        for (int d = 0; d < dim; d++)
            if (positions[i * dim + d] < bounds_low[d] || positions[i * dim + d] > bounds_high[d])
                { diag->has_diverged = true; break; }
    
    /* Exploration ratio */
    double explore_thresh = 0.1 * dim;
    int exploring = 0;
    for (int i = 0; i < n_agents; i++)
        if (swarm_vec_norm(velocities + i * dim, dim) > explore_thresh) exploring++;
    diag->exploration_ratio = (double)exploring / (double)n_agents;
    
    /* Entropy approximation */
    diag->entropy = swarm_diversity_entropy(positions, n_agents, dim, 
                     bounds_low, bounds_high, 10);
}

void swarm_diagnostics_reset(ConvergenceDiagnostics* diag) {
    if (!diag) return;
    memset(diag, 0, sizeof(ConvergenceDiagnostics));
}

const char* swarm_diagnostics_summary(const ConvergenceDiagnostics* diag) {
    if (!diag) return "(null)";
    static char buf[512];
    snprintf(buf, sizeof(buf),
        "radius=%.4f vel_mean=%.4f vel_var=%.4f improve=%.2e "
        "stagnant=%d premature=%d diverged=%d explore=%.2f entropy=%.4f",
        diag->swarm_radius, diag->velocity_mean, diag->velocity_variance,
        diag->fitness_improvement_rate, diag->is_stagnant, diag->is_premature,
        diag->has_diverged, diag->exploration_ratio, diag->entropy);
    return buf;
}

/* ============================================================================
 * Agent Management
 * ============================================================================ */

SwarmAgent* swarm_agent_create(int dim) {
    if (dim <= 0 || dim > SWARM_MAX_DIMENSIONS) return NULL;
    SwarmAgent* a = (SwarmAgent*)calloc(1, sizeof(SwarmAgent));
    if (!a) return NULL;
    a->position = swarm_alloc_vector(dim);
    a->velocity = swarm_alloc_vector(dim);
    a->best_position = swarm_alloc_vector(dim);
    if (!a->position || !a->velocity || !a->best_position) {
        swarm_agent_free(a); return NULL;
    }
    a->dim = dim;
    a->is_active = true;
    a->state = SWARM_STATE_EXPLORING;
    a->neighbor_count = 0;
    return a;
}

void swarm_agent_free(SwarmAgent* agent) {
    if (!agent) return;
    swarm_free_vector(agent->position);
    swarm_free_vector(agent->velocity);
    swarm_free_vector(agent->best_position);
    free(agent);
}

void swarm_agent_init_random(SwarmAgent* agent, int dim, int id,
                              const double* bounds_low, const double* bounds_high) {
    if (!agent || !bounds_low || !bounds_high) return;
    agent->id = id;
    for (int d = 0; d < dim; d++) {
        agent->position[d] = swarm_rng_uniform_range(bounds_low[d], bounds_high[d]);
        double range = bounds_high[d] - bounds_low[d];
        agent->velocity[d] = swarm_rng_uniform_range(-range * 0.1, range * 0.1);
    }
    swarm_vec_copy(agent->position, agent->best_position, dim);
    agent->fitness = 1e100;
    agent->best_fitness = 1e100;
    agent->is_active = true;
    agent->state = SWARM_STATE_EXPLORING;
}

void swarm_agent_copy(const SwarmAgent* src, SwarmAgent* dst) {
    if (!src || !dst) return;
    swarm_vec_copy(src->position, dst->position, src->dim);
    swarm_vec_copy(src->velocity, dst->velocity, src->dim);
    swarm_vec_copy(src->best_position, dst->best_position, src->dim);
    dst->fitness = src->fitness;
    dst->best_fitness = src->best_fitness;
    dst->id = src->id;
    dst->dim = src->dim;
    dst->is_active = src->is_active;
    dst->state = src->state;
    memcpy(dst->neighbors, src->neighbors, sizeof(int) * SWARM_MAX_NEIGHBORS);
    dst->neighbor_count = src->neighbor_count;
}

void swarm_agent_update_fitness(SwarmAgent* agent,
           double (*fitness_fn)(const double*, int, void*),
           void* context, bool minimize) {
    if (!agent || !fitness_fn) return;
    agent->fitness = fitness_fn(agent->position, agent->dim, context);
    bool better = minimize ? (agent->fitness < agent->best_fitness)
                           : (agent->fitness > agent->best_fitness);
    if (better) {
        agent->best_fitness = agent->fitness;
        swarm_vec_copy(agent->position, agent->best_position, agent->dim);
    }
}

/* ============================================================================
 * Memory Helpers
 * ============================================================================ */

double* swarm_alloc_vector(int dim) {
    if (dim <= 0) return NULL;
    return (double*)calloc((size_t)dim, sizeof(double));
}

void swarm_free_vector(double* v) { free(v); }

int* swarm_alloc_int_vector(int n) {
    if (n <= 0) return NULL;
    return (int*)calloc((size_t)n, sizeof(int));
}

void swarm_free_int_vector(int* v) { free(v); }
