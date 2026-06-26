/**
 * ga_core.h — Core Genetic Algorithm Definitions
 *
 * Genetic Algorithms (GAs) are adaptive heuristic search algorithms premised
 * on the ideas of natural selection and genetics. Developed by John Holland
 * (1975), GAs represent an intelligent exploitation of a random search within
 * a defined search space to solve optimization problems.
 *
 * Reference: Holland "Adaptation in Natural and Artificial Systems" (1975, 1992)
 *            Goldberg "Genetic Algorithms in Search, Optimization & ML" (1989)
 *            Mitchell "An Introduction to Genetic Algorithms" (1998)
 *
 * Knowledge: L1 Definitions, L2 Core Concepts, L3 Math Structures
 */

#ifndef GA_CORE_H
#define GA_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ====================================================================
 * L1: Core Definitions — Gene, Chromosome, Individual
 * ==================================================================== */

#define GA_MAX_GENES        1024
#define GA_MAX_POPULATION   10000
#define GA_MAX_ARCHIVE      500
#define GA_NAME_LEN         128

/** Gene: the atomic unit of heredity (L1) */
typedef enum {
    GENE_BINARY,       /* 0 or 1 allele */
    GENE_INTEGER,      /* integer allele in [lo, hi] */
    GENE_REAL,         /* double allele in [lo, hi] */
    GENE_PERMUTATION,  /* permutation index allele */
    GENE_SYMBOLIC      /* symbolic/categorical allele */
} GeneType;

/** Allele: specific value of a gene at a locus (L1) */
typedef union {
    bool     binary_val;
    int      integer_val;
    double   real_val;
    char     symbol_val;
} Allele;

/** Locus: position of a gene on a chromosome (L1) */
typedef struct {
    int      position;       /* index on chromosome */
    GeneType type;
    double   lower_bound;    /* for numeric genes */
    double   upper_bound;
    double   mutation_rate;  /* per-locus mutation probability */
    char     name[64];       /* semantic label */
} Locus;

/** Chromosome: ordered sequence of genes (L1) */
typedef struct {
    int      num_genes;
    Locus    loci[GA_MAX_GENES];
    Allele   alleles[GA_MAX_GENES];
    double   fitness;
    double   scaled_fitness;
    bool     evaluated;
    uint64_t id;             /* unique identifier */
    uint64_t birth_generation;
    int      parent_ids[2];  /* parent IDs for lineage tracing */
} Chromosome;

/** Population: collection of chromosomes in a generation (L1) */
typedef struct {
    int         size;
    Chromosome *individuals;
    int         num_genes;
    Locus      *loci_def;    /* shared locus definitions */
    double      total_fitness;
    double      avg_fitness;
    double      max_fitness;
    double      min_fitness;
    double      std_fitness;
    int         best_index;
    int         generation;
    uint64_t    next_id;
} Population;

/** GA Configuration (L2: Core algorithmic parameters) */
typedef enum {
    SELECT_ROULETTE,         /* fitness-proportionate selection */
    SELECT_TOURNAMENT,       /* tournament selection */
    SELECT_RANK,             /* rank-based selection */
    SELECT_SUS,              /* stochastic universal sampling */
    SELECT_TRUNCATION        /* truncation selection */
} SelectionMethod;

typedef enum {
    CROSSOVER_SINGLE_POINT,  /* one-point crossover */
    CROSSOVER_TWO_POINT,     /* two-point crossover */
    CROSSOVER_UNIFORM,       /* uniform crossover */
    CROSSOVER_ARITHMETIC,    /* arithmetic (blend) crossover for real genes */
    CROSSOVER_OX,            /* order crossover for permutations */
    CROSSOVER_PMX            /* partially mapped crossover */
} CrossoverMethod;

typedef enum {
    MUTATE_BIT_FLIP,         /* flip 0↔1 */
    MUTATE_GAUSSIAN,         /* N(0,σ) perturbation for real genes */
    MUTATE_UNIFORM,          /* uniform random reset */
    MUTATE_SWAP,             /* swap two genes (permutations) */
    MUTATE_INVERSION         /* invert gene subsequence */
} MutationMethod;

typedef enum {
    REPLACE_GENERATIONAL,    /* entire population replaced */
    REPLACE_STEADY_STATE,    /* replace worst or random */
    REPLACE_ELITIST,         /* keep best N, replace rest */
    REPLACE_MU_PLUS_LAMBDA,  /* (μ+λ) strategy */
    REPLACE_MU_COMMA_LAMBDA  /* (μ,λ) strategy */
} ReplacementMethod;

typedef struct {
    int              population_size;
    int              num_generations;
    double           crossover_rate;
    double           mutation_rate;
    SelectionMethod  select_method;
    CrossoverMethod  crossover_method;
    MutationMethod   mutation_method;
    ReplacementMethod replace_method;
    int              elitism_count;      /* number of elites to preserve */
    int              tournament_size;    /* k for k-tournament */
    bool             minimize;           /* true for minimization problems */
    double           target_fitness;     /* stop if reached */
    int              stall_generations;  /* early stopping criterion */
    uint64_t         random_seed;
    bool             verbose;
} GAConfig;

/** GA Runtime State (L2) */
typedef struct {
    Population *parent_pop;
    Population *offspring_pop;
    Population *archive;       /* external archive for multi-objective */
    GAConfig    config;
    int         current_gen;
    double     (*fitness_func)(const Chromosome *c, void *user_data);
    void       *user_data;
    double      best_fitness_history[GA_MAX_POPULATION];
    double      avg_fitness_history[GA_MAX_POPULATION];
    int         history_len;
    double      diversity_history[GA_MAX_POPULATION];
    bool        converged;
    int         stall_count;
    double      total_evaluations;
    uint64_t    start_time_ms;
} GAState;

/* ====================================================================
 * L2: Core GA Operations
 * ==================================================================== */

/**
 * Initialize population with random chromosomes.
 * Complexity: O(N * L) where N=pop_size, L=num_genes
 */
Population* ga_population_create(int size, const Locus *loci_def, int num_genes);
void        ga_population_destroy(Population *pop);
void        ga_population_evaluate(Population *pop,
                                   double (*fitness_func)(const Chromosome*, void*),
                                   void *user_data);
void        ga_population_sort(Population *pop);
void        ga_population_stats(Population *pop);
Chromosome* ga_population_best(Population *pop);
void        ga_population_clone(Population *dest, const Population *src);

/**
 * Initialize GA state with configuration.
 * Complexity: O(N * L) for population initialization
 */
GAState*    ga_init(const GAConfig *config,
                    double (*fitness_func)(const Chromosome*, void*),
                    void *user_data);
void        ga_destroy(GAState *ga);

/**
 * Execute one generation: select → crossover → mutate → replace → evaluate.
 * Returns number of fitness evaluations performed.
 * Complexity: O(N * L + N log N) for selection+sorted replacement
 */
int         ga_generation(GAState *ga);

/**
 * Run GA for full number of generations or until convergence.
 * Returns total fitness evaluations performed.
 */
int         ga_run(GAState *ga);

/**
 * Run GA with callback for progress monitoring.
 * Callback receives (generation, best_fitness, avg_fitness, user_data).
 * Returns true on user-requested stop.
 */
int         ga_run_with_callback(GAState *ga,
                 bool (*callback)(int gen, double best, double avg, void *data),
                 void *cb_data);

/* ====================================================================
 * L2: Convergence and Diversity Analysis
 * ==================================================================== */

/** Check if GA has converged (stall detection + fitness variance check) */
bool        ga_has_converged(const GAState *ga, double tolerance);

/**
 * Compute population diversity using pairwise Hamming or Euclidean distance.
 * Diversity = (1 / N^2) Σ_i Σ_j distance(chrom_i, chrom_j)
 */
double      ga_population_diversity(const Population *pop);

/** Compute entropy-based diversity of gene distributions */
double      ga_gene_entropy(const Population *pop);

/** Compute effective population size: 1 / Σ(p_i^2) where p_i is selection prob */
double      ga_effective_population_size(const Population *pop, double *sel_probs);

/**
 * Selection pressure metric: ratio of best individual's expected offspring
 * to average expected offspring under current selection scheme.
 */
double      ga_selection_pressure(const Population *pop,
                                  double *sel_probs, int n);

/** Restart population with fresh random individuals (preserving best few) */
void        ga_restart(GAState *ga, int keep_best_n);

/* ====================================================================
 * L3: Mathematical Structures — Chromosome Operations
 * ==================================================================== */

/**
 * Compute Hamming distance between two binary chromosomes.
 * d_H(a,b) = count_{i}(a[i] ≠ b[i])
 */
int         ga_hamming_distance(const Chromosome *a, const Chromosome *b);

/**
 * Compute Euclidean distance between two real-valued chromosomes.
 * d_E(a,b) = sqrt(Σ(a_i - b_i)²)
 */
double      ga_euclidean_distance(const Chromosome *a, const Chromosome *b);

/** Check if chromosome satisfies all locus constraints */
bool        ga_is_valid(const Chromosome *c);

/** Deep copy a chromosome */
Chromosome  ga_chromosome_copy(const Chromosome *src);

/** Initialize a chromosome randomly within locus bounds */
void        ga_chromosome_init_random(Chromosome *c);

/** Compute the entropy of a population's gene pool per locus */
void        ga_per_locus_entropy(const Population *pop, double *entropy_out);

#endif /* GA_CORE_H */
