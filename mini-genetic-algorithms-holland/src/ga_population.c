/**
 * ga_population.c — Population Management, Niching, Island Models,
 *                    Spatial GA, Speciation, and Co-Evolution
 *
 * Handles population-level operations beyond the basic selection-crossover-
 * mutation loop. Implements advanced diversity preservation, multi-population
 * models, and co-evolutionary dynamics.
 *
 * Reference: Goldberg & Richardson "Genetic Algorithms with Sharing" (1987)
 *            Cantú-Paz "Efficient & Accurate Parallel GAs" (2000)
 *            Alba & Dorronsoro "Cellular Genetic Algorithms" (2008)
 *            Deb et al. "NSGA-II" (IEEE TEC, 2002)
 *
 * Knowledge: L2 Core Concepts, L3 Math Structures, L7 Applications, L8 Advanced
 */

#include "ga_population.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

/* Local PRNG */
static uint64_t _pop_rand_state = 161803398ULL;

static uint64_t _pop_rand64(void) {
    _pop_rand_state ^= _pop_rand_state >> 12;
    _pop_rand_state ^= _pop_rand_state << 25;
    _pop_rand_state ^= _pop_rand_state >> 27;
    return _pop_rand_state * 0x2545F4914F6CDD1DULL;
}

static double _pop_rand_double(void) {
    return (double)(_pop_rand64() >> 11) / (double)(1ULL << 53);
}

static int _pop_rand_int(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (int)(_pop_rand64() % (uint64_t)(hi - lo));
}

/* ====================================================================
 * L2: Population Statistics
 * ==================================================================== */

void ga_population_compute_stats(Population *pop) {
    if (!pop || pop->size <= 0) return;

    pop->total_fitness = 0.0;
    pop->max_fitness = -DBL_MAX;
    pop->min_fitness = DBL_MAX;
    pop->best_index = 0;

    for (int i = 0; i < pop->size; i++) {
        double f = pop->individuals[i].fitness;
        pop->total_fitness += f;
        if (f > pop->max_fitness) { pop->max_fitness = f; pop->best_index = i; }
        if (f < pop->min_fitness) pop->min_fitness = f;
    }
    pop->avg_fitness = pop->total_fitness / (double)pop->size;

    /* Std deviation */
    double sum_sq = 0.0;
    for (int i = 0; i < pop->size; i++) {
        double diff = pop->individuals[i].fitness - pop->avg_fitness;
        sum_sq += diff * diff;
    }
    pop->std_fitness = sqrt(sum_sq / (double)pop->size);
}

Chromosome* ga_population_get_best(Population *pop) {
    if (!pop || pop->size <= 0) return NULL;
    ga_population_compute_stats(pop);
    return &pop->individuals[pop->best_index];
}

Chromosome* ga_population_get_worst(Population *pop) {
    if (!pop || pop->size <= 0) return NULL;
    int worst_idx = 0;
    double worst_fit = pop->individuals[0].fitness;
    for (int i = 1; i < pop->size; i++) {
        if (pop->individuals[i].fitness < worst_fit) {
            worst_fit = pop->individuals[i].fitness;
            worst_idx = i;
        }
    }
    return &pop->individuals[worst_idx];
}

int ga_population_median_index(const Population *pop) {
    if (!pop || pop->size <= 0) return -1;

    /* Partial sort to find median without full O(N log N) sort.
     * For small populations we do a simple approach. */
    double *fits = (double*)malloc(sizeof(double) * (size_t)pop->size);
    if (!fits) return 0;

    for (int i = 0; i < pop->size; i++) fits[i] = pop->individuals[i].fitness;

    /* Quickselect-like: find median by sorting */
    for (int i = 0; i < pop->size; i++) {
        for (int j = i + 1; j < pop->size; j++) {
            if (fits[j] > fits[i]) {
                double tmp = fits[i];
                fits[i] = fits[j];
                fits[j] = tmp;
            }
        }
    }

    int med = pop->size / 2;
    double med_val = fits[med];
    free(fits);

    /* Find index matching median value */
    for (int i = 0; i < pop->size; i++) {
        if (fabs(pop->individuals[i].fitness - med_val) < 1e-12) return i;
    }
    return med;
}

double ga_population_fitness_variance(const Population *pop) {
    if (!pop || pop->size <= 1) return 0.0;
    double mean = pop->avg_fitness;
    double var = 0.0;
    for (int i = 0; i < pop->size; i++) {
        double diff = pop->individuals[i].fitness - mean;
        var += diff * diff;
    }
    return var / (double)pop->size;
}

double ga_population_fitness_skewness(const Population *pop) {
    if (!pop || pop->size <= 1) return 0.0;
    double mean = pop->avg_fitness;
    double var = ga_population_fitness_variance(pop);
    if (var < 1e-12) return 0.0;
    double std = sqrt(var);

    double m3 = 0.0;
    for (int i = 0; i < pop->size; i++) {
        double diff = (pop->individuals[i].fitness - mean) / std;
        m3 += diff * diff * diff;
    }
    return m3 / (double)pop->size;
}

double ga_population_fitness_kurtosis(const Population *pop) {
    if (!pop || pop->size <= 1) return 0.0;
    double mean = pop->avg_fitness;
    double var = ga_population_fitness_variance(pop);
    if (var < 1e-12) return 0.0;
    double std = sqrt(var);

    double m4 = 0.0;
    for (int i = 0; i < pop->size; i++) {
        double diff = (pop->individuals[i].fitness - mean) / std;
        m4 += diff * diff * diff * diff;
    }
    return m4 / (double)pop->size - 3.0; /* excess kurtosis */
}

double ga_population_fitness_entropy(const Population *pop, int num_bins) {
    if (!pop || pop->size <= 0 || num_bins <= 0) return 0.0;
    if (pop->max_fitness <= pop->min_fitness) return 0.0;

    int *bins = (int*)calloc((size_t)num_bins, sizeof(int));
    if (!bins) return 0.0;

    double range = pop->max_fitness - pop->min_fitness;
    for (int i = 0; i < pop->size; i++) {
        double normalized = (pop->individuals[i].fitness - pop->min_fitness) / range;
        int bin = (int)(normalized * (double)num_bins);
        if (bin < 0) bin = 0;
        if (bin >= num_bins) bin = num_bins - 1;
        bins[bin]++;
    }

    double entropy = 0.0;
    for (int b = 0; b < num_bins; b++) {
        double p = (double)bins[b] / (double)pop->size;
        if (p > 1e-12) entropy -= p * log2(p);
    }

    free(bins);
    return entropy;
}

/* ====================================================================
 * L7: Niching and Diversity Preservation
 * ==================================================================== */

void ga_fitness_sharing(Population *pop, double sigma_share, double alpha) {
    if (!pop || pop->size <= 1 || sigma_share <= 0.0) return;

    double *niche_count = (double*)calloc((size_t)pop->size, sizeof(double));
    if (!niche_count) return;

    /* Compute sharing function between all pairs */
    for (int i = 0; i < pop->size; i++) {
        for (int j = 0; j < pop->size; j++) {
            double d = ga_euclidean_distance(&pop->individuals[i],
                                              &pop->individuals[j]);
            if (d < sigma_share) {
                double sh = 1.0 - pow(d / sigma_share, alpha);
                if (sh < 0.0) sh = 0.0;
                niche_count[i] += sh;
            }
        }
    }

    /* Apply shared fitness */
    for (int i = 0; i < pop->size; i++) {
        if (niche_count[i] > 1e-12) {
            pop->individuals[i].scaled_fitness =
                pop->individuals[i].fitness / niche_count[i];
        } else {
            pop->individuals[i].scaled_fitness = pop->individuals[i].fitness;
        }
    }

    free(niche_count);
}

double* ga_pairwise_distances(const Population *pop) {
    if (!pop || pop->size <= 0) return NULL;

    int N = pop->size;
    double *matrix = (double*)malloc(sizeof(double) * (size_t)(N * N));
    if (!matrix) return NULL;

    for (int i = 0; i < N; i++) {
        matrix[i * N + i] = 0.0;
        for (int j = i + 1; j < N; j++) {
            double d = ga_euclidean_distance(&pop->individuals[i],
                                              &pop->individuals[j]);
            matrix[i * N + j] = d;
            matrix[j * N + i] = d;
        }
    }
    return matrix;
}

void ga_deterministic_crowding(Population *parent, Population *offspring) {
    if (!parent || !offspring) return;

    /* For each offspring pair, compete with nearest parent */
    int n_pairs = offspring->size / 2;
    for (int p = 0; p < n_pairs; p++) {
        int c1_idx = p * 2;
        int c2_idx = p * 2 + 1;
        if (c2_idx >= offspring->size) break;

        Chromosome *c1 = &offspring->individuals[c1_idx];
        Chromosome *c2 = &offspring->individuals[c2_idx];

        /* Find nearest parents to each offspring */
        int nearest_c1 = 0, nearest_c2 = 0;
        double min_d1 = DBL_MAX, min_d2 = DBL_MAX;

        for (int i = 0; i < parent->size; i++) {
            double d1 = ga_euclidean_distance(c1, &parent->individuals[i]);
            double d2 = ga_euclidean_distance(c2, &parent->individuals[i]);
            if (d1 < min_d1) { min_d1 = d1; nearest_c1 = i; }
            if (d2 < min_d2) { min_d2 = d2; nearest_c2 = i; }
        }

        /* Replace if offspring is fitter */
        if (c1->fitness > parent->individuals[nearest_c1].fitness) {
            parent->individuals[nearest_c1] = ga_chromosome_copy(c1);
        }
        if (c2->fitness > parent->individuals[nearest_c2].fitness) {
            parent->individuals[nearest_c2] = ga_chromosome_copy(c2);
        }
    }
}

int ga_restricted_tournament_selection(const Population *pop,
                                        int tournament_size,
                                        double window_size) {
    if (!pop || pop->size <= 0) return -1;

    int best_idx = _pop_rand_int(0, pop->size);
    double best_fit = pop->individuals[best_idx].fitness;

    for (int k = 1; k < tournament_size; k++) {
        int idx = _pop_rand_int(0, pop->size);
        double dist = ga_euclidean_distance(&pop->individuals[best_idx],
                                             &pop->individuals[idx]);
        /* Only compete within local neighborhood */
        if (dist <= window_size && pop->individuals[idx].fitness > best_fit) {
            best_fit = pop->individuals[idx].fitness;
            best_idx = idx;
        }
    }
    return best_idx;
}

void ga_niche_counts(const Population *pop, double sigma_share,
                      int *niche_count) {
    if (!pop || !niche_count || sigma_share <= 0.0) return;

    for (int i = 0; i < pop->size; i++) {
        niche_count[i] = 0;
        for (int j = 0; j < pop->size; j++) {
            double d = ga_euclidean_distance(&pop->individuals[i],
                                              &pop->individuals[j]);
            if (d < sigma_share) niche_count[i]++;
        }
    }
}

/* ====================================================================
 * L7: Island Model (Multi-Population Parallel GA)
 * ==================================================================== */

IslandModel* ga_island_model_create(int num_islands, int island_size,
                                     const Locus *loci, int num_genes,
                                     int migration_interval, int migration_size,
                                     int topology) {
    if (num_islands <= 0 || island_size <= 0 || !loci || num_genes <= 0)
        return NULL;

    IslandModel *im = (IslandModel*)calloc(1, sizeof(IslandModel));
    if (!im) return NULL;

    im->num_islands = num_islands;
    im->island_size = island_size;
    im->num_genes = num_genes;
    im->migration_interval = migration_interval;
    im->migration_size = migration_size;
    im->topology = topology;
    im->current_generation = 0;

    im->shared_loci = (Locus*)malloc(sizeof(Locus) * (size_t)num_genes);
    if (!im->shared_loci) { free(im); return NULL; }
    memcpy(im->shared_loci, loci, sizeof(Locus) * (size_t)num_genes);

    im->islands = (Population**)calloc((size_t)num_islands, sizeof(Population*));
    if (!im->islands) {
        free(im->shared_loci);
        free(im);
        return NULL;
    }

    for (int i = 0; i < num_islands; i++) {
        im->islands[i] = ga_population_create(island_size, loci, num_genes);
        if (!im->islands[i]) {
            for (int j = 0; j < i; j++) ga_population_destroy(im->islands[j]);
            free(im->islands);
            free(im->shared_loci);
            free(im);
            return NULL;
        }
    }

    return im;
}

void ga_island_model_destroy(IslandModel *im) {
    if (!im) return;
    for (int i = 0; i < im->num_islands; i++) {
        ga_population_destroy(im->islands[i]);
    }
    free(im->islands);
    free(im->shared_loci);
    free(im);
}

void ga_island_model_generation(IslandModel *im,
                                 double (*fitness_func)(const Chromosome*, void*),
                                 void *user_data,
                                 double crossover_rate, double mutation_rate) {
    if (!im) return;

    for (int i = 0; i < im->num_islands; i++) {
        Population *isl = im->islands[i];

        /* Evaluate if not already */
        ga_population_compute_stats(isl);

        /* Select parents and produce offspring */
        int n_offspring = (isl->size / 2) * 2;
        Population *offspring = ga_population_create(isl->size,
                                                      im->shared_loci,
                                                      im->num_genes);
        if (!offspring) continue;

        /* Simple GA step per island */
        for (int o = 0; o < n_offspring; o += 2) {
            int p1 = _pop_rand_int(0, isl->size);
            int p2 = _pop_rand_int(0, isl->size);

            /* Tournament selection */
            for (int t = 1; t < 3 && t < isl->size; t++) {
                int c = _pop_rand_int(0, isl->size);
                if (isl->individuals[c].fitness > isl->individuals[p1].fitness)
                    p1 = c;
            }
            for (int t = 1; t < 3 && t < isl->size; t++) {
                int c = _pop_rand_int(0, isl->size);
                if (isl->individuals[c].fitness > isl->individuals[p2].fitness)
                    p2 = c;
            }

            Chromosome c1 = ga_chromosome_copy(&isl->individuals[p1]);
            Chromosome c2 = ga_chromosome_copy(&isl->individuals[p2]);

            /* One-point crossover */
            if (_pop_rand_double() < crossover_rate && isl->num_genes > 1) {
                int cp = _pop_rand_int(1, isl->num_genes);
                for (int g = cp; g < isl->num_genes; g++) {
                    Allele tmp = c1.alleles[g];
                    c1.alleles[g] = c2.alleles[g];
                    c2.alleles[g] = tmp;
                }
            }

            /* Bit-flip mutation */
            for (int g = 0; g < isl->num_genes; g++) {
                if (c1.loci[g].type == GENE_BINARY &&
                    _pop_rand_double() < mutation_rate) {
                    c1.alleles[g].binary_val = !c1.alleles[g].binary_val;
                }
                if (c2.loci[g].type == GENE_BINARY &&
                    _pop_rand_double() < mutation_rate) {
                    c2.alleles[g].binary_val = !c2.alleles[g].binary_val;
                }
            }

            if (o < offspring->size) offspring->individuals[o] = c1;
            if (o + 1 < offspring->size) offspring->individuals[o + 1] = c2;
        }

        /* Evaluate offspring */
        for (int o = 0; o < offspring->size; o++) {
            offspring->individuals[o].fitness =
                fitness_func(&offspring->individuals[o], user_data);
        }

        /* Replace worst parents */
        /* Find worst */
        int worst_count = n_offspring;
        if (worst_count > isl->size) worst_count = isl->size;

        /* Sort parents by fitness, keep best */
        for (int a = 0; a < isl->size; a++) {
            for (int b = a + 1; b < isl->size; b++) {
                if (isl->individuals[b].fitness > isl->individuals[a].fitness) {
                    Chromosome tmp = isl->individuals[a];
                    isl->individuals[a] = isl->individuals[b];
                    isl->individuals[b] = tmp;
                }
            }
        }

        /* Sort offspring by fitness */
        for (int a = 0; a < offspring->size; a++) {
            for (int b = a + 1; b < offspring->size; b++) {
                if (offspring->individuals[b].fitness >
                    offspring->individuals[a].fitness) {
                    Chromosome tmp = offspring->individuals[a];
                    offspring->individuals[a] = offspring->individuals[b];
                    offspring->individuals[b] = tmp;
                }
            }
        }

        /* Replace worst parents with best offspring */
        for (int o = 0; o < worst_count && o < offspring->size; o++) {
            int parent_idx = isl->size - 1 - o;
            if (parent_idx >= 0 &&
                offspring->individuals[o].fitness >
                isl->individuals[parent_idx].fitness) {
                isl->individuals[parent_idx] = offspring->individuals[o];
            }
        }

        ga_population_destroy(offspring);
        ga_population_compute_stats(isl);
    }

    im->current_generation++;

    /* Migration */
    if (im->migration_interval > 0 &&
        im->current_generation % im->migration_interval == 0) {
        ga_island_model_migrate(im);
    }
}

void ga_island_model_migrate(IslandModel *im) {
    if (!im || im->num_islands <= 1) return;

    int mig_size = (im->migration_size < im->island_size)
                   ? im->migration_size : im->island_size - 1;
    if (mig_size <= 0) return;

    /* Ring topology migration */
    for (int i = 0; i < im->num_islands; i++) {
        int dest = (i + 1) % im->num_islands;
        Population *src = im->islands[i];
        Population *dst = im->islands[dest];

        ga_population_compute_stats(src);
        ga_population_compute_stats(dst);

        /* Send best mig_size individuals from src to dst */
        /* Sort src by fitness (descending) */
        for (int a = 0; a < src->size; a++) {
            for (int b = a + 1; b < src->size; b++) {
                if (src->individuals[b].fitness > src->individuals[a].fitness) {
                    Chromosome tmp = src->individuals[a];
                    src->individuals[a] = src->individuals[b];
                    src->individuals[b] = tmp;
                }
            }
        }

        /* Replace worst in dst with best from src */
        for (int m = 0; m < mig_size; m++) {
            int worst_idx = 0;
            double worst_fit = dst->individuals[0].fitness;
            for (int j = 1; j < dst->size; j++) {
                if (dst->individuals[j].fitness < worst_fit) {
                    worst_fit = dst->individuals[j].fitness;
                    worst_idx = j;
                }
            }
            if (src->individuals[m].fitness > worst_fit) {
                dst->individuals[worst_idx] =
                    ga_chromosome_copy(&src->individuals[m]);
            }
        }
    }
}

Chromosome* ga_island_model_best(const IslandModel *im) {
    if (!im || im->num_islands <= 0) return NULL;

    Chromosome *best = NULL;
    double best_fit = -DBL_MAX;

    for (int i = 0; i < im->num_islands; i++) {
        Chromosome *b = ga_population_get_best(im->islands[i]);
        if (b && b->fitness > best_fit) {
            best_fit = b->fitness;
            best = b;
        }
    }
    return best;
}

void ga_island_model_stats(const IslandModel *im,
                            double *best_global,
                            double *avg_global,
                            double *diversity_global) {
    if (!im) return;

    double best = -DBL_MAX, sum_avg = 0.0, sum_div = 0.0;

    for (int i = 0; i < im->num_islands; i++) {
        Population *isl = im->islands[i];
        ga_population_compute_stats(isl);
        if (isl->max_fitness > best) best = isl->max_fitness;
        sum_avg += isl->avg_fitness;
        sum_div += ga_population_fitness_variance(isl);
    }

    if (best_global) *best_global = best;
    if (avg_global) *avg_global = sum_avg / (double)im->num_islands;
    if (diversity_global) *diversity_global = sum_div / (double)im->num_islands;
}

/* ====================================================================
 * L7: Spatial / Cellular GA
 * ==================================================================== */

SpatialGA* ga_spatial_create(int rows, int cols,
                              const Locus *loci, int num_genes,
                              int neighborhood_type) {
    if (rows <= 0 || cols <= 0 || !loci || num_genes <= 0) return NULL;

    SpatialGA *sga = (SpatialGA*)calloc(1, sizeof(SpatialGA));
    if (!sga) return NULL;

    sga->rows = rows;
    sga->cols = cols;
    sga->num_genes = num_genes;

    sga->loci_def = (Locus*)malloc(sizeof(Locus) * (size_t)num_genes);
    if (!sga->loci_def) { free(sga); return NULL; }
    memcpy(sga->loci_def, loci, sizeof(Locus) * (size_t)num_genes);

    int size = rows * cols;
    sga->grid = (Chromosome*)malloc(sizeof(Chromosome) * (size_t)size);
    if (!sga->grid) { free(sga->loci_def); free(sga); return NULL; }

    /* Initialize grid */
    for (int i = 0; i < size; i++) {
        int r = i / cols, c = i % cols;
        Chromosome *ch = &sga->grid[i];
        ch->num_genes = num_genes;
        memcpy(ch->loci, loci, sizeof(Locus) * (size_t)num_genes);
        for (int g = 0; g < num_genes; g++) {
            ch->alleles[g].binary_val = (_pop_rand_double() < 0.5);
        }
        ch->fitness = 0.0;
        (void)r; (void)c; /* suppress unused warning */
    }

    /* Set up neighborhood */
    if (neighborhood_type == 8) {
        /* Moore (8-neighbor) */
        int moore[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},
                           {0,1},{1,-1},{1,0},{1,1}};
        sga->num_neighbors = 8;
        sga->neighbor_offsets = (int*)malloc(sizeof(int) * 8);
        if (sga->neighbor_offsets) {
            for (int n = 0; n < 8; n++) {
                sga->neighbor_offsets[n] = moore[n][0] * cols + moore[n][1];
            }
        }
    } else {
        /* von Neumann (4-neighbor) */
        int vn[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        sga->num_neighbors = 4;
        sga->neighbor_offsets = (int*)malloc(sizeof(int) * 4);
        if (sga->neighbor_offsets) {
            for (int n = 0; n < 4; n++) {
                sga->neighbor_offsets[n] = vn[n][0] * cols + vn[n][1];
            }
        }
    }

    return sga;
}

void ga_spatial_destroy(SpatialGA *sga) {
    if (!sga) return;
    free(sga->grid);
    free(sga->loci_def);
    free(sga->neighbor_offsets);
    free(sga);
}

void ga_spatial_generation(SpatialGA *sga,
                            double (*fitness_func)(const Chromosome*, void*),
                            void *user_data,
                            double crossover_rate, double mutation_rate) {
    if (!sga || !fitness_func) return;

    int size = sga->rows * sga->cols;

    /* Evaluate all individuals */
    for (int i = 0; i < size; i++) {
        sga->grid[i].fitness = fitness_func(&sga->grid[i], user_data);
    }

    /* Create new grid */
    Chromosome *new_grid = (Chromosome*)malloc(sizeof(Chromosome) * (size_t)size);
    if (!new_grid) return;

    for (int i = 0; i < size; i++) {
        int r = i / sga->cols;
        int c = i % sga->cols;

        /* Find best neighbor (including self) */
        int best = i;
        double best_fit = sga->grid[i].fitness;

        for (int n = 0; n < sga->num_neighbors; n++) {
            int nr = r + sga->neighbor_offsets[n] / sga->cols;
            int nc = c + sga->neighbor_offsets[n] % sga->cols;

            /* Wrap-around boundaries */
            if (nr < 0) nr = sga->rows - 1;
            if (nr >= sga->rows) nr = 0;
            if (nc < 0) nc = sga->cols - 1;
            if (nc >= sga->cols) nc = 0;

            int ni = nr * sga->cols + nc;
            if (sga->grid[ni].fitness > best_fit) {
                best_fit = sga->grid[ni].fitness;
                best = ni;
            }
        }

        /* Individual competes with best neighbor */
        /* Select a second parent randomly from neighbors */
        int second = i;
        if (_pop_rand_double() < crossover_rate) {
            int nidx = _pop_rand_int(0, sga->num_neighbors);
            int nr = r + sga->neighbor_offsets[nidx] / sga->cols;
            int nc = c + sga->neighbor_offsets[nidx] % sga->cols;
            if (nr < 0) nr = sga->rows - 1;
            if (nr >= sga->rows) nr = 0;
            if (nc < 0) nc = sga->cols - 1;
            if (nc >= sga->cols) nc = 0;
            second = nr * sga->cols + nc;
        }

        /* Crossover */
        new_grid[i] = ga_chromosome_copy(&sga->grid[best]);
        if (crossover_rate > 0.0 && _pop_rand_double() < crossover_rate
            && sga->num_genes > 1) {
            int cp = _pop_rand_int(1, sga->num_genes);
            for (int g = cp; g < sga->num_genes; g++) {
                new_grid[i].alleles[g] = sga->grid[second].alleles[g];
            }
        }

        /* Mutation */
        for (int g = 0; g < sga->num_genes; g++) {
            if (_pop_rand_double() < mutation_rate) {
                new_grid[i].alleles[g].binary_val =
                    !new_grid[i].alleles[g].binary_val;
            }
        }
    }

    /* Replace old grid */
    for (int i = 0; i < size; i++) {
        sga->grid[i] = new_grid[i];
    }
    free(new_grid);
}

Chromosome* ga_spatial_best(SpatialGA *sga) {
    if (!sga) return NULL;
    int size = sga->rows * sga->cols;
    int best_idx = 0;
    double best_fit = sga->grid[0].fitness;
    for (int i = 1; i < size; i++) {
        if (sga->grid[i].fitness > best_fit) {
            best_fit = sga->grid[i].fitness;
            best_idx = i;
        }
    }
    return &sga->grid[best_idx];
}

/* ====================================================================
 * L8: Speciation
 * ==================================================================== */

int ga_speciate(const Population *pop, double distance_threshold,
                 int *species_labels, int max_species) {
    if (!pop || !species_labels || max_species <= 0) return 0;

    /* Simple agglomerative clustering for speciation */
    for (int i = 0; i < pop->size; i++) species_labels[i] = -1;

    int num_species = 0;
    for (int i = 0; i < pop->size; i++) {
        if (species_labels[i] >= 0) continue;

        /* Start new species */
        species_labels[i] = num_species;

        for (int j = i + 1; j < pop->size; j++) {
            if (species_labels[j] >= 0) continue;
            double dist = ga_euclidean_distance(&pop->individuals[i],
                                                 &pop->individuals[j]);
            if (dist <= distance_threshold) {
                species_labels[j] = num_species;
            }
        }
        num_species++;
        if (num_species >= max_species) break;
    }

    return num_species;
}

/* ====================================================================
 * L8: Multi-Objective (NSGA-II style) and Co-Evolution
 * ==================================================================== */

void ga_nondominated_sort(const double *objectives, int n, int n_obj,
                           int *ranks, int *crowding_dist) {
    if (!objectives || !ranks || n <= 0) return;

    /* Initialize ranks */
    for (int i = 0; i < n; i++) ranks[i] = 0;

    /* Count how many individuals dominate each individual */
    int *dom_count = (int*)calloc((size_t)n, sizeof(int));
    /* Lists of dominated individuals per individual */
    int **dominates = (int**)malloc(sizeof(int*) * (size_t)n);
    int *dom_size = (int*)calloc((size_t)n, sizeof(int));
    int *dom_cap = (int*)calloc((size_t)n, sizeof(int));

    if (!dom_count || !dominates || !dom_size || !dom_cap) {
        free(dom_count); free(dominates); free(dom_size); free(dom_cap);
        return;
    }

    /* Pareto dominance check */
    for (int p = 0; p < n; p++) {
        dominates[p] = (int*)malloc(sizeof(int) * (size_t)n);
        dom_cap[p] = n;
        for (int q = 0; q < n; q++) {
            if (p == q) continue;
            /* Check if p dominates q */
            bool p_dominates_q = true;
            bool p_worse = false;
            for (int o = 0; o < n_obj; o++) {
                double vp = objectives[p * n_obj + o];
                double vq = objectives[q * n_obj + o];
                if (vp < vq) { p_dominates_q = false; break; }
                if (vp > vq) p_worse = false;
            }
            (void)p_worse;
            if (p_dominates_q) {
                dominates[p][dom_size[p]++] = q;
            }
        }
    }

    /* Count domination */
    for (int p = 0; p < n; p++) {
        for (int q = 0; q < n; q++) {
            if (p == q) continue;
            bool q_dominates_p = true;
            for (int o = 0; o < n_obj; o++) {
                double vp = objectives[p * n_obj + o];
                double vq = objectives[q * n_obj + o];
                if (vq < vp) { q_dominates_p = false; break; }
            }
            if (q_dominates_p) dom_count[p]++;
        }
    }

    /* Front 0 = individuals not dominated by anyone */
    int *current_front = (int*)malloc(sizeof(int) * (size_t)n);
    int front_size = 0;
    for (int i = 0; i < n; i++) {
        if (dom_count[i] == 0) {
            ranks[i] = 0;
            current_front[front_size++] = i;
        }
    }

    /* Iteratively find higher fronts */
    int current_rank = 0;
    while (front_size > 0) {
        int *next_front = (int*)malloc(sizeof(int) * (size_t)n);
        int next_size = 0;

        for (int fi = 0; fi < front_size; fi++) {
            int p = current_front[fi];
            for (int d = 0; d < dom_size[p]; d++) {
                int q = dominates[p][d];
                dom_count[q]--;
                if (dom_count[q] == 0) {
                    ranks[q] = current_rank + 1;
                    next_front[next_size++] = q;
                }
            }
        }

        free(current_front);
        current_front = next_front;
        front_size = next_size;
        current_rank++;
    }
    free(current_front);

    /* Compute crowding distance if requested */
    if (crowding_dist) {
        /* Crowding distance computation */
        double *cd = (double*)calloc((size_t)n, sizeof(double));
        if (cd) {
            for (int obj = 0; obj < n_obj; obj++) {
                /* Sort by this objective */
                int *sorted = (int*)malloc(sizeof(int) * (size_t)n);
                if (!sorted) { free(cd); break; }
                for (int i = 0; i < n; i++) sorted[i] = i;

                for (int i = 0; i < n; i++) {
                    for (int j = i + 1; j < n; j++) {
                        if (objectives[sorted[j] * n_obj + obj] >
                            objectives[sorted[i] * n_obj + obj]) {
                            int tmp = sorted[i];
                            sorted[i] = sorted[j];
                            sorted[j] = tmp;
                        }
                    }
                }

                cd[sorted[0]] = 1e10;
                cd[sorted[n-1]] = 1e10;

                double fmax = objectives[sorted[0] * n_obj + obj];
                double fmin = objectives[sorted[n-1] * n_obj + obj];
                double range = fmax - fmin;
                if (fabs(range) < 1e-12) range = 1.0;

                for (int i = 1; i < n - 1; i++) {
                    double next = objectives[sorted[i+1] * n_obj + obj];
                    double prev = objectives[sorted[i-1] * n_obj + obj];
                    cd[sorted[i]] += (next - prev) / range;
                }
                free(sorted);
            }
            for (int i = 0; i < n; i++) crowding_dist[i] = (int)(cd[i] * 1000.0);
            free(cd);
        }
    }

    /* Cleanup */
    for (int i = 0; i < n; i++) free(dominates[i]);
    free(dominates);
    free(dom_count);
    free(dom_size);
    free(dom_cap);
}

double ga_coevolution_fitness(const Chromosome *c,
                               const Population *competitors,
                               double (*interaction_func)(
                                   const Chromosome*, const Chromosome*)) {
    if (!c || !competitors || !interaction_func) return 0.0;

    double total = 0.0;
    for (int i = 0; i < competitors->size; i++) {
        total += interaction_func(c, &competitors->individuals[i]);
    }
    return total / (double)competitors->size;
}

double ga_competitive_fitness(const Chromosome *a, const Chromosome *b) {
    /* Competitive fitness: returns +1 if a wins, -1 if b wins, 0 if tie.
     * Comparison based on fitness values. */
    if (!a || !b) return 0.0;
    if (a->fitness > b->fitness) return 1.0;
    if (a->fitness < b->fitness) return -1.0;
    return 0.0;
}

double ga_cooperative_fitness(const Chromosome *a, const Chromosome *b) {
    /* Cooperative fitness: synergy bonus. Two individuals cooperate to
     * solve complementary sub-problems. */
    if (!a || !b) return 0.0;
    /* Synergy = product of their individual fitnesses */
    return a->fitness * b->fitness / (fabs(a->fitness) + fabs(b->fitness) + 1.0);
}

/* ====================================================================
 * L8: External Archive for Multi-Objective Optimization
 * ==================================================================== */

void ga_archive_init(Population *archive, int max_size,
                      const Locus *loci, int num_genes) {
    if (!archive || !loci || num_genes <= 0) return;
    archive->size = 0;
    archive->num_genes = num_genes;
    memcpy(archive->loci_def, loci, sizeof(Locus) * (size_t)num_genes);
    /* Allocate space for max_size individuals */
    archive->individuals = (Chromosome*)realloc(archive->individuals,
        sizeof(Chromosome) * (size_t)max_size);
    (void)max_size;
}

void ga_archive_update(Population *archive, const Chromosome *new_solution,
                        int num_objectives,
                        double (*obj_func)(const Chromosome*, int obj_idx)) {
    if (!archive || !new_solution || !obj_func) return;

    /* Check if new_solution is dominated by any archive member */
    for (int i = 0; i < archive->size; i++) {
        bool dominated = true;
        bool equal = true;
        for (int o = 0; o < num_objectives; o++) {
            double arch_val = obj_func(&archive->individuals[i], o);
            double new_val = obj_func(new_solution, o);
            if (new_val < arch_val) { dominated = false; break; }
            if (new_val != arch_val) equal = false;
        }
        if (dominated && !equal) return; /* new is dominated, reject */
    }

    /* Remove archive members dominated by new_solution */
    int write_idx = 0;
    for (int i = 0; i < archive->size; i++) {
        bool dominated_by_new = true;
        for (int o = 0; o < num_objectives; o++) {
            double arch_val = obj_func(&archive->individuals[i], o);
            double new_val = obj_func(new_solution, o);
            if (arch_val > new_val) { dominated_by_new = false; break; }
        }
        if (!dominated_by_new) {
            archive->individuals[write_idx++] = archive->individuals[i];
        }
    }
    archive->size = write_idx;

    /* Add new solution if space permits */
    if (archive->size < GA_MAX_ARCHIVE) {
        archive->individuals[archive->size++] = ga_chromosome_copy(new_solution);
    }
}

int ga_archive_pareto_front(const Population *archive, int *front_indices,
                             int max_front) {
    if (!archive || !front_indices) return 0;
    int count = 0;
    for (int i = 0; i < archive->size && count < max_front; i++) {
        front_indices[count++] = i;
    }
    return count;
}
