/**
 * @file    example_schelling_segregation.c
 * @brief   SFI Methodology: Schelling Segregation Model (1971)
 *
 * Demonstrates Thomas Schelling's canonical SFI tipping-point model.
 * Two types of agents on a grid.  Each agent is "happy" if at least
 * a fraction F of its neighbours are of the same type.  Unhappy
 * agents move to a random empty cell.
 *
 * Emergent property: even with F = 30% (agent wants only 30% similar
 * neighbours), the macro outcome is near-complete segregation.
 * This is a classic SFI result: mild individual preferences can
 * produce extreme collective outcomes — a phase transition in
 * social space.
 *
 * Knowledge: L1 (agent types), L2 (tipping points, emergence),
 *   L5 (cellular automaton), L6 (canonical SFI problem)
 *
 * Reference: Schelling, T.C. (1971) "Dynamic Models of Segregation",
 *   Journal of Mathematical Sociology 1(2), 143-186.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GRID_SIZE      40
#define MAX_TICKS     200
#define EMPTY_FRAC     0.1
#define SIMILARITY_REQ 0.3

typedef enum { EMPTY = 0, TYPE_A = 1, TYPE_B = 2 } cell_t;

static int grid[GRID_SIZE][GRID_SIZE];

/* Count similar neighbours in 8-cell Moore neighbourhood (torus) */
static int count_similar(int x, int y) {
    cell_t me = (cell_t)grid[y][x];
    if (me == EMPTY) return 0;
    int similar = 0, total = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = (x + dx + GRID_SIZE) % GRID_SIZE;
            int ny = (y + dy + GRID_SIZE) % GRID_SIZE;
            if (grid[ny][nx] != EMPTY) {
                total++;
                if (grid[ny][nx] == me) similar++;
            }
        }
    }
    return (total > 0) ? similar : 0;
}

static int is_happy(int x, int y, double threshold) {
    cell_t me = (cell_t)grid[y][x];
    if (me == EMPTY) return 1;
    int similar = 0, total = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = (x + dx + GRID_SIZE) % GRID_SIZE;
            int ny = (y + dy + GRID_SIZE) % GRID_SIZE;
            if (grid[ny][nx] != EMPTY) {
                total++;
                if (grid[ny][nx] == me) similar++;
            }
        }
    }
    if (total == 0) return 1;  /* No neighbours = happy by default */
    return ((double)similar / (double)total) >= threshold;
}

/* Compute segregation index: fraction of same-type neighbours */
static double segregation_index(void) {
    double total_same = 0.0;
    int total_pairs = 0;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x] == EMPTY) continue;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = (x + dx + GRID_SIZE) % GRID_SIZE;
                    int ny = (y + dy + GRID_SIZE) % GRID_SIZE;
                    if (grid[ny][nx] != EMPTY) {
                        total_pairs++;
                        if (grid[ny][nx] == grid[y][x]) total_same += 1.0;
                    }
                }
            }
        }
    }
    return (total_pairs > 0) ? total_same / (double)total_pairs : 0.0;
}

static void print_grid(void) {
    printf("  +");
    for (int x = 0; x < GRID_SIZE; x++) printf("-");
    printf("+\n");
    for (int y = 0; y < GRID_SIZE; y++) {
        printf("  |");
        for (int x = 0; x < GRID_SIZE; x++) {
            if (grid[y][x] == TYPE_A) printf("A");
            else if (grid[y][x] == TYPE_B) printf("B");
            else printf(".");
        }
        printf("|\n");
    }
    printf("  +");
    for (int x = 0; x < GRID_SIZE; x++) printf("-");
    printf("+\n");
}

int main(void) {
    printf("=== Schelling Segregation Model (1971) ===\n");
    printf("Grid: %dx%d, Similarity threshold: %.0f%%, Max ticks: %d\n\n",
           GRID_SIZE, GRID_SIZE, SIMILARITY_REQ * 100.0, MAX_TICKS);

    srand(54321);

    /* Initialise grid: random placement with some empty cells */
    memset(grid, 0, sizeof(grid));
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            double r = (double)rand() / (double)RAND_MAX;
            if (r < EMPTY_FRAC) {
                grid[y][x] = EMPTY;
            } else if (r < EMPTY_FRAC + (1.0 - EMPTY_FRAC) / 2.0) {
                grid[y][x] = TYPE_A;
            } else {
                grid[y][x] = TYPE_B;
            }
        }
    }

    printf("Initial segregation index: %.4f\n", segregation_index());

    /* Simulation loop */
    for (int tick = 1; tick <= MAX_TICKS; tick++) {
        int moved = 0;

        /* Collect all unhappy agents */
        int unhappy_list[GRID_SIZE * GRID_SIZE][2];
        int unhappy_count = 0;
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                if (!is_happy(x, y, SIMILARITY_REQ)) {
                    unhappy_list[unhappy_count][0] = x;
                    unhappy_list[unhappy_count][1] = y;
                    unhappy_count++;
                }
            }
        }

        /* Move each unhappy agent to a random empty cell */
        for (int u = 0; u < unhappy_count; u++) {
            int ux = unhappy_list[u][0];
            int uy = unhappy_list[u][1];
            if (is_happy(ux, uy, SIMILARITY_REQ)) continue;  /* May have become happy */

            /* Find random empty cell */
            int empty_list[GRID_SIZE * GRID_SIZE][2];
            int empty_count = 0;
            for (int y = 0; y < GRID_SIZE; y++)
                for (int x = 0; x < GRID_SIZE; x++)
                    if (grid[y][x] == EMPTY) {
                        empty_list[empty_count][0] = x;
                        empty_list[empty_count][1] = y;
                        empty_count++;
                    }

            if (empty_count > 0) {
                int target = rand() % empty_count;
                int tx = empty_list[target][0];
                int ty = empty_list[target][1];

                /* Move agent */
                grid[ty][tx] = grid[uy][ux];
                grid[uy][ux] = EMPTY;
                moved++;
            }
        }

        if (tick % 40 == 0 || tick == 1) {
            double seg = segregation_index();
            printf("Tick %3d: Segregation = %.4f, Moved = %d\n", tick, seg, moved);
        }

        if (moved == 0) {
            printf("Equilibrium reached at tick %d\n", tick);
            break;
        }
    }

    double final_seg = segregation_index();
    printf("\nFinal segregation index: %.4f\n", final_seg);
    printf("\n=== SFI Methodology Insight ===\n");
    printf("Schelling's model shows that even with a mild preference\n");
    printf("(only %.0f%% similar neighbours desired), the MACRO outcome\n", SIMILARITY_REQ * 100.0);
    printf("is near-complete segregation (%.1f%% same-type neighbours).\n\n",
           final_seg * 100.0);

    if (final_seg > 0.7) {
        printf("This is a PHASE TRANSITION: the macro pattern cannot be\n");
        printf("predicted from the micro preference alone.  You MUST run\n");
        printf("the simulation to see the emergent outcome.\n");
        printf("This is the essence of the SFI generative methodology.\n");
    }

    return 0;
}
