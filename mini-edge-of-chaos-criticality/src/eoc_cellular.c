#include "eoc_cellular.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Cellular Automata ? Wolfram Classes, Langton Lambda, Edge of Chaos
 *
 * L2 Core Concepts: Cellular automata as minimal models of spatially-extended
 *   computation. The edge of chaos hypothesis (Langton 1990, Packard 1988):
 *   systems capable of universal computation exist at the phase transition
 *   between order and chaos.
 *
 * L3 Mathematical Structures:
 *   - 1D CA: state set S, radius r, rule f: S^{2r+1} -> S
 *   - Rule space size: |S|^{|S|^{2r+1}} (e.g., 2^256 for binary r=2)
 *   - Langton's lambda: fraction of non-quiescent outputs
 *
 * L4 Fundamental Laws:
 *   - Wolfram's classification (Classes I-IV)
 *   - Cook's proof that Rule 110 is Turing-complete (2004)
 *   - Conway's Game of Life is Turing-complete (Berlekamp, Conway, Guy 1982)
 *
 * L5 Algorithms:
 *   - CA evolution with efficient boundary conditions
 *   - Wolfram class identification via entropy + density + transients
 *   - Langton's lambda phase transition measurement
 * ============================================================================ */

/* ============================================================================
 * 1D Elementary Cellular Automata (k=2, r=1)
 *
 * Rule number encodes the 8-bit output for neighborhoods 111 to 000.
 * Rule 110: 01101110 = neighborhoods {111:0, 110:1, 101:1, 100:0, 011:1, 010:1, 001:1, 000:0}
 * ============================================================================ */

int eoc_ca_rule_eval(int rule_number, int left, int center, int right) {
    /* Encode 3-bit neighborhood: left*4 + center*2 + right */
    int neighborhood = ((left & 1) << 2) | ((center & 1) << 1) | (right & 1);
    return (rule_number >> neighborhood) & 1;
}

void eoc_ca_evolve_1d(int rule_number, int* lattice, int width,
                       int n_steps, int** state_history, bool periodic) {
    if (!lattice || width < 3) return;

    int* current = (int*)malloc(width * sizeof(int));
    int* next = (int*)malloc(width * sizeof(int));
    memcpy(current, lattice, width * sizeof(int));

    if (state_history) {
        for (int i = 0; i < width; i++) state_history[0][i] = current[i];
    }

    for (int step = 1; step <= n_steps; step++) {
        for (int i = 0; i < width; i++) {
            int left, center, right;
            center = current[i];

            if (periodic) {
                left = current[(i - 1 + width) % width];
                right = current[(i + 1) % width];
            } else {
                left = (i > 0) ? current[i - 1] : 0;
                right = (i < width - 1) ? current[i + 1] : 0;
            }

            next[i] = eoc_ca_rule_eval(rule_number, left, center, right);
        }

        memcpy(current, next, width * sizeof(int));
        if (state_history) {
            for (int i = 0; i < width; i++) state_history[step][i] = current[i];
        }
    }

    memcpy(lattice, current, width * sizeof(int));
    free(current);
    free(next);
}

/* ============================================================================
 * Langton's Lambda Parameter
 *
 * lambda = (total_states - n_quiescent) / total_states
 * For elementary CA (k=2, r=1, 8 rules):
 *   Quiescent state q: transitions where neighborhood yields q.
 *   lambda in [0, 1], step size 1/8.
 * ============================================================================ */

double eoc_ca_lambda_1d(int rule_number, int quiescent_state) {
    int n_quiescent = 0;
    int total = 8; /* 2^3 neighborhoods */

    for (int n = 0; n < total; n++) {
        int output = (rule_number >> n) & 1;
        if (output == quiescent_state) n_quiescent++;
    }

    return (double)(total - n_quiescent) / total;
}

/* ============================================================================
 * Wolfram Class Classification for 1D CA
 *
 * Class I: Evolves to homogeneous state (entropy -> 0, density -> constant)
 * Class II: Simple periodic or stable structures (low entropy, periodic)
 * Class III: Chaotic patterns (high entropy, aperiodic)
 * Class IV: Complex localized structures (intermediate entropy, long transients)
 *
 * Method (based on Wuensche 1992, Langton 1990):
 *   1. Measure asymptotic spatial entropy
 *   2. Measure temporal entropy (patterns across time)
 *   3. Classify based on combination
 * ============================================================================ */

WolframClass eoc_ca_classify_1d(int rule_number, int width, int n_steps) {
    int* lattice = (int*)calloc(width, sizeof(int));
    /* Start with single cell in center (canonical initial condition) */
    lattice[width / 2] = 1;

    int** history = (int**)malloc((n_steps + 1) * sizeof(int*));
    for (int s = 0; s <= n_steps; s++) {
        history[s] = (int*)malloc(width * sizeof(int));
    }

    eoc_ca_evolve_1d(rule_number, lattice, width, n_steps, history, true);

    /* Compute spatial entropy of final configuration */
    double final_density = 0.0;
    for (int i = 0; i < width; i++) {
        final_density += history[n_steps][i];
    }
    final_density /= width;

    /* Compute temporal variance (how much patterns change) */
    double* densities = (double*)malloc((n_steps + 1) * sizeof(double));
    for (int s = 0; s <= n_steps; s++) {
        densities[s] = 0.0;
        for (int i = 0; i < width; i++) densities[s] += history[s][i];
        densities[s] /= width;
    }

    /* Variance of density over last half of evolution */
    int half = n_steps / 2;
    double mean_dens = 0.0;
    for (int s = half; s <= n_steps; s++) mean_dens += densities[s];
    mean_dens /= (n_steps - half + 1);

    double var_dens = 0.0;
    for (int s = half; s <= n_steps; s++) {
        double d = densities[s] - mean_dens;
        var_dens += d * d;
    }
    var_dens /= (n_steps - half + 1);

    /* Spatial entropy of final state */
    double spatial_entropy = eoc_ca_spatial_entropy(history[n_steps], width, 1, 2);

    /* Classification logic */
    WolframClass wc;

    if (var_dens < 0.0001 && spatial_entropy < 0.1) {
        /* Fixed point ? Class I */
        wc = WOLFRAM_CLASS_I;
    } else if (var_dens < 0.01 && spatial_entropy < 0.5) {
        /* Periodic ? Class II */
        wc = WOLFRAM_CLASS_II;
    } else if (spatial_entropy > 0.65) {
        /* High entropy, chaotic ? Class III */
        wc = WOLFRAM_CLASS_III;
    } else {
        /* Intermediate ? Class IV (edge of chaos) */
        wc = WOLFRAM_CLASS_IV;
    }

    for (int s = 0; s <= n_steps; s++) free(history[s]);
    free(history);
    free(lattice);
    free(densities);

    return wc;
}

/* ============================================================================
 * Conway's Game of Life (B3/S23)
 * ============================================================================ */

void eoc_game_of_life_step(int* grid, int* next_grid, int width, int height) {
    if (!grid || !next_grid) return;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            /* Count live Moore neighbors (8) with toroidal boundaries */
            int neighbors = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = (x + dx + width) % width;
                    int ny = (y + dy + height) % height;
                    neighbors += grid[ny * width + nx];
                }
            }

            int idx = y * width + x;
            if (grid[idx] == 1) {
                /* Survival: 2 or 3 neighbors */
                next_grid[idx] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                /* Birth: exactly 3 neighbors */
                next_grid[idx] = (neighbors == 3) ? 1 : 0;
            }
        }
    }
}

void eoc_game_of_life_evolve(int* grid, int width, int height,
                              int n_steps, double* density_history) {
    if (!grid) return;

    int N = width * height;
    int* buf_a = (int*)malloc(N * sizeof(int));
    int* buf_b = (int*)malloc(N * sizeof(int));
    memcpy(buf_a, grid, N * sizeof(int));

    for (int step = 0; step < n_steps; step++) {
        eoc_game_of_life_step(buf_a, buf_b, width, height);

        if (density_history) {
            double density = 0.0;
            for (int i = 0; i < N; i++) density += buf_b[i];
            density_history[step] = density / N;
        }

        /* Swap buffers */
        int* tmp = buf_a;
        buf_a = buf_b;
        buf_b = tmp;
    }

    memcpy(grid, buf_a, N * sizeof(int));
    free(buf_a);
    free(buf_b);
}

/* ============================================================================
 * 2D CA Classification
 * ============================================================================ */

WolframClass eoc_ca_classify_2d(int* grid, int width, int height) {
    if (!grid) return WOLFRAM_CLASS_I;

    int N = width * height;
    double density = 0.0;
    for (int i = 0; i < N; i++) density += grid[i];
    density /= N;

    double entropy = eoc_ca_spatial_entropy(grid, width, height, 3);

    if (density < 0.01 || density > 0.99) return WOLFRAM_CLASS_I;
    if (entropy < 0.3) return WOLFRAM_CLASS_II;
    if (entropy > 0.7) return WOLFRAM_CLASS_III;
    return WOLFRAM_CLASS_IV;
}

/* ============================================================================
 * Spatial Entropy of CA Grid
 *
 * Uses block-entropy method: partition grid into blocks of size BxB,
 * count occurrences of each block pattern, compute Shannon entropy.
 * ============================================================================ */

double eoc_ca_spatial_entropy(int* grid, int width, int height, int block_size) {
    if (!grid || width < block_size || height < block_size) return 0.0;

    int n_blocks_x = width / block_size;
    int n_blocks_y = height / block_size;
    int n_blocks = n_blocks_x * n_blocks_y;

    if (n_blocks < 2) return 0.0;

    /* For binary CA, count [0,1] proportions within each block */
    double* block_densities = (double*)malloc(n_blocks * sizeof(double));
    int bs2 = block_size * block_size;

    for (int by = 0; by < n_blocks_y; by++) {
        for (int bx = 0; bx < n_blocks_x; bx++) {
            int sum = 0;
            for (int dy = 0; dy < block_size; dy++) {
                for (int dx = 0; dx < block_size; dx++) {
                    int px = bx * block_size + dx;
                    int py = by * block_size + dy;
                    sum += grid[py * width + px];
                }
            }
            block_densities[by * n_blocks_x + bx] = (double)sum / bs2;
        }
    }

    /* Shannon entropy of block density distribution */
    int hist_bins = 10;
    double* hist = (double*)calloc(hist_bins, sizeof(double));
    for (int i = 0; i < n_blocks; i++) {
        int bin = (int)(block_densities[i] * (hist_bins - 1));
        if (bin < 0) bin = 0;
        if (bin >= hist_bins) bin = hist_bins - 1;
        hist[bin] += 1.0;
    }

    double entropy = 0.0;
    for (int b = 0; b < hist_bins; b++) {
        double p = hist[b] / n_blocks;
        if (p > 0.0) entropy -= p * log(p);
    }
    /* Normalize by maximum entropy */
    double max_entropy = log((double)hist_bins);
    entropy = (max_entropy > 0.0) ? entropy / max_entropy : 0.0;

    free(block_densities);
    free(hist);
    return entropy;
}

/* ============================================================================
 * Mutual Information Between CA Configurations
 *
 * I(X;Y) = H(X) + H(Y) - H(X,Y)
 * Used as order parameter in Langton's edge-of-chaos analysis.
 * At the phase transition (lambda ~ 0.273), I(X;Y) peaks.
 * ============================================================================ */

double eoc_ca_mutual_information(int* config1, int* config2, int n) {
    if (!config1 || !config2 || n < 1) return 0.0;

    /* 2x2 joint probability table */
    double p[2][2] = {{0, 0}, {0, 0}};
    for (int i = 0; i < n; i++) {
        int a = (config1[i] != 0) ? 1 : 0;
        int b = (config2[i] != 0) ? 1 : 0;
        p[a][b] += 1.0;
    }

    /* Normalize */
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            p[i][j] /= n;

    /* Marginals */
    double px[2] = {p[0][0] + p[0][1], p[1][0] + p[1][1]};
    double py[2] = {p[0][0] + p[1][0], p[0][1] + p[1][1]};

    /* Mutual Information */
    double mi = 0.0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (p[i][j] > 0.0 && px[i] > 0.0 && py[j] > 0.0) {
                mi += p[i][j] * log(p[i][j] / (px[i] * py[j]));
            }
        }
    }

    return mi;
}

/* ============================================================================
 * Langton's Ant
 *
 * A 2D Turing machine: an "ant" moves on a grid of black/white cells.
 * Rules: on white -> turn right, flip to black; on black -> turn left, flip to white.
 * After ~10,000 steps, emerges a "highway" pattern ? emergence of order from
 * simple rules (similar to edge of chaos behavior).
 * ============================================================================ */

void eoc_langtons_ant(int* grid, int width, int height,
                       int* ant_x, int* ant_y, int* ant_dir, int n_steps) {
    if (!grid || !ant_x || !ant_y || !ant_dir) return;

    /* Directions: 0=up, 1=right, 2=down, 3=left */
    int dx[4] = {0, 1, 0, -1};
    int dy[4] = {-1, 0, 1, 0};

    int x = *ant_x;
    int y = *ant_y;
    int dir = *ant_dir;

    for (int step = 0; step < n_steps; step++) {
        int idx = y * width + x;
        int cell = grid[idx];

        /* Turn: right on 0, left on 1 */
        if (cell == 0) {
            dir = (dir + 1) % 4; /* Right turn */
        } else {
            dir = (dir + 3) % 4; /* Left turn */
        }

        /* Flip cell */
        grid[idx] = 1 - cell;

        /* Move forward */
        x = (x + dx[dir] + width) % width;
        y = (y + dy[dir] + height) % height;
    }

    *ant_x = x;
    *ant_y = y;
    *ant_dir = dir;
}

/* ============================================================================
 * Brian's Brain CA (/2/3)
 *
 * Three states: 0=dead, 1=dying (was alive last step), 2=alive.
 * Rule: B2/S/3 (birth on 2, no survival, state 3 = dying).
 * A dead cell with exactly 2 live neighbors becomes alive.
 * An alive cell becomes dying.
 * A dying cell becomes dead.
 * Produces glider-like waves at the edge of chaos.
 * ============================================================================ */

void eoc_brians_brain_step(int* grid, int* next_grid, int width, int height) {
    if (!grid || !next_grid) return;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            /* Count alive (state 2) Moore neighbors */
            int live_neighbors = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = (x + dx + width) % width;
                    int ny = (y + dy + height) % height;
                    if (grid[ny * width + nx] == 2) live_neighbors++;
                }
            }

            int idx = y * width + x;
            int state = grid[idx];

            if (state == 2) {
                /* Alive -> dying */
                next_grid[idx] = 1;
            } else if (state == 1) {
                /* Dying -> dead */
                next_grid[idx] = 0;
            } else {
                /* Dead -> alive if exactly 2 live neighbors */
                next_grid[idx] = (live_neighbors == 2) ? 2 : 0;
            }
        }
    }
}

/* ============================================================================
 * Wireworld CA
 *
 * 4 states: 0=empty, 1=conductor, 2=electron-head, 3=electron-tail.
 * Electron-head -> electron-tail.
 * Electron-tail -> conductor.
 * Conductor with 1 or 2 electron-head neighbors -> electron-head.
 * Turing-complete: can simulate logic gates and wires.
 * ============================================================================ */

void eoc_wireworld_step(int* grid, int* next_grid, int width, int height) {
    if (!grid || !next_grid) return;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            int state = grid[idx];

            if (state == 0) {
                next_grid[idx] = 0; /* Empty stays empty */
            } else if (state == 2) {
                next_grid[idx] = 3; /* Electron head -> tail */
            } else if (state == 3) {
                next_grid[idx] = 1; /* Electron tail -> conductor */
            } else {
                /* Conductor: count electron-head neighbors */
                int head_count = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = (x + dx + width) % width;
                        int ny = (y + dy + height) % height;
                        if (grid[ny * width + nx] == 2) head_count++;
                    }
                }
                /* Conductor with 1 or 2 head neighbors becomes head */
                next_grid[idx] = (head_count == 1 || head_count == 2) ? 2 : 1;
            }
        }
    }
}

/* ============================================================================
 * Langton's Lambda Phase Transition
 *
 * Hypothesis (Langton 1990): There exists a critical value lambda_c ~ 0.273
 * at which CA exhibit Class IV behavior ? maximal mutual information between
 * successive configurations, indicating computational capability.
 *
 * Method:
 * 1. For each lambda value (bin), generate random rules with that lambda.
 * 2. Evolve each rule on random initial condition.
 * 3. Measure mutual information I(t, t+1) between successive time steps.
 * 4. Average over rules in each lambda bin.
 *
 * Expected result: Mutual information peaks at lambda ~ 0.273.
 * ============================================================================ */

void eoc_langton_phase_transition(int n_lambda, int n_rules_per_bin,
                                   int width, int n_steps,
                                   double** lambdas, double** mi_avg,
                                   int* n_points) {
    *n_points = n_lambda;
    *lambdas = (double*)malloc(n_lambda * sizeof(double));
    *mi_avg = (double*)malloc(n_lambda * sizeof(double));

    for (int i = 0; i < n_lambda; i++) {
        double lambda = (i + 1.0) / n_lambda; /* 0 < lambda <= 1 */
        (*lambdas)[i] = lambda;

        double mi_sum = 0.0;
        int valid_rules = 0;

        /* Generate random rules with given lambda */
        for (int r = 0; r < n_rules_per_bin; r++) {
            int rule = 0;
            int n_ones = (int)(lambda * 8.0 + 0.5); /* For k=2, r=1, 8 entries */
            if (n_ones < 0) n_ones = 0;
            if (n_ones > 8) n_ones = 8;

            /* Randomly set n_ones bits */
            int bits[8] = {0};
            for (int b = 0; b < n_ones; b++) {
                int pos;
                do { pos = rand() % 8; } while (bits[pos] == 1);
                bits[pos] = 1;
            }
            rule = 0;
            for (int b = 0; b < 8; b++) {
                if (bits[b]) rule |= (1 << b);
            }

            /* Evolve on random initial condition */
            int* lattice_prev = (int*)malloc(width * sizeof(int));
            int* lattice_curr = (int*)malloc(width * sizeof(int));
            for (int j = 0; j < width; j++) lattice_curr[j] = rand() % 2;

            /* Evolve and measure MI between consecutive steps */
            double mi_rule_sum = 0.0;
            int mi_samples = 0;

            for (int step = 0; step < n_steps; step++) {
                memcpy(lattice_prev, lattice_curr, width * sizeof(int));

                /* One step of evolution */
                for (int j = 0; j < width; j++) {
                    int left = lattice_prev[(j - 1 + width) % width];
                    int center = lattice_prev[j];
                    int right = lattice_prev[(j + 1) % width];
                    lattice_curr[j] = eoc_ca_rule_eval(rule, left, center, right);
                }

                /* Compute MI between t and t+1 (skip first few transients) */
                if (step >= n_steps / 2) {
                    mi_rule_sum += eoc_ca_mutual_information(lattice_prev, lattice_curr, width);
                    mi_samples++;
                }
            }

            if (mi_samples > 0) {
                mi_sum += mi_rule_sum / mi_samples;
                valid_rules++;
            }

            free(lattice_prev);
            free(lattice_curr);
        }

        (*mi_avg)[i] = (valid_rules > 0) ? mi_sum / valid_rules : 0.0;
    }
}

/* ============================================================================
 * Wuensche Basin of Attraction Analysis
 *
 * For a 1D CA with periodic boundaries:
 * Each of the 2^width states evolves to an attractor (fixed point or cycle).
 * The basin of attraction is the set of all states that flow to a given attractor.
 *
 * Metrics:
 *   - Number of fixed point attractors
 *   - Number of cycle attractors
 *   - Average transient length to attractor
 *   - Basin entropy: H = -sum p_i * ln(p_i) where p_i is probability of landing
 *     in basin i (from random initial state).
 * ============================================================================ */

void eoc_ca_basin_analysis(int rule_number, int width, int max_steps,
                            int* n_fixed, int* n_cycles,
                            double* avg_transient, double* basin_entropy) {
    *n_fixed = 0;
    *n_cycles = 0;
    *avg_transient = 0.0;
    if (basin_entropy) *basin_entropy = 0.0;

    if (width > 10) return; /* Exhaustive only for small widths (2^width states) */

    int N_states = 1 << width;
    if (N_states > 10000) return;

    int* state_to_attractor = (int*)malloc(N_states * sizeof(int));
    int* attractor_type = (int*)malloc(N_states * sizeof(int)); /* 0=unknown, 1=fixed, 2=cycle */
    int* attractor_period = (int*)malloc(N_states * sizeof(int));
    int* attractor_sizes = (int*)calloc(N_states, sizeof(int));
    int n_attractors = 0;
    double total_transient = 0.0;

    for (int s = 0; s < N_states; s++) state_to_attractor[s] = -1;

    for (int s = 0; s < N_states; s++) {
        if (state_to_attractor[s] >= 0) continue; /* Already classified */

        /* Evolve forward until cycle detected */
        int* visited = (int*)calloc(N_states, sizeof(int));
        int* path = (int*)malloc(max_steps * sizeof(int));
        int path_len = 0;
        int current = s;
        visited[current] = 1;
        path[path_len++] = current;

        for (int step = 1; step < max_steps; step++) {
            /* Compute next state */
            int next = 0;
            for (int bit = 0; bit < width; bit++) {
                int left = (current >> ((bit - 1 + width) % width)) & 1;
                int center = (current >> bit) & 1;
                int right = (current >> ((bit + 1) % width)) & 1;
                int result = eoc_ca_rule_eval(rule_number, left, center, right);
                next |= (result << bit);
            }

            if (visited[next]) {
                /* Found cycle */
                /* Find cycle start in path */
                int cycle_start = 0;
                while (path[cycle_start] != next) cycle_start++;

                int transient = cycle_start;
                int period = path_len - cycle_start;
                total_transient += transient;

                /* Assign attractor ID */
                int attr_id;
                /* Check if this cycle was already assigned */
                bool found = false;
                for (int j = cycle_start; j < path_len && !found; j++) {
                    if (state_to_attractor[path[j]] >= 0) {
                        attr_id = state_to_attractor[path[j]];
                        found = true;
                    }
                }

                if (!found) {
                    attr_id = n_attractors++;
                    attractor_type[attr_id] = (period == 1) ? 1 : 2;
                    attractor_period[attr_id] = period;
                }

                /* Map all transient + cycle states to attractor */
                for (int j = 0; j < path_len; j++) {
                    state_to_attractor[path[j]] = attr_id;
                }
                attractor_sizes[attr_id] += transient + 1;
                break;
            }

            visited[next] = 1;
            path[path_len++] = next;
            current = next;
        }

        free(visited);
        free(path);
    }

    /* Count attractor types */
    for (int a = 0; a < n_attractors; a++) {
        if (attractor_type[a] == 1) (*n_fixed)++;
        else if (attractor_type[a] == 2) (*n_cycles)++;
    }

    *avg_transient = total_transient / N_states;

    /* Basin entropy */
    if (basin_entropy && n_attractors > 0) {
        double H = 0.0;
        for (int a = 0; a < n_attractors; a++) {
            double p = (double)attractor_sizes[a] / N_states;
            if (p > 0.0) H -= p * log(p);
        }
        *basin_entropy = H;
    }

    free(state_to_attractor);
    free(attractor_type);
    free(attractor_period);
    free(attractor_sizes);
}
