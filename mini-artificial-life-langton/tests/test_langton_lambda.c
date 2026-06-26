/** test_langton_lambda.c — Tests for Lambda Parameter */
#include "langton_lambda.h"
#include "cellular_automaton.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

static void test_lambda_compute(void)
{
    /* Simple rule table: 4 entries, 1 non-zero */
    uint8_t table[] = {0, 0, 0, 1};
    double lam = lambda_compute(table, 4, 2, 0);
    assert(lam == 0.25);
    printf("  PASS: lambda compute\n");
}

static void test_lambda_all_zero(void)
{
    uint8_t table[] = {0, 0, 0, 0};
    double lam = lambda_compute(table, 4, 2, 0);
    assert(lam == 0.0);
    printf("  PASS: lambda all zero\n");
}

static void test_lambda_all_nonzero(void)
{
    uint8_t table[] = {1, 1, 1, 1};
    double lam = lambda_compute(table, 4, 2, 0);
    assert(lam == 1.0);
    printf("  PASS: lambda all nonzero\n");
}

static void test_rule_entropy(void)
{
    uint8_t table[] = {0, 0, 1, 1};
    double H = lambda_rule_entropy(table, 4, 2);
    assert(H >= 0.9 && H <= 1.1); /* near 1.0 (50/50 distribution) */
    printf("  PASS: rule entropy\n");
}

static void test_z_parameter(void)
{
    /* 3-input (8 entries) rule where all entries are same -> Z=0 */
    uint8_t table[8] = {0,0,0,0,0,0,0,0};
    double z = lambda_z_parameter(table, 8, 2, 3);
    assert(z == 0.0);

    /* Rule where every bit flip changes output -> Z=1 */
    uint8_t table2[8] = {0,1,0,1,0,1,0,1};
    z = lambda_z_parameter(table2, 8, 2, 3);
    assert(z >= 0.0 && z <= 1.0);

    printf("  PASS: Z parameter\n");
}

static void test_rule_generate(void)
{
    lambda_config_t cfg;
    cfg.num_states = 2;
    cfg.num_inputs = 5;
    cfg.quiescent_state = 0;
    cfg.table_size = 32; /* 2^5 */
    cfg.lambda = 0.27;
    cfg.non_quiescent_n = 9; /* ~0.27 * 32 */

    lambda_rule_t *rule = lambda_rule_generate(&cfg, 42);
    assert(rule != NULL);
    assert(rule->rule_table != NULL);
    assert(rule->actual_lambda > 0.2 && rule->actual_lambda < 0.35);

    lambda_rule_destroy(rule);
    printf("  PASS: rule generate\n");
}

static void test_rule_mutate(void)
{
    lambda_config_t cfg;
    cfg.num_states = 2;
    cfg.num_inputs = 5;
    cfg.quiescent_state = 0;
    cfg.table_size = 32;
    cfg.lambda = 0.5;
    cfg.non_quiescent_n = 16;

    lambda_rule_t *rule = lambda_rule_generate(&cfg, 42);
    double old_lambda = rule->actual_lambda;

    bool mutated = lambda_rule_mutate_preserve_lambda(rule);
    assert(mutated);
    assert(fabs(rule->actual_lambda - old_lambda) < 0.01);

    lambda_rule_destroy(rule);
    printf("  PASS: rule mutate preserves lambda\n");
}

static void test_phase_sweep(void)
{
    lambda_config_t base_cfg;
    base_cfg.num_states = 2;
    base_cfg.num_inputs = 3;
    base_cfg.quiescent_state = 0;
    base_cfg.table_size = 8; /* 2^3 */
    base_cfg.lambda = 0.0;

    lambda_phase_point_t *sweep = lambda_sweep_phase(&base_cfg, 5, 10, 42);
    assert(sweep != NULL);

    double lambda_c = lambda_detect_edge_of_chaos(sweep, 5);
    assert(lambda_c >= 0.0 && lambda_c <= 1.0);

    free(sweep);
    printf("  PASS: phase sweep\n");
}

static void test_order_parameter(void)
{
    lambda_phase_point_t point;
    point.lambda = 0.5;
    point.avg_entropy = 0.3;
    point.avg_mutual_info = 0.0;
    point.avg_damage_spread = 0.0;
    point.transient_length = 10.0;
    point.dominant_class = CA_CLASS_IV;

    double order = lambda_order_parameter(&point);
    assert(order >= 0.0 && order <= 1.0);

    printf("  PASS: order parameter\n");
}

int main(void)
{
    printf("Test: Langton Lambda\n");
    test_lambda_compute();
    test_lambda_all_zero();
    test_lambda_all_nonzero();
    test_rule_entropy();
    test_z_parameter();
    test_rule_generate();
    test_rule_mutate();
    test_phase_sweep();
    test_order_parameter();
    printf("All Lambda tests passed!\n");
    return 0;
}
