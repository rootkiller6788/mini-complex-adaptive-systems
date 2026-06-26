/**
 * @file    sfi_network_methods.h
 * @brief   SFI Methodology: Network science methods for CAS
 *
 * Implements the SFI network-science toolkit: network generation
 * (Erdos-R?nyi, Barab?si-Albert, Watts-Strogatz), community
 * detection (Newman-Girvan, Louvain), centrality measures,
 * and network-based emergence detection.
 *
 * References: Newman "Networks" (2018); Barab?si "Network Science"
 * (2016); Watts & Strogatz (1998); Girvan & Newman (2002)
 *
 * Knowledge Levels: L1 (Definitions), L2 (Concepts),
 *                   L3 (Math), L5 (Algorithms)
 */

#ifndef SFI_NETWORK_METHODS_H_
#define SFI_NETWORK_METHODS_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * L1: Network Definitions
 * ================================================================ */

/**
 * L1: sfi_network_t ? Adjacency-list network representation
 *
 * SFI emphasis: networks are NOT just data structures;
 * they encode the interaction topology that shapes
 * emergent behaviour.
 */
typedef struct {
    uint32_t  n_nodes;               /**< Number of vertices */
    uint64_t  n_edges;               /**< Number of directed edges */
    uint32_t *degree;                /**< Out-degree of each node */
    uint32_t *in_degree;             /**< In-degree (directed) */
    uint32_t **neighbours;           /**< Adjacency lists (row=source) */
    uint32_t *neighbour_capacities;  /**< Allocated size per row */
    double   *node_values;           /**< Scalar value per node (e.g., opinion, wealth) */
    uint32_t  is_directed;           /**< 1 = directed, 0 = undirected */
    uint32_t  is_weighted;           /**< 1 = use weights array */
    double   *edge_weights;          /**< Weight per edge (flattened) */
} sfi_network_t;

/**
 * L1: sfi_community_t ? A detected community/cluster
 */
typedef struct {
    uint32_t  id;
    uint32_t *members;              /**< Node indices in this community */
    uint32_t  size;
    double    internal_density;     /**< Edges inside / possible inside */
    double    conductance;           /**< Cut size / min volume */
    double    modularity_contrib;    /**< Contribution to overall modularity Q */
} sfi_community_t;

/**
 * L1: sfi_community_set_t ? Complete community partition
 */
typedef struct {
    sfi_community_t *communities;
    uint32_t          n_communities;
    double            modularity;     /**< Newman-Girvan modularity Q */
    uint32_t         *membership;     /**< Community ID per node */
} sfi_community_set_t;

/* ================================================================
 * L3: Network Statistics
 * ================================================================ */

/**
 * L3: sfi_network_stats_t ? Full network statistics profile
 */
typedef struct {
    double   mean_degree;              /**< Average degree <k> */
    double   degree_variance;          /**< Var(k) */
    double   degree_assortativity;     /**< Degree correlation coefficient r */
    double   clustering_coefficient;   /**< Global clustering C */
    double   average_path_length;      /**< Mean geodesic distance L */
    double   diameter;                 /**< Maximum geodesic distance d_max */
    double   density;                  /**< Edge density 2m/n(n-1) */
    double   reciprocity;              /**< Mutual edge fraction (directed) */
    double   transitivity;             /**< Triangle fraction T */
    double   powerlaw_exponent;        /**< Fitted alpha: P(k) ~ k^{-alpha} */
    double   powerlaw_xmin;            /**< Lower cutoff for power-law fit */
    double   powerlaw_ks;              /**< KS statistic for power-law test */
    uint32_t num_components;           /**< Number of connected components */
    uint32_t giant_component_size;     /**< Size of largest component */
    double   largest_component_frac;   /**< Giant component size fraction */
    double   bow_tie_scc_frac;         /**< SCC fraction (directed) */
    uint32_t triangles;                /**< Total triangle count */
} sfi_network_stats_t;

/* ================================================================
 * L5: Network Generation Algorithms
 * ================================================================ */

/**
 * L5: Generate Erdos-R?nyi G(n, p) random graph.
 *
 * Each of n(n-1)/2 edges present independently with prob p.
 * SFI uses this as the null model ? no structure beyond
 * statistical expectation.
 * Complexity: O(n?) ? checking all possible edges.
 */
int sfi_network_generate_erdos_renyi(sfi_network_t *net,
    uint32_t n, double p, uint64_t seed);

/**
 * L5: Generate Barab?si-Albert scale-free network.
 *
 * Preferential attachment: new node connects to m existing
 * nodes with probability proportional to their current degree.
 * SFI foundational result: this generates power-law degree
 * distributions P(k) ~ k^{-3}.
 * Complexity: O(n * m).
 */
int sfi_network_generate_barabasi_albert(sfi_network_t *net,
    uint32_t n, uint32_t m0, uint32_t m, uint64_t seed);

/**
 * L5: Generate Watts-Strogatz small-world network.
 *
 * Start with ring lattice of degree k, rewire each edge
 * with probability beta.  Beta=0 ? regular lattice;
 * beta=1 ? random graph.  Small-world regime at small beta.
 * Complexity: O(n * k).
 */
int sfi_network_generate_watts_strogatz(sfi_network_t *net,
    uint32_t n, uint32_t k, double beta, uint64_t seed);

/* ---- L5: Network Analysis Algorithms ---- */

/**
 * L5: Compute full network statistics.
 * Complexity: O(n * <k>) for most measures.
 */
int sfi_network_compute_stats(const sfi_network_t *net,
                              sfi_network_stats_t *stats);

/**
 * L5: Detect communities using Newman-Girvan edge-betweenness.
 *
 * SFI classic: iteratively remove edge with highest betweenness.
 * Hierarchical decomposition of network into communities.
 * Complexity: O(m? n) or O(n?) for dense networks.
 */
int sfi_network_community_girvan_newman(const sfi_network_t *net,
    sfi_community_set_t *communities, uint32_t max_communities);

/**
 * L5: Compute Newman-Girvan modularity Q of a partition.
 *
 * Q = (1/2m) ?_ij [A_ij - k_i k_j / 2m] ?(c_i, c_j)
 * Measures quality of community structure.
 * Complexity: O(n + m).
 */
double sfi_network_modularity(const sfi_network_t *net,
                              const uint32_t *membership);

/**
 * L5: Compute betweenness centrality for all nodes.
 *
 * Brandes algorithm.  SFI uses centrality as a proxy
 * for agent influence in social/economic CAS.
 * Complexity: O(n * m) unweighted, O(n m + n? log n) weighted.
 */
int sfi_network_betweenness_centrality(const sfi_network_t *net,
                                       double *centrality);

/**
 * L5: Compute closeness centrality for all nodes.
 *
 * C_c(i) = (n-1) / ?_j d(i,j) where d = geodesic distance.
 * Complexity: O(n * (n + m)) via BFS/Dijkstra per node.
 */
int sfi_network_closeness_centrality(const sfi_network_t *net,
                                     double *centrality);

/**
 * L5: Compute eigenvector centrality (PageRank variant).
 *
 * xi = alpha * A^T xi + (1-alpha)/n * 1
 * Complexity: O(m * iterations) via power method.
 */
int sfi_network_pagerank(const sfi_network_t *net,
                         double alpha, double tolerance,
                         uint32_t max_iter, double *pagerank);

/**
 * L5: Free network resources.
 */
void sfi_network_destroy(sfi_network_t *net);

/**
 * L5: Free community set resources.
 */
void sfi_community_set_destroy(sfi_community_set_t *cs);

#ifdef __cplusplus
}
#endif

#endif /* SFI_NETWORK_METHODS_H_ */
