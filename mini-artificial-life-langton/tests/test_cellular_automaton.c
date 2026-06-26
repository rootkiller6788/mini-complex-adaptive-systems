/** test_cellular_automaton.c — Tests for CA engine */
#include "cellular_automaton.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

static void test_config_default(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_1D, 100, 1);
    assert(cfg.dim == CA_DIM_1D);
    assert(cfg.width == 100);
    assert(cfg.height == 1);
    assert(cfg.num_states == 2);
    assert(cfg.boundary == CA_BOUNDARY_PERIODIC);
    printf("  PASS: config default\n");
}

static void test_elementary_rule_create(void)
{
    /* Rule 30: 00011110 = 30 */
    ca_rule_elementary_t r30 = ca_rule_elementary_create(30);
    assert(r30.rule_number == 30);
    /* Rule 30: bits[0] = 0 (000->0), bits[1] = 1 (001->1), ... */
    assert(r30.rule_bits[0] == 0); /* 000 -> 0 */
    assert(r30.rule_bits[7] == 0); /* 111 -> 0 */
    printf("  PASS: elementary rule create\n");
}

static void test_totalistic_rule_create(void)
{
    /* Game of Life: B3/S23 */
    ca_rule_totalistic_t gol = ca_rule_totalistic_create(2, "3", "23");
    assert(gol.num_states == 2);
    assert(gol.birth[3] == 1);
    assert(gol.survival[2] == 1);
    assert(gol.survival[3] == 1);
    free(gol.birth);
    free(gol.survival);
    printf("  PASS: totalistic rule create\n");
}

static void test_state_create_destroy(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_1D, 64, 1);
    ca_state_t *s = ca_state_create(&cfg);
    assert(s != NULL);
    assert(s->width == 64);
    assert(s->height == 1);
    assert(s->generation == 0);
    ca_state_destroy(s);
    printf("  PASS: state create/destroy\n");
}

static void test_state_set_get(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_2D, 50, 50);
    cfg.boundary = CA_BOUNDARY_FIXED;
    ca_state_t *s = ca_state_create(&cfg);
    assert(s != NULL);

    ca_state_set(s, 10, 10, 1);
    assert(ca_state_get(s, 10, 10) == 1);
    assert(ca_state_get(s, 0, 0) == 0);

    /* Fixed boundary: out of bounds -> 0 */
    assert(ca_state_get(s, -1, 0) == 0);
    assert(ca_state_get(s, 100, 0) == 0);

    ca_state_destroy(s);
    printf("  PASS: state set/get\n");
}

static void test_periodic_boundary(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_2D, 10, 10);
    cfg.boundary = CA_BOUNDARY_PERIODIC;
    ca_state_t *s = ca_state_create(&cfg);

    ca_state_set(s, 0, 0, 1);
    /* Periodic: (10, 0) maps to (0, 0) -> 1 */
    assert(ca_state_get(s, 10, 0) == 1);
    /* (-1, 0) wraps to (width-1, 0) = (9, 0) -> 0 */
    assert(ca_state_get(s, -1, 0) == 0);

    ca_state_destroy(s);
    printf("  PASS: periodic boundary\n");
}

static void test_neighbor_sum_2d(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_2D, 10, 10);
    ca_state_t *s = ca_state_create(&cfg);

    /* Set all Moore neighbors around (5,5) to 1 */
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            ca_state_set(s, 5 + dx, 5 + dy, 1);
        }
    }

    int sum = ca_neighbor_sum_2d(s, 5, 5, CA_NEIGH_MOORE);
    assert(sum == 8);

    ca_state_destroy(s);
    printf("  PASS: neighbor sum\n");
}

static void test_gol_rule(void)
{
    /* Set up a blinker: 3 horizontal cells */
    ca_config_t cfg = ca_config_default(CA_DIM_2D, 5, 5);
    cfg.boundary = CA_BOUNDARY_FIXED;
    ca_rule_totalistic_t gol_rule = ca_rule_totalistic_create(2, "3", "23");
    ca_config_set_totalistic(&cfg, &gol_rule);
    ca_state_t *s = ca_state_create(&cfg);

    /* Horizontal blinker at center */
    ca_state_set(s, 1, 2, 1);
    ca_state_set(s, 2, 2, 1);
    ca_state_set(s, 3, 2, 1);

    /* Evolve one step: should become vertical blinker */
    ca_evolve_2d_outer_totalistic(s);

    /* After one step with outer-totalistic: check center cell */
    /* Center cell (2,2) had 2 live neighbors -> survives */
    assert(ca_state_get(s, 2, 2) == 1);

    ca_state_destroy(s);
    free(gol_rule.birth);
    free(gol_rule.survival);
    ca_config_free(&cfg);
    printf("  PASS: Game of Life rule\n");
}

static void test_spatial_entropy(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_1D, 100, 1);
    ca_state_t *s = ca_state_create(&cfg);
    ca_state_randomize(s, 42);

    double H = ca_spatial_entropy(s);
    assert(H >= 0.0);
    assert(H <= 1.0);

    /* All zeros -> zero entropy */
    ca_state_fill(s, 0);
    H = ca_spatial_entropy(s);
    assert(H < 0.01);

    ca_state_destroy(s);
    printf("  PASS: spatial entropy\n");
}

static void test_lambda_parameter(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_1D, 100, 1);
    ca_rule_elementary_t r110 = ca_rule_elementary_create(110);
    ca_config_set_elementary(&cfg, &r110);

    double lam = ca_lambda_parameter(&cfg);
    assert(lam >= 0.0);
    assert(lam <= 1.0);

    ca_config_free(&cfg);
    printf("  PASS: lambda parameter\n");
}

static void test_wolfram_classify(void)
{
    ca_config_t cfg = ca_config_default(CA_DIM_1D, 100, 1);
    ca_rule_elementary_t r90 = ca_rule_elementary_create(90);
    ca_config_set_elementary(&cfg, &r90);

    ca_state_t *s = ca_state_create(&cfg);
    ca_state_point_init(s, 50, 0);

    ca_wolfram_class_t cls = ca_classify_wolfram_1d(s, 100);
    assert(cls >= CA_CLASS_I && cls <= CA_CLASS_UNKNOWN);

    ca_state_destroy(s);
    ca_config_free(&cfg);
    printf("  PASS: wolfram classify\n");
}

int main(void)
{
    printf("Test: Cellular Automaton\n");
    test_config_default();
    test_elementary_rule_create();
    test_totalistic_rule_create();
    test_state_create_destroy();
    test_state_set_get();
    test_periodic_boundary();
    test_neighbor_sum_2d();
    test_gol_rule();
    test_spatial_entropy();
    test_lambda_parameter();
    test_wolfram_classify();
    printf("All CA tests passed!\n");
    return 0;
}
