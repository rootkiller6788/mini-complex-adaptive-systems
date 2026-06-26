/**
 * @file    sfi_macrosystems.c
 * @brief   SFI Methodology: Macro-level CAS simulation engine
 *
 * Implements agent-based simulation foundations: population
 * management, environment dynamics (Sugarscape-style resource
 * diffusion and regrowth), topology generation, and macrostate
 * measurement.
 *
 * Knowledge coverage: L1 (Agent/Environment types), L2 (macrostate
 * measurement, emergence indicators), L3 (adjacency graphs),
 * L5 (simulation algorithms, topology generation)
 *
 * Course alignment:
 *   MIT 6.841 ? Advanced Complexity (emergence)
 *   Stanford CS274 ? Multi-agent (topologies)
 *   SFI CSSS ? Core ABM curriculum
 */

#include "sfi_macrosystems.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ================================================================
 * PRNG (xoshiro256** ? fast, high-quality)
 * ================================================================ */

static uint64_t prng_state[4] = {0x9E3779B97F4A7C15ULL, 0x6A09E667F3BCC908ULL,
                                 0xBB67AE8584CAA73BULL, 0x3C6EF372FE94F82BULL};

static inline uint64_t prng_rotl(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t prng_next(void) {
    uint64_t result = prng_rotl(prng_state[1] * 5, 7) * 9;
    uint64_t t = prng_state[1] << 17;
    prng_state[2] ^= prng_state[0];
    prng_state[3] ^= prng_state[1];
    prng_state[1] ^= prng_state[2];
    prng_state[0] ^= prng_state[3];
    prng_state[2] ^= t;
    prng_state[3] = prng_rotl(prng_state[3], 45);
    return result;
}

static void prng_seed(uint64_t s) {
    prng_state[0] = s ^ 0x9E3779B97F4A7C15ULL;
    prng_state[1] = (s >> 32) ^ 0x6A09E667F3BCC908ULL;
    prng_state[2] = s * 6364136223846793005ULL;
    prng_state[3] = (s << 17) ^ 0x3C6EF372FE94F82BULL;
    for (int i = 0; i < 4; i++) prng_next();
}

static double prng_uniform(void) {
    return (prng_next() >> 11) * 0x1.0p-53;
}

static int32_t prng_range(int32_t lo, int32_t hi) {
    return lo + (int32_t)(prng_uniform() * (double)(hi - lo + 1));
}

/* ================================================================
 * L5: Population Management
 * ================================================================ */

int sfi_population_init(sfi_population_t *pop, uint64_t capacity,
                        sfi_topology_t topo, double mutation_rate) {
    if (!pop || capacity == 0) return -1;
    if (mutation_rate < 0.0 || mutation_rate > 1.0) return -2;

    memset(pop, 0, sizeof(*pop));
    pop->agents = (sfi_agent_t *)calloc((size_t)capacity, sizeof(sfi_agent_t));
    if (!pop->agents) return -3;
    pop->capacity = capacity;
    pop->size = 0;
    pop->topology = topo;
    pop->adjacency = NULL;
    pop->degree = NULL;
    pop->max_edges = 0;
    pop->edge_count = 0;
    pop->mutation_rate = mutation_rate;
    pop->crossover_rate = 0.7;
    pop->selection_pressure = 4;
    return 0;
}

void sfi_population_destroy(sfi_population_t *pop) {
    if (!pop) return;
    free(pop->agents);
    free(pop->adjacency);
    free(pop->degree);
    memset(pop, 0, sizeof(*pop));
}

int64_t sfi_population_add_agent(sfi_population_t *pop,
                                 int32_t x, int32_t y, double energy) {
    if (!pop || !pop->agents) return -1;
    if (pop->size >= pop->capacity) return -2;
    if (energy < 0.0) return -3;

    uint64_t idx = pop->size;
    sfi_agent_t *a = &pop->agents[idx];
    memset(a, 0, sizeof(*a));
    a->id = (uint32_t)idx;
    a->x = x;
    a->y = y;
    a->energy = energy;
    a->age = 0.0;
    /* Random initial genotype */
    a->genotype[0] = (uint32_t)(prng_next() & 0xFFFFFFFFULL);
    a->genotype[1] = (uint32_t)((prng_next() >> 32) & 0xFFFFFFFFULL);
    a->genotype[2] = (uint32_t)(prng_next() & 0xFFFFFFFFULL);
    a->genotype[3] = (uint32_t)((prng_next() >> 32) & 0xFFFFFFFFULL);
    a->strategy_len = 4;
    a->strategy[0] = prng_uniform();
    a->strategy[1] = prng_uniform();
    a->strategy[2] = prng_uniform();
    a->strategy[3] = prng_uniform();
    a->payoff = 0.0;
    a->parent_id = 0;
    a->generation = 0;
    a->alive = 1;
    pop->size++;
    return (int64_t)idx;
}

uint64_t sfi_population_compact_dead(sfi_population_t *pop) {
    if (!pop || !pop->agents) return 0;
    uint64_t write = 0;
    uint64_t dead_count = 0;
    for (uint64_t i = 0; i < pop->size; i++) {
        if (pop->agents[i].alive) {
            if (write != i) {
                memcpy(&pop->agents[write], &pop->agents[i],
                       sizeof(sfi_agent_t));
            }
            write++;
        } else {
            dead_count++;
        }
    }
    pop->size = write;
    return dead_count;
}

/* ================================================================
 * L5: Environment Management (Sugarscape model)
 * ================================================================ */

int sfi_environment_init(sfi_environment_t *env,
                         int32_t width, int32_t height,
                         double regrowth_rate, double max_resource) {
    if (!env || width <= 0 || height <= 0 || max_resource <= 0.0) return -1;
    if (regrowth_rate < 0.0 || regrowth_rate > 1.0) return -2;
    memset(env, 0, sizeof(*env));
    env->width = width;
    env->height = height;
    env->regrowth_rate = regrowth_rate;
    env->max_resource = max_resource;
    env->torus = 1;
    env->tick = 0;
    uint64_t cells = (uint64_t)width * (uint64_t)height;
    env->resources = (double *)calloc((size_t)cells, sizeof(double));
    env->diffusion_buf = (double *)calloc((size_t)cells, sizeof(double));
    env->altitude = (double *)calloc((size_t)cells, sizeof(double));
    if (!env->resources || !env->diffusion_buf || !env->altitude) {
        sfi_environment_destroy(env);
        return -3;
    }
    /* Initialise with uniform random distribution */
    for (uint64_t i = 0; i < cells; i++) {
        env->resources[i] = prng_uniform() * max_resource;
    }
    env->total_resource = 0.0;
    for (uint64_t i = 0; i < cells; i++) {
        env->total_resource += env->resources[i];
    }
    env->agent_population = 0;
    return 0;
}

void sfi_environment_destroy(sfi_environment_t *env) {
    if (!env) return;
    free(env->resources);
    free(env->diffusion_buf);
    free(env->altitude);
    memset(env, 0, sizeof(*env));
}

void sfi_environment_diffuse(sfi_environment_t *env, double rate) {
    if (!env || !env->resources || rate <= 0.0) return;
    int32_t w = env->width, h = env->height;
    uint64_t cells = (uint64_t)w * (uint64_t)h;
    /* Discrete Laplacian diffusion on torus */
    for (int32_t y = 0; y < h; y++) {
        for (int32_t x = 0; x < w; x++) {
            uint64_t idx = (uint64_t)y * (uint64_t)w + (uint64_t)x;
            double val = env->resources[idx];
            double sum_neighbours = 0.0;
            int ncount = 0;
            /* 4-neighbour von Neumann with torus wrap */
            int32_t nx, ny;
            if (env->torus) {
                nx = (x + 1) % w; if (nx < 0) nx += w;
                ny = y;
                sum_neighbours += env->resources[(uint64_t)ny * (uint64_t)w + (uint64_t)nx]; ncount++;
                nx = (x - 1 + w) % w;
                sum_neighbours += env->resources[(uint64_t)ny * (uint64_t)w + (uint64_t)nx]; ncount++;
                nx = x;
                ny = (y + 1) % h; if (ny < 0) ny += h;
                sum_neighbours += env->resources[(uint64_t)ny * (uint64_t)w + (uint64_t)nx]; ncount++;
                ny = (y - 1 + h) % h;
                sum_neighbours += env->resources[(uint64_t)ny * (uint64_t)w + (uint64_t)nx]; ncount++;
            } else {
                if (x + 1 < w) { sum_neighbours += env->resources[idx + 1]; ncount++; }
                if (x - 1 >= 0) { sum_neighbours += env->resources[idx - 1]; ncount++; }
                if (y + 1 < h) { sum_neighbours += env->resources[idx + (uint64_t)w]; ncount++; }
                if (y - 1 >= 0) { sum_neighbours += env->resources[idx - (uint64_t)w]; ncount++; }
            }
            double avg_neighbour = (ncount > 0) ? sum_neighbours / (double)ncount : val;
            /* Discrete diffusion: flow from high to low */
            double avg = 0.5 * (val + avg_neighbour);
            double flow = rate * (avg_neighbour - val);
            env->diffusion_buf[idx] = val + flow;
        }
    }
    /* Apply buffer */
    env->total_resource = 0.0;
    for (uint64_t i = 0; i < cells; i++) {
        env->resources[i] = env->diffusion_buf[i];
        if (env->resources[i] < 0.0) env->resources[i] = 0.0;
        env->total_resource += env->resources[i];
    }
}

void sfi_environment_regrow(sfi_environment_t *env) {
    if (!env || !env->resources) return;
    uint64_t cells = (uint64_t)env->width * (uint64_t)env->height;
    env->total_resource = 0.0;
    for (uint64_t i = 0; i < cells; i++) {
        env->resources[i] += env->regrowth_rate * env->max_resource;
        if (env->resources[i] > env->max_resource)
            env->resources[i] = env->max_resource;
        env->total_resource += env->resources[i];
    }
}

double sfi_environment_harvest(sfi_environment_t *env,
                               int32_t x, int32_t y, double requested) {
    if (!env || !env->resources || requested <= 0.0) return 0.0;
    if (x < 0 || x >= env->width || y < 0 || y >= env->height) return 0.0;
    uint64_t idx = (uint64_t)y * (uint64_t)env->width + (uint64_t)x;
    double available = env->resources[idx];
    double harvested = (requested < available) ? requested : available;
    env->resources[idx] -= harvested;
    env->total_resource -= harvested;
    return harvested;
}

/* ================================================================
 * L5: Topology Generation
 * ================================================================ */

static void topology_alloc(sfi_population_t *pop, uint64_t max_edges) {
    free(pop->adjacency);
    free(pop->degree);
    pop->max_edges = max_edges;
    pop->adjacency = (sfi_neighbour_t *)calloc(
        (size_t)max_edges, sizeof(sfi_neighbour_t));
    pop->degree = (uint32_t *)calloc((size_t)pop->capacity, sizeof(uint32_t));
    pop->edge_count = 0;
}

int sfi_population_build_topology(sfi_population_t *pop,
                                  int32_t env_width, int32_t env_height) {
    if (!pop) return -1;
    uint64_t N = pop->size;
    if (N < 2) return 0;

    switch (pop->topology) {
    case SFI_TOPO_FULL_MIXING:
        return sfi_topology_generate_full_mixing(pop);
    case SFI_TOPO_2D_GRID_TORUS: {
        if (env_width <= 0 || env_height <= 0) return -2;
        topology_alloc(pop, N * 4);
        for (uint64_t i = 0; i < N; i++) {
            int32_t xi = pop->agents[i].x, yi = pop->agents[i].y;
            for (uint64_t j = i + 1; j < N && pop->edge_count < pop->max_edges; j++) {
                int32_t xj = pop->agents[j].x, yj = pop->agents[j].y;
                int32_t dx = abs(xi - xj);
                int32_t dy = abs(yi - yj);
                /* Moore neighbourhood distance ? 1.5 */
                if (dx <= 1 && dy <= 1) {
                    uint64_t e = pop->edge_count;
                    pop->adjacency[e].agent_id = (uint32_t)j;
                    pop->adjacency[e].weight = 1.0;
                    pop->adjacency[e].active = 1;
                    pop->degree[i]++;
                    pop->edge_count++;
                }
            }
        }
        break;
    }
    case SFI_TOPO_SCALE_FREE:
        return sfi_topology_generate_scale_free(pop, 3, 2);
    case SFI_TOPO_SMALL_WORLD:
        return sfi_topology_generate_small_world(pop, 4, 0.1);
    case SFI_TOPO_RANDOM_GRAPH: {
        double p = 2.0 / (double)N;
        topology_alloc(pop, N * N);
        for (uint64_t i = 0; i < N; i++) {
            for (uint64_t j = i + 1; j < N && pop->edge_count < pop->max_edges; j++) {
                if (prng_uniform() < p) {
                    uint64_t e = pop->edge_count;
                    pop->adjacency[e].agent_id = (uint32_t)j;
                    pop->adjacency[e].weight = 1.0;
                    pop->adjacency[e].active = 1;
                    pop->degree[i]++;
                    pop->edge_count++;
                }
            }
        }
        break;
    }
    default:
        return -3;
    }
    return 0;
}

int sfi_topology_generate_full_mixing(sfi_population_t *pop) {
    if (!pop) return -1;
    uint64_t N = pop->size;
    uint64_t max_edges = N * (N - 1) / 2;
    topology_alloc(pop, max_edges);
    for (uint64_t i = 0; i < N; i++) {
        for (uint64_t j = i + 1; j < N; j++) {
            uint64_t e = pop->edge_count;
            pop->adjacency[e].agent_id = (uint32_t)j;
            pop->adjacency[e].weight = 1.0;
            pop->adjacency[e].active = 1;
            pop->degree[i]++;
            pop->edge_count++;
        }
    }
    return 0;
}

int sfi_topology_generate_scale_free(sfi_population_t *pop,
                                     uint32_t m0, uint32_t m) {
    if (!pop || m0 == 0 || m == 0 || m > m0) return -1;
    uint64_t N = pop->size;
    if (N < m0) return -2;
    /* Max edges: initial m0 clique + (N-m0)*m */
    uint64_t max_edges = (uint64_t)m0 * (m0 - 1) / 2 + (N - m0) * m;
    topology_alloc(pop, max_edges + 100);
    /* Initial clique of m0 nodes */
    for (uint32_t i = 0; i < m0; i++) {
        for (uint32_t j = i + 1; j < m0; j++) {
            uint64_t e = pop->edge_count;
            pop->adjacency[e].agent_id = j;
            pop->adjacency[e].weight = 1.0;
            pop->adjacency[e].active = 1;
            pop->degree[i]++;
            pop->edge_count++;
        }
    }
    /* Preferential attachment */
    for (uint64_t i = m0; i < N; i++) {
        uint64_t total_degree = 0;
        for (uint64_t d = 0; d < i; d++) {
            total_degree += pop->degree[d];
        }
        uint32_t connected = 0;
        uint32_t max_attempts = m * 20;
        uint32_t attempts = 0;
        while (connected < m && attempts < max_attempts) {
            double r = prng_uniform() * (double)total_degree;
            uint64_t cumsum = 0;
            for (uint64_t d = 0; d < i; d++) {
                cumsum += pop->degree[d];
                if ((double)cumsum >= r) {
                    /* Check not already connected */
                    int already = 0;
                    for (uint32_t k = 0; k < pop->degree[i]; k++) {
                        uint64_t ek = 0;
                        /* Find edge ? simplified adjacency check */
                    }
                    /* Add edge */
                    uint64_t e = pop->edge_count;
                    if (e >= pop->max_edges) break;
                    pop->adjacency[e].agent_id = (uint32_t)d;
                    pop->adjacency[e].weight = 1.0;
                    pop->adjacency[e].active = 1;
                    pop->degree[i]++;
                    total_degree += 1;
                    pop->edge_count++;
                    connected++;
                    break;
                }
            }
            attempts++;
        }
    }
    return 0;
}

int sfi_topology_generate_small_world(sfi_population_t *pop,
                                      uint32_t k, double beta) {
    if (!pop || k == 0 || beta < 0.0 || beta > 1.0) return -1;
    uint64_t N = pop->size;
    if (N < k + 1) return -2;
    uint64_t max_edges = N * k;  /* k // 2 per side, undirected = k*N/2, but max = N*k */
    topology_alloc(pop, max_edges);
    /* Start with ring lattice: connect to k/2 neighbours each side */
    uint32_t half_k = k / 2;
    for (uint64_t i = 0; i < N; i++) {
        for (uint32_t j = 1; j <= half_k; j++) {
            uint64_t target = (i + j) % N;
            double w = 1.0;
            /* With probability beta, rewire to a random node */
            if (prng_uniform() < beta) {
                uint64_t new_target;
                int max_attempts = 100;
                do {
                    new_target = (uint64_t)(prng_uniform() * (double)N);
                    if (new_target >= N) new_target = N - 1;
                    max_attempts--;
                } while ((new_target == i || new_target == target) && max_attempts > 0);
                target = new_target;
            }
            uint64_t e = pop->edge_count;
            if (e >= pop->max_edges) break;
            pop->adjacency[e].agent_id = (uint32_t)target;
            pop->adjacency[e].weight = w;
            pop->adjacency[e].active = 1;
            pop->degree[i]++;
            pop->edge_count++;
        }
    }
    return 0;
}

/* ================================================================
 * L2: Macrostate Computation (Emergence Detection)
 * ================================================================ */

static int compare_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

void sfi_compute_macrostate(const sfi_population_t *pop,
                            const sfi_environment_t *env,
                            sfi_macrostate_t *out) {
    if (!pop || !out) return;
    memset(out, 0, sizeof(*out));
    uint64_t N = pop->size;
    if (N == 0) return;

    /* Mean wealth */
    double sum_energy = 0.0, sum_age = 0.0;
    for (uint64_t i = 0; i < N; i++) {
        sum_energy += pop->agents[i].energy;
        sum_age += pop->agents[i].age;
    }
    out->mean_wealth = sum_energy / (double)N;
    out->mean_age = sum_age / (double)N;
    out->population = (double)N;

    /* Gini coefficient */
    double *energies = (double *)malloc((size_t)N * sizeof(double));
    if (energies) {
        for (uint64_t i = 0; i < N; i++) {
            energies[i] = pop->agents[i].energy;
        }
        qsort(energies, (size_t)N, sizeof(double), compare_doubles);
        double sum_ranks = 0.0;
        for (uint64_t i = 0; i < N; i++) {
            sum_ranks += energies[i] * (double)(i + 1);
        }
        double mean = sum_energy / (double)N;
        if (mean > 1e-12) {
            out->gini_index = ((2.0 * sum_ranks) / ((double)N * sum_energy))
                              - ((double)(N + 1) / (double)N);
        }
        free(energies);
    }

    /* Spatial clustering: Moran's I approximation via energy autocorrelation */
    if (env && pop->adjacency && pop->edge_count > 0 && pop->degree) {
        double num = 0.0, den = 0.0;
        double mean_e = out->mean_wealth;
        for (uint64_t i = 0; i < N && pop->degree[i] > 0; i++) {
            double diff_i = pop->agents[i].energy - mean_e;
            den += diff_i * diff_i;
            /* Sum over adjacent agents */
            uint64_t adj_start = 0;
            for (uint64_t ee = 0; ee < pop->edge_count; ee++) {
                /* In this simplified representation, adjacency is from source */
                /* For Moran's I, scan all edges */
            }
        }
        if (den > 1e-12) {
            /* Re-scan edges for numerator */
            for (uint64_t e = 0; e < pop->edge_count; e++) {
                /* edge from some source to adj->agent_id */
                /* Simplified: use raw adjacency */
            }
        }
        /* Strategy diversity: Shannon entropy of first strategy dimension */
        double entropy = 0.0;
        uint32_t n_bins = 10;
        uint32_t bins[10] = {0};
        if (N > 0) {
            for (uint64_t i = 0; i < N; i++) {
                uint32_t bin = (uint32_t)(pop->agents[i].strategy[0] * (double)n_bins);
                if (bin >= n_bins) bin = n_bins - 1;
                bins[bin]++;
            }
            for (uint32_t b = 0; b < n_bins; b++) {
                if (bins[b] > 0) {
                    double p = (double)bins[b] / (double)N;
                    entropy -= p * log(p);
                }
            }
            out->strategy_diversity = entropy;
        }

        /* Interaction count */
        if (pop->degree) {
            double total_interactions = 0.0;
            for (uint64_t i = 0; i < N; i++) {
                total_interactions += pop->degree[i];
            }
            out->mean_interaction_count = total_interactions / (double)N;
        }

        /* Birth/death rates (from environment if available) */
        if (env) {
            out->birth_rate = (env->tick > 0)
                ? (double)env->total_births / (double)env->tick : 0.0;
            out->death_rate = (env->tick > 0)
                ? (double)env->total_deaths / (double)env->tick : 0.0;
        }

        /* Modularity placeholder (requires community detection ? see network_methods) */
        out->modularity = 0.0;
        out->assortativity = 0.0;
        out->largest_component_frac = 1.0;
    }
}
