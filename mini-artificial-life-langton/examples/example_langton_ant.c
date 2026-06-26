/**
 * example_langton_ant.c — Full demonstration of Langton's Ant
 *
 * Runs a single Langton ant simulation with position tracking
 * and highway detection. Demonstrates:
 *   - Grid creation and ant initialization
 *   - Multi-step simulation
 *   - Highway detection
 *   - Grid visualization output
 */
#include "langton_ant.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void)
{
    printf("=== Langton's Ant Simulation ===\n\n");

    lant_config_t cfg;
    cfg.grid_width = 512;
    cfg.grid_height = 512;
    cfg.max_steps = 15000;
    cfg.seed = 42;
    cfg.toroidal = false;
    cfg.auto_expand = true;
    cfg.track_history = true;

    lant_sim_t *sim = lant_sim_create(&cfg);
    if (!sim) {
        printf("ERROR: Failed to create simulation\n");
        return 1;
    }

    /* Place a single ant at grid center */
    lant_sim_add_ant(sim, 256, 256, LANT_DIR_NORTH);

    printf("Starting simulation with 1 ant at (256, 256) facing NORTH\n");
    printf("Maximum steps: %lld\n\n", (long long)cfg.max_steps);

    /* Run simulation in stages, printing progress */
    int stages[] = {100, 500, 1000, 2000, 5000, 10000, 15000};
    int n_stages = 7;

    for (int i = 0; i < n_stages; i++) {
        uint64_t target = (uint64_t)stages[i];
        if (sim->step_count < target) {
            uint64_t to_run = target - sim->step_count;
            uint64_t ran = lant_sim_run(sim, to_run);

            printf("Step %6llu: flips=%6llu, bbox=[%lld,%lld]x[%lld,%lld]",
                   (unsigned long long)sim->step_count,
                   (unsigned long long)sim->grid->total_flips,
                   (long long)sim->grid->min_x,
                   (long long)sim->grid->max_x,
                   (long long)sim->grid->min_y,
                   (long long)sim->grid->max_y);

            if (sim->highway_detected) {
                printf(" *** HIGHWAY DETECTED at step %llu! ***",
                       (unsigned long long)sim->highway_start_step);
                double angle = lant_highway_angle(sim);
                double vel = lant_highway_velocity(sim);
                printf(" angle=%.1fdeg, velocity=%.4f", angle, vel);
            }

            printf("\n");

            if (sim->highway_detected) break;
        }
    }

    printf("\n=== Final Statistics ===\n");
    printf("Total steps:       %llu\n", (unsigned long long)sim->step_count);
    printf("Total flips:       %llu\n", (unsigned long long)sim->grid->total_flips);
    printf("Grid size:         %lld x %lld\n",
           (long long)sim->grid->width, (long long)sim->grid->height);
    printf("Bounding box:      [%lld,%lld] x [%lld,%lld]\n",
           (long long)sim->grid->min_x, (long long)sim->grid->max_x,
           (long long)sim->grid->min_y, (long long)sim->grid->max_y);
    printf("Black cell count:  %llu\n",
           (unsigned long long)lant_grid_count_black(sim->grid));
    printf("Grid density:      %.6f\n", lant_grid_density(sim->grid));
    printf("Highway detected:  %s\n", sim->highway_detected ? "YES" : "no");

    if (sim->highway_detected) {
        printf("Highway start:     step %llu\n",
               (unsigned long long)sim->highway_start_step);
        printf("Highway angle:     %.1f degrees\n", lant_highway_angle(sim));
        printf("Highway velocity:  %.4f cells/step\n", lant_highway_velocity(sim));
    }

    if (sim->config.track_history && sim->history_len > 0) {
        uint64_t period = lant_find_highway_period(sim->x_history,
                                                     sim->y_history,
                                                     sim->history_len);
        printf("Estimated period:  %llu\n", (unsigned long long)period);
    }

    /* Save grid visualization */
    lant_grid_save_pgm(sim->grid, "langton_ant_output.pgm");
    printf("\nGrid visualization saved to langton_ant_output.pgm\n");

    lant_sim_destroy(sim);
    printf("\n=== Simulation Complete ===\n");
    return 0;
}
