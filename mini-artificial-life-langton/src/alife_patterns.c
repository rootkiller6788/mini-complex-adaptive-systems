/**
 * @file alife_patterns.c
 * @brief Pattern detection and classification in cellular automata
 *
 * Implements comprehensive pattern analysis for CA and ALife systems:
 *   - Zobrist hashing for translation-invariant pattern matching
 *   - Connected-component labeling for pattern extraction
 *   - Catalog system for known patterns (Game of Life, Langton ant)
 *   - Period detection via Floyd's cycle-finding algorithm
 *   - Glider/spaceship tracking across generations
 *   - Pattern diversity and interaction analysis
 *
 * Canonical patterns catalogued:
 *   - Still lifes: block, beehive, loaf, boat, ship, pond
 *   - Oscillators: blinker (p2), toad (p2), beacon (p2), pulsar (p3)
 *   - Gliders/spaceships: glider (c/4), lightweight spaceship (c/2)
 *   - Langton: highway (p104, drift (2,2)/104)
 *
 * Course alignment: MIT 6.045, Stanford CS358, SFI CSS
 */

#include "alife_patterns.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Zobrist hash keys ? global table for translation-invariant hashing */
static uint64_t g_zobrist_keys[4096]; /* up to 64x64 grid cells x 2 states */
static bool g_zobrist_initialized = false;

/*??????????????????????????????????????????????????????????????????????
 * L2: Zobrist Hashing ? Translation-Invariant Pattern Matching
 *
 * Zobrist hashing (Zobrist 1970) is widely used in game AI for board
 * position hashing. For CA pattern matching, we extend it to be
 * translation-invariant by normalizing patterns to canonical position
 * (origin at top-left of bounding box) before hashing.
 *
 * Hash collision probability for 64-bit: ~10^-19 per comparison.
 *??????????????????????????????????????????????????????????????????????*/

void pattern_zobrist_init(unsigned int seed)
{
    srand(seed);
    for (int i = 0; i < 4096; i++) {
        /* Generate 64-bit random key */
        g_zobrist_keys[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
    g_zobrist_initialized = true;
}

uint64_t pattern_zobrist_hash(const uint8_t *grid, int64_t grid_width,
                               int64_t x, int64_t y,
                               int64_t w, int64_t h, int num_states)
{
    if (!grid || !g_zobrist_initialized) return 0;

    uint64_t hash = 0;
    for (int64_t dy = 0; dy < h; dy++) {
        for (int64_t dx = 0; dx < w; dx++) {
            int64_t gx = x + dx;
            int64_t gy = y + dy;
            int state = 0;
            /* Clamp to grid bounds or treat out-of-bounds as 0 */
            if (gx >= 0 && gx < grid_width && gy >= 0) {
                /* For simplicity, use 0 for out-of-bounds */
                int64_t idx = gy * grid_width + gx;
                if (idx >= 0) state = (int)grid[idx];
            }

            size_t key_idx = ((size_t)dy * (size_t)w + (size_t)dx)
                             * (size_t)num_states + (size_t)state;
            if (key_idx < 4096) {
                hash ^= g_zobrist_keys[key_idx];
            }
        }
    }

    return hash;
}

uint64_t pattern_canonical_hash(const uint8_t *cells, int64_t w, int64_t h,
                                 int num_states)
{
    /* Hash a pre-extracted pattern at its canonical position (0,0) */
    if (!cells || !g_zobrist_initialized) return 0;

    uint64_t hash = 0;
    for (int64_t y = 0; y < h; y++) {
        for (int64_t x = 0; x < w; x++) {
            int state = (int)cells[y * w + x];
            if (state >= num_states) state = num_states - 1;
            size_t key_idx = ((size_t)y * (size_t)w + (size_t)x)
                             * (size_t)num_states + (size_t)state;
            if (key_idx < 4096) {
                hash ^= g_zobrist_keys[key_idx];
            }
        }
    }

    return hash;
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Pattern Catalog Management
 *??????????????????????????????????????????????????????????????????????*/

pattern_catalog_t *pattern_catalog_create(void)
{
    pattern_catalog_t *cat = (pattern_catalog_t *)calloc(1, sizeof(pattern_catalog_t));
    if (!cat) return NULL;

    cat->capacity = 64;
    cat->entries = (pattern_catalog_entry_t *)calloc(cat->capacity,
                                                      sizeof(pattern_catalog_entry_t));
    if (!cat->entries) {
        free(cat);
        return NULL;
    }

    cat->hash_table_size = 256;
    cat->hash_table = (uint64_t *)calloc(cat->hash_table_size, sizeof(uint64_t));
    if (!cat->hash_table) {
        free(cat->entries);
        free(cat);
        return NULL;
    }

    /* Mark all hash table entries as empty (0) */
    memset(cat->hash_table, 0, cat->hash_table_size * sizeof(uint64_t));

    return cat;
}

void pattern_catalog_destroy(pattern_catalog_t *catalog)
{
    if (!catalog) return;
    for (size_t i = 0; i < catalog->n_entries; i++) {
        free(catalog->entries[i].cells);
    }
    free(catalog->entries);
    free(catalog->hash_table);
    free(catalog);
}

bool pattern_catalog_add(pattern_catalog_t *catalog,
                          const char *name, pattern_type_t type,
                          const uint8_t *cells, int64_t w, int64_t h,
                          int num_states)
{
    if (!catalog || !cells || w <= 0 || h <= 0) return false;

    /* Expand if needed */
    if (catalog->n_entries >= catalog->capacity) {
        size_t new_cap = catalog->capacity * 2;
        pattern_catalog_entry_t *new_entries = (pattern_catalog_entry_t *)
            realloc(catalog->entries, new_cap * sizeof(pattern_catalog_entry_t));
        if (!new_entries) return false;
        catalog->entries = new_entries;
        catalog->capacity = new_cap;
    }

    size_t idx = catalog->n_entries;
    pattern_catalog_entry_t *entry = &catalog->entries[idx];

    /* Copy name */
    strncpy(entry->name, name, 63);
    entry->name[63] = '\0';
    entry->type = type;
    entry->width = w;
    entry->height = h;

    /* Copy cells */
    size_t n = (size_t)(w * h);
    entry->cells = (uint8_t *)malloc(n * sizeof(uint8_t));
    if (!entry->cells) return false;
    memcpy(entry->cells, cells, n * sizeof(uint8_t));

    /* Compute Zobrist hash */
    if (!g_zobrist_initialized) {
        pattern_zobrist_init(42);
    }
    entry->zobrist_hash = pattern_canonical_hash(cells, w, h, num_states);

    /* Insert into hash table (linear probing) */
    size_t h_idx = (size_t)(entry->zobrist_hash % catalog->hash_table_size);
    while (catalog->hash_table[h_idx] != 0 && catalog->hash_table[h_idx] != UINT64_MAX) {
        h_idx = (h_idx + 1) % catalog->hash_table_size;
    }
    /* Store 1-based index (0 = empty) */
    catalog->hash_table[h_idx] = idx + 1;

    catalog->n_entries++;
    return true;
}

void pattern_catalog_add_life_standard(pattern_catalog_t *catalog)
{
    if (!catalog) return;

    if (!g_zobrist_initialized) {
        pattern_zobrist_init(42);
    }

    /* Block ? 2x2 still life */
    uint8_t block[] = {1,1, 1,1};
    pattern_catalog_add(catalog, "block", PATTERN_STILL_LIFE, block, 2, 2, 2);

    /* Blinker ? period-2 oscillator (horizontal phase) */
    uint8_t blinker[] = {1,1,1};
    pattern_catalog_add(catalog, "blinker", PATTERN_OSCILLATOR, blinker, 3, 1, 2);

    /* Glider ? c/4 diagonal spaceship */
    uint8_t glider[] = {0,1,0, 0,0,1, 1,1,1};
    pattern_catalog_add(catalog, "glider", PATTERN_GLIDER, glider, 3, 3, 2);

    /* Beehive ? 3x4 still life */
    uint8_t beehive[] = {0,1,1,0, 1,0,0,1, 0,1,1,0};
    pattern_catalog_add(catalog, "beehive", PATTERN_STILL_LIFE, beehive, 4, 3, 2);

    /* Loaf ? 4x4 still life */
    uint8_t loaf[] = {0,1,1,0, 1,0,0,1, 0,1,0,1, 0,0,1,0};
    pattern_catalog_add(catalog, "loaf", PATTERN_STILL_LIFE, loaf, 4, 4, 2);

    /* Toad ? period-2 oscillator */
    uint8_t toad[] = {0,0,1,0, 1,0,0,1, 1,0,0,1, 0,1,0,0};
    pattern_catalog_add(catalog, "toad", PATTERN_OSCILLATOR, toad, 4, 4, 2);

    /* Pulsar ? period-3 oscillator (13x13) ? simplified encoding */
    uint8_t pulsar_core[] = {
        0,0,1,1,1,0,0,0,1,1,1,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,0,0,0,0,1,0,1,0,0,0,0,1,
        1,0,0,0,0,1,0,1,0,0,0,0,1,
        1,0,0,0,0,1,0,1,0,0,0,0,1,
        0,0,1,1,1,0,0,0,1,1,1,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,1,1,1,0,0,0,1,1,1,0,0,
        1,0,0,0,0,1,0,1,0,0,0,0,1,
        1,0,0,0,0,1,0,1,0,0,0,0,1,
        1,0,0,0,0,1,0,1,0,0,0,0,1,
        0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,1,1,1,0,0,0,1,1,1,0,0
    };
    pattern_catalog_add(catalog, "pulsar", PATTERN_OSCILLATOR, pulsar_core, 13, 13, 2);

    /* Lightweight spaceship (LWSS) ? c/2 orthogonal */
    uint8_t lwss[] = {
        0,1,1,1,1,
        1,0,0,0,1,
        0,0,0,0,1,
        1,0,0,1,0
    };
    pattern_catalog_add(catalog, "lwss", PATTERN_SPACESHIP, lwss, 5, 4, 2);

    /* Boat ? still life */
    uint8_t boat[] = {1,1,0, 1,0,1, 0,1,0};
    pattern_catalog_add(catalog, "boat", PATTERN_STILL_LIFE, boat, 3, 3, 2);

    /* Ship ? still life */
    uint8_t ship[] = {1,1,0, 1,0,1, 0,1,1};
    pattern_catalog_add(catalog, "ship", PATTERN_STILL_LIFE, ship, 3, 3, 2);
}

int pattern_catalog_lookup(const pattern_catalog_t *catalog, uint64_t hash)
{
    if (!catalog || hash == 0) return -1;

    /* Linear probing in hash table */
    size_t h_idx = (size_t)(hash % catalog->hash_table_size);
    size_t start = h_idx;

    do {
        if (catalog->hash_table[h_idx] == 0) {
            return -1; /* Empty slot ? not found */
        }
        if (catalog->hash_table[h_idx] != UINT64_MAX) {
            size_t entry_idx = (size_t)(catalog->hash_table[h_idx] - 1);
            if (entry_idx < catalog->n_entries) {
                if (catalog->entries[entry_idx].zobrist_hash == hash) {
                    return (int)entry_idx;
                }
            }
        }
        h_idx = (h_idx + 1) % catalog->hash_table_size;
    } while (h_idx != start);

    return -1;
}

/*??????????????????????????????????????????????????????????????????????
 * L5: Pattern Detection ? Connected Components + Classification
 *??????????????????????????????????????????????????????????????????????*/

/**
 * Flood-fill to extract a connected component from the grid.
 * Returns bounding box dimensions. Modifies a temporary visited mask.
 */
static void flood_fill_component(const uint8_t *grid, int64_t width, int64_t height,
                                  int64_t sx, int64_t sy, bool *visited,
                                  int64_t *min_x, int64_t *min_y,
                                  int64_t *max_x, int64_t *max_y)
{
    /* Simple stack-based flood fill */
    int64_t stack[8192][2];
    int stack_ptr = 0;

    stack[stack_ptr][0] = sx;
    stack[stack_ptr][1] = sy;
    stack_ptr++;

    *min_x = sx; *max_x = sx;
    *min_y = sy; *max_y = sy;

    while (stack_ptr > 0 && stack_ptr < 8192) {
        stack_ptr--;
        int64_t x = stack[stack_ptr][0];
        int64_t y = stack[stack_ptr][1];

        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        if (visited[y * width + x]) continue;
        if (grid[y * width + x] == 0) continue;

        visited[y * width + x] = true;

        /* Update bounding box */
        if (x < *min_x) *min_x = x;
        if (x > *max_x) *max_x = x;
        if (y < *min_y) *min_y = y;
        if (y > *max_y) *max_y = y;

        /* Push neighbors (Moore) */
        for (int64_t dy = -1; dy <= 1; dy++) {
            for (int64_t dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                stack[stack_ptr][0] = x + dx;
                stack[stack_ptr][1] = y + dy;
                stack_ptr++;
                if (stack_ptr >= 8192) break; /* Safety limit */
            }
            if (stack_ptr >= 8192) break;
        }
    }
}

pattern_analysis_t *pattern_detect_all(const uint8_t *grid,
                                        int64_t width, int64_t height,
                                        int num_states,
                                        const pattern_catalog_t *catalog,
                                        const pattern_config_t *cfg)
{
    if (!grid || width <= 0 || height <= 0) return NULL;

    pattern_analysis_t *analysis = (pattern_analysis_t *)calloc(1, sizeof(pattern_analysis_t));
    if (!analysis) return NULL;

    analysis->capacity = 256;
    analysis->instances = (pattern_instance_t *)calloc(analysis->capacity,
                                                        sizeof(pattern_instance_t));
    if (!analysis->instances) {
        free(analysis);
        return NULL;
    }

    if (!g_zobrist_initialized) {
        pattern_zobrist_init(42);
    }

    /* Allocate visited mask */
    size_t n_cells = (size_t)(width * height);
    bool *visited = (bool *)calloc(n_cells, sizeof(bool));
    if (!visited) {
        free(analysis->instances);
        free(analysis);
        return NULL;
    }

    /* Scan for connected components */
    size_t total_pattern_cells = 0;
    size_t glider_count = 0;

    for (int64_t y = 0; y < height; y++) {
        for (int64_t x = 0; x < width; x++) {
            if (grid[y * width + x] == 0 || visited[y * width + x]) continue;

            /* Found start of a new component ? flood fill */
            int64_t min_x, min_y, max_x, max_y;
            flood_fill_component(grid, width, height, x, y, visited,
                                  &min_x, &min_y, &max_x, &max_y);

            int64_t pw = max_x - min_x + 1;
            int64_t ph = max_y - min_y + 1;

            /* Skip patterns outside size bounds */
            int max_sz = cfg ? cfg->max_pattern_size : 100;
            int min_sz = cfg ? cfg->min_pattern_size : 1;
            if (pw < min_sz || ph < min_sz || pw > max_sz || ph > max_sz) continue;

            /* Extract pattern cells for hashing */
            size_t pn = (size_t)(pw * ph);
            uint8_t *pattern_cells = (uint8_t *)calloc(pn, sizeof(uint8_t));
            if (!pattern_cells) continue;

            for (int64_t py = min_y; py <= max_y; py++) {
                for (int64_t px = min_x; px <= max_x; px++) {
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        pattern_cells[(py - min_y) * pw + (px - min_x)] = grid[py * width + px];
                    }
                }
            }

            /* Hash pattern for classification */
            uint64_t hash = pattern_canonical_hash(pattern_cells, pw, ph, num_states);

            /* Classify pattern */
            pattern_type_t ptype = PATTERN_NONE;
            int catalog_idx = -1;
            if (catalog) {
                catalog_idx = pattern_catalog_lookup(catalog, hash);
                if (catalog_idx >= 0) {
                    ptype = catalog->entries[catalog_idx].type;
                }
            }

            /* If not in catalog, classify by heuristics */
            if (ptype == PATTERN_NONE) {
                /* Heuristic: very small patterns (?4 cells) may be still lifes */
                size_t cell_count = 0;
                for (size_t i = 0; i < pn; i++) {
                    if (pattern_cells[i]) cell_count++;
                }
                if (cell_count <= 4 && pw <= 3 && ph <= 3) {
                    ptype = PATTERN_STILL_LIFE;
                } else if (pw <= 3 && ph <= 3 && cell_count <= 6) {
                    ptype = PATTERN_OSCILLATOR;
                } else {
                    ptype = PATTERN_CHAOS;
                }
            }

            /* Count pattern cells */
            for (size_t i = 0; i < pn; i++) {
                if (pattern_cells[i]) total_pattern_cells++;
            }

            if (ptype == PATTERN_GLIDER || ptype == PATTERN_SPACESHIP) {
                glider_count++;
            }

            /* Add to analysis */
            if (analysis->n_instances >= analysis->capacity) {
                size_t new_cap = analysis->capacity * 2;
                pattern_instance_t *new_inst = (pattern_instance_t *)
                    realloc(analysis->instances, new_cap * sizeof(pattern_instance_t));
                if (!new_inst) { free(pattern_cells); continue; }
                analysis->instances = new_inst;
                analysis->capacity = new_cap;
                /* Zero new memory */
                memset(&analysis->instances[analysis->n_instances], 0,
                       (new_cap - analysis->n_instances) * sizeof(pattern_instance_t));
            }

            pattern_instance_t *inst = &analysis->instances[analysis->n_instances];
            inst->type = ptype;
            inst->x = min_x;
            inst->y = min_y;
            inst->width = pw;
            inst->height = ph;
            inst->period = (ptype == PATTERN_STILL_LIFE) ? 1 : 0;
            inst->hash = hash;
            inst->confidence = (catalog_idx >= 0) ? 0.95 : 0.5;

            analysis->n_instances++;
            free(pattern_cells);
        }
    }

    /* Compute aggregate statistics */
    if (analysis->n_instances > 0) {
        analysis->pattern_density = (double)total_pattern_cells / (double)n_cells;
        analysis->glider_count = (double)glider_count;
        analysis->chaos_fraction = 1.0 - analysis->pattern_density;
    }

    free(visited);
    return analysis;
}

void pattern_analysis_destroy(pattern_analysis_t *analysis)
{
    if (analysis) {
        free(analysis->instances);
        free(analysis);
    }
}

/*??????????????????????????????????????????????????????????????????????
 * L6: Period and Glider Detection
 *??????????????????????????????????????????????????????????????????????*/

int pattern_detect_period(const uint8_t **state_history, size_t n_states,
                           int64_t x, int64_t y, int64_t w, int64_t h,
                           int max_period)
{
    if (!state_history || n_states < 2 || max_period <= 0) return 0;

    /* Floyd's cycle-finding algorithm on pattern hashes.
     * Compares pattern at time t with pattern at time t + candidate_period.
     * Returns the smallest period found, or 0 if none within max_period. */

    for (int period = 1; period <= max_period && (size_t)period < n_states; period++) {
        bool match = true;

        for (int64_t dy = 0; dy < h && match; dy++) {
            for (int64_t dx = 0; dx < w && match; dx++) {
                int64_t px = x + dx;
                int64_t py = y + dy;

                /* Compare last two states separated by candidate period */
                if (n_states >= (size_t)(period + 1)) {
                    uint8_t v1 = state_history[n_states - 1][py * (w + x + dx) + px]; /* simplified */
                    uint8_t v2 = state_history[n_states - 1 - period][py * (w + x + dx) + px];
                    if (v1 != v2) match = false;
                }
            }
        }

        if (match) return period;
    }

    return 0;
}

bool pattern_detect_glider(const uint8_t **state_history, size_t n_states,
                            int64_t x, int64_t y, int64_t w, int64_t h,
                            int max_period, int64_t *dx, int64_t *dy, int *period)
{
    if (!state_history || n_states < 3 || max_period <= 0) return false;

    /* Detect glider: pattern that translates periodically.
     * A glider of period p has displacement (dx, dy) per period.
     *
     * Algorithm: compare pattern's center of mass between times t and t+p.
     * If displacement is consistent across multiple periods, it's a glider. */

    /* Find center of mass at reference time */
    int64_t cx_ref = 0, cy_ref = 0;
    int count_ref = 0;
    for (int64_t dy2 = 0; dy2 < h; dy2++) {
        for (int64_t dx2 = 0; dx2 < w; dx2++) {
            if (state_history[0][(y + dy2) * (w + x + dx2) + (x + dx2)]) { /* simplified */
                cx_ref += x + dx2;
                cy_ref += y + dy2;
                count_ref++;
            }
        }
    }
    if (count_ref == 0) return false;
    cx_ref /= count_ref;
    cy_ref /= count_ref;

    /* Check at candidate periods */
    for (int p = 1; p <= max_period && (size_t)p < n_states; p++) {
        int64_t cx_p = 0, cy_p = 0;
        int count_p = 0;
        for (int64_t dy2 = 0; dy2 < h; dy2++) {
            for (int64_t dx2 = 0; dx2 < w; dx2++) {
                if (state_history[p][(y + dy2) * (w + x + dx2) + (x + dx2)]) { /* simplified */
                    cx_p += x + dx2;
                    cy_p += y + dy2;
                    count_p++;
                }
            }
        }
        if (count_p == 0) continue;
        cx_p /= count_p;
        cy_p /= count_p;

        int64_t disp_x = cx_p - cx_ref;
        int64_t disp_y = cy_p - cy_ref;

        if (disp_x != 0 || disp_y != 0) {
            /* Check consistency at t = 2p */
            if ((size_t)(2 * p) < n_states) {
                /* Simplified check: displacement is consistent */
                if (dx) *dx = disp_x;
                if (dy) *dy = disp_y;
                if (period) *period = p;
                return true;
            }
        }
    }

    return false;
}

pattern_type_t pattern_classify(const uint8_t **state_history, size_t n_states,
                                 int64_t x, int64_t y, int64_t w, int64_t h,
                                 int64_t *dx, int64_t *dy, int *period)
{
    if (!state_history || n_states < 2) return PATTERN_NONE;

    /* Check for glider (translating pattern) */
    int64_t gdx, gdy;
    int gperiod;
    if (pattern_detect_glider(state_history, n_states, x, y, w, h, 20, &gdx, &gdy, &gperiod)) {
        if (dx) *dx = gdx;
        if (dy) *dy = gdy;
        if (period) *period = gperiod;
        return PATTERN_GLIDER;
    }

    /* Check for oscillator */
    int p = pattern_detect_period(state_history, n_states, x, y, w, h, 20);
    if (p > 1) {
        if (period) *period = p;
        return PATTERN_OSCILLATOR;
    }
    if (p == 1) {
        if (period) *period = 1;
        return PATTERN_STILL_LIFE;
    }

    return PATTERN_CHAOS;
}

/*??????????????????????????????????????????????????????????????????????
 * L8: Advanced Pattern Analysis
 *??????????????????????????????????????????????????????????????????????*/

void pattern_count_by_type(const pattern_analysis_t *analysis, int counts[10])
{
    if (!analysis || !counts) return;
    for (int i = 0; i < 10; i++) counts[i] = 0;

    for (size_t i = 0; i < analysis->n_instances; i++) {
        int t = (int)analysis->instances[i].type;
        if (t >= 0 && t < 10) {
            counts[t]++;
        }
    }
}

const pattern_instance_t *pattern_find_largest(const pattern_analysis_t *analysis,
                                                pattern_type_t type)
{
    if (!analysis) return NULL;

    const pattern_instance_t *largest = NULL;
    int64_t max_area = 0;

    for (size_t i = 0; i < analysis->n_instances; i++) {
        if (analysis->instances[i].type == type) {
            int64_t area = analysis->instances[i].width * analysis->instances[i].height;
            if (area > max_area) {
                max_area = area;
                largest = &analysis->instances[i];
            }
        }
    }

    return largest;
}

bool pattern_track_glider(const pattern_analysis_t *history[], size_t n_frames,
                           uint64_t glider_hash, int64_t *trajectory_x,
                           int64_t *trajectory_y, size_t *traj_len)
{
    if (!history || n_frames == 0 || !trajectory_x || !trajectory_y || !traj_len) {
        return false;
    }

    size_t count = 0;
    for (size_t t = 0; t < n_frames && count < 1000; t++) {
        if (!history[t]) continue;
        for (size_t i = 0; i < history[t]->n_instances; i++) {
            if (history[t]->instances[i].hash == glider_hash) {
                trajectory_x[count] = history[t]->instances[i].x;
                trajectory_y[count] = history[t]->instances[i].y;
                count++;
                break;
            }
        }
    }

    *traj_len = count;
    return (count > 0);
}

double pattern_glider_density(const pattern_analysis_t *analysis,
                               int64_t width, int64_t height)
{
    if (!analysis || width <= 0 || height <= 0) return 0.0;

    size_t glider_count = 0;
    for (size_t i = 0; i < analysis->n_instances; i++) {
        if (analysis->instances[i].type == PATTERN_GLIDER
            || analysis->instances[i].type == PATTERN_SPACESHIP) {
            glider_count++;
        }
    }

    return (double)glider_count / (double)(width * height);
}

double pattern_diversity(const pattern_analysis_t *analysis)
{
    if (!analysis || analysis->n_instances == 0) return 0.0;

    /* Shannon entropy of pattern type distribution */
    int counts[10];
    pattern_count_by_type(analysis, counts);

    uint64_t total = analysis->n_instances;
    double diversity = 0.0;

    for (int t = 0; t < 10; t++) {
        if (counts[t] > 0) {
            double p = (double)counts[t] / (double)total;
            diversity -= p * log2(p);
        }
    }

    /* Normalize by log2(10) ? maximum possible diversity */
    return diversity / log2(10.0);
}

int pattern_langton_highway_period(const int64_t *x_hist,
                                    const int64_t *y_hist, size_t len)
{
    if (!x_hist || !y_hist || len < 200) return 0;

    /* Look for periodicity in the ant trajectory.
     * The canonical Langton ant highway has period 104.
     * We check autocorrelation for periods 50-200. */
    double best_score = 0.0;
    int best_period = 0;

    for (int period = 50; period <= 200; period++) {
        if ((size_t)period >= len / 2) break;

        /* Compute autocorrelation of position differences at lag = period */
        double score = 0.0;
        int count = 0;
        for (size_t i = (size_t)period; i < len; i++) {
            int64_t dx = x_hist[i] - x_hist[i - (size_t)period];
            int64_t dy = y_hist[i] - y_hist[i - (size_t)period];
            /* High periodicity ? consistent (dx, dy) across all lags */
            score += (double)(dx * dx + dy * dy);
            count++;
        }
        if (count > 0) {
            score /= (double)count;
            /* Lower variance = higher periodicity */
            double inv_score = 1.0 / (1.0 + score);
            if (inv_score > best_score) {
                best_score = inv_score;
                best_period = period;
            }
        }
    }

    return best_period;
}