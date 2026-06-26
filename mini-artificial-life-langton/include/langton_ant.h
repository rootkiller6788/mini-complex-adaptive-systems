/**
 * @file langton_ant.h
 * @brief Langton's Ant ? the canonical artificial life cellular automaton
 *
 * Chris Langton (1986) introduced this 2-state, 2D Turing machine on a
 * square grid. The ant follows a simple deterministic rule:
 *   - On a white cell: turn right 90?, flip to black, move forward
 *   - On a black cell: turn left 90?, flip to white, move forward
 *
 * The system exhibits a phase transition: after ~10,000 steps of chaotic
 * motion, the ant spontaneously constructs a periodic "highway" pattern.
 * This is one of the simplest known systems displaying emergent order.
 *
 * Key references:
 *   - Langton, C.G. (1986) "Studying artificial life with cellular automata"
 *   - Gajardo, A. et al. (2006) "Langton's ant is universal"
 *   - Bunimovich, L.A. & Troubetzkoy, S.E. (1992) "Recurrence properties
 *     of Lorentz lattice gas cellular automata"
 *
 * Course alignment: Santa Fe Institute CSS, MIT 6.045, Caltech CS154
 */

#ifndef LANGTON_ANT_H
#define LANGTON_ANT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* L1: Core Definitions ? Grid, Ant, Direction, Color */

/** Grid cell color ? Langton's Ant uses only two states */
typedef enum {
    LANT_COLOR_WHITE = 0,
    LANT_COLOR_BLACK = 1
} lant_color_t;

/** Cardinal direction for ant orientation */
typedef enum {
    LANT_DIR_NORTH = 0,
    LANT_DIR_EAST  = 1,
    LANT_DIR_SOUTH = 2,
    LANT_DIR_WEST  = 3
} lant_dir_t;

/** A single Langton ant agent */
typedef struct {
    int64_t x;
    int64_t y;
    lant_dir_t dir;
    uint64_t steps;
} langton_ant_t;

/** Grid configuration ? the CA universe */
typedef struct {
    uint8_t *cells;
    int64_t width;
    int64_t height;
    uint64_t total_flips;
    int64_t min_x;
    int64_t max_x;
    int64_t min_y;
    int64_t max_y;
} langton_grid_t;

/** Simulation configuration */
typedef struct {
    int64_t grid_width;
    int64_t grid_height;
    int64_t max_steps;
    unsigned int seed;
    bool toroidal;
    bool auto_expand;
    bool track_history;
} lant_config_t;

/** Simulation state and metadata */
typedef struct {
    langton_grid_t *grid;
    langton_ant_t *ants;
    size_t num_ants;
    size_t ants_capacity;
    uint64_t step_count;
    bool highway_detected;
    uint64_t highway_start_step;
    double entropy_estimate;
    lant_config_t config;
    int64_t *x_history;
    int64_t *y_history;
    size_t history_len;
    size_t history_cap;
} lant_sim_t;

/* L2: Core Concepts ? Grid Operations */

/** Allocate and initialize an empty grid (all cells white). O(width*height) */
langton_grid_t *lant_grid_create(int64_t width, int64_t height);

/** Free grid memory */
void lant_grid_destroy(langton_grid_t *grid);

/** Get cell color at (x,y). For toroidal grids: wraps coordinates.
 *  For non-toroidal grids: returns WHITE for out-of-bounds. */
lant_color_t lant_grid_get(const langton_grid_t *grid, int64_t x, int64_t y, bool toroidal);

/** Set cell color and update bounding box. O(1) */
void lant_grid_set(langton_grid_t *grid, int64_t x, int64_t y, lant_color_t color);

/** Flip a cell (white->black, black->white), return new color. O(1) */
lant_color_t lant_grid_flip(langton_grid_t *grid, int64_t x, int64_t y);

/** Expand grid to accommodate new dimensions. O(new_width*new_height). */
bool lant_grid_expand(langton_grid_t *grid, int64_t new_width, int64_t new_height);

/** Count black cells in the grid. O(width*height). */
uint64_t lant_grid_count_black(const langton_grid_t *grid);

/** Compute grid density rho = N_black / (width * height) */
double lant_grid_density(const langton_grid_t *grid);

/** Count black cells in a rectangular sub-region */
uint64_t lant_grid_region_count(const langton_grid_t *grid,
                                int64_t x0, int64_t y0, int64_t x1, int64_t y1);

/** Deep copy of grid */
langton_grid_t *lant_grid_clone(const langton_grid_t *src);

/** Save grid to PGM file for visualization */
int64_t lant_grid_save_pgm(const langton_grid_t *grid, const char *filename);

/** Print compact text representation of grid */
void lant_grid_print(const langton_grid_t *grid);

/** Create an ant at (x,y) facing direction dir */
langton_ant_t lant_ant_create(int64_t x, int64_t y, lant_dir_t dir);

/** Execute one Langton step for a single ant on the grid.
 *  Canonical rule: white->turn right, flip black, move; black->turn left, flip white, move.
 *  Returns old cell color before flipping. */
lant_color_t lant_ant_step(langton_ant_t *ant, langton_grid_t *grid, bool toroidal);

/** Turn ant direction: clockwise (+90deg) or counter-clockwise (-90deg) */
void lant_ant_turn(langton_ant_t *ant, bool clockwise);

/** Compute ant next forward position based on current direction */
void lant_ant_move_forward(const langton_ant_t *ant, int64_t *new_x, int64_t *new_y);

/* L5: Simulation Engine */

/** Create a new simulation with the given configuration */
lant_sim_t *lant_sim_create(const lant_config_t *config);

/** Free simulation and all associated memory */
void lant_sim_destroy(lant_sim_t *sim);

/** Add an ant to the simulation. Returns ant index, or -1 on failure. */
int lant_sim_add_ant(lant_sim_t *sim, int64_t x, int64_t y, lant_dir_t dir);

/** Run simulation for n steps. Returns steps actually executed. */
uint64_t lant_sim_run(lant_sim_t *sim, uint64_t n_steps);

/** Run simulation until highway is detected or max_steps reached. */
uint64_t lant_sim_run_until_highway(lant_sim_t *sim);

/** Run one step for all ants (round-robin) */
void lant_sim_step_all(lant_sim_t *sim);

/** Reset simulation to initial state */
void lant_sim_reset(lant_sim_t *sim);

/* L4: Highway Detection Algorithm */

/** Highway detection using bounding-box trajectory analysis.
 *  Characterized by: linear bounding-box growth, periodic trajectory,
 *  canonical period of 104 steps. */
bool lant_detect_highway(const lant_sim_t *sim);

/** Compute highway repeat period via autocorrelation of position deltas */
uint64_t lant_find_highway_period(const int64_t *x_hist, const int64_t *y_hist, size_t len);

/** Compute highway direction angle in degrees from x-axis */
double lant_highway_angle(const lant_sim_t *sim);

/** Compute highway drift velocity (cells per step) */
double lant_highway_velocity(const lant_sim_t *sim);

/* L8: Multi-Ant Extensions */

/** Spawn n_ants randomly distributed on the grid */
void lant_sim_spawn_random_ants(lant_sim_t *sim, size_t n_ants);

/** Resolve ant collisions (two ants same cell): both turn 180, swap colors.
 *  Returns number of collisions resolved. */
size_t lant_resolve_collisions(lant_sim_t *sim);

/** Ant-ant interaction rate: fraction of steps where ants occupy adjacent cells */
double lant_interaction_rate(const lant_sim_t *sim);

/** Run multi-ant simulation with collision detection */
void lant_multi_ant_run(lant_sim_t *sim, uint64_t n_steps);

/** Mean displacement of all ants from origin */
double lant_colony_radius(const lant_sim_t *sim);

/** Anisotropy of the ant colony (PCA ratio of spatial distribution) */
double lant_colony_anisotropy(const lant_sim_t *sim);

#endif /* LANGTON_ANT_H */