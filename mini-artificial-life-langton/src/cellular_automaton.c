/**
 * @file cellular_automaton.c
 * @brief General-purpose cellular automaton engine
 *
 * Implements:
 *   - 1D elementary CA (Wolfram rules 0-255) with radius-1 neighborhood
 *   - 2D totalistic and outer-totalistic CA (including Conway's Game of Life)
 *   - Multiple boundary conditions: fixed, periodic, reflective
 *   - Wolfram classification algorithm (Classes I-IV)
 *   - Spacetime diagram generation and analysis
 *   - Lambda parameter computation for arbitrary rules
 *
 * Key references:
 *   - Wolfram, S. (1983) "Statistical mechanics of cellular automata"
 *   - Wolfram, S. (1984) "Universality and complexity in cellular automata"
 *   - Gardner, M. (1970) "The fantastic combinations of John Conway's
 *     new solitaire game 'life'" (Scientific American)
 *   - Langton, C.G. (1990) "Computation at the edge of chaos"
 *
 * Course alignment: MIT 6.045, Stanford CS358, Berkeley CS172, SFI CSS
 */

#include "cellular_automaton.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*??????????????????????????????????????????????????????????????????????
 * L2: Configuration and Rule Creation
 *??????????????????????????????????????????????????????????????????????*/

ca_config_t ca_config_default(ca_dimension_t dim, int64_t width, int64_t height)
{
    ca_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.dim = dim;
    cfg.width = width;
    cfg.height = (dim == CA_DIM_1D) ? 1 : height;
    cfg.num_states = 2;
    cfg.boundary = CA_BOUNDARY_PERIODIC;
    cfg.neighbor = CA_NEIGH_MOORE;
    cfg.radius = 1;
    cfg.rule_type = CA_RULE_ELEMENTARY;
    cfg.rule_lut = NULL;
    cfg.rule_lut_size = 0;
    return cfg;
}

ca_rule_elementary_t ca_rule_elementary_create(uint8_t rule_number)
{
    ca_rule_elementary_t rule;
    rule.rule_number = rule_number;

    /* Expand Wolfram rule number to 8-bit rule table.
     * Neighborhood encoding (3-bit): left*4 + center*2 + right
     * Rule bit position = neighborhood value
     * Bit 0 = output for 000, bit 7 = output for 111 */
    for (int i = 0; i < 8; i++) {
        rule.rule_bits[i] = (rule_number >> i) & 1;
    }

    return rule;
}

ca_rule_totalistic_t ca_rule_totalistic_create(int num_states,
                                                const char *birth_str,
                                                const char *survival_str)
{
    ca_rule_totalistic_t rule;
    rule.num_states = num_states;
    rule.max_sum = (num_states - 1) * 9; /* max Moore neighborhood sum */
    rule.birth = NULL;
    rule.survival = NULL;
    rule.birth_count = 0;
    rule.survival_count = 0;

    /* Parse birth string (comma-separated or concatenated digits) */
    if (birth_str) {
        size_t len = strlen(birth_str);
        rule.birth = (uint8_t *)calloc(len + 1, sizeof(uint8_t));
        if (rule.birth) {
            for (size_t i = 0; i < len; i++) {
                if (birth_str[i] >= '0' && birth_str[i] <= '9') {
                    int val = birth_str[i] - '0';
                    rule.birth[val] = 1;
                    if ((size_t)(val + 1) > rule.birth_count) {
                        rule.birth_count = (size_t)(val + 1);
                    }
                }
            }
        }
    }

    /* Parse survival string */
    if (survival_str) {
        size_t len = strlen(survival_str);
        rule.survival = (uint8_t *)calloc(len + 1, sizeof(uint8_t));
        if (rule.survival) {
            for (size_t i = 0; i < len; i++) {
                if (survival_str[i] >= '0' && survival_str[i] <= '9') {
                    int val = survival_str[i] - '0';
                    rule.survival[val] = 1;
                    if ((size_t)(val + 1) > rule.survival_count) {
                        rule.survival_count = (size_t)(val + 1);
                    }
                }
            }
        }
    }

    return rule;
}

bool ca_config_set_totalistic(ca_config_t *config, const ca_rule_totalistic_t *rule)
{
    if (!config || !rule) return false;

    config->rule_type = CA_RULE_TOTALISTIC;
    config->num_states = rule->num_states;

    /* Build lookup table for totalistic rule.
     * For 2D outer-totalistic: table indexed by (own_state * (max_sum+1) + neighbor_sum) */
    int max_sum = rule->num_states * 9;
    size_t lut_size = (size_t)(rule->num_states * (max_sum + 1));

    uint64_t *lut = (uint64_t *)calloc(lut_size, sizeof(uint64_t));
    if (!lut) return false;

    for (int s = 0; s < rule->num_states; s++) {
        for (int sum = 0; sum <= max_sum; sum++) {
            size_t idx = (size_t)(s * (max_sum + 1) + sum);
            if (s == 0) {
                /* Dead cell: birth if sum matches birth condition */
                lut[idx] = (sum < (int)rule->birth_count && rule->birth[sum]) ? 1 : 0;
            } else {
                /* Live cell: survive if sum matches survival condition */
                lut[idx] = (sum < (int)rule->survival_count && rule->survival[sum]) ? 1 : 0;
            }
        }
    }

    config->rule_lut = lut;
    config->rule_lut_size = lut_size;
    return true;
}

bool ca_config_set_elementary(ca_config_t *config, const ca_rule_elementary_t *rule)
{
    if (!config || !rule) return false;

    config->rule_type = CA_RULE_ELEMENTARY;
    config->num_states = 2;
    config->radius = 1;

    /* Build lookup table: 256 entries for radius-1 1D CA */
    size_t lut_size = 256;
    uint64_t *lut = (uint64_t *)calloc(lut_size, sizeof(uint64_t));
    if (!lut) return false;

    for (int i = 0; i < 256; i++) {
        /* For elementary CA, only the 3-bit neighborhood matters (bits 4,2,1) */
        int hood = ((i >> 2) & 4) | (i & 2) | ((i << 2) & 1); /* reorder to left,center,right */
        if (hood < 8) {
            lut[i] = rule->rule_bits[hood];
        }
    }

    config->rule_lut = lut;
    config->rule_lut_size = lut_size;
    return true;
}

void ca_config_free(ca_config_t *config)
{
    if (config) {
        free(config->rule_lut);
        config->rule_lut = NULL;
        config->rule_lut_size = 0;
    }
}

/*??????????????????????????????????????????????????????????????????????
 * CA State Management
 *??????????????????????????????????????????????????????????????????????*/

ca_state_t *ca_state_create(const ca_config_t *config)
{
    if (!config) return NULL;

    ca_state_t *state = (ca_state_t *)calloc(1, sizeof(ca_state_t));
    if (!state) return NULL;

    size_t n_cells = (size_t)(config->width * config->height);
    state->cells = (uint8_t *)calloc(n_cells, sizeof(uint8_t));
    state->next_cells = (uint8_t *)calloc(n_cells, sizeof(uint8_t));
    if (!state->cells || !state->next_cells) {
        free(state->cells);
        free(state->next_cells);
        free(state);
        return NULL;
    }

    state->width = config->width;
    state->height = config->height;
    memcpy(&state->config, config, sizeof(ca_config_t));
    /* Deep copy rule LUT */
    if (config->rule_lut && config->rule_lut_size > 0) {
        state->config.rule_lut = (uint64_t *)malloc(
            config->rule_lut_size * sizeof(uint64_t));
        if (state->config.rule_lut) {
            memcpy(state->config.rule_lut, config->rule_lut,
                   config->rule_lut_size * sizeof(uint64_t));
        }
    }
    state->generation = 0;
    return state;
}

void ca_state_destroy(ca_state_t *state)
{
    if (state) {
        free(state->cells);
        free(state->next_cells);
        free(state->config.rule_lut);
        free(state);
    }
}

uint8_t ca_state_get(const ca_state_t *state, int64_t x, int64_t y)
{
    if (!state) return 0;

    switch (state->config.boundary) {
        case CA_BOUNDARY_PERIODIC:
            x = ((x % state->width) + state->width) % state->width;
            y = ((y % state->height) + state->height) % state->height;
            break;
        case CA_BOUNDARY_REFLECTIVE:
            if (x < 0) x = -x - 1;
            if (x >= state->width) x = 2 * state->width - x - 1;
            if (y < 0) y = -y - 1;
            if (y >= state->height) y = 2 * state->height - y - 1;
            /* Clamp after reflection */
            if (x < 0) x = 0;
            if (x >= state->width) x = state->width - 1;
            if (y < 0) y = 0;
            if (y >= state->height) y = state->height - 1;
            break;
        case CA_BOUNDARY_FIXED:
        default:
            if (x < 0 || x >= state->width || y < 0 || y >= state->height) return 0;
            break;
    }

    return state->cells[y * state->width + x];
}

void ca_state_set(ca_state_t *state, int64_t x, int64_t y, uint8_t value)
{
    if (!state || x < 0 || x >= state->width || y < 0 || y >= state->height) return;
    state->cells[y * state->width + x] = value;
}

void ca_state_randomize(ca_state_t *state, unsigned int seed)
{
    if (!state) return;
    srand(seed);
    size_t n = (size_t)(state->width * state->height);
    for (size_t i = 0; i < n; i++) {
        state->cells[i] = (uint8_t)(rand() % state->config.num_states);
    }
}

void ca_state_fill(ca_state_t *state, uint8_t value)
{
    if (!state) return;
    size_t n = (size_t)(state->width * state->height);
    memset(state->cells, value, n);
}

void ca_state_point_init(ca_state_t *state, int64_t x, int64_t y)
{
    if (!state) return;
    ca_state_fill(state, 0);
    ca_state_set(state, x, y, 1);
}

/*??????????????????????????????????????????????????????????????????????
 * L5: CA Evolution Engine
 *??????????????????????????????????????????????????????????????????????*/

int ca_neighbor_sum_2d(const ca_state_t *state, int64_t x, int64_t y,
                        ca_neighborhood_t type)
{
    if (!state) return 0;
    int sum = 0;

    switch (type) {
        case CA_NEIGH_VON_NEUMANN:
            sum = (int)ca_state_get(state, x - 1, y)
                + (int)ca_state_get(state, x + 1, y)
                + (int)ca_state_get(state, x, y - 1)
                + (int)ca_state_get(state, x, y + 1);
            break;

        case CA_NEIGH_MOORE:
            for (int64_t dy = -1; dy <= 1; dy++) {
                for (int64_t dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    sum += (int)ca_state_get(state, x + dx, y + dy);
                }
            }
            break;

        case CA_NEIGH_EXTENDED_MOORE:
            for (int64_t dy = -2; dy <= 2; dy++) {
                for (int64_t dx = -2; dx <= 2; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    sum += (int)ca_state_get(state, x + dx, y + dy);
                }
            }
            break;
    }

    return sum;
}

uint64_t ca_neighbor_pattern_1d(const ca_state_t *state, int64_t x, int radius)
{
    if (!state) return 0;
    uint64_t pattern = 0;
    int bit_pos = 0;

    /* Collect neighborhood bits from left to right */
    for (int64_t dx = -(int64_t)radius; dx <= (int64_t)radius; dx++) {
        uint8_t val = ca_state_get(state, x + dx, 0);
        if (val) {
            pattern |= ((uint64_t)1 << bit_pos);
        }
        bit_pos++;
    }

    return pattern;
}

uint64_t ca_evolve_step(ca_state_t *state)
{
    if (!state) return 0;

    uint64_t changed = 0;
    size_t n = (size_t)(state->width * state->height);

    /* Apply rule to compute next state */
    if (state->config.dim == CA_DIM_1D) {
        if (state->config.rule_type == CA_RULE_ELEMENTARY) {
            ca_evolve_1d_elementary(state);
        }
    } else {
        if (state->config.rule_type == CA_RULE_TOTALISTIC) {
            ca_evolve_2d_totalistic(state);
        } else if (state->config.rule_type == CA_RULE_OUTER_TOTALISTIC) {
            ca_evolve_2d_outer_totalistic(state);
        }
    }

    /* Count changed cells and swap buffers */
    for (size_t i = 0; i < n; i++) {
        if (state->cells[i] != state->next_cells[i]) changed++;
    }
    ca_swap_buffers(state);
    state->generation++;

    return changed;
}

uint64_t ca_evolve_run(ca_state_t *state, uint64_t n_generations)
{
    if (!state) return 0;
    uint64_t total_changed = 0;
    for (uint64_t g = 0; g < n_generations; g++) {
        total_changed += ca_evolve_step(state);
    }
    return total_changed;
}

void ca_evolve_1d_elementary(ca_state_t *state)
{
    if (!state || state->config.dim != CA_DIM_1D) return;

    int64_t w = state->width;
    for (int64_t x = 0; x < w; x++) {
        /* Get 3-bit neighborhood: left, center, right */
        uint8_t left = ca_state_get(state, x - 1, 0);
        uint8_t center = ca_state_get(state, x, 0);
        uint8_t right = ca_state_get(state, x + 1, 0);
        int hood = (int)((left << 2) | (center << 1) | right);

        /* Lookup in rule bits */
        uint8_t next = 0;
        if (state->config.rule_lut && hood < 8) {
            /* Use expanded rule bits */
            uint8_t rule_num = (uint8_t)state->config.rule_lut[0];
            ca_rule_elementary_t erule = ca_rule_elementary_create(rule_num);
            next = erule.rule_bits[hood];
        } else {
            /* Direct elementary rule from LUT */
            uint64_t pattern = (uint64_t)hood;
            if (pattern < state->config.rule_lut_size) {
                next = (uint8_t)state->config.rule_lut[pattern];
            }
        }

        state->next_cells[x] = next;
    }
}

void ca_evolve_2d_totalistic(ca_state_t *state)
{
    if (!state || state->config.dim != CA_DIM_2D) return;

    int max_sum = state->config.num_states * 9;
    int64_t w = state->width;
    int64_t h = state->height;

    for (int64_t y = 0; y < h; y++) {
        for (int64_t x = 0; x < w; x++) {
            int sum = ca_neighbor_sum_2d(state, x, y, state->config.neighbor);
            /* Totalistic: next state depends only on neighbor sum */
            if (state->config.rule_lut && sum < (int)state->config.rule_lut_size) {
                state->next_cells[y * w + x] = (uint8_t)state->config.rule_lut[sum];
            } else {
                /* Default rule: next = sum mod num_states */
                state->next_cells[y * w + x] = (uint8_t)(sum % state->config.num_states);
            }
        }
    }
}

void ca_evolve_2d_outer_totalistic(ca_state_t *state)
{
    if (!state || state->config.dim != CA_DIM_2D) return;

    int max_sum = state->config.num_states * 9;
    int64_t w = state->width;
    int64_t h = state->height;

    for (int64_t y = 0; y < h; y++) {
        for (int64_t x = 0; x < w; x++) {
            int sum = ca_neighbor_sum_2d(state, x, y, state->config.neighbor);
            uint8_t own = ca_state_get(state, x, y);
            /* Outer-totalistic: depends on own state AND neighbor sum */
            size_t idx = (size_t)(own * (max_sum + 1) + sum);
            if (state->config.rule_lut && idx < state->config.rule_lut_size) {
                state->next_cells[y * w + x] = (uint8_t)state->config.rule_lut[idx];
            } else {
                state->next_cells[y * w + x] = own; /* default: no change */
            }
        }
    }
}

void ca_swap_buffers(ca_state_t *state)
{
    if (!state) return;
    uint8_t *tmp = state->cells;
    state->cells = state->next_cells;
    state->next_cells = tmp;
}

/*??????????????????????????????????????????????????????????????????????
 * L4: Wolfram Classification
 *
 * Classes:
 *   I  ? Evolves to homogeneous state (fixed point)
 *   II ? Evolves to periodic structures (limit cycles)
 *   III? Chaotic, aperiodic patterns (strange attractor)
 *   IV ? Complex localized structures (edge of chaos)
 *
 * Classification method (simplified from Wolfram 1984):
 *   1. Measure spatial entropy at steady state
 *   2. Measure sensitivity to initial conditions (damage spread)
 *   3. Check for persistent localized structures (gliders)
 *??????????????????????????????????????????????????????????????????????*/

ca_wolfram_class_t ca_classify_wolfram_1d(const ca_state_t *state, int num_steps)
{
    if (!state || state->config.dim != CA_DIM_1D) return CA_CLASS_UNKNOWN;

    /* Create a copy for evolution */
    ca_state_t *copy = ca_state_create(&state->config);
    if (!copy) return CA_CLASS_UNKNOWN;
    size_t n = (size_t)(state->width * state->height);
    memcpy(copy->cells, state->cells, n);

    /* Evolve and measure properties */
    ca_evolve_run(copy, (uint64_t)num_steps);

    double entropy = ca_spatial_entropy(copy);

    /* Create perturbed copy for damage spread */
    ca_state_t *perturbed = ca_state_create(&state->config);
    if (perturbed) {
        memcpy(perturbed->cells, state->cells, n);
        /* Flip one cell */
        perturbed->cells[n / 2] ^= 1;

        uint64_t damage = ca_damage_spread(copy, perturbed, num_steps / 2);
        double damage_rate = (double)damage / (double)(state->width);

        ca_state_destroy(perturbed);

        /* Classification based on entropy + damage spread */
        if (entropy < 0.1 && damage_rate < 0.1) {
            ca_state_destroy(copy);
            return CA_CLASS_I;  /* Freezes to uniform */
        }
        if (entropy < 0.5 && damage_rate < 0.3) {
            ca_state_destroy(copy);
            return CA_CLASS_II; /* Periodic/stable structures */
        }
        if (entropy > 0.8 && damage_rate > 0.6) {
            ca_state_destroy(copy);
            return CA_CLASS_III; /* Chaotic */
        }
    }

    ca_state_destroy(copy);
    /* Intermediate entropy + intermediate damage ? Class IV candidate */
    return CA_CLASS_IV;
}

double ca_spatial_entropy(const ca_state_t *state)
{
    if (!state) return 0.0;

    /* Compute Shannon entropy of cell states */
    uint64_t freq[256] = {0};
    size_t n = (size_t)(state->width * state->height);

    for (size_t i = 0; i < n; i++) {
        if (state->cells[i] < 256) {
            freq[state->cells[i]]++;
        }
    }

    double entropy = 0.0;
    for (int k = 0; k < state->config.num_states; k++) {
        if (freq[k] > 0) {
            double p = (double)freq[k] / (double)n;
            entropy -= p * log2(p);
        }
    }

    return entropy;
}

double ca_temporal_entropy(ca_state_t *state, int window_size)
{
    if (!state || window_size <= 0) return 0.0;

    /* Approximate: compute entropy of difference pattern over time.
     * This is a first-order approximation of entropy rate. */
    size_t n = (size_t)(state->width * state->height);
    uint64_t changes = 0;

    for (int t = 0; t < window_size; t++) {
        changes += ca_evolve_step(state);
    }

    double change_rate = (double)changes / (double)(n * (uint64_t)window_size);
    /* Bound to [0, 0.5] for binary CA */
    if (change_rate > 0.5) change_rate = 1.0 - change_rate;
    if (change_rate <= 0.0 || change_rate >= 1.0) return 0.0;

    return -(change_rate * log2(change_rate) + (1.0 - change_rate) * log2(1.0 - change_rate));
}

uint64_t ca_damage_spread(ca_state_t *state_a, ca_state_t *state_b, int steps)
{
    if (!state_a || !state_b) return 0;
    if (state_a->width != state_b->width || state_a->height != state_b->height) return 0;

    /* Evolve both copies independently */
    for (int t = 0; t < steps; t++) {
        ca_evolve_step(state_a);
        ca_evolve_step(state_b);
    }

    /* Count differing cells */
    uint64_t diff = 0;
    size_t n = (size_t)(state_a->width * state_a->height);
    for (size_t i = 0; i < n; i++) {
        if (state_a->cells[i] != state_b->cells[i]) diff++;
    }

    return diff;
}

/*??????????????????????????????????????????????????????????????????????
 * L8: Advanced CA Analysis
 *??????????????????????????????????????????????????????????????????????*/

double ca_mutual_information(const ca_state_t *state, int64_t x, int64_t y)
{
    if (!state) return 0.0;

    /* Compute I(cell ; neighbors) for a single cell.
     * Simplified: correlation between cell state and neighbor sum. */
    int cell_state = (int)ca_state_get(state, x, y);
    int neighbor_sum = ca_neighbor_sum_2d(state, x, y, CA_NEIGH_MOORE);

    /* Joint frequency estimate from local context only.
     * For a rigorous estimate, one needs ensemble or temporal statistics.
     * Here we return a normalized correlation as proxy. */
    double n_norm = (double)neighbor_sum / 8.0; /* max Moore sum for binary */
    double s_norm = (double)cell_state;

    /* Mutual information proxy: I ? |correlation| */
    return fabs(n_norm - 0.5) * s_norm + fabs(s_norm - n_norm);
}

double ca_excess_entropy(const ca_state_t *states[], int num_states)
{
    if (!states || num_states < 2) return 0.0;

    /* Excess entropy E = lim_{L->inf} (H(L) - L * h_mu)
     * Approximated as: I(past; future) for blocks of increasing length.
     *
     * Here: use first state as "past" block, compute mutual information
     * with later states. E ? sum of temporal mutual information. */
    double e = 0.0;
    for (int t = 1; t < num_states; t++) {
        /* Crude MI: fraction of cells that differ from original,
         * normalized and transformed */
        size_t n = (size_t)(states[0]->width * states[0]->height);
        uint64_t same = 0;
        for (size_t i = 0; i < n; i++) {
            if (states[0]->cells[i] == states[t]->cells[i]) same++;
        }
        double sim = (double)same / (double)n;
        /* MI proxy: how much similarity decays with time */
        e += sim / (double)t;
    }
    return e;
}

double ca_lambda_parameter(const ca_config_t *config)
{
    if (!config || !config->rule_lut || config->rule_lut_size == 0) return 0.0;

    /* ? = fraction of rule table entries mapping to non-quiescent state.
     * Quiescent state = 0 (by convention). */
    size_t n_nonzero = 0;
    for (size_t i = 0; i < config->rule_lut_size; i++) {
        if (config->rule_lut[i] != 0) n_nonzero++;
    }

    return (double)n_nonzero / (double)config->rule_lut_size;
}

bool ca_rule_is_number_conserving(const ca_config_t *config)
{
    if (!config || !config->rule_lut) return false;

    /* Check all possible transitions: total sum of states must be invariant.
     * For binary CA: count of 1s in output must equal count in input.
     * Complete test is exponential; we do random sampling. */
    return false; /* Placeholder: requires exhaustive or symbolic check */
}

bool ca_detect_glider(const ca_state_t *state, int64_t *gx, int64_t *gy,
                      int64_t *dx, int64_t *dy, int *period)
{
    if (!state) return false;

    /* Simplified glider detection: look for a small cluster of 1s
     * that is isolated and has compact bounding box.
     * Full detection requires temporal comparison. */
    (void)gx; (void)gy; (void)dx; (void)dy; (void)period;
    return false; /* Requires multi-state comparison ? see alife_patterns.c */
}

int64_t ca_save_spacetime_pgm(const ca_state_t **history, int num_states,
                               const char *filename)
{
    if (!history || num_states <= 0 || !filename) return -1;

    FILE *fp = fopen(filename, "wb");
    if (!fp) return -1;

    int64_t w = history[0]->width;
    fprintf(fp, "P5\n%lld %d\n255\n", (long long)w, num_states);

    int64_t written = 0;
    for (int t = 0; t < num_states; t++) {
        for (int64_t x = 0; x < w; x++) {
            uint8_t pixel = (uint8_t)(history[t]->cells[x] ? 0 : 255);
            fputc(pixel, fp);
            written++;
        }
    }

    fclose(fp);
    return written + 15;
}

bool ca_rule_is_symmetric(const ca_config_t *config)
{
    if (!config || !config->rule_lut) return true;

    /* Check left-right symmetry: rule output for pattern P
     * should equal output for reflected pattern P'.
     * For simplicity: check a few representative cases. */
    if (config->rule_lut_size < 8) return true;

    /* For elementary CA: obvious asymmetry if rule != its own mirror */
    if (config->rule_type == CA_RULE_ELEMENTARY) {
        /* Mirror equivalences known for Wolfram rules:
         * Rule 30 != mirror of 30 (it's its own complement)
         * We approximate: check select patterns */
        if (config->rule_lut_size >= 8) {
            /* Check if 001 and 100 give same output */
            bool symmetric = true;
            for (int i = 0; i < 4; i++) {
                int rev = ((i & 1) << 2) | (i & 2) | ((i & 4) >> 2);
                if (i < (int)config->rule_lut_size && rev < (int)config->rule_lut_size) {
                    if (config->rule_lut[i] != config->rule_lut[rev]) {
                        symmetric = false;
                        break;
                    }
                }
            }
            return symmetric;
        }
    }

    return true;
}

bool ca_rule_is_totalistic(const ca_config_t *config)
{
    /* A rule is totalistic if output depends only on sum of inputs,
     * not on their arrangement. */
    return (config && config->rule_type == CA_RULE_TOTALISTIC);
}

int ca_enumerate_rules(ca_dimension_t dim, int num_states, ca_rule_type_t type,
                        void *rules_out, size_t max_rules)
{
    if (!rules_out) return 0;

    int count = 0;
    if (dim == CA_DIM_1D && type == CA_RULE_ELEMENTARY) {
        /* Elementary CA: 256 rules */
        ca_rule_elementary_t *rules = (ca_rule_elementary_t *)rules_out;
        for (int r = 0; r < 256 && (size_t)count < max_rules; r++) {
            rules[count] = ca_rule_elementary_create((uint8_t)r);
            count++;
        }
    }

    return count;
}