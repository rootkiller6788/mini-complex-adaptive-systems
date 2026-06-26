/**
 * @file    example_sugarscape_simulation.c
 * @brief   Sugarscape agent-based model (Epstein & Axtell 1996)
 *
 * Full Sugarscape ABM with heterogeneous agents, resource diffusion,
 * regrowth, agent vision, metabolism, reproduction, and death.
 * Demonstrates emergent wealth inequality (Gini) from purely local rules.
 *
 * Knowledge: L1 (definitions), L2 (emergence), L5 (ABM loop), L6 (canonical)
 */

#include "../include/sfi_macrosystems.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define AGENT_VISION      2
#define AGENT_METABOLISM  3.0
#define INITIAL_AGENTS    50
#define GRID_WIDTH        50
#define GRID_HEIGHT       50
#define MAX_TICKS         100

static void print_grid_slice(sfi_environment_t *env, sfi_population_t *pop) {
    printf("  Resource grid (10x10 top-left slice):\n");
    for (int y = 0; y < 10; y++) {
        printf("  ");
        for (int x = 0; x < 10; x++) {
            uint64_t idx = (uint64_t)y * (uint64_t)env->width + (uint64_t)x;
            char c = '.';
            double r = env->resources[idx];
            if (r > 75.0) c = '#';
            else if (r > 50.0) c = 'O';
            else if (r > 25.0) c = 'o';
            else if (r > 0.0) c = '.';
            else c = ' ';
            int has_agent = 0;
            for (uint64_t a = 0; a < pop->size; a++) {
                if (pop->agents[a].alive && pop->agents[a].x == x && pop->agents[a].y == y) {
                    c = 'A'; has_agent = 1; break;
                }
            }
            printf("%c", c);
        }
        printf("\n");
    }
}

int main(void) {
    printf("=== SFI Sugarscape Simulation ===\n");
    printf("Epstein & Axtell (1996) -- Growing Artificial Societies\n");
    printf("Grid: %dx%d, Initial agents: %d, Ticks: %d\n\n",
           GRID_WIDTH, GRID_HEIGHT, INITIAL_AGENTS, MAX_TICKS);

    srand(42);

    sfi_environment_t env;
    if (sfi_environment_init(&env, GRID_WIDTH, GRID_HEIGHT, 0.05, 100.0) != 0) {
        fprintf(stderr, "Failed to initialise environment\n");
        return 1;
    }

    /* Create two rich sugar mountains */
    for (int y = 10; y < 20; y++)
        for (int x = 10; x < 20; x++) {
            uint64_t idx = (uint64_t)y * (uint64_t)env.width + (uint64_t)x;
            env.resources[idx] = 90.0 + (rand() % 10);
        }
    for (int y = 30; y < 40; y++)
        for (int x = 30; x < 40; x++) {
            uint64_t idx = (uint64_t)y * (uint64_t)env.width + (uint64_t)x;
            env.resources[idx] = 85.0 + (rand() % 15);
        }
    env.total_resource = 0.0;
    for (uint64_t i = 0; i < (uint64_t)GRID_WIDTH * GRID_HEIGHT; i++)
        env.total_resource += env.resources[i];

    sfi_population_t pop;
    if (sfi_population_init(&pop, INITIAL_AGENTS * 2, SFI_TOPO_2D_GRID_TORUS, 0.01) != 0) {
        fprintf(stderr, "Failed to initialise population\n");
        sfi_environment_destroy(&env);
        return 1;
    }

    for (int i = 0; i < INITIAL_AGENTS; i++) {
        int x = rand() % GRID_WIDTH;
        int y = rand() % GRID_HEIGHT;
        sfi_population_add_agent(&pop, x, y, 50.0 + (rand() % 51));
    }

    printf("Initial state:\n");
    print_grid_slice(&env, &pop);
    printf("  Agents: %llu, Total resource: %.1f\n\n",
           (unsigned long long)pop.size, env.total_resource);

    for (int tick = 1; tick <= MAX_TICKS; tick++) {
        env.tick = tick;

        for (uint64_t a = 0; a < pop.size; a++) {
            if (!pop.agents[a].alive) continue;
            sfi_agent_t *ag = &pop.agents[a];

            int best_x = ag->x, best_y = ag->y;
            double best_resource = 0.0;
            for (int dy = -AGENT_VISION; dy <= AGENT_VISION; dy++) {
                for (int dx = -AGENT_VISION; dx <= AGENT_VISION; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = ag->x + dx;
                    int ny = ag->y + dy;
                    if (env.torus) {
                        nx = (nx % env.width + env.width) % env.width;
                        ny = (ny % env.height + env.height) % env.height;
                    } else {
                        if (nx < 0 || nx >= env.width || ny < 0 || ny >= env.height)
                            continue;
                    }
                    uint64_t idx = (uint64_t)ny * (uint64_t)env.width + (uint64_t)nx;
                    if (env.resources[idx] > best_resource) {
                        best_resource = env.resources[idx];
                        best_x = nx; best_y = ny;
                    }
                }
            }

            if (best_resource > 0.0) { ag->x = best_x; ag->y = best_y; }
            double harvested = sfi_environment_harvest(&env, ag->x, ag->y,
                AGENT_METABOLISM * 2.0);
            ag->energy += harvested;
            ag->energy -= AGENT_METABOLISM;
            ag->age += 1.0;
            if (ag->energy <= 0.0) { ag->alive = 0; env.total_deaths++; }
        }

        sfi_environment_diffuse(&env, 0.1);
        sfi_environment_regrow(&env);

        for (uint64_t a = 0; a < pop.size; a++) {
            if (!pop.agents[a].alive) continue;
            if (pop.agents[a].energy > 100.0 && pop.size < pop.capacity) {
                pop.agents[a].energy -= 50.0;
                int64_t child = sfi_population_add_agent(&pop,
                    pop.agents[a].x, pop.agents[a].y, 50.0);
                if (child >= 0) {
                    pop.agents[child].parent_id = pop.agents[a].id;
                    pop.agents[child].generation = pop.agents[a].generation + 1;
                    pop.agents[child].genotype[0] = pop.agents[a].genotype[0]
                        ^ ((uint32_t)rand());
                    env.total_births++;
                }
            }
        }

        sfi_population_compact_dead(&pop);

        if (tick % 20 == 0 || tick == 1) {
            sfi_macrostate_t ms;
            sfi_compute_macrostate(&pop, &env, &ms);
            printf("Tick %3d: Pop=%5.0f  MeanWealth=%6.1f  Gini=%5.3f  "
                   "Births=%5llu  Deaths=%5llu\n",
                   tick, ms.population, ms.mean_wealth, ms.gini_index,
                   (unsigned long long)env.total_births,
                   (unsigned long long)env.total_deaths);
        }
    }

    printf("\nFinal state (tick %d):\n", MAX_TICKS);
    print_grid_slice(&env, &pop);

    sfi_macrostate_t ms;
    sfi_compute_macrostate(&pop, &env, &ms);
    printf("\n=== Final Macrostate ===\n");
    printf("  Population:          %.0f\n", ms.population);
    printf("  Mean wealth:         %.2f\n", ms.mean_wealth);
    printf("  Gini coefficient:    %.4f\n", ms.gini_index);
    printf("  Mean age:            %.1f\n", ms.mean_age);
    printf("  Strategy diversity:  %.4f\n", ms.strategy_diversity);
    printf("  Total births:        %llu\n", (unsigned long long)env.total_births);
    printf("  Total deaths:        %llu\n", (unsigned long long)env.total_deaths);

    printf("\n=== SFI Methodology Insight ===\n");
    printf("The Gini coefficient (%.4f) emerges from purely local\n", ms.gini_index);
    printf("agent interactions -- no central planner, no global redistribution.\n");
    printf("This is weak emergence: the pattern is computable only by\n");
    printf("simulating all agent-level rules (Bedau 1997).\n");

    sfi_population_destroy(&pop);
    sfi_environment_destroy(&env);
    printf("\nSimulation complete.\n");
    return 0;
}
