/**
 * @file langton_ant.c
 * @brief Langton's Ant implementation ? core simulation engine
 *
 * Implements the complete Langton's Ant cellular automaton:
 *   - Single-ant and multi-ant simulation
 *   - Dynamic grid expansion with data preservation
 *   - Highway detection via trajectory autocorrelation
 *   - Collision resolution for multi-ant scenarios
 *   - Entropy tracking and emergence analysis
 *
 * The canonical Langton's Ant rule (2-state, square grid):
 *   - On a white cell (0): turn RIGHT (CW), flip to BLACK (1), step forward
 *   - On a black cell (1): turn LEFT (CCW), flip to WHITE (0), step forward
 *
 * Key behaviors:
 *   - Steps 0-500: Simple symmetric patterns
 *   - Steps 500-10,000: Apparently chaotic (pseudo-random) motion
 *   - Steps 10,000+: Spontaneous emergence of periodic diagonal "highway"
 *   - Multiple ants: Complex interaction dynamics, mutual annihilation
 *
 * Theorem (Bunimovich & Troubetzkoy 1992):
 *   The Langton ant trajectory is unbounded for almost all initial configurations.
 *
 * Theorem (Gajardo et al. 2006):
 *   Langton's Ant with finite initial configuration is Turing universal.
 *   It can simulate any Turing machine via appropriate track configuration.
 *
 * Course alignment:
 *   - Santa Fe Institute CSS: Artificial Life unit
 *   - MIT 6.045/18.400: Cellular automata and emergence
 *   - Caltech CS 154: Limits of computation in natural systems
 */

#include "langton_ant.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Internal constants */
#define LANT_GRID_GROWTH_FACTOR 2.0
#define LANT_MIN_GRID_SIZE 64
#define LANT_MAX_HISTORY 100000
#define LANT_HIGHWAY_MIN_STEPS 5000
#define LANT_HIGHWAY_WINDOW 2000
#define LANT_HIGHWAY_PERIOD_CHECK 104
#define LANT_COLLISION_TURN true

/*??????????????????????????????????????????????????????????????????????
 * L2: Grid Allocation and Memory Management
 *??????????????????????????????????????????????????????????????????????*/

langton_grid_t *lant_grid_create(int64_t width, int64_t height)
{
    /* Clamp to minimum size */
    if (width < LANT_MIN_GRID_SIZE) width = LANT_MIN_GRID_SIZE;
    if (height < LANT_MIN_GRID_SIZE) height = LANT_MIN_GRID_SIZE;
    if (width <= 0 || height <= 0) return NULL;

    langton_grid_t *grid = (langton_grid_t *)calloc(1, sizeof(langton_grid_t));
    if (!grid) return NULL;

    grid->cells = (uint8_t *)calloc((size_t)(width * height), sizeof(uint8_t));
    if (!grid->cells) {
        free(grid);
        return NULL;
    }

    grid->width = width;
    grid->height = height;
    grid->total_flips = 0;
    grid->min_x = width;
    grid->max_x = 0;
    grid->min_y = height;
    grid->max_y = 0;
    return grid;
}

void lant_grid_destroy(langton_grid_t *grid)
{
    if (grid) {
        free(grid->cells);
        free(grid);
    }
}

/*??????????????????????????????????????????????????????????????????????
 * Grid Access with Boundary Handling
 *??????????????????????????????????????????????????????????????????????*/

lant_color_t lant_grid_get(const langton_grid_t *grid, int64_t x, int64_t y, bool toroidal)
{
    if (!grid) return LANT_COLOR_WHITE;

    if (toroidal) {
        /* Periodic boundary: map coordinates modulo grid dimensions */
        x = ((x % grid->width) + grid->width) % grid->width;
        y = ((y % grid->height) + grid->height) % grid->height;
    } else {
        /* Fixed boundary: return white (quiescent) for out-of-bounds */
        if (x < 0 || x >= grid->width || y < 0 || y >= grid->height) {
            return LANT_COLOR_WHITE;
        }
    }

    return (lant_color_t)grid->cells[y * grid->width + x];
}

void lant_grid_set(langton_grid_t *grid, int64_t x, int64_t y, lant_color_t color)
{
    if (!grid || x < 0 || x >= grid->width || y < 0 || y >= grid->height) return;

    grid->cells[y * grid->width + x] = (uint8_t)color;

    /* Update bounding box */
    if (x < grid->min_x) grid->min_x = x;
    if (x > grid->max_x) grid->max_x = x;
    if (y < grid->min_y) grid->min_y = y;
    if (y > grid->max_y) grid->max_y = y;
}

lant_color_t lant_grid_flip(langton_grid_t *grid, int64_t x, int64_t y)
{
    if (!grid || x < 0 || x >= grid->width || y < 0 || y >= grid->height) {
        return LANT_COLOR_WHITE;
    }

    lant_color_t old = (lant_color_t)grid->cells[y * grid->width + x];
    lant_color_t new_color = (old == LANT_COLOR_WHITE) ? LANT_COLOR_BLACK : LANT_COLOR_WHITE;
    grid->cells[y * grid->width + x] = (uint8_t)new_color;
    grid->total_flips++;

    /* Update bounding box */
    if (x < grid->min_x) grid->min_x = x;
    if (x > grid->max_x) grid->max_x = x;
    if (y < grid->min_y) grid->min_y = y;
    if (y > grid->max_y) grid->max_y = y;

    return old;
}

/*??????????????????????????????????????????????????????????????????????
 * Grid Expansion ? Dynamic Memory with Data Preservation
 *??????????????????????????????????????????????????????????????????????*/

bool lant_grid_expand(langton_grid_t *grid, int64_t new_width, int64_t new_height)
{
    if (!grid || new_width <= grid->width || new_height <= grid->height) {
        return false;
    }

    size_t new_size = (size_t)(new_width * new_height);
    uint8_t *new_cells = (uint8_t *)calloc(new_size, sizeof(uint8_t));
    if (!new_cells) return false;

    /* Copy old data preserving positions (top-left aligned) */
    for (int64_t y = 0; y < grid->height; y++) {
        memcpy(&new_cells[y * new_width],
               &grid->cells[y * grid->width],
               (size_t)grid->width * sizeof(uint8_t));
    }

    free(grid->cells);
    grid->cells = new_cells;
    grid->width = new_width;
    grid->height = new_height;
    return true;
}

/*??????????????????????????????????????????????????????????????????????
 * Grid Statistics
 *??????????????????????????????????????????????????????????????????????*/

uint64_t lant_grid_count_black(const langton_grid_t *grid)
{
    if (!grid) return 0;
    uint64_t count = 0;
    size_t total = (size_t)(grid->width * grid->height);
    for (size_t i = 0; i < total; i++) {
        if (grid->cells[i] == LANT_COLOR_BLACK) count++;
    }
    return count;
}

double lant_grid_density(const langton_grid_t *grid)
{
    if (!grid || grid->width <= 0 || grid->height <= 0) return 0.0;
    uint64_t n = lant_grid_count_black(grid);
    return (double)n / (double)(grid->width * grid->height);
}

uint64_t lant_grid_region_count(const langton_grid_t *grid,
                                int64_t x0, int64_t y0, int64_t x1, int64_t y1)
{
    if (!grid) return 0;

    /* Clamp coordinates to grid bounds */
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > grid->width) x1 = grid->width;
    if (y1 > grid->height) y1 = grid->height;
    if (x0 >= x1 || y0 >= y1) return 0;

    uint64_t count = 0;
    for (int64_t y = y0; y < y1; y++) {
        for (int64_t x = x0; x < x1; x++) {
            if (grid->cells[y * grid->width + x] == LANT_COLOR_BLACK) {
                count++;
            }
        }
    }
    return count;
}

langton_grid_t *lant_grid_clone(const langton_grid_t *src)
{
    if (!src) return NULL;
    langton_grid_t *dst = lant_grid_create(src->width, src->height);
    if (!dst) return NULL;

    size_t n = (size_t)(src->width * src->height);
    memcpy(dst->cells, src->cells, n * sizeof(uint8_t));
    dst->total_flips = src->total_flips;
    dst->min_x = src->min_x;
    dst->max_x = src->max_x;
    dst->min_y = src->min_y;
    dst->max_y = src->max_y;
    return dst;
}

/*??????????????????????????????????????????????????????????????????????
 * Grid Visualization
 *??????????????????????????????????????????????????????????????????????*/

int64_t lant_grid_save_pgm(const langton_grid_t *grid, const char *filename)
{
    if (!grid || !filename) return -1;

    FILE *fp = fopen(filename, "wb");
    if (!fp) return -1;

    /* PGM header: P5 (binary grayscale) */
    fprintf(fp, "P5\n%lld %lld\n255\n",
            (long long)grid->width, (long long)grid->height);

    /* Write pixel data: WHITE->255 (white), BLACK->0 (black) */
    size_t n = (size_t)(grid->width * grid->height);
    uint8_t *pixels = (uint8_t *)malloc(n);
    if (!pixels) { fclose(fp); return -1; }

    for (size_t i = 0; i < n; i++) {
        pixels[i] = (grid->cells[i] == LANT_COLOR_BLACK) ? 0 : 255;
    }

    size_t written = fwrite(pixels, 1, n, fp);
    free(pixels);
    fclose(fp);
    return (int64_t)written + (int64_t)15; /* header overhead */
}

void lant_grid_print(const langton_grid_t *grid)
{
    if (!grid) { printf("(null grid)\n"); return; }

    printf("Grid %lldx%lld, flips=%llu, bbox=[%lld,%lld]x[%lld,%lld]\n",
           (long long)grid->width, (long long)grid->height,
           (unsigned long long)grid->total_flips,
           (long long)grid->min_x, (long long)grid->max_x,
           (long long)grid->min_y, (long long)grid->max_y);

    /* Print a compact view: only the populated region + margin */
    int64_t margin = 2;
    int64_t x0 = (grid->min_x > margin) ? grid->min_x - margin : 0;
    int64_t y0 = (grid->min_y > margin) ? grid->min_y - margin : 0;
    int64_t x1 = (grid->max_x + margin < grid->width) ? grid->max_x + margin + 1 : grid->width;
    int64_t y1 = (grid->max_y + margin < grid->height) ? grid->max_y + margin + 1 : grid->height;

    /* Limit to reasonable display size */
    if (x1 - x0 > 80) { x1 = x0 + 80; }
    if (y1 - y0 > 40) { y1 = y0 + 40; }

    for (int64_t y = y0; y < y1; y++) {
        for (int64_t x = x0; x < x1; x++) {
            putchar(grid->cells[y * grid->width + x] ? '#' : '.');
        }
        putchar('\n');
    }
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Ant Core Operations
 *??????????????????????????????????????????????????????????????????????*/

langton_ant_t lant_ant_create(int64_t x, int64_t y, lant_dir_t dir)
{
    langton_ant_t ant;
    ant.x = x;
    ant.y = y;
    ant.dir = dir;
    ant.steps = 0;
    return ant;
}

void lant_ant_turn(langton_ant_t *ant, bool clockwise)
{
    if (!ant) return;
    if (clockwise) {
        /* Right turn: N->E, E->S, S->W, W->N */
        ant->dir = (lant_dir_t)(((int)ant->dir + 1) & 3);
    } else {
        /* Left turn: N->W, W->S, S->E, E->N */
        ant->dir = (lant_dir_t)(((int)ant->dir + 3) & 3);
    }
}

void lant_ant_move_forward(const langton_ant_t *ant, int64_t *new_x, int64_t *new_y)
{
    if (!ant || !new_x || !new_y) return;
    *new_x = ant->x;
    *new_y = ant->y;

    switch (ant->dir) {
        case LANT_DIR_NORTH: (*new_y)--; break;
        case LANT_DIR_EAST:  (*new_x)++; break;
        case LANT_DIR_SOUTH: (*new_y)++; break;
        case LANT_DIR_WEST:  (*new_x)--; break;
    }
}

/**
 * Execute one Langton step. This is the canonical rule:
 *   White cell ? turn right, flip to black, move forward
 *   Black cell ? turn left, flip to white, move forward
 *
 * Complexity: O(1) for the step itself; O(wh) for grid expansion if needed.
 */
lant_color_t lant_ant_step(langton_ant_t *ant, langton_grid_t *grid, bool toroidal)
{
    if (!ant || !grid) return LANT_COLOR_WHITE;

    /* Read current cell color */
    lant_color_t old_color = lant_grid_get(grid, ant->x, ant->y, toroidal);

    /* Apply Langton's rule */
    if (old_color == LANT_COLOR_WHITE) {
        lant_ant_turn(ant, true);   /* turn RIGHT */
    } else {
        lant_ant_turn(ant, false);  /* turn LEFT */
    }

    /* Flip the cell */
    lant_grid_flip(grid, ant->x, ant->y);

    /* Compute next position */
    int64_t nx, ny;
    lant_ant_move_forward(ant, &nx, &ny);

    /* Handle grid expansion if needed (non-toroidal mode) */
    if (!toroidal) {
        if (nx < 0 || nx >= grid->width || ny < 0 || ny >= grid->height) {
            int64_t new_w = grid->width;
            int64_t new_h = grid->height;
            if (nx < 0) new_w = grid->width * 2;
            if (nx >= grid->width) new_w = grid->width * 2;
            if (ny < 0) new_h = grid->height * 2;
            if (ny >= grid->height) new_h = grid->height * 2;
            lant_grid_expand(grid, new_w, new_h);
        }
    }

    /* Move forward */
    ant->x = nx;
    ant->y = ny;
    ant->steps++;

    return old_color;
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Simulation Engine Implementation
 *??????????????????????????????????????????????????????????????????????*/

lant_sim_t *lant_sim_create(const lant_config_t *config)
{
    if (!config) return NULL;

    lant_sim_t *sim = (lant_sim_t *)calloc(1, sizeof(lant_sim_t));
    if (!sim) return NULL;

    sim->config = *config;

    /* Create grid */
    sim->grid = lant_grid_create(config->grid_width, config->grid_height);
    if (!sim->grid) {
        free(sim);
        return NULL;
    }

    /* Allocate ant array */
    sim->ants_capacity = 16;
    sim->ants = (langton_ant_t *)calloc(sim->ants_capacity, sizeof(langton_ant_t));
    if (!sim->ants) {
        lant_grid_destroy(sim->grid);
        free(sim);
        return NULL;
    }

    sim->num_ants = 0;
    sim->step_count = 0;
    sim->highway_detected = false;
    sim->highway_start_step = 0;
    sim->entropy_estimate = 0.0;

    /* Initialize history tracking if requested */
    if (config->track_history) {
        sim->history_cap = LANT_MAX_HISTORY;
        sim->x_history = (int64_t *)malloc(sim->history_cap * sizeof(int64_t));
        sim->y_history = (int64_t *)malloc(sim->history_cap * sizeof(int64_t));
        sim->history_len = 0;
        if (!sim->x_history || !sim->y_history) {
            free(sim->x_history); free(sim->y_history);
            lant_grid_destroy(sim->grid);
            free(sim->ants);
            free(sim);
            return NULL;
        }
    }

    return sim;
}

void lant_sim_destroy(lant_sim_t *sim)
{
    if (!sim) return;
    lant_grid_destroy(sim->grid);
    free(sim->ants);
    free(sim->x_history);
    free(sim->y_history);
    free(sim);
}

int lant_sim_add_ant(lant_sim_t *sim, int64_t x, int64_t y, lant_dir_t dir)
{
    if (!sim) return -1;

    /* Expand ant array if needed */
    if (sim->num_ants >= sim->ants_capacity) {
        size_t new_cap = sim->ants_capacity * 2;
        langton_ant_t *new_ants = (langton_ant_t *)realloc(sim->ants,
                                    new_cap * sizeof(langton_ant_t));
        if (!new_ants) return -1;
        sim->ants = new_ants;
        sim->ants_capacity = new_cap;
    }

    int idx = (int)sim->num_ants;
    sim->ants[idx] = lant_ant_create(x, y, dir);

    /* Ensure coordinates are within grid */
    if (x >= sim->grid->width) lant_grid_expand(sim->grid, x + LANT_MIN_GRID_SIZE, sim->grid->height);
    if (y >= sim->grid->height) lant_grid_expand(sim->grid, sim->grid->width, y + LANT_MIN_GRID_SIZE);

    sim->num_ants++;
    return idx;
}

void lant_sim_step_all(lant_sim_t *sim)
{
    if (!sim || sim->num_ants == 0) return;

    /* Round-robin: step each ant once */
    for (size_t i = 0; i < sim->num_ants; i++) {
        lant_ant_step(&sim->ants[i], sim->grid, sim->config.toroidal);
    }

    sim->step_count++;

    /* Record history for first ant (or all ants averaged) */
    if (sim->config.track_history && sim->history_len < sim->history_cap) {
        /* Track centroid of all ants */
        int64_t cx = 0, cy = 0;
        for (size_t i = 0; i < sim->num_ants; i++) {
            cx += sim->ants[i].x;
            cy += sim->ants[i].y;
        }
        cx /= (int64_t)sim->num_ants;
        cy /= (int64_t)sim->num_ants;
        sim->x_history[sim->history_len] = cx;
        sim->y_history[sim->history_len] = cy;
        sim->history_len++;
    }

    /* Running entropy estimate (spatial entropy of last 100x100 window) */
    if (sim->step_count % 100 == 0) {
        sim->entropy_estimate = lant_grid_density(sim->grid);
        /* Clamp entropy proxy to [0, 1] */
        double p = sim->entropy_estimate;
        if (p > 0.0 && p < 1.0) {
            sim->entropy_estimate = -(p * log2(p) + (1.0 - p) * log2(1.0 - p));
        }
    }
}

uint64_t lant_sim_run(lant_sim_t *sim, uint64_t n_steps)
{
    if (!sim) return 0;

    uint64_t limit = n_steps;
    if (sim->config.max_steps > 0 && sim->step_count + n_steps > (uint64_t)sim->config.max_steps) {
        limit = (uint64_t)sim->config.max_steps - sim->step_count;
    }

    for (uint64_t i = 0; i < limit; i++) {
        lant_sim_step_all(sim);

        /* Periodic highway check (not on every step for efficiency) */
        if (!sim->highway_detected && sim->config.track_history
            && sim->step_count % 500 == 0
            && sim->step_count > (uint64_t)LANT_HIGHWAY_MIN_STEPS) {
            if (lant_detect_highway(sim)) {
                sim->highway_detected = true;
                sim->highway_start_step = sim->step_count;
            }
        }
    }

    return limit;
}

uint64_t lant_sim_run_until_highway(lant_sim_t *sim)
{
    if (!sim) return 0;

    uint64_t max_steps = (sim->config.max_steps > 0)
                         ? (uint64_t)sim->config.max_steps : UINT64_MAX;

    while (sim->step_count < max_steps && !sim->highway_detected) {
        lant_sim_run(sim, 100);
    }

    return sim->step_count;
}

void lant_sim_reset(lant_sim_t *sim)
{
    if (!sim) return;

    /* Clear grid */
    size_t n = (size_t)(sim->grid->width * sim->grid->height);
    memset(sim->grid->cells, 0, n);
    sim->grid->total_flips = 0;
    sim->grid->min_x = sim->grid->width;
    sim->grid->max_x = 0;
    sim->grid->min_y = sim->grid->height;
    sim->grid->max_y = 0;

    /* Reset ants to origin */
    for (size_t i = 0; i < sim->num_ants; i++) {
        sim->ants[i].x = sim->grid->width / 2;
        sim->ants[i].y = sim->grid->height / 2;
        sim->ants[i].dir = LANT_DIR_NORTH;
        sim->ants[i].steps = 0;
    }

    sim->step_count = 0;
    sim->highway_detected = false;
    sim->highway_start_step = 0;
    sim->entropy_estimate = 0.0;
    sim->history_len = 0;
}

/*??????????????????????????????????????????????????????????????????????
 * L4: Highway Detection Algorithm
 *
 * Theorem (Bunimovich & Troubetzkoy 1992):
 *   The Langton ant exhibits unbounded growth. The "highway" is a
 *   diagonal pattern with period 104 and net displacement (2,2) per cycle.
 *
 * Detection algorithm:
 *   1. Compute position deltas between samples spaced by candidate periods
 *   2. Check for consistent diagonal displacement over a window
 *   3. Verify periodicity using autocorrelation
 *??????????????????????????????????????????????????????????????????????*/

bool lant_detect_highway(const lant_sim_t *sim)
{
    if (!sim || !sim->config.track_history || sim->history_len < (size_t)LANT_HIGHWAY_WINDOW) {
        return false;
    }

    if (sim->step_count < (uint64_t)LANT_HIGHWAY_MIN_STEPS) {
        return false;
    }

    /* Check for consistent diagonal displacement */
    size_t start = sim->history_len - (size_t)LANT_HIGHWAY_WINDOW;
    if (start > sim->history_len) start = 0;

    int64_t period = (int64_t)LANT_HIGHWAY_PERIOD_CHECK;

    /* Count diagonal steps over the window */
    int diagonal_steps = 0;
    int total_steps = 0;
    int64_t net_dx = 0, net_dy = 0;

    for (size_t i = start + (size_t)period; i < sim->history_len; i++) {
        int64_t dx = sim->x_history[i] - sim->x_history[i - (size_t)period];
        int64_t dy = sim->y_history[i] - sim->y_history[i - (size_t)period];
        net_dx += dx;
        net_dy += dy;
        total_steps++;

        /* Check if displacement is along diagonal (|dx| ? |dy| ? k * period/104) */
        if (dx != 0 || dy != 0) {
            if (dx > 0 && dy > 0) diagonal_steps++;  /* SE highway */
            if (dx < 0 && dy < 0) diagonal_steps++;  /* NW variant */
        }
    }

    if (total_steps < 100) return false;

    /* Highway criterion: >85% of periodic displacements are diagonal */
    double diagonal_ratio = (double)diagonal_steps / (double)total_steps;

    /* Also check that net displacement is substantial and diagonal */
    double displacement_magnitude = sqrt((double)(net_dx * net_dx + net_dy * net_dy));
    int64_t steps_in_window = (int64_t)(sim->history_len - start);
    double velocity = (steps_in_window > 0)
                      ? displacement_magnitude / (double)steps_in_window : 0.0;

    /* Highway detected if: high diagonal ratio AND significant velocity */
    return (diagonal_ratio > 0.85 && velocity > 0.01);
}

uint64_t lant_find_highway_period(const int64_t *x_hist, const int64_t *y_hist, size_t len)
{
    if (!x_hist || !y_hist || len < 200) return 0;

    /* Compute autocorrelation of position deltas for candidate periods.
     * The canonical period is 104, but we search 50-200. */
    double best_corr = 0.0;
    uint64_t best_period = 0;

    for (uint64_t period = 50; period <= 200 && period < len / 2; period++) {
        double corr = 0.0;
        size_t count = 0;
        for (size_t i = period; i < len; i++) {
            int64_t dx = x_hist[i] - x_hist[i - (size_t)period];
            int64_t dy = y_hist[i] - y_hist[i - (size_t)period];
            /* Consistency measure: variance of (dx, dy) over window */
            corr += (double)(dx * dx + dy * dy);
            count++;
        }
        if (count > 0) {
            corr /= (double)count;
            /* Low variance = high periodicity. Invert for scoring. */
            double score = 1.0 / (1.0 + corr);
            if (score > best_corr) {
                best_corr = score;
                best_period = period;
            }
        }
    }

    return best_period;
}

double lant_highway_angle(const lant_sim_t *sim)
{
    if (!sim || !sim->config.track_history || sim->history_len < 100) return 0.0;

    size_t n = sim->history_len;
    int64_t dx = sim->x_history[n - 1] - sim->x_history[n - 100];
    int64_t dy = sim->y_history[n - 1] - sim->y_history[n - 100];

    return atan2((double)dy, (double)dx) * 180.0 / M_PI;
}

double lant_highway_velocity(const lant_sim_t *sim)
{
    if (!sim || !sim->config.track_history || sim->history_len < 100) return 0.0;

    size_t n = sim->history_len;
    int64_t dx = sim->x_history[n - 1] - sim->x_history[n - 100];
    int64_t dy = sim->y_history[n - 1] - sim->y_history[n - 100];

    return sqrt((double)(dx * dx + dy * dy)) / 100.0;
}

/*??????????????????????????????????????????????????????????????????????
 * L8: Multi-Ant Extensions and Collective Behavior
 *??????????????????????????????????????????????????????????????????????*/

void lant_sim_spawn_random_ants(lant_sim_t *sim, size_t n_ants)
{
    if (!sim) return;

    srand(sim->config.seed ? sim->config.seed : (unsigned int)time(NULL));

    int64_t cx = sim->grid->width / 2;
    int64_t cy = sim->grid->height / 2;
    int64_t spread = (cx < cy ? cx : cy) / 2;
    if (spread < 10) spread = 10;

    for (size_t i = 0; i < n_ants; i++) {
        int64_t x = cx + (int64_t)(rand() % (2 * spread)) - spread;
        int64_t y = cy + (int64_t)(rand() % (2 * spread)) - spread;
        lant_dir_t dir = (lant_dir_t)(rand() & 3);
        lant_sim_add_ant(sim, x, y, dir);
    }
}

size_t lant_resolve_collisions(lant_sim_t *sim)
{
    if (!sim) return 0;

    size_t collisions = 0;

    /* O(n^2) collision detection for multi-ant simulation */
    for (size_t i = 0; i < sim->num_ants; i++) {
        for (size_t j = i + 1; j < sim->num_ants; j++) {
            if (sim->ants[i].x == sim->ants[j].x && sim->ants[i].y == sim->ants[j].y) {
                /* Collision: both ants turn 180 degrees */
                lant_ant_turn(&sim->ants[i], true);
                lant_ant_turn(&sim->ants[i], true);
                lant_ant_turn(&sim->ants[j], true);
                lant_ant_turn(&sim->ants[j], true);

                /* Swap their cell colors at collision point */
                (void) lant_grid_get(sim->grid, sim->ants[i].x,
                                                 sim->ants[i].y, sim->config.toroidal);
                lant_grid_flip(sim->grid, sim->ants[i].x, sim->ants[i].y);
                collisions++;
            }
        }
    }

    return collisions;
}

double lant_interaction_rate(const lant_sim_t *sim)
{
    if (!sim || sim->num_ants < 2) return 0.0;

    /* Count adjacent ant pairs */
    size_t adjacent = 0;
    for (size_t i = 0; i < sim->num_ants; i++) {
        for (size_t j = i + 1; j < sim->num_ants; j++) {
            int64_t dx = sim->ants[i].x - sim->ants[j].x;
            int64_t dy = sim->ants[i].y - sim->ants[j].y;
            if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
                adjacent++;
            }
        }
    }

    size_t max_pairs = sim->num_ants * (sim->num_ants - 1) / 2;
    return (max_pairs > 0) ? (double)adjacent / (double)max_pairs : 0.0;
}

void lant_multi_ant_run(lant_sim_t *sim, uint64_t n_steps)
{
    if (!sim) return;

    for (uint64_t step = 0; step < n_steps; step++) {
        lant_sim_step_all(sim);
        lant_resolve_collisions(sim);

        /* Multi-ant emergence tracking: log interaction rate every 100 steps */
        if (sim->config.track_history && step % 100 == 0) {
            double int_rate = lant_interaction_rate(sim);
            /* Interaction rate can serve as an order parameter */
            if (sim->history_len > 0) {
                sim->entropy_estimate = 0.9 * sim->entropy_estimate + 0.1 * int_rate;
            }
        }
    }
}

double lant_colony_radius(const lant_sim_t *sim)
{
    if (!sim || sim->num_ants == 0) return 0.0;

    /* Compute centroid */
    double cx = 0.0, cy = 0.0;
    for (size_t i = 0; i < sim->num_ants; i++) {
        cx += (double)sim->ants[i].x;
        cy += (double)sim->ants[i].y;
    }
    cx /= (double)sim->num_ants;
    cy /= (double)sim->num_ants;

    /* Compute mean distance from centroid */
    double sum_dist = 0.0;
    for (size_t i = 0; i < sim->num_ants; i++) {
        double dx = (double)sim->ants[i].x - cx;
        double dy = (double)sim->ants[i].y - cy;
        sum_dist += sqrt(dx * dx + dy * dy);
    }

    return sum_dist / (double)sim->num_ants;
}

double lant_colony_anisotropy(const lant_sim_t *sim)
{
    if (!sim || sim->num_ants < 2) return 1.0;

    /* Compute centroid */
    double cx = 0.0, cy = 0.0;
    for (size_t i = 0; i < sim->num_ants; i++) {
        cx += (double)sim->ants[i].x;
        cy += (double)sim->ants[i].y;
    }
    cx /= (double)sim->num_ants;
    cy /= (double)sim->num_ants;

    /* Compute covariance matrix */
    double cxx = 0.0, cyy = 0.0, cxy = 0.0;
    for (size_t i = 0; i < sim->num_ants; i++) {
        double dx = (double)sim->ants[i].x - cx;
        double dy = (double)sim->ants[i].y - cy;
        cxx += dx * dx;
        cyy += dy * dy;
        cxy += dx * dy;
    }
    cxx /= (double)sim->num_ants;
    cyy /= (double)sim->num_ants;
    cxy /= (double)sim->num_ants;

    /* Eigenvalues of 2x2 covariance matrix */
    double trace = cxx + cyy;
    double det = cxx * cyy - cxy * cxy;
    double disc = trace * trace - 4.0 * det;
    if (disc < 0.0) disc = 0.0;
    double lambda1 = (trace + sqrt(disc)) / 2.0;
    double lambda2 = (trace - sqrt(disc)) / 2.0;

    /* Anisotropy = ratio of principal components */
    if (lambda2 < 1e-10) return 1e10; /* degenerate */
    return lambda1 / lambda2;
}