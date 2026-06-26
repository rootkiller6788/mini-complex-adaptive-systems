/**
 * ga_encoding.c — Encoding Scheme Implementations
 *
 * Implements binary encoding, Gray coding, real-valued encoding,
 * permutation encoding (including random keys), and the messy GA
 * representation (Goldberg's variable-length encoding).
 *
 * Reference: Rothlauf "Representations for GAs" (2006)
 *            Michalewicz "Genetic Algorithms + Data Structures" (1996)
 *            Goldberg, Korb & Deb "Messy Genetic Algorithms" (1989)
 *            Bean "Random Keys" (1994)
 *
 * Knowledge: L2 Core Concepts, L3 Math Structures, L5 Algorithms, L8 Advanced
 */

#include "ga_encoding.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Local PRNG */
static uint64_t _enc_rand_state = 271828182ULL;

static uint64_t _enc_rand64(void) {
    _enc_rand_state ^= _enc_rand_state >> 12;
    _enc_rand_state ^= _enc_rand_state << 25;
    _enc_rand_state ^= _enc_rand_state >> 27;
    return _enc_rand_state * 0x2545F4914F6CDD1DULL;
}

static double _enc_rand_double(void) {
    return (double)(_enc_rand64() >> 11) / (double)(1ULL << 53);
}

static int _enc_rand_int(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (int)(_enc_rand64() % (uint64_t)(hi - lo));
}

/* ====================================================================
 * L5: Binary Encoding
 * ==================================================================== */

void ga_encode_binary_from_real(const double *real_vec, int n_vars,
                                 int bits_per_var,
                                 const double *lower, const double *upper,
                                 Chromosome *out) {
    if (!real_vec || !out || n_vars <= 0 || bits_per_var <= 0) return;

    int total_bits = n_vars * bits_per_var;
    out->num_genes = (total_bits < GA_MAX_GENES) ? total_bits : GA_MAX_GENES;

    for (int v = 0; v < n_vars; v++) {
        /* Clamp to bounds */
        double clamped = real_vec[v];
        if (clamped < lower[v]) clamped = lower[v];
        if (clamped > upper[v]) clamped = upper[v];

        /* Convert to integer in [0, 2^b - 1] */
        double range = upper[v] - lower[v];
        if (range < 1e-12) range = 1.0;
        double normalized = (clamped - lower[v]) / range;
        int max_val = (1 << bits_per_var) - 1;
        int int_val = (int)(normalized * (double)max_val + 0.5);
        if (int_val < 0) int_val = 0;
        if (int_val > max_val) int_val = max_val;

        /* Write bits */
        for (int b = 0; b < bits_per_var; b++) {
            int bit_pos = v * bits_per_var + b;
            if (bit_pos >= GA_MAX_GENES) break;
            out->alleles[bit_pos].binary_val = ((int_val >> (bits_per_var - 1 - b)) & 1);
            out->loci[bit_pos].type = GENE_BINARY;
            out->loci[bit_pos].lower_bound = 0.0;
            out->loci[bit_pos].upper_bound = 1.0;
            out->loci[bit_pos].position = bit_pos;
        }
    }
}

void ga_decode_binary_to_real(const Chromosome *c,
                               int n_vars, int bits_per_var,
                               const double *lower, const double *upper,
                               double *real_vec) {
    if (!c || !real_vec || n_vars <= 0 || bits_per_var <= 0) return;

    for (int v = 0; v < n_vars; v++) {
        int int_val = 0;
        for (int b = 0; b < bits_per_var; b++) {
            int bit_pos = v * bits_per_var + b;
            if (bit_pos < c->num_genes && c->alleles[bit_pos].binary_val) {
                int_val |= (1 << (bits_per_var - 1 - b));
            }
        }

        int max_val = (1 << bits_per_var) - 1;
        double range = upper[v] - lower[v];
        if (range < 1e-12) range = 1.0;
        real_vec[v] = lower[v] + ((double)int_val / (double)max_val) * range;
    }
}

void ga_binary_to_gray(Chromosome *c) {
    if (!c) return;
    /* Gray coding: g_i = b_i XOR b_{i+1}, with b_n = 0 */
    bool prev = false;
    for (int i = 0; i < c->num_genes; i++) {
        if (c->loci[i].type != GENE_BINARY) continue;
        bool current = c->alleles[i].binary_val;
        c->alleles[i].binary_val = current ^ prev;
        prev = current;
    }
}

void ga_gray_to_binary(Chromosome *c) {
    if (!c) return;
    /* Gray to binary: b_i = b_{i-1} XOR g_i */
    bool prev = false;
    for (int i = 0; i < c->num_genes; i++) {
        if (c->loci[i].type != GENE_BINARY) continue;
        bool gray = c->alleles[i].binary_val;
        c->alleles[i].binary_val = gray ^ prev;
        prev = c->alleles[i].binary_val;
    }
}

double ga_binary_resolution(double lower, double upper, int bits) {
    if (bits <= 0) return upper - lower;
    int max_val = (1 << bits) - 1;
    return (upper - lower) / (double)max_val;
}

/* ====================================================================
 * L5: Real-valued Encoding
 * ==================================================================== */

void ga_encode_real_init(Chromosome *c, int n_vars,
                          const double *lower, const double *upper) {
    if (!c || n_vars <= 0) return;
    c->num_genes = (n_vars < GA_MAX_GENES) ? n_vars : GA_MAX_GENES;

    for (int i = 0; i < c->num_genes; i++) {
        c->loci[i].type = GENE_REAL;
        c->loci[i].lower_bound = lower[i];
        c->loci[i].upper_bound = upper[i];
        c->loci[i].position = i;
        c->alleles[i].real_val = lower[i] +
            _enc_rand_double() * (upper[i] - lower[i]);
    }
}

void ga_encode_integer_to_real(const int *int_vec, int n_vars,
                                const int *cardinalities, Chromosome *out) {
    if (!int_vec || !out || n_vars <= 0 || !cardinalities) return;
    out->num_genes = (n_vars < GA_MAX_GENES) ? n_vars : GA_MAX_GENES;

    for (int i = 0; i < out->num_genes; i++) {
        out->loci[i].type = GENE_REAL;
        out->loci[i].lower_bound = 0.0;
        out->loci[i].upper_bound = (double)(cardinalities[i] - 1);
        out->loci[i].position = i;
        out->alleles[i].real_val = (double)int_vec[i];
    }
}

void ga_decode_real_to_integer(const Chromosome *c,
                                int n_vars, const int *cardinalities,
                                int *int_vec) {
    if (!c || !int_vec || n_vars <= 0 || !cardinalities) return;
    int n = (n_vars < c->num_genes) ? n_vars : c->num_genes;

    for (int i = 0; i < n; i++) {
        double val = (c->loci[i].type == GENE_REAL)
                     ? c->alleles[i].real_val
                     : (double)c->alleles[i].integer_val;
        int rounded = (int)round(val);
        if (rounded < 0) rounded = 0;
        if (rounded >= cardinalities[i]) rounded = cardinalities[i] - 1;
        int_vec[i] = rounded;
    }
}

/* ====================================================================
 * L5: Permutation Encoding
 * ==================================================================== */

void ga_encode_permutation_init(Chromosome *c, int n) {
    if (!c || n <= 0) return;
    c->num_genes = (n < GA_MAX_GENES) ? n : GA_MAX_GENES;

    /* Initialize with identity permutation */
    for (int i = 0; i < c->num_genes; i++) {
        c->loci[i].type = GENE_PERMUTATION;
        c->loci[i].lower_bound = 0.0;
        c->loci[i].upper_bound = (double)(c->num_genes - 1);
        c->loci[i].position = i;
        c->alleles[i].integer_val = i;
    }

    /* Fisher-Yates shuffle */
    for (int i = c->num_genes - 1; i > 0; i--) {
        int j = _enc_rand_int(0, i + 1);
        int tmp = c->alleles[i].integer_val;
        c->alleles[i].integer_val = c->alleles[j].integer_val;
        c->alleles[j].integer_val = tmp;
    }
}

bool ga_is_valid_permutation(const Chromosome *c) {
    if (!c) return false;

    bool seen[GA_MAX_GENES] = {false};
    for (int i = 0; i < c->num_genes; i++) {
        int val = c->alleles[i].integer_val;
        if (val < 0 || val >= c->num_genes) return false;
        if (seen[val]) return false;
        seen[val] = true;
    }
    return true;
}

void ga_encode_permutation_set(const int *perm, int n, Chromosome *out) {
    if (!perm || !out || n <= 0) return;
    out->num_genes = (n < GA_MAX_GENES) ? n : GA_MAX_GENES;

    for (int i = 0; i < out->num_genes; i++) {
        out->loci[i].type = GENE_PERMUTATION;
        out->alleles[i].integer_val = perm[i];
    }
}

void ga_decode_permutation(const Chromosome *c, int *perm_out, int n) {
    if (!c || !perm_out || n <= 0) return;
    int len = (n < c->num_genes) ? n : c->num_genes;
    for (int i = 0; i < len; i++) {
        perm_out[i] = c->alleles[i].integer_val;
    }
}

void ga_encode_random_keys_init(Chromosome *c, int n, double lo, double hi) {
    if (!c || n <= 0) return;
    c->num_genes = (n < GA_MAX_GENES) ? n : GA_MAX_GENES;

    for (int i = 0; i < c->num_genes; i++) {
        c->loci[i].type = GENE_REAL;
        c->loci[i].lower_bound = lo;
        c->loci[i].upper_bound = hi;
        c->loci[i].position = i;
        c->alleles[i].real_val = lo + _enc_rand_double() * (hi - lo);
    }
}

void ga_decode_random_keys(const Chromosome *c, int *perm_out, int n) {
    if (!c || !perm_out || n <= 0) return;
    int len = (n < c->num_genes) ? n : c->num_genes;

    /* Sort indices by their random key values */
    int *indices = (int*)malloc(sizeof(int) * (size_t)len);
    if (!indices) return;
    for (int i = 0; i < len; i++) indices[i] = i;

    /* Insertion sort by key value */
    for (int i = 1; i < len; i++) {
        int idx = indices[i];
        double key = c->alleles[idx].real_val;
        int j = i - 1;
        while (j >= 0 && c->alleles[indices[j]].real_val > key) {
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = idx;
    }

    for (int i = 0; i < len; i++) {
        perm_out[i] = indices[i];
    }
    free(indices);
}

/* ====================================================================
 * L8: Messy GA Encoding
 * ==================================================================== */

void ga_messy_init(MessyChromosome *mc, int k,
                    const Locus *loci, int num_loci) {
    if (!mc || k <= 0 || !loci || num_loci <= 0) return;

    mc->length = (k < GA_MAX_GENES * 2) ? k : GA_MAX_GENES * 2;
    mc->fitness = 0.0;

    for (int i = 0; i < mc->length; i++) {
        mc->genes[i].locus = _enc_rand_int(0, num_loci);
        int li = mc->genes[i].locus;
        switch (loci[li].type) {
            case GENE_BINARY:
                mc->genes[i].allele.binary_val = (_enc_rand_double() < 0.5);
                break;
            case GENE_INTEGER:
                mc->genes[i].allele.integer_val =
                    _enc_rand_int((int)loci[li].lower_bound,
                                  (int)loci[li].upper_bound + 1);
                break;
            case GENE_REAL:
                mc->genes[i].allele.real_val = loci[li].lower_bound +
                    _enc_rand_double() * (loci[li].upper_bound - loci[li].lower_bound);
                break;
            default:
                mc->genes[i].allele.binary_val = false;
        }
    }
}

void ga_messy_resolve(const MessyChromosome *mc,
                       Chromosome *full_chromosome,
                       const Locus *loci, int num_loci) {
    if (!mc || !full_chromosome || !loci || num_loci <= 0) return;

    memcpy(full_chromosome->loci, loci, sizeof(Locus) * (size_t)num_loci);
    full_chromosome->num_genes = num_loci;

    bool filled[GA_MAX_GENES] = {false};

    /* First pass: left-to-right, first gene for each locus wins */
    for (int i = 0; i < mc->length; i++) {
        int locus = mc->genes[i].locus;
        if (locus < 0 || locus >= num_loci) continue;
        if (!filled[locus]) {
            full_chromosome->alleles[locus] = mc->genes[i].allele;
            filled[locus] = true;
        }
    }

    /* Fill missing loci with random values from template */
    for (int i = 0; i < num_loci; i++) {
        if (!filled[i]) {
            switch (loci[i].type) {
                case GENE_BINARY:
                    full_chromosome->alleles[i].binary_val =
                        (_enc_rand_double() < 0.5);
                    break;
                case GENE_INTEGER:
                    full_chromosome->alleles[i].integer_val =
                        _enc_rand_int((int)loci[i].lower_bound,
                                      (int)loci[i].upper_bound + 1);
                    break;
                case GENE_REAL:
                    full_chromosome->alleles[i].real_val = loci[i].lower_bound +
                        _enc_rand_double() * (loci[i].upper_bound - loci[i].lower_bound);
                    break;
                default:
                    full_chromosome->alleles[i].binary_val = false;
            }
        }
    }
}

void ga_messy_cut_splice(const MessyChromosome *p1,
                          const MessyChromosome *p2,
                          MessyChromosome *offspring,
                          double splice_prob) {
    if (!p1 || !p2 || !offspring) return;

    if (_enc_rand_double() >= splice_prob) {
        /* No splice: copy p1 */
        memcpy(offspring, p1, sizeof(MessyChromosome));
        return;
    }

    /* Cut p1 at random position, cut p2 at random position */
    int cut1 = (p1->length > 0) ? _enc_rand_int(0, p1->length + 1) : 0;
    int cut2 = (p2->length > 0) ? _enc_rand_int(0, p2->length + 1) : 0;

    /* Offspring = p1[0..cut1) + p2[cut2..length) */
    int new_len = cut1 + (p2->length - cut2);
    if (new_len > GA_MAX_GENES * 2) new_len = GA_MAX_GENES * 2;

    offspring->length = new_len;
    for (int i = 0; i < cut1; i++) {
        offspring->genes[i] = p1->genes[i];
    }
    for (int i = 0; i < p2->length - cut2; i++) {
        offspring->genes[cut1 + i] = p2->genes[cut2 + i];
    }
    offspring->fitness = 0.0;
}

/* ====================================================================
 * L3: Encoding Analysis Functions
 * ==================================================================== */

double ga_encoding_locality(int num_samples, int chrom_len,
                             void (*decode)(const Chromosome*, double*, int),
                             int pheno_dim) {
    /* Average phenotypic distance / average genotypic distance
     * over random genotype pairs. High locality = good encoding. */
    if (num_samples <= 0 || chrom_len <= 0 || !decode || pheno_dim <= 0) return 0.0;

    double total_geno_dist = 0.0, total_pheno_dist = 0.0;
    int count = 0;

    for (int s = 0; s < num_samples; s++) {
        /* Generate two random chromosomes */
        Chromosome c1, c2;
        memset(&c1, 0, sizeof(c1));
        memset(&c2, 0, sizeof(c2));
        c1.num_genes = chrom_len;
        c2.num_genes = chrom_len;

        /* Initialize with random bits */
        for (int i = 0; i < chrom_len && i < GA_MAX_GENES; i++) {
            c1.loci[i].type = GENE_BINARY;
            c2.loci[i].type = GENE_BINARY;
            c1.alleles[i].binary_val = (_enc_rand_double() < 0.5);
            c2.alleles[i].binary_val = (_enc_rand_double() < 0.5);
        }

        double pheno1[32], pheno2[32];
        decode(&c1, pheno1, pheno_dim);
        decode(&c2, pheno2, pheno_dim);

        /* Genotype distance: Hamming */
        int g_dist = 0;
        for (int i = 0; i < chrom_len; i++) {
            if (c1.alleles[i].binary_val != c2.alleles[i].binary_val) g_dist++;
        }

        /* Phenotype distance: Euclidean */
        double p_dist = 0.0;
        for (int i = 0; i < pheno_dim; i++) {
            double diff = pheno1[i] - pheno2[i];
            p_dist += diff * diff;
        }
        p_dist = sqrt(p_dist);

        total_geno_dist += (double)g_dist;
        total_pheno_dist += p_dist;
        count++;
    }

    if (total_geno_dist < 1e-12) return 0.0;
    return total_pheno_dist / total_geno_dist;
}

double ga_encoding_redundancy(int chrom_len, int pheno_dim) {
    /* Redundancy = (genotypes / phenotypes) ratio.
     * For binary encoding of real variables: 2^(bits*n) genotypes
     * mapping to continuum of phenotypes. */
    if (pheno_dim <= 0) return 0.0;
    /* Approximate: each phenotype can be encoded many ways.
     * As a ratio: genotypes ~ 2^chrom_len, phenotypes ~ continuous.
     * We approximate as ratio of discrete encodings per phenotype. */
    return exp2((double)chrom_len) / (double)pheno_dim;
}

double ga_encoding_bias(int num_samples, int chrom_len,
                         void (*decode)(const Chromosome*, double*, int),
                         int pheno_dim) {
    /* Bias = deviation from uniform phenotype sampling.
     * If encoding samples all phenotypes uniformly, bias = 0.
     * Compute from sample histogram entropy. */
    if (num_samples <= 0 || !decode || pheno_dim <= 0) return 0.0;

    /* Monte Carlo: generate random chromosomes, decode to phenotype,
     * compute entropy of phenotype distribution. Compare to maximum. */
    (void)chrom_len;
    double max_entropy = log2((double)num_samples);

    /* For first phenotype dimension only, bin the values */
    int bins = 20;
    int *hist = (int*)calloc((size_t)bins, sizeof(int));
    if (!hist) return 0.0;

    double pheno_min = 1e10, pheno_max = -1e10;

    /* First pass: find range */
    for (int s = 0; s < num_samples; s++) {
        Chromosome c;
        memset(&c, 0, sizeof(c));
        c.num_genes = chrom_len;
        for (int i = 0; i < chrom_len && i < GA_MAX_GENES; i++) {
            c.loci[i].type = GENE_BINARY;
            c.alleles[i].binary_val = (_enc_rand_double() < 0.5);
        }
        double pheno[32];
        decode(&c, pheno, pheno_dim);
        if (pheno[0] < pheno_min) pheno_min = pheno[0];
        if (pheno[0] > pheno_max) pheno_max = pheno[0];
    }

    /* Second pass: build histogram */
    for (int s = 0; s < num_samples; s++) {
        Chromosome c;
        memset(&c, 0, sizeof(c));
        c.num_genes = chrom_len;
        for (int i = 0; i < chrom_len && i < GA_MAX_GENES; i++) {
            c.loci[i].type = GENE_BINARY;
            c.alleles[i].binary_val = (_enc_rand_double() < 0.5);
        }
        double pheno[32];
        decode(&c, pheno, pheno_dim);
        double range = pheno_max - pheno_min;
        if (range < 1e-12) range = 1.0;
        int bin = (int)((pheno[0] - pheno_min) / range * (double)bins);
        if (bin < 0) bin = 0;
        if (bin >= bins) bin = bins - 1;
        hist[bin]++;
    }

    /* Compute entropy */
    double entropy = 0.0;
    for (int b = 0; b < bins; b++) {
        double p = (double)hist[b] / (double)num_samples;
        if (p > 1e-12) entropy -= p * log2(p);
    }

    free(hist);
    if (max_entropy < 1e-12) return 0.0;
    return 1.0 - entropy / max_entropy; /* 0 = unbiased, 1 = maximally biased */
}

int ga_encode_run_length(const Chromosome *c,
                          int *locus_out, int *count_out, int max_pairs) {
    if (!c || !locus_out || !count_out || max_pairs <= 0) return 0;
    if (c->num_genes <= 0) return 0;

    int pairs = 0;
    int run_start = 0;

    for (int i = 1; i <= c->num_genes; i++) {
        /* Check if run continues */
        bool same = (i < c->num_genes) &&
                    (c->alleles[i].binary_val == c->alleles[run_start].binary_val);

        if (!same || i == c->num_genes) {
            if (pairs < max_pairs) {
                locus_out[pairs] = run_start;
                count_out[pairs] = i - run_start;
                pairs++;
            }
            run_start = i;
        }
    }
    return pairs;
}
