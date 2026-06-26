/**
 * test_langton_ant.c — Tests for Langton's Ant simulation engine
 */
#include "langton_ant.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

static void test_grid_create_destroy(void)
{
    langton_grid_t *g = lant_grid_create(100, 100);
    assert(g != NULL);
    assert(g->width == 100);
    assert(g->height == 100);
    assert(lant_grid_count_black(g) == 0);
    lant_grid_destroy(g);
    printf("  PASS: grid create/destroy\n");
}

static void test_grid_set_get(void)
{
    langton_grid_t *g = lant_grid_create(50, 50);
    assert(g != NULL);

    lant_grid_set(g, 10, 10, LANT_COLOR_BLACK);
    assert(lant_grid_get(g, 10, 10, false) == LANT_COLOR_BLACK);
    assert(lant_grid_get(g, 0, 0, false) == LANT_COLOR_WHITE);

    lant_grid_destroy(g);
    printf("  PASS: grid set/get\n");
}

static void test_grid_flip(void)
{
    langton_grid_t *g = lant_grid_create(50, 50);
    assert(g != NULL);

    lant_color_t old = lant_grid_flip(g, 5, 5);
    assert(old == LANT_COLOR_WHITE);
    assert(lant_grid_get(g, 5, 5, false) == LANT_COLOR_BLACK);
    assert(g->total_flips == 1);

    old = lant_grid_flip(g, 5, 5);
    assert(old == LANT_COLOR_BLACK);
    assert(lant_grid_get(g, 5, 5, false) == LANT_COLOR_WHITE);
    assert(g->total_flips == 2);

    lant_grid_destroy(g);
    printf("  PASS: grid flip\n");
}

static void test_grid_count(void)
{
    /* lant_grid_create clamps to minimum 64x64 */
    langton_grid_t *g = lant_grid_create(64, 64);
    assert(g != NULL);

    /* Set 10x10 = 100 cells in top-left corner (all within bounds) */
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            lant_grid_set(g, x, y, LANT_COLOR_BLACK);
        }
    }
    assert(lant_grid_count_black(g) == 100);

    double rho = lant_grid_density(g);
    assert(rho > 0.02 && rho < 0.03); /* 100/(64*64) = 0.0244 */

    lant_grid_destroy(g);
    printf("  PASS: grid count/density\n");
}

static void test_grid_clone(void)
{
    langton_grid_t *g = lant_grid_create(64, 64);
    lant_grid_set(g, 5, 5, LANT_COLOR_BLACK);
    lant_grid_set(g, 10, 10, LANT_COLOR_BLACK);

    langton_grid_t *clone = lant_grid_clone(g);
    assert(clone != NULL);
    assert(clone->width == 64);
    assert(clone->height == 64);
    assert(lant_grid_get(clone, 5, 5, false) == LANT_COLOR_BLACK);
    assert(lant_grid_get(clone, 10, 10, false) == LANT_COLOR_BLACK);
    assert(lant_grid_count_black(clone) == 2);

    lant_grid_destroy(g);
    lant_grid_destroy(clone);
    printf("  PASS: grid clone\n");
}

static void test_grid_expand(void)
{
    langton_grid_t *g = lant_grid_create(64, 64);
    lant_grid_set(g, 0, 0, LANT_COLOR_BLACK);

    bool expanded = lant_grid_expand(g, 128, 128);
    assert(expanded);
    assert(g->width == 128);
    assert(g->height == 128);
    assert(lant_grid_get(g, 0, 0, false) == LANT_COLOR_BLACK);
    assert(lant_grid_get(g, 100, 100, false) == LANT_COLOR_WHITE);

    lant_grid_destroy(g);
    printf("  PASS: grid expand\n");
}

static void test_ant_create(void)
{
    langton_ant_t ant = lant_ant_create(50, 50, LANT_DIR_NORTH);
    assert(ant.x == 50);
    assert(ant.y == 50);
    assert(ant.dir == LANT_DIR_NORTH);
    assert(ant.steps == 0);
    printf("  PASS: ant create\n");
}

static void test_ant_turn(void)
{
    langton_ant_t ant = lant_ant_create(0, 0, LANT_DIR_NORTH);

    lant_ant_turn(&ant, true); /* right */
    assert(ant.dir == LANT_DIR_EAST);
    lant_ant_turn(&ant, true);
    assert(ant.dir == LANT_DIR_SOUTH);
    lant_ant_turn(&ant, false); /* left */
    assert(ant.dir == LANT_DIR_EAST);
    lant_ant_turn(&ant, false);
    assert(ant.dir == LANT_DIR_NORTH);

    printf("  PASS: ant turn\n");
}

static void test_ant_move_forward(void)
{
    langton_ant_t ant = lant_ant_create(10, 10, LANT_DIR_NORTH);
    int64_t nx, ny;

    lant_ant_move_forward(&ant, &nx, &ny);
    assert(nx == 10 && ny == 9);

    ant.dir = LANT_DIR_EAST;
    lant_ant_move_forward(&ant, &nx, &ny);
    assert(nx == 11 && ny == 10);

    printf("  PASS: ant move forward\n");
}

static void test_ant_step(void)
{
    langton_grid_t *g = lant_grid_create(100, 100);
    langton_ant_t ant = lant_ant_create(50, 50, LANT_DIR_NORTH);

    /* Step 1: on white cell, turn right, flip, move */
    lant_color_t old = lant_ant_step(&ant, g, false);
    assert(old == LANT_COLOR_WHITE);
    assert(ant.steps == 1);
    assert(lant_grid_get(g, 50, 50, false) == LANT_COLOR_BLACK);
    /* After white->right(N->E)->forward: ant at (51, 50) facing E */
    assert(ant.x == 51 && ant.y == 50);
    assert(ant.dir == LANT_DIR_EAST);

    /* Step 2: on white cell again (moved to new cell) */
    old = lant_ant_step(&ant, g, false);
    assert(old == LANT_COLOR_WHITE);
    assert(ant.steps == 2);

    lant_grid_destroy(g);
    printf("  PASS: ant step\n");
}

static void test_sim_create_destroy(void)
{
    lant_config_t cfg = {0};
    cfg.grid_width = 64;
    cfg.grid_height = 64;
    cfg.max_steps = 100000;
    cfg.track_history = true;

    lant_sim_t *sim = lant_sim_create(&cfg);
    assert(sim != NULL);
    assert(sim->grid != NULL);
    assert(sim->step_count == 0);

    lant_sim_destroy(sim);
    printf("  PASS: sim create/destroy\n");
}

static void test_sim_add_ant(void)
{
    lant_config_t cfg = {0};
    cfg.grid_width = 100;
    cfg.grid_height = 100;
    cfg.track_history = false;

    lant_sim_t *sim = lant_sim_create(&cfg);
    assert(sim != NULL);

    int idx = lant_sim_add_ant(sim, 50, 50, LANT_DIR_NORTH);
    assert(idx == 0);
    assert(sim->num_ants == 1);

    lant_sim_destroy(sim);
    printf("  PASS: sim add ant\n");
}

static void test_sim_run_steps(void)
{
    lant_config_t cfg = {0};
    cfg.grid_width = 256;
    cfg.grid_height = 256;
    cfg.max_steps = 10000;
    cfg.toroidal = true;
    cfg.track_history = false;

    lant_sim_t *sim = lant_sim_create(&cfg);
    lant_sim_add_ant(sim, 128, 128, LANT_DIR_NORTH);

    uint64_t n = lant_sim_run(sim, 100);
    assert(n == 100);
    assert(sim->step_count == 100);
    assert(sim->grid->total_flips == 100);

    lant_sim_destroy(sim);
    printf("  PASS: sim run steps\n");
}

static void test_highway_detect_no_false_positive(void)
{
    lant_config_t cfg = {0};
    cfg.grid_width = 256;
    cfg.grid_height = 256;
    cfg.toroidal = true;
    cfg.track_history = true;

    lant_sim_t *sim = lant_sim_create(&cfg);
    lant_sim_add_ant(sim, 128, 128, LANT_DIR_NORTH);

    /* Run only 500 steps — too early for highway */
    lant_sim_run(sim, 500);
    assert(!sim->highway_detected);

    lant_sim_destroy(sim);
    printf("  PASS: no false positive highway detect\n");
}

static void test_colony_radius(void)
{
    lant_config_t cfg = {0};
    cfg.grid_width = 200;
    cfg.grid_height = 200;
    cfg.toroidal = true;

    lant_sim_t *sim = lant_sim_create(&cfg);
    lant_sim_add_ant(sim, 100, 100, LANT_DIR_NORTH);
    lant_sim_add_ant(sim, 100, 100, LANT_DIR_EAST);

    double r = lant_colony_radius(sim);
    assert(r >= 0.0);

    lant_sim_destroy(sim);
    printf("  PASS: colony radius\n");
}

int main(void)
{
    printf("Test: Langton Ant\n");
    test_grid_create_destroy();
    test_grid_set_get();
    test_grid_flip();
    test_grid_count();
    test_grid_clone();
    test_grid_expand();
    test_ant_create();
    test_ant_turn();
    test_ant_move_forward();
    test_ant_step();
    test_sim_create_destroy();
    test_sim_add_ant();
    test_sim_run_steps();
    test_highway_detect_no_false_positive();
    test_colony_radius();
    printf("All Langton Ant tests passed!\n");
    return 0;
}
