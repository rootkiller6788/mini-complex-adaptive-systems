/**
 * @file alife_patterns.h
 * @brief Pattern detection and classification in cellular automata
 *
 * Detects and classifies emergent structures in CA evolution:
 *   - Still lifes (period-1 stationary patterns)
 *   - Oscillators (periodic patterns, period > 1)
 *   - Gliders / Spaceships (translating periodic patterns)
 *   - Blinkers, blocks, beehives, etc. (canonical Life patterns)
 *   - Highways (Langton's Ant diagonal patterns)
 *
 * Uses pattern hashing with Zobrist hashing for efficient detection
 * and translation-invariant matching.
 *
 * References:
 *   - Conway, J.H. et al. (1982) "Winning Ways for Your Mathematical Plays"
 *   - Adamatzky, A. (2010) "Game of Life Cellular Automata"
 *   - Wuensche, A. (1999) "Classifying cellular automata automatically"
 *
 * Course alignment: MIT 6.045, Stanford CS358, Berkeley CS172
 */

#ifndef ALIFE_PATTERNS_H
#define ALIFE_PATTERNS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* L1: Core Definitions */

/** Pattern type classification */
typedef enum {
    PATTERN_NONE,           /**< no recognized pattern */
    PATTERN_STILL_LIFE,     /**< period-1 static pattern */
    PATTERN_OSCILLATOR,     /**< period > 1, no net translation */
    PATTERN_GLIDER,         /**< translating periodic pattern */
    PATTERN_SPACESHIP,      /**< glider with period > 1 */
    PATTERN_PUFFER,         /**< moving pattern that leaves debris */
    PATTERN_GUN,            /**< stationary pattern that emits gliders */
    PATTERN_HIGHWAY,        /**< Langton ant diagonal periodic pattern */
    PATTERN_WICK,           /**< stable/oscillating linear structure */
    PATTERN_CHAOS           /**< non-periodic, non-repeating region */
} pattern_type_t;

/** Detected pattern instance */
typedef struct {
    pattern_type_t type;    /**< pattern classification */
    int64_t x;              /**< current x position (top-left of bounding box) */
    int64_t y;              /**< current y position */
    int64_t width;          /**< bounding box width */
    int64_t height;         /**< bounding box height */
    int period;             /**< 0=unknown, 1=still, >1=period */
    int64_t dx;             /**< x displacement per period (gliders) */
    int64_t dy;             /**< y displacement per period (gliders) */
    uint64_t hash;          /**< Zobrist hash of canonical pattern */
    double confidence;      /**< classification confidence [0, 1] */
    uint64_t first_seen;    /**< generation when first detected */
    uint64_t last_seen;     /**< generation when last confirmed */
} pattern_instance_t;

/** Pattern database entry (catalogued pattern) */
typedef struct {
    char name[64];          /**< human-readable name (e.g., "glider", "block") */
    pattern_type_t type;    /**< pattern type */
    int64_t width;          /**< pattern width */
    int64_t height;         /**< pattern height */
    uint8_t *cells;         /**< cell data [width * height] */
    uint64_t zobrist_hash;  /**< precomputed Zobrist hash */
} pattern_catalog_entry_t;

/** Pattern detection configuration */
typedef struct {
    int max_pattern_size;   /**< maximum bounding box dimension for patterns */
    int min_pattern_size;   /**< minimum bounding box dimension */
    double confidence_threshold; /**< minimum confidence for classification */
    bool track_gliders;     /**< whether to track glider trajectories */
    bool detect_oscillators;/**< whether to detect oscillators */
    int max_period;         /**< maximum period to search for */
    bool use_zobrist;       /**< use Zobrist hashing for pattern matching */
} pattern_config_t;

/** Catalog of known patterns with lookup capability */
typedef struct {
    pattern_catalog_entry_t *entries;  /**< pattern entries */
    size_t n_entries;                  /**< number of catalogued patterns */
    size_t capacity;                   /**< allocated capacity */
    uint64_t *hash_table;              /**< Zobrist hash -> index lookup */
    size_t hash_table_size;            /**< size of hash table */
} pattern_catalog_t;

/** Full pattern analysis result */
typedef struct {
    pattern_instance_t *instances;     /**< detected pattern instances */
    size_t n_instances;                /**< number of instances */
    size_t capacity;                   /**< allocated capacity */
    double pattern_density;            /**< fraction of grid covered by patterns */
    double glider_count;               /**< number of active gliders */
    double chaos_fraction;             /**< fraction of grid classified as chaotic */
} pattern_analysis_t;

/* L2: Core Concepts ? Pattern Hashing */

/**
 * Initialize Zobrist hashing with random keys.
 * Zobrist hashing enables O(1) translation-invariant pattern matching.
 * Must be called once before using pattern hashing functions.
 */
void pattern_zobrist_init(unsigned int seed);

/** Compute Zobrist hash of a rectangular region of the grid.
 *  This hash is translation-invariant (same pattern at different
 *  locations produces same hash). */
uint64_t pattern_zobrist_hash(const uint8_t *grid, int64_t grid_width,
                               int64_t x, int64_t y,
                               int64_t w, int64_t h, int num_states);

/** Compute Zobrist hash normalized to canonical translation (origin at 0,0) */
uint64_t pattern_canonical_hash(const uint8_t *cells, int64_t w, int64_t h,
                                 int num_states);

/* L5: Pattern Detection Algorithms */

/** Create a pattern catalog (empty) */
pattern_catalog_t *pattern_catalog_create(void);

/** Destroy pattern catalog */
void pattern_catalog_destroy(pattern_catalog_t *catalog);

/** Add a pattern to the catalog */
bool pattern_catalog_add(pattern_catalog_t *catalog,
                          const char *name, pattern_type_t type,
                          const uint8_t *cells, int64_t w, int64_t h,
                          int num_states);

/** Add canonical Game of Life patterns to catalog (block, blinker, glider, etc.) */
void pattern_catalog_add_life_standard(pattern_catalog_t *catalog);

/** Look up a pattern by Zobrist hash. Returns index or -1 if not found. */
int pattern_catalog_lookup(const pattern_catalog_t *catalog, uint64_t hash);

/** Detect all patterns in a CA state.
 *  Uses connected-component labeling + pattern matching.
 *  O(width * height * max_pattern_size^2). */
pattern_analysis_t *pattern_detect_all(const uint8_t *grid,
                                        int64_t width, int64_t height,
                                        int num_states,
                                        const pattern_catalog_t *catalog,
                                        const pattern_config_t *cfg);

/** Free pattern_analysis_t */
void pattern_analysis_destroy(pattern_analysis_t *analysis);

/** Detect period of an oscillating pattern by comparing states at different times.
 *  Uses cycle detection with Floyd's algorithm on pattern hashes. */
int pattern_detect_period(const uint8_t **state_history, size_t n_states,
                           int64_t x, int64_t y, int64_t w, int64_t h,
                           int max_period);

/** Detect if a pattern is a glider (translates periodically).
 *  Returns (dx, dy) displacement per period, or (0,0) if not a glider.
 *  O(max_period * w * h). */
bool pattern_detect_glider(const uint8_t **state_history, size_t n_states,
                            int64_t x, int64_t y, int64_t w, int64_t h,
                            int max_period, int64_t *dx, int64_t *dy, int *period);

/** Classify a pattern based on its evolution over time */
pattern_type_t pattern_classify(const uint8_t **state_history, size_t n_states,
                                 int64_t x, int64_t y, int64_t w, int64_t h,
                                 int64_t *dx, int64_t *dy, int *period);

/* L6: Canonical Pattern Operations */

/** Count the total number of distinct patterns of each type */
void pattern_count_by_type(const pattern_analysis_t *analysis,
                            int counts[10]);

/** Find the largest pattern of a given type */
const pattern_instance_t *pattern_find_largest(const pattern_analysis_t *analysis,
                                                pattern_type_t type);

/** Track a specific glider across multiple generations (by hash) */
bool pattern_track_glider(const pattern_analysis_t *history[], size_t n_frames,
                           uint64_t glider_hash, int64_t *trajectory_x,
                           int64_t *trajectory_y, size_t *traj_len);

/** Compute glider density (gliders per unit area) */
double pattern_glider_density(const pattern_analysis_t *analysis,
                               int64_t width, int64_t height);

/* L8: Advanced Pattern Analysis */

/** Extract all sub-patterns of given size from grid, cluster by similarity.
 *  Returns n_clusters found. Uses min-hash for scalable clustering. */
int pattern_cluster_subpatterns(const uint8_t *grid, int64_t width, int64_t height,
                                 int sub_w, int sub_h, int num_states,
                                 int *cluster_assignments, int max_clusters);

/** Compute pattern diversity (Shannon entropy of pattern type distribution) */
double pattern_diversity(const pattern_analysis_t *analysis);

/** Detect "replicators" ? patterns that make copies of themselves over time.
 *  These are the basis of von Neumann's self-reproduction. */
bool pattern_detect_replicator(const uint8_t **state_history, size_t n_states,
                                int64_t width, int64_t height,
                                int64_t *replicator_x, int64_t *replicator_y,
                                int64_t *replicator_w, int64_t *replicator_h);

/** Compute the pattern interaction graph: which pattern types tend to
 *  appear near each other. Returns adjacency matrix (n_types x n_types). */
void pattern_interaction_graph(const pattern_analysis_t *analysis,
                                int64_t width, int64_t height,
                                double *adj_matrix, int n_types);

/** Compute Langton's Ant highway pattern period using autocorrelation.
 *  The canonical highway period is 104 steps. */
int pattern_langton_highway_period(const int64_t *x_hist,
                                    const int64_t *y_hist, size_t len);

#endif /* ALIFE_PATTERNS_H */