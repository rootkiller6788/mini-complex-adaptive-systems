/**
 * ga_population.h — Population Management and Statistics
 *
 * Manages populations as the central unit of evolutionary computation.
 * Provides statistical analysis, niching, sharing, speciation, and
 * multi-population island models.
 *
 * Reference: Goldberg & Richardson "Genetic Algorithms with Sharing" (1987)
 *            Horn "Niching Methods for Genetic Algorithms" (1997)
 *            Cantú-Paz "Efficient and Accurate Parallel Genetic Algorithms" (2000)
 *
 * Knowledge: L2 Core Concepts, L3 Math Structures, L7 Applications
 */

#ifndef GA_POPULATION_H
#define GA_POPULATION_H

#include "ga_core.h"
#include "ga_fitness.h"

/* ====================================================================
 * L2: Population Statistics
 * ==================================================================== */

/** Compute comprehensive population statistics */
void    ga_population_compute_stats(Population *pop);

/** Find and return the best chromosome (by fitness) */
Chromosome* ga_population_get_best(Population *pop);

/** Find and return the worst chromosome */
Chromosome* ga_population_get_worst(Population *pop);

/** Find the median fitness chromosome index */
int     ga_population_median_index(const Population *pop);

/** Compute fitness variance σ² = (1/N) Σ(f_i - f̄)² */
double  ga_population_fitness_variance(const Population *pop);

/** Compute skewness of fitness distribution */
double  ga_population_fitness_skewness(const Population *pop);

/** Compute kurtosis of fitness distribution */
double  ga_population_fitness_kurtosis(const Population *pop);

/** Compute Shannon entropy of the fitness distribution */
double  ga_population_fitness_entropy(const Population *pop, int num_bins);

/* ====================================================================
 * L2: Niching and Diversity Preservation
 *
 * Niching maintains subpopulations adapted to different peaks in a
 * multimodal fitness landscape, preventing the GA from converging
 * to a single optimum.
 * ==================================================================== */

/**
 * Fitness Sharing: penalize similar individuals by sharing fitness.
 * f'_i = f_i / Σ_j sh(d_{ij})
 * where sh(d) = 1 - (d/σ_share)^α for d < σ_share, else 0.
 *
 * Reference: Goldberg & Richardson (1987)
 */
void    ga_fitness_sharing(Population *pop, double sigma_share, double alpha);

/**
 * Compute pairwise distance matrix for population.
 * Returns allocated N×N matrix (caller frees).
 */
double* ga_pairwise_distances(const Population *pop);

/**
 * Crowding: offspring replaces most similar individual in local neighborhood.
 * Deterministic crowding (Mahfoud 1995) pairs offspring with nearest parent.
 */
void    ga_deterministic_crowding(Population *parent, Population *offspring);

/**
 * Restricted Tournament Selection (RTS): tournament within local neighborhood.
 *
 * Reference: Harik (1995)
 */
int     ga_restricted_tournament_selection(const Population *pop,
                                            int tournament_size,
                                            double window_size);

/** Compute niche count for each individual (number of neighbors within σ_share) */
void    ga_niche_counts(const Population *pop, double sigma_share,
                         int *niche_count);

/* ====================================================================
 * L7: Multi-Population / Island Models
 *
 * Island Model GA: multiple subpopulations evolve in parallel with
 * periodic migration of individuals between islands.
 *
 * Reference: Cohoon et al. (1987), Cantú-Paz (2000)
 * ==================================================================== */

typedef struct {
    Population **islands;
    int          num_islands;
    int          island_size;
    Locus       *shared_loci;
    int          num_genes;
    int          migration_interval;    /* generations between migrations */
    int          migration_size;        /* number of migrants per exchange */
    int          topology;              /* 0=ring, 1=complete graph, 2=grid */
    int          current_generation;
} IslandModel;

/** Create island model with num_islands subpopulations */
IslandModel* ga_island_model_create(int num_islands, int island_size,
                                     const Locus *loci, int num_genes,
                                     int migration_interval, int migration_size,
                                     int topology);

/** Destroy island model */
void         ga_island_model_destroy(IslandModel *im);

/** Execute one generation on all islands */
void         ga_island_model_generation(IslandModel *im,
                    double (*fitness_func)(const Chromosome*, void*),
                    void *user_data,
                    double crossover_rate, double mutation_rate);

/**
 * Perform migration: islands exchange their best individuals
 * according to the topology.
 */
void         ga_island_model_migrate(IslandModel *im);

/** Get best individual across all islands */
Chromosome*  ga_island_model_best(const IslandModel *im);

/** Get overall statistics across all islands */
void         ga_island_model_stats(const IslandModel *im,
                                    double *best_global,
                                    double *avg_global,
                                    double *diversity_global);

/* ====================================================================
 * L7: Spatial GA (Cellular GA)
 *
 * Spatial/ Cellular GA: Individuals are placed on a 2D grid and
 * interact only with neighbors. Promotes slow diffusion of information
 * and natural niching.
 *
 * Reference: Manderick & Spiessens (1989), Alba & Dorronsoro (2008)
 * ==================================================================== */

typedef struct {
    Chromosome *grid;          /* 2D grid stored as 1D array */
    int         rows, cols;    /* grid dimensions */
    int         num_genes;
    Locus      *loci_def;
    int        *neighbor_offsets; /* relative neighbor indices */
    int         num_neighbors;    /* e.g., 4 (von Neumann) or 8 (Moore) */
} SpatialGA;

/** Create a spatial GA with given grid size and neighborhood */
SpatialGA*  ga_spatial_create(int rows, int cols,
                               const Locus *loci, int num_genes,
                               int neighborhood_type); /* 4=von Neumann, 8=Moore */

void        ga_spatial_destroy(SpatialGA *sga);

/** Execute one generation: each cell competes with neighbors */
void        ga_spatial_generation(SpatialGA *sga,
                    double (*fitness_func)(const Chromosome*, void*),
                    void *user_data,
                    double crossover_rate, double mutation_rate);

/** Get best individual on grid */
Chromosome* ga_spatial_best(SpatialGA *sga);

/* ====================================================================
 * L8: Speciation and Co-Evolution
 * ==================================================================== */

/**
 * Speciation: cluster population into species based on genetic distance.
 * Returns number of species found.
 */
int     ga_speciate(const Population *pop, double distance_threshold,
                     int *species_labels, int max_species);

/**
 * Multi-Objective GA (NSGA-II style front ranking):
 * Assign non-dominated front ranks to population.
 *
 * Reference: Deb et al. "NSGA-II" (2002)
 */
void    ga_nondominated_sort(const double *objectives, int n, int n_obj,
                              int *ranks, int *crowding_dist);

/**
 * Co-evolutionary fitness: fitness depends on interactions with
 * other individuals in the population.
 */
double  ga_coevolution_fitness(const Chromosome *c,
                                const Population *competitors,
                                double (*interaction_func)(
                                    const Chromosome*, const Chromosome*));

/** Competitive coevolution: pairwise tournament fitness */
double  ga_competitive_fitness(const Chromosome *a, const Chromosome *b);

/** Cooperative coevolution: complementary fitness evaluation */
double  ga_cooperative_fitness(const Chromosome *a, const Chromosome *b);

/* ====================================================================
 * L8: External Archive for Multi-Objective Optimization
 * ==================================================================== */

/** Initialize external archive for Pareto-optimal solutions */
void    ga_archive_init(Population *archive, int max_size,
                         const Locus *loci, int num_genes);

/** Update archive with new solutions, maintaining non-dominated set */
void    ga_archive_update(Population *archive, const Chromosome *new_solution,
                           int num_objectives,
                           double (*obj_func)(const Chromosome*, int obj_idx));

/** Get Pareto front from archive */
int     ga_archive_pareto_front(const Population *archive, int *front_indices,
                                 int max_front);

#endif /* GA_POPULATION_H */
