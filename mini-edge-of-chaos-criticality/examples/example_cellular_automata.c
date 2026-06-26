#include "eoc_core.h"
#include "eoc_cellular.h"
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * Example 3: Cellular Automata ? Wolfram Classes and Edge of Chaos
 *
 * Demonstrates the four Wolfram classes of 1D cellular automata and
 * Langton's lambda phase transition from order to chaos.
 *
 * Key concepts:
 *   - Wolfram Classes I-IV
 *   - Langton's lambda parameter as order-chaos phase transition
 *   - Mutual information peaks at the edge of chaos (lambda ~ 0.273)
 *   - Game of Life and Langton's Ant as examples of complex behavior
 *   - Wireworld and Brian's Brain as edge-of-chaos CAs
 * ============================================================================ */

static void print_rule_info(int rule, const char* desc) {
    double lambda = eoc_ca_lambda_1d(rule, 0);
    WolframClass wc = eoc_ca_classify_1d(rule, 32, 60);

    const char* class_name;
    switch (wc) {
        case WOLFRAM_CLASS_I:   class_name = "Class I (Fixed Point)"; break;
        case WOLFRAM_CLASS_II:  class_name = "Class II (Periodic)"; break;
        case WOLFRAM_CLASS_III: class_name = "Class III (Chaotic)"; break;
        case WOLFRAM_CLASS_IV:  class_name = "Class IV (Complex/Edge of Chaos)"; break;
        default:                class_name = "Unknown"; break;
    }

    printf("  Rule %3d (%s): lambda=%.3f, %s\n", rule, desc, lambda, class_name);
}

int main(void) {
    printf("=== Cellular Automata ? Edge of Chaos ===\n\n");

    /* Part 1: Wolfram Class Examples */
    printf("--- Wolfram Classification of 1D Rules ---\n");

    print_rule_info(0,   "all zeros      ");
    print_rule_info(40,  "class I variant ");
    print_rule_info(50,  "class II variant");
    print_rule_info(30,  "class III       ");
    print_rule_info(110, "Rule 110 (Turing-complete)");
    print_rule_info(54,  "amphichiral     ");
    print_rule_info(90,  "class III       ");
    print_rule_info(184, "traffic rule    ");

    /* Part 2: Langton's Lambda Phase Transition */
    printf("\n--- Langton Lambda Phase Transition ---\n");
    printf("(Measuring mutual information vs. lambda)\n");

    double* lambdas, *mi_avg;
    int n_points;
    eoc_langton_phase_transition(20, 10, 40, 30, &lambdas, &mi_avg, &n_points);

    printf("lambda   |  Mutual Information\n");
    printf("---------|--------------------\n");
    int shown = 0;
    for (int i = 0; i < n_points && shown < 10; i++) {
        if (mi_avg[i] > 0.001 || i % 3 == 0) {
            printf("  %.3f   |  %.6f", lambdas[i], mi_avg[i]);
            /* Mark the expected edge-of-chaos peak */
            if (fabs(lambdas[i] - 0.273) < 0.05) printf("  <-- Edge of Chaos");
            printf("\n");
            shown++;
        }
    }
    free(lambdas);
    free(mi_avg);

    /* Part 3: Classic CA Patterns */
    printf("\n--- Classic 2D Cellular Automata ---\n");

    /* Game of Life ? Blinker */
    printf("\nGame of Life ? Blinker (period 2 oscillator):\n");
    int w = 20, h = 10;
    int* gol = (int*)calloc(w * h, sizeof(int));
    int* gol_next = (int*)calloc(w * h, sizeof(int));

    /* Set up blinker */
    gol[5 * w + 9] = 1;
    gol[5 * w + 10] = 1;
    gol[5 * w + 11] = 1;

    for (int step = 0; step < 4; step++) {
        printf("  t=%d: ", step);
        for (int x = 7; x < 14; x++) printf("%c", gol[5 * w + x] ? 'O' : '.');
        printf("\n");
        eoc_game_of_life_step(gol, gol_next, w, h);
        int* tmp = gol; gol = gol_next; gol_next = tmp;
    }

    free(gol);
    free(gol_next);

    /* Langton's Ant */
    printf("\nLangton's Ant ? Emergent Highway Pattern:\n");
    int aw = 20, ah = 20;
    int* ant_grid = (int*)calloc(aw * ah, sizeof(int));
    int ax = 10, ay = 10, adir = 0;

    printf("  Initial: ant at (%d,%d) facing North\n", ax, ay);
    eoc_langtons_ant(ant_grid, aw, ah, &ax, &ay, &adir, 500);

    int black_count = 0;
    for (int i = 0; i < aw * ah; i++) if (ant_grid[i]) black_count++;
    printf("  After 500 steps: %d black cells, ant at (%d,%d)\n",
           black_count, ax, ay);

    /* Show grid */
    for (int y = 0; y < ah; y++) {
        printf("  ");
        for (int x = 0; x < aw; x++) {
            if (x == ax && y == ay) printf("@");
            else printf("%c", ant_grid[y * aw + x] ? '#' : '.');
        }
        printf("\n");
    }
    free(ant_grid);

    /* Part 4: CA Basin of Attraction Analysis (small width) */
    printf("\n--- Basin of Attraction Analysis (width=5) ---\n");
    int test_rules[] = {0, 40, 110, 30, 45};
    for (int i = 0; i < 5; i++) {
        int n_fixed, n_cycles;
        double avg_trans, basin_H;
        eoc_ca_basin_analysis(test_rules[i], 5, 100,
                              &n_fixed, &n_cycles, &avg_trans, &basin_H);
        printf("  Rule %3d: %d fixed, %d cycles, avg_trans=%.1f, basin_H=%.3f\n",
               test_rules[i], n_fixed, n_cycles, avg_trans, basin_H);
    }

    printf("\n=== Example Complete ===\n");
    return 0;
}
