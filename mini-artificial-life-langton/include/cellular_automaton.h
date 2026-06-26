/**
 * @file cellular_automaton.h
 * @brief General-purpose Cellular Automaton framework
 *
 * Supports 1D and 2D cellular automata with configurable:
 *   - Neighborhoods (von Neumann, Moore, extended)
 *   - State alphabets (binary, k-state)
 *   - Rule types (elementary, totalistic, outer totalistic)
 *   - Boundary conditions (fixed, periodic, reflective)
 *
 * Key references:
 *   - von Neumann, J. (1966) "Theory of Self-Reproducing Automata"
 *   - Wolfram, S. (2002) "A New Kind of Science"
 *   - Conway, J.H. (1970) "The Game of Life"
 *   - Toffoli, T. & Margolus, N. (1987) "Cellular Automata Machines"
 *
 * Course alignment: MIT 6.045, Stanford CS358, Santa Fe Institute CSS
 */

#ifndef CELLULAR_AUTOMATON_H
#define CELLULAR_AUTOMATON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* L1: Core Definitions */

/** CA dimension */
typedef enum {
    CA_DIM_1D = 1,
    CA_DIM_2D = 2
} ca_dimension_t;

/** Boundary condition type */
typedef enum {
    CA_BOUNDARY_FIXED,      /**< cells outside grid are state 0 */
    CA_BOUNDARY_PERIODIC,   /**< wrap-around (toroidal) */
    CA_BOUNDARY_REFLECTIVE  /**< mirror at boundary */
} ca_boundary_t;

/** Neighborhood type for 2D CA */
typedef enum {
    CA_NEIGH_VON_NEUMANN,   /**< 4 orthogonal neighbors */
    CA_NEIGH_MOORE,         /**< 8 surrounding neighbors */
    CA_NEIGH_EXTENDED_MOORE /**< radius-2 Moore neighborhood (24 neighbors) */
} ca_neighborhood_t;

/** Rule type classification */
typedef enum {
    CA_RULE_ELEMENTARY,       /**< 1D binary, radius-1 (Wolfram rules 0-255) */
    CA_RULE_TOTALISTIC,       /**< depends only on sum of neighbor states */
    CA_RULE_OUTER_TOTALISTIC, /**< depends on own state + sum of neighbors */
    CA_RULE_GENERAL           /**< arbitrary lookup table */
} ca_rule_type_t;

/** Wolfram class (qualitative CA behavior) */
typedef enum {
    CA_CLASS_I,     /**< Evolution to homogeneous state */
    CA_CLASS_II,    /**< Evolution to stable/periodic structures */
    CA_CLASS_III,   /**< Chaotic, aperiodic patterns */
    CA_CLASS_IV,    /**< Complex, localized structures (edge of chaos) */
    CA_CLASS_UNKNOWN
} ca_wolfram_class_t;

/** CA configuration parameters */
typedef struct {
    ca_dimension_t dim;           /**< 1D or 2D */
    int64_t width;                /**< x-dimension */
    int64_t height;               /**< y-dimension (1 for 1D) */
    int num_states;               /**< alphabet size k (2 for binary) */
    ca_boundary_t boundary;       /**< boundary condition */
    ca_neighborhood_t neighbor;   /**< neighborhood type (2D only) */
    int radius;                   /**< neighborhood radius (1D: radius, 2D: unused) */
    ca_rule_type_t rule_type;     /**< rule semantics */
    void *rule_data;              /**< rule-specific data (lookup table, etc.) */
    uint64_t *rule_lut;           /**< precomputed lookup table */
    size_t rule_lut_size;         /**< size of lookup table */
} ca_config_t;

/** CA state (one time step) */
typedef struct {
    uint8_t *cells;         /**< current cell states [width * height] */
    uint8_t *next_cells;    /**< buffer for next generation */
    int64_t width;
    int64_t height;
    ca_config_t config;     /**< CA configuration */
    uint64_t generation;    /**< current generation number */
} ca_state_t;

/** 1D CA rule (Wolfram encoding for elementary CA) */
typedef struct {
    uint8_t rule_number;    /**< Wolfram rule number 0-255 */
    uint8_t rule_bits[8];   /**< expanded rule: output for each 3-bit neighborhood */
} ca_rule_elementary_t;

/** 2D totalistic rule (e.g., Game of Life: B3/S23) */
typedef struct {
    int num_states;         /**< k */
    int max_sum;            /**< maximum possible neighbor sum */
    uint8_t *birth;         /**< birth conditions (when sum matches, 0->1) */
    uint8_t *survival;      /**< survival conditions (when sum matches, 1->1) */
    size_t birth_count;     /**< number of birth rules */
    size_t survival_count;  /**< number of survival rules */
} ca_rule_totalistic_t;

/* L2: Core Concepts ? CA Operations */

/** Create a CA configuration with default parameters */
ca_config_t ca_config_default(ca_dimension_t dim, int64_t width, int64_t height);

/** Create an elementary 1D CA rule from Wolfram number (0-255) */
ca_rule_elementary_t ca_rule_elementary_create(uint8_t rule_number);

/** Create a 2D totalistic rule (B/S notation). e.g., ca_rule_totalistic_create(2, "3", "23") for Life */
ca_rule_totalistic_t ca_rule_totalistic_create(int num_states, const char *birth_str, const char *survival_str);

/** Set totalistic rule into CA config */
bool ca_config_set_totalistic(ca_config_t *config, const ca_rule_totalistic_t *rule);

/** Set elementary rule into CA config */
bool ca_config_set_elementary(ca_config_t *config, const ca_rule_elementary_t *rule);

/** Free rule data inside config */
void ca_config_free(ca_config_t *config);

/** Create a CA state with given config. Allocates cell buffers. */
ca_state_t *ca_state_create(const ca_config_t *config);

/** Free CA state */
void ca_state_destroy(ca_state_t *state);

/** Get cell state at (x,y), handling boundary conditions */
uint8_t ca_state_get(const ca_state_t *state, int64_t x, int64_t y);

/** Set cell state at (x,y) */
void ca_state_set(ca_state_t *state, int64_t x, int64_t y, uint8_t value);

/** Randomize all cells with uniform distribution over num_states */
void ca_state_randomize(ca_state_t *state, unsigned int seed);

/** Set all cells to a specific value */
void ca_state_fill(ca_state_t *state, uint8_t value);

/** Set a single cell to 1, all others to 0 (point initial condition) */
void ca_state_point_init(ca_state_t *state, int64_t x, int64_t y);

/* L5: CA Evolution Engine */

/** Compute sum of neighbor states for cell (x,y) */
int ca_neighbor_sum_2d(const ca_state_t *state, int64_t x, int64_t y, ca_neighborhood_t type);

/** Compute neighborhood pattern index for 1D CA (radius r) */
uint64_t ca_neighbor_pattern_1d(const ca_state_t *state, int64_t x, int radius);

/** Evolve the CA one generation. Returns number of cells changed. */
uint64_t ca_evolve_step(ca_state_t *state);

/** Evolve CA for n generations */
uint64_t ca_evolve_run(ca_state_t *state, uint64_t n_generations);

/** Apply 1D elementary CA rule to entire row */
void ca_evolve_1d_elementary(ca_state_t *state);

/** Apply 2D totalistic rule */
void ca_evolve_2d_totalistic(ca_state_t *state);

/** Apply 2D outer-totalistic rule */
void ca_evolve_2d_outer_totalistic(ca_state_t *state);

/** Swap cell buffer pointers (double-buffering) */
void ca_swap_buffers(ca_state_t *state);

/* L4: Wolfram Classification */

/** Classify a 1D CA rule into Wolfram classes I-IV.
 *  Uses: sensitivity to initial conditions, entropy evolution,
 *  Lyapunov exponent estimation on CA spacetime patterns. */
ca_wolfram_class_t ca_classify_wolfram_1d(const ca_state_t *state, int num_steps);

/** Compute spatial entropy of CA configuration */
double ca_spatial_entropy(const ca_state_t *state);

/** Compute temporal entropy (entropy rate over time) */
double ca_temporal_entropy(ca_state_t *state, int window_size);

/** Compute difference pattern: XOR of two CA evolutions */
uint64_t ca_damage_spread(ca_state_t *state_a, ca_state_t *state_b, int steps);

/** Determine if rule is left/right symmetric */
bool ca_rule_is_symmetric(const ca_config_t *config);

/** Determine if rule is totalistic */
bool ca_rule_is_totalistic(const ca_config_t *config);

/** Enumerate all rules of a given type for systematic exploration */
int ca_enumerate_rules(ca_dimension_t dim, int num_states, ca_rule_type_t type,
                       void *rules_out, size_t max_rules);

/* L8: Advanced CA Analysis */

/** Compute mutual information between a cell and its neighbors */
double ca_mutual_information(const ca_state_t *state, int64_t x, int64_t y);

/** Compute excess entropy (effective measure complexity) of CA spacetime */
double ca_excess_entropy(const ca_state_t *states[], int num_states);

/** Compute the lambda parameter (Langton's edge-of-chaos measure) for a CA rule.
 *  ? = fraction of rule table entries that map to non-quiescent state.
 *  Edge of chaos is typically at ? ? 0.273 for k=2, N=5. */
double ca_lambda_parameter(const ca_config_t *config);

/** Test if CA rule is number-conserving (total count of non-zero states invariant) */
bool ca_rule_is_number_conserving(const ca_config_t *config);

/** Detect gliders in 2D CA: localized patterns that translate periodically */
bool ca_detect_glider(const ca_state_t *state, int64_t *gx, int64_t *gy,
                      int64_t *dx, int64_t *dy, int *period);

/** Save CA spacetime as PGM image (1D CA: x=space, y=time) */
int64_t ca_save_spacetime_pgm(const ca_state_t **history, int num_states,
                              const char *filename);

#endif /* CELLULAR_AUTOMATON_H */