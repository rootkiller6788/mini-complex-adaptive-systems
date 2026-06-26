/**
 * @file    sfi_macrosystems.h
 * @brief   SFI Methodology: Macro-level Complex Adaptive System types
 *
 * Implements the foundational SFI (Santa Fe Institute) approach to
 * modeling complex adaptive systems as populations of interacting
 * heterogeneous agents.  Captures the macro-state / micro-state
 * duality that is central to the SFI methodology.
 *
 * Reference: Holland, "Hidden Order" (1995); Miller & Page,
 * "Complex Adaptive Systems" (2007)
 *
 * Knowledge Levels: L1 (Definitions), L2 (Core Concepts),
 *                   L3 (Mathematical Structures)
 *
 * Course alignment: SFI Summer School (CSSS) core curriculum
 */

#ifndef SFI_MACROSYSTEMS_H_
#define SFI_MACROSYSTEMS_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SFI_MAX_STRATEGY_LEN     128
#define SFI_MAX_NEIGHBOURS        64
#define SFI_MAX_RULES            256

/**
 * L1: sfi_agent_t — Atomic unit of a CAS
 */
typedef struct {
    uint32_t  id;
    int32_t   x, y;
    double    energy;
    double    age;
    uint32_t  genotype[4];
    double    strategy[SFI_MAX_STRATEGY_LEN];
    uint32_t  strategy_len;
    double    payoff;
    uint32_t  parent_id;
    uint32_t  generation;
    int32_t   alive;
} sfi_agent_t;

typedef enum {
    SFI_TOPO_FULL_MIXING      = 0,
    SFI_TOPO_2D_GRID_TORUS    = 1,
    SFI_TOPO_RANDOM_GRAPH     = 2,
    SFI_TOPO_SCALE_FREE       = 3,
    SFI_TOPO_SMALL_WORLD      = 4,
    SFI_TOPO_CA_GEOGRAPHIC    = 5,
    SFI_TOPO_NETWORK_STATIC   = 6
} sfi_topology_t;

typedef struct {
    uint32_t  agent_id;
    double    weight;
    int32_t   active;
} sfi_neighbour_t;

typedef struct {
    int32_t   width;
    int32_t   height;
    double   *resources;
    double   *diffusion_buf;
    double    regrowth_rate;
    double    max_resource;
    int32_t   torus;
    double   *altitude;
    uint64_t  tick;
    double    gini_coefficient;
    double    mean_wealth;
    double    total_resource;
    uint64_t  agent_population;
    uint64_t  total_births;
    uint64_t  total_deaths;
} sfi_environment_t;

typedef struct {
    double    mean_wealth;
    double    gini_index;
    double    spatial_clustering;
    double    strategy_diversity;
    double    population;
    double    mean_age;
    double    birth_rate;
    double    death_rate;
    double    mean_interaction_count;
    double    modularity;
    double    assortativity;
    double    largest_component_frac;
} sfi_macrostate_t;

typedef struct {
    sfi_agent_t    *agents;
    uint64_t        capacity;
    uint64_t        size;
    sfi_topology_t  topology;
    sfi_neighbour_t *adjacency;
    uint32_t       *degree;
    uint64_t        max_edges;
    uint64_t        edge_count;
    double          mutation_rate;
    double          crossover_rate;
    int32_t         selection_pressure;
} sfi_population_t;

/* ---- Population API ---- */
int     sfi_population_init(sfi_population_t *pop, uint64_t capacity,
                            sfi_topology_t topo, double mutation_rate);
void    sfi_population_destroy(sfi_population_t *pop);
int64_t sfi_population_add_agent(sfi_population_t *pop,
                                 int32_t x, int32_t y, double energy);
uint64_t sfi_population_compact_dead(sfi_population_t *pop);
int     sfi_population_build_topology(sfi_population_t *pop,
                                      int32_t env_width, int32_t env_height);
void    sfi_compute_macrostate(const sfi_population_t *pop,
                               const sfi_environment_t *env,
                               sfi_macrostate_t *out);

/* ---- Environment API ---- */
int    sfi_environment_init(sfi_environment_t *env,
                            int32_t width, int32_t height,
                            double regrowth_rate, double max_resource);
void   sfi_environment_destroy(sfi_environment_t *env);
void   sfi_environment_diffuse(sfi_environment_t *env, double rate);
void   sfi_environment_regrow(sfi_environment_t *env);
double sfi_environment_harvest(sfi_environment_t *env,
                               int32_t x, int32_t y, double requested);

/* ---- Topology API ---- */
int sfi_topology_generate_scale_free(sfi_population_t *pop,
                                     uint32_t m0, uint32_t m);
int sfi_topology_generate_small_world(sfi_population_t *pop,
                                      uint32_t k, double beta);
int sfi_topology_generate_full_mixing(sfi_population_t *pop);

#ifdef __cplusplus
}
#endif

#endif /* SFI_MACROSYSTEMS_H_ */
