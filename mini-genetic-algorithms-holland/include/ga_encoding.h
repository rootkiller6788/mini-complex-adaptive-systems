/**
 * ga_encoding.h — Encoding Schemes for Genetic Algorithms
 *
 * The choice of representation (encoding) is one of the most critical
 * design decisions in a GA. It determines the search space topology,
 * the effectiveness of variation operators, and the feasibility of
 * solutions.
 *
 * Reference: Rothlauf "Representations for Genetic and Evolutionary
 *            Algorithms" (2006)
 *            Michalewicz "Genetic Algorithms + Data Structures =
 *            Evolution Programs" (1996)
 *
 * Knowledge: L2 Core Concepts, L3 Math Structures, L5 Algorithms
 */

#ifndef GA_ENCODING_H
#define GA_ENCODING_H

#include "ga_core.h"

/* ====================================================================
 * L2: Encoding Strategy Definitions
 * ==================================================================== */

typedef enum {
    ENC_BINARY,           /* Binary string encoding */
    ENC_GRAY,             /* Gray code (adjacent values differ by 1 bit) */
    ENC_REAL_VECTOR,      /* Real-valued vector */
    ENC_INTEGER_VECTOR,   /* Integer-valued vector */
    ENC_PERMUTATION,      /* Permutation of [0..N-1] */
    ENC_TREE,             /* Tree encoding (GP) */
    ENC_MESSY             /* Variable-length messy encoding */
} EncodingType;

typedef struct {
    EncodingType type;
    int          num_variables;
    int          bits_per_var;     /* for binary encoding */
    double      *lower_bounds;     /* per-variable lower bound */
    double      *upper_bounds;     /* per-variable upper bound */
    int         *cardinalities;    /* for integer domains */
    int          chromosome_length; /* total length in genes */
} EncodingConfig;

/* ====================================================================
 * L5: Binary Encoding
 * ==================================================================== */

/**
 * Encode a real-valued vector into a binary chromosome using
 * fixed-point representation with bits_per_var bits per variable.
 *
 * binary_value = round((x - lo) / (hi - lo) * (2^b - 1))
 */
void    ga_encode_binary_from_real(const double *real_vec, int n_vars,
                                    int bits_per_var,
                                    const double *lower, const double *upper,
                                    Chromosome *out);

/**
 * Decode a binary chromosome back to a real-valued vector.
 * x = lo + (binary_value / (2^b - 1)) * (hi - lo)
 */
void    ga_decode_binary_to_real(const Chromosome *c,
                                  int n_vars, int bits_per_var,
                                  const double *lower, const double *upper,
                                  double *real_vec);

/**
 * Convert binary encoding to Gray code.
 * Gray: g_i = b_i XOR b_{i+1} (with b_n = 0)
 * Advantage: adjacent integers differ by exactly 1 bit (Hamming distance = 1)
 */
void    ga_binary_to_gray(Chromosome *c);

/** Convert Gray code back to standard binary */
void    ga_gray_to_binary(Chromosome *c);

/** Compute the resolution of a binary-encoded variable */
double  ga_binary_resolution(double lower, double upper, int bits);

/* ====================================================================
 * L5: Real-valued Representation
 * ==================================================================== */

/**
 * Initialize a real-valued chromosome with random values within bounds.
 */
void    ga_encode_real_init(Chromosome *c, int n_vars,
                             const double *lower, const double *upper);

/**
 * Encode an integer vector as a real-valued chromosome.
 * integer_vec[i] ∈ [0, cardinalities[i]-1].
 */
void    ga_encode_integer_to_real(const int *int_vec, int n_vars,
                                   const int *cardinalities, Chromosome *out);

/**
 * Decode a real-valued chromosome to integer values.
 */
void    ga_decode_real_to_integer(const Chromosome *c,
                                   int n_vars, const int *cardinalities,
                                   int *int_vec);

/* ====================================================================
 * L5: Permutation Encoding
 * ==================================================================== */

/**
 * Initialize a random permutation chromosome of length n.
 * Uses Fisher-Yates shuffle.
 */
void    ga_encode_permutation_init(Chromosome *c, int n);

/**
 * Check if a chromosome is a valid permutation (no duplicates).
 */
bool    ga_is_valid_permutation(const Chromosome *c);

/**
 * Encode a permutation as an integer vector stored in allele integer_val.
 */
void    ga_encode_permutation_set(const int *perm, int n, Chromosome *out);

/**
 * Decode a permutation chromosome to an integer array.
 */
void    ga_decode_permutation(const Chromosome *c, int *perm_out, int n);

/**
 * Random Keys encoding: encode a permutation as a real-valued vector,
 * where sorting the vector yields the permutation order.
 *
 * Reference: Bean (1994)
 */
void    ga_encode_random_keys_init(Chromosome *c, int n, double lo, double hi);

/** Decode random keys: sorted indices give the permutation */
void    ga_decode_random_keys(const Chromosome *c, int *perm_out, int n);

/* ====================================================================
 * L8: Messy GA Encoding (Goldberg, Korb, & Deb, 1989)
 *
 * Messy GAs use variable-length chromosomes with (locus, allele) pairs.
 * This allows underspecification (missing genes, filled by templates)
 * and overspecification (duplicate genes, resolved by left-to-right scan).
 * ==================================================================== */

typedef struct {
    int     locus;
    Allele  allele;
} MessyGene;

typedef struct {
    int         length;
    MessyGene   genes[GA_MAX_GENES * 2];
    double      fitness;
} MessyChromosome;

/**
 * Initialize a messy chromosome with k random (locus, allele) pairs.
 */
void    ga_messy_init(MessyChromosome *mc, int k,
                       const Locus *loci, int num_loci);

/**
 * Resolve overspecification: process genes left-to-right,
 * first occurrence of each locus wins.
 */
void    ga_messy_resolve(const MessyChromosome *mc,
                          Chromosome *full_chromosome,
                          const Locus *loci, int num_loci);

/**
 * Cut and splice operator for messy GA:
 * Cut both parents at random positions, splice head of one with tail of other.
 */
void    ga_messy_cut_splice(const MessyChromosome *p1,
                             const MessyChromosome *p2,
                             MessyChromosome *offspring,
                             double splice_prob);

/* ====================================================================
 * L3: Encoding Analysis
 * ==================================================================== */

/**
 * Compute the locality of an encoding: how well does a small change
 * in genotype correspond to a small change in phenotype?
 *
 * Locality = average phenotypic distance / average genotypic distance
 * over random genotype pairs.
 *
 * Reference: Rothlauf (2006)
 */
double  ga_encoding_locality(int num_samples, int chrom_len,
                              void (*decode)(const Chromosome*, double*, int),
                              int pheno_dim);

/**
 * Compute the redundancy of an encoding: ratio of genotypes to
 * distinct phenotypes.
 */
double  ga_encoding_redundancy(int chrom_len, int pheno_dim);

/**
 * Compute the bias of an encoding: deviation from uniform sampling
 * of the phenotype space.
 */
double  ga_encoding_bias(int num_samples, int chrom_len,
                          void (*decode)(const Chromosome*, double*, int),
                          int pheno_dim);

/* ====================================================================
 * L3: Mapper/Interface for generic encoding
 * ==================================================================== */

/** Generic encoder: convert phenotype array to chromosome */
typedef void (*EncoderFunc)(const void *phenotype, int pheno_size,
                             Chromosome *out);

/** Generic decoder: convert chromosome to phenotype array */
typedef void (*DecoderFunc)(const Chromosome *c,
                             void *phenotype, int pheno_size);

/** Run length encoding for chromosomes with repeated genes */
int     ga_encode_run_length(const Chromosome *c,
                              int *locus_out, int *count_out, int max_pairs);

#endif /* GA_ENCODING_H */
