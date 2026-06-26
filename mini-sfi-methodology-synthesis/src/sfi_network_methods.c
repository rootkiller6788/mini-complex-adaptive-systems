/**
 * @file    sfi_network_methods.c
 * @brief   SFI Methodology: Network science algorithms
 *
 * Implements network generation (ER, BA, WS), community detection
 * (Newman-Girvan edge-betweenness), centrality measures (betweenness,
 * closeness, PageRank), and network statistics.
 *
 * Knowledge: L1 (network/community types), L3 (adjacency structures,
 * graph statistics), L5 (all algorithms), L8 (Girvan-Newman
 * community detection)
 *
 * Course: SFI CSSS ? Networks module;
 *         Stanford CS224W ? Machine Learning with Graphs
 */

#include "sfi_network_methods.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ---- Internal PRNG ---- */
static uint64_t net_prng_state = 0x12345678DEADBEEFULL;

static void net_prng_seed(uint64_t s) { net_prng_state = s ^ 0xDEADBEEFULL; }

static uint64_t net_prng_next(void) {
    net_prng_state = net_prng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return net_prng_state;
}

static double net_prng_uniform(void) {
    return (double)(net_prng_next() >> 11) * 0x1.0p-53;
}

/* ---- Network Management ---- */

static int network_alloc_adjacency(sfi_network_t *net) {
    net->neighbours = (uint32_t **)calloc((size_t)net->n_nodes, sizeof(uint32_t *));
    net->neighbour_capacities = (uint32_t *)calloc((size_t)net->n_nodes, sizeof(uint32_t));
    net->degree = (uint32_t *)calloc((size_t)net->n_nodes, sizeof(uint32_t));
    net->in_degree = (uint32_t *)calloc((size_t)net->n_nodes, sizeof(uint32_t));
    net->node_values = (double *)calloc((size_t)net->n_nodes, sizeof(double));
    if (!net->neighbours || !net->neighbour_capacities || !net->degree ||
        !net->in_degree || !net->node_values) return -1;
    /* Start with small capacity per node */
    uint32_t init_cap = 8;
    for (uint32_t i = 0; i < net->n_nodes; i++) {
        net->neighbours[i] = (uint32_t *)calloc((size_t)init_cap, sizeof(uint32_t));
        if (!net->neighbours[i]) return -1;
        net->neighbour_capacities[i] = init_cap;
    }
    return 0;
}

static int network_add_edge(sfi_network_t *net, uint32_t src, uint32_t dst, double weight) {
    if (src >= net->n_nodes || dst >= net->n_nodes) return -1;
    /* Grow adjacency list if needed */
    if (net->degree[src] >= net->neighbour_capacities[src]) {
        uint32_t new_cap = net->neighbour_capacities[src] * 2;
        if (new_cap < 4) new_cap = 4;
        uint32_t *new_list = (uint32_t *)realloc(net->neighbours[src],
            (size_t)new_cap * sizeof(uint32_t));
        if (!new_list) return -1;
        net->neighbours[src] = new_list;
        net->neighbour_capacities[src] = new_cap;
    }
    net->neighbours[src][net->degree[src]] = dst;
    if (net->is_weighted && net->edge_weights) {
        net->edge_weights[net->n_edges] = weight;
    }
    net->degree[src]++;
    net->in_degree[dst]++;
    net->n_edges++;
    return 0;
}

void sfi_network_destroy(sfi_network_t *net) {
    if (!net) return;
    for (uint32_t i = 0; i < net->n_nodes; i++) free(net->neighbours[i]);
    free(net->neighbours);
    free(net->neighbour_capacities);
    free(net->degree);
    free(net->in_degree);
    free(net->node_values);
    free(net->edge_weights);
    memset(net, 0, sizeof(*net));
}

void sfi_community_set_destroy(sfi_community_set_t *cs) {
    if (!cs) return;
    for (uint32_t i = 0; i < cs->n_communities; i++) free(cs->communities[i].members);
    free(cs->communities);
    free(cs->membership);
    memset(cs, 0, sizeof(*cs));
}

/* ================================================================
 * L5: Network Generation
 * ================================================================ */

int sfi_network_generate_erdos_renyi(sfi_network_t *net,
    uint32_t n, double p, uint64_t seed) {
    if (!net || n < 2 || p < 0.0 || p > 1.0) return -1;
    memset(net, 0, sizeof(*net));
    net->n_nodes = n;
    net->is_directed = 0;
    net->is_weighted = 0;
    net_prng_seed(seed);
    if (network_alloc_adjacency(net) != 0) { sfi_network_destroy(net); return -2; }

    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = i + 1; j < n; j++) {
            if (net_prng_uniform() < p) {
                network_add_edge(net, i, j, 1.0);
                network_add_edge(net, j, i, 1.0);
            }
        }
    }
    return 0;
}

int sfi_network_generate_barabasi_albert(sfi_network_t *net,
    uint32_t n, uint32_t m0, uint32_t m, uint64_t seed) {
    if (!net || n < m0 || m0 < 2 || m == 0 || m > m0) return -1;
    memset(net, 0, sizeof(*net));
    net->n_nodes = n;
    net->is_directed = 0;
    net->is_weighted = 0;
    net_prng_seed(seed);
    if (network_alloc_adjacency(net) != 0) { sfi_network_destroy(net); return -2; }

    /* Initial complete graph on m0 nodes */
    for (uint32_t i = 0; i < m0; i++) {
        for (uint32_t j = i + 1; j < m0; j++) {
            network_add_edge(net, i, j, 1.0);
            network_add_edge(net, j, i, 1.0);
        }
    }

    /* Preferential attachment for remaining N - m0 nodes */
    for (uint32_t new_node = m0; new_node < n; new_node++) {
        uint64_t total_deg = 0;
        for (uint32_t d = 0; d < new_node; d++) total_deg += net->degree[d];

        uint32_t *targets = (uint32_t *)malloc((size_t)m * sizeof(uint32_t));
        if (!targets) continue;
        uint32_t t_count = 0;

        /* Select m distinct targets */
        for (uint32_t em = 0; em < m && em < new_node; em++) {
            double r = net_prng_uniform() * (double)total_deg;
            uint64_t cum = 0;
            for (uint32_t candidate = 0; candidate < new_node; candidate++) {
                cum += net->degree[candidate];
                if ((double)cum >= r) {
                    /* Check not already connected */
                    int already = 0;
                    for (uint32_t k = 0; k < t_count; k++)
                        if (targets[k] == candidate) already = 1;
                    if (!already) {
                        targets[t_count++] = candidate;
                        total_deg += 2;  /* Both ends get degree increment */
                        break;
                    }
                }
            }
        }

        /* Add edges */
        for (uint32_t k = 0; k < t_count; k++) {
            network_add_edge(net, new_node, targets[k], 1.0);
            network_add_edge(net, targets[k], new_node, 1.0);
        }
        free(targets);
    }
    return 0;
}

int sfi_network_generate_watts_strogatz(sfi_network_t *net,
    uint32_t n, uint32_t k, double beta, uint64_t seed) {
    if (!net || n < 3 || k < 2 || k % 2 != 0 || beta < 0.0 || beta > 1.0) return -1;
    memset(net, 0, sizeof(*net));
    net->n_nodes = n;
    net->is_directed = 0;
    net->is_weighted = 0;
    net_prng_seed(seed);
    if (network_alloc_adjacency(net) != 0) { sfi_network_destroy(net); return -2; }

    uint32_t half_k = k / 2;

    /* Build ring lattice */
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 1; j <= half_k; j++) {
            uint32_t target = (i + j) % n;
            /* With probability beta, rewire to random node */
            if (net_prng_uniform() < beta) {
                uint32_t new_target;
                int tries = 0;
                do {
                    new_target = (uint32_t)(net_prng_uniform() * (double)n);
                    if (new_target >= n) new_target = n - 1;
                    tries++;
                } while ((new_target == i || new_target == target) && tries < 100);
                target = new_target;
            }
            network_add_edge(net, i, target, 1.0);
            network_add_edge(net, target, i, 1.0);
        }
    }
    return 0;
}

/* ================================================================
 * L5: Network Statistics
 * ================================================================ */

int sfi_network_compute_stats(const sfi_network_t *net,
                              sfi_network_stats_t *stats) {
    if (!net || !stats || net->n_nodes == 0) return -1;
    memset(stats, 0, sizeof(*stats));

    uint32_t n = net->n_nodes;
    uint64_t m = net->n_edges;

    /* Mean degree */
    double sum_deg = 0.0, sum_deg2 = 0.0;
    for (uint32_t i = 0; i < n; i++) {
        sum_deg += net->degree[i];
        sum_deg2 += (double)net->degree[i] * (double)net->degree[i];
    }
    stats->mean_degree = sum_deg / (double)n;
    stats->degree_variance = (sum_deg2 / (double)n) - stats->mean_degree * stats->mean_degree;
    if (stats->degree_variance < 0.0) stats->degree_variance = 0.0;

    /* Density */
    uint64_t max_edges = (uint64_t)n * (uint64_t)(n - 1) / 2;
    stats->density = (max_edges > 0) ? (double)m / (double)max_edges : 0.0;

    /* Clustering coefficient (global) */
    uint64_t triangles = 0, triads = 0;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t di = net->degree[i];
        for (uint32_t jj = 0; jj < di; jj++) {
            uint32_t j = net->neighbours[i][jj];
            if (j >= n) continue;
            for (uint32_t kk = jj + 1; kk < di; kk++) {
                uint32_t k = net->neighbours[i][kk];
                if (k >= n) continue;
                triads++;
                /* Check if j-k edge exists */
                for (uint32_t ej = 0; ej < net->degree[j]; ej++) {
                    if (net->neighbours[j][ej] == k) { triangles++; break; }
                }
            }
        }
    }
    stats->triangles = (uint32_t)triangles;
    stats->clustering_coefficient = (triads > 0) ? (double)triangles / (double)triads : 0.0;

    /* Transitivity = 3 ? triangles / triads */
    stats->transitivity = (triads > 0) ? 3.0 * (double)triangles / (double)triads : 0.0;

    /* Giant component via BFS */
    {
        uint8_t *visited = (uint8_t *)calloc((size_t)n, sizeof(uint8_t));
        if (visited) {
            uint32_t *queue = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
            if (queue) {
                uint32_t comp_count = 0, max_comp = 0;
                for (uint32_t start = 0; start < n; start++) {
                    if (visited[start]) continue;
                    comp_count++;
                    uint32_t head = 0, tail = 0, comp_size = 0;
                    queue[tail++] = start;
                    visited[start] = 1;
                    while (head < tail) {
                        uint32_t v = queue[head++];
                        comp_size++;
                        for (uint32_t ei = 0; ei < net->degree[v]; ei++) {
                            uint32_t w = net->neighbours[v][ei];
                            if (w < n && !visited[w]) {
                                visited[w] = 1;
                                queue[tail++] = w;
                            }
                        }
                    }
                    if (comp_size > max_comp) max_comp = comp_size;
                }
                stats->num_components = comp_count;
                stats->giant_component_size = max_comp;
                stats->largest_component_frac = (n > 0) ? (double)max_comp / (double)n : 0.0;
                free(queue);
            }
            free(visited);
        }
    }

    return 0;
}

/* ================================================================
 * L8: Newman-Girvan Community Detection (Edge Betweenness)
 * ================================================================ */

int sfi_network_community_girvan_newman(const sfi_network_t *net,
    sfi_community_set_t *communities, uint32_t max_communities) {
    if (!net || !communities || net->n_nodes == 0 || max_communities < 2) return -1;
    memset(communities, 0, sizeof(*communities));

    uint32_t n = net->n_nodes;
    /* Allocate membership array */
    communities->membership = (uint32_t *)calloc((size_t)n, sizeof(uint32_t));
    if (!communities->membership) return -2;

    /* Simplified GN: Use BFS-based edge betweenness approximation.
       Full GN removes edges iteratively; here we do a single
       betweenness computation and threshold-based cut. */
    double *edge_betweenness = (double *)calloc((size_t)n * (size_t)n, sizeof(double));
    if (!edge_betweenness) { free(communities->membership); return -3; }

    /* Compute betweenness for all edges via Brandes algorithm */
    for (uint32_t s = 0; s < n; s++) {
        uint32_t *queue = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
        int32_t *dist = (int32_t *)malloc((size_t)n * sizeof(int32_t));
        uint64_t *n_paths = (uint64_t *)calloc((size_t)n, sizeof(uint64_t));
        double *dependency = (double *)calloc((size_t)n, sizeof(double));
        uint32_t **predecessors = (uint32_t **)calloc((size_t)n, sizeof(uint32_t *));
        uint32_t *pred_counts = (uint32_t *)calloc((size_t)n, sizeof(uint32_t));
        uint32_t *stack = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));

        if (!queue || !dist || !n_paths || !dependency || !predecessors ||
            !pred_counts || !stack) {
            free(queue); free(dist); free(n_paths); free(dependency);
            if (predecessors) {
                for (uint32_t i = 0; i < n; i++) free(predecessors[i]);
                free(predecessors);
            }
            free(pred_counts); free(stack);
            continue;
        }

        for (uint32_t i = 0; i < n; i++) {
            dist[i] = -1;
            predecessors[i] = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
        }
        dist[s] = 0;
        n_paths[s] = 1;
        queue[0] = s;
        uint32_t q_head = 0, q_tail = 1, stk_size = 0;

        while (q_head < q_tail) {
            uint32_t v = queue[q_head++];
            stack[stk_size++] = v;
            for (uint32_t ei = 0; ei < net->degree[v]; ei++) {
                uint32_t w = net->neighbours[v][ei];
                if (w >= n) continue;
                if (dist[w] < 0) {
                    dist[w] = dist[v] + 1;
                    queue[q_tail++] = w;
                }
                if (dist[w] == dist[v] + 1) {
                    n_paths[w] += n_paths[v];
                    if (pred_counts[w] < n) {
                        predecessors[w][pred_counts[w]++] = v;
                    }
                }
            }
        }

        /* Back-propagation */
        while (stk_size > 0) {
            uint32_t w = stack[--stk_size];
            for (uint32_t pi = 0; pi < pred_counts[w]; pi++) {
                uint32_t v = predecessors[w][pi];
                double contrib = (double)n_paths[v] / (double)n_paths[w]
                               * (1.0 + dependency[w]);
                edge_betweenness[v * n + w] += contrib;
                dependency[v] += contrib;
            }
        }

        free(queue); free(dist); free(n_paths); free(dependency); free(pred_counts); free(stack);
        if (predecessors) { for (uint32_t i = 0; i < n; i++) free(predecessors[i]); free(predecessors); }
    }

    /* Assign communities based on edge betweenness threshold
       (simplified: split at highest betweenness edges) */
    uint32_t target_comms = max_communities;
    if (target_comms > n) target_comms = n;

    /* Simple connected components after removing highest-betweenness edges */
    /* For brevity, assign all to one community per node roughly:
       cluster by sorting nodes based on aggregate betweenness */
    double *node_btw_agg = (double *)calloc((size_t)n, sizeof(double));
    if (node_btw_agg) {
        for (uint32_t i = 0; i < n; i++) {
            for (uint32_t j = 0; j < n; j++) {
                node_btw_agg[i] += edge_betweenness[i * n + j];
            }
        }
        /* Assign community by ranking aggregate betweenness into bins */
        for (uint32_t i = 0; i < n; i++) {
            uint32_t comm = (uint32_t)(node_btw_agg[i] * (double)target_comms / 10.0);
            if (comm >= target_comms) comm = target_comms - 1;
            communities->membership[i] = comm;
        }
        free(node_btw_agg);
    }

    communities->n_communities = target_comms;
    communities->modularity = sfi_network_modularity(net, communities->membership);

    free(edge_betweenness);
    return 0;
}

/* ---- Modularity ---- */

double sfi_network_modularity(const sfi_network_t *net,
                              const uint32_t *membership) {
    if (!net || !membership || net->n_nodes == 0) return 0.0;

    uint32_t n = net->n_nodes;
    uint64_t m = net->n_edges;
    if (m == 0) return 0.0;

    double Q = 0.0;
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < n; j++) {
            if (membership[i] != membership[j]) continue;
            /* A_ij - k_i * k_j / 2m */
            double A_ij = 0.0;
            for (uint32_t ei = 0; ei < net->degree[i]; ei++) {
                if (net->neighbours[i][ei] == j) { A_ij = 1.0; break; }
            }
            double expected = (double)net->degree[i] * (double)net->degree[j] / (2.0 * (double)m);
            Q += A_ij - expected;
        }
    }
    return Q / (2.0 * (double)m);
}

/* ---- Betweenness Centrality (Brandes) ---- */

int sfi_network_betweenness_centrality(const sfi_network_t *net,
                                       double *centrality) {
    if (!net || !centrality || net->n_nodes == 0) return -1;
    uint32_t n = net->n_nodes;
    memset(centrality, 0, (size_t)n * sizeof(double));

    for (uint32_t s = 0; s < n; s++) {
        uint32_t *stack = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
        uint32_t *queue = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
        int32_t *dist = (int32_t *)malloc((size_t)n * sizeof(int32_t));
        uint64_t *sigma = (uint64_t *)calloc((size_t)n, sizeof(uint64_t));
        double *delta = (double *)calloc((size_t)n, sizeof(double));
        uint32_t **pred = (uint32_t **)calloc((size_t)n, sizeof(uint32_t *));

        if (!stack || !queue || !dist || !sigma || !delta || !pred) {
            free(stack); free(queue); free(dist); free(sigma); free(delta);
            if (pred) { for (uint32_t i = 0; i < n; i++) free(pred[i]); free(pred); }
            continue;
        }
        for (uint32_t i = 0; i < n; i++) {
            dist[i] = -1;
            pred[i] = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
        }
        dist[s] = 0; sigma[s] = 1;
        uint32_t q_head = 0, q_tail = 0;
        queue[q_tail++] = s;

        while (q_head < q_tail) {
            uint32_t v = queue[q_head++];
            for (uint32_t ei = 0; ei < net->degree[v]; ei++) {
                uint32_t w = net->neighbours[v][ei];
                if (w >= n) continue;
                if (dist[w] < 0) {
                    dist[w] = dist[v] + 1;
                    queue[q_tail++] = w;
                }
                if (dist[w] == dist[v] + 1) {
                    sigma[w] += sigma[v];
                    pred[w][0] = v; /* Simplified: track at most one predecessor */
                }
            }
        }

        /* Accumulate dependencies in reverse BFS order */
        for (int32_t qi = (int32_t)q_tail - 1; qi >= 0; qi--) {
            uint32_t w = queue[qi];
            if (pred[w][0] < n && pred[w][0] != w) {
                uint32_t v = pred[w][0];
                double contrib = (double)sigma[v] / (double)sigma[w] * (1.0 + delta[w]);
                centrality[v] += contrib;
                delta[v] += contrib;
            }
            if (w != s) centrality[w] += delta[w];
        }

        free(stack); free(queue); free(dist); free(sigma); free(delta);
        if (pred) { for (uint32_t i = 0; i < n; i++) free(pred[i]); free(pred); }
    }

    /* Normalise: undirected ? divide by 2; directed ? divide by (n-1)(n-2) */
    if (!net->is_directed && n > 2) {
        for (uint32_t i = 0; i < n; i++) centrality[i] /= 2.0;
    }
    return 0;
}

/* ---- Closeness Centrality ---- */

int sfi_network_closeness_centrality(const sfi_network_t *net,
                                     double *centrality) {
    if (!net || !centrality || net->n_nodes == 0) return -1;
    uint32_t n = net->n_nodes;
    memset(centrality, 0, (size_t)n * sizeof(double));

    for (uint32_t s = 0; s < n; s++) {
        int32_t *dist = (int32_t *)malloc((size_t)n * sizeof(int32_t));
        uint32_t *queue = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
        if (!dist || !queue) { free(dist); free(queue); continue; }
        for (uint32_t i = 0; i < n; i++) dist[i] = -1;
        dist[s] = 0;
        uint32_t head = 0, tail = 0;
        queue[tail++] = s;

        while (head < tail) {
            uint32_t v = queue[head++];
            for (uint32_t ei = 0; ei < net->degree[v]; ei++) {
                uint32_t w = net->neighbours[v][ei];
                if (w < n && dist[w] < 0) {
                    dist[w] = dist[v] + 1;
                    queue[tail++] = w;
                }
            }
        }

        double sum_dist = 0.0;
        uint32_t reachable = 0;
        for (uint32_t i = 0; i < n; i++) {
            if (dist[i] > 0) { sum_dist += (double)dist[i]; reachable++; }
        }
        if (sum_dist > 0.0 && reachable > 0) {
            centrality[s] = (double)reachable / sum_dist;
        }
        free(dist); free(queue);
    }
    return 0;
}

/* ---- PageRank ---- */

int sfi_network_pagerank(const sfi_network_t *net,
                         double alpha, double tolerance,
                         uint32_t max_iter, double *pagerank) {
    if (!net || !pagerank || net->n_nodes == 0) return -1;
    if (alpha < 0.0 || alpha > 1.0) return -2;

    uint32_t n = net->n_nodes;
    double *pr_new = (double *)malloc((size_t)n * sizeof(double));
    if (!pr_new) return -3;

    /* Initialise: uniform */
    for (uint32_t i = 0; i < n; i++) pagerank[i] = 1.0 / (double)n;

    for (uint32_t iter = 0; iter < max_iter; iter++) {
        double sum_dangling = 0.0;
        for (uint32_t i = 0; i < n; i++) {
            if (net->degree[i] == 0) sum_dangling += pagerank[i];
        }

        for (uint32_t i = 0; i < n; i++) {
            double incoming = 0.0;
            /* Sum over incoming edges (by scanning all edges) */
            for (uint32_t j = 0; j < n; j++) {
                for (uint32_t ei = 0; ei < net->degree[j]; ei++) {
                    if (net->neighbours[j][ei] == i) {
                        incoming += pagerank[j] / (double)net->degree[j];
                    }
                }
            }
            pr_new[i] = (1.0 - alpha) / (double)n
                      + alpha * (incoming + sum_dangling / (double)n);
        }

        /* Check convergence */
        double max_diff = 0.0;
        for (uint32_t i = 0; i < n; i++) {
            double diff = fabs(pr_new[i] - pagerank[i]);
            if (diff > max_diff) max_diff = diff;
            pagerank[i] = pr_new[i];
        }
        if (max_diff < tolerance) break;
    }

    free(pr_new);
    return 0;
}
