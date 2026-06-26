/**
 * @file    test_sfi_macrosystems.c
 * @brief   Tests for SFI macro-level CAS engine
 *
 * Tests: population initialisation, agent addition,
 * dead agent compaction, environment initialisation,
 * diffusion, regrowth, harvesting, and macrostate computation.
 */

#include "../include/sfi_macrosystems.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

#define TEST_PASS() printf("  PASS: %s\n", __func__)

void test_population_init_destroy(void) {
    sfi_population_t pop;
    int rc = sfi_population_init(&pop, 100, SFI_TOPO_FULL_MIXING, 0.01);
    assert(rc == 0);
    assert(pop.capacity == 100);
    assert(pop.size == 0);
    assert(pop.topology == SFI_TOPO_FULL_MIXING);
    assert(pop.mutation_rate == 0.01);
    assert(pop.agents != NULL);

    sfi_population_destroy(&pop);
    assert(pop.agents == NULL);
    TEST_PASS();
}

void test_population_null(void) {
    assert(sfi_population_init(NULL, 100, SFI_TOPO_FULL_MIXING, 0.01) == -1);
    sfi_population_t pop;
    assert(sfi_population_init(&pop, 0, SFI_TOPO_FULL_MIXING, 0.01) == -1);
    sfi_population_destroy(NULL);
    TEST_PASS();
}

void test_add_agent(void) {
    sfi_population_t pop;
    sfi_population_init(&pop, 10, SFI_TOPO_FULL_MIXING, 0.01);
    int64_t idx = sfi_population_add_agent(&pop, 5, 10, 100.0);
    assert(idx == 0);
    assert(pop.size == 1);
    assert(pop.agents[0].id == 0);
    assert(pop.agents[0].x == 5);
    assert(pop.agents[0].y == 10);
    assert(pop.agents[0].energy == 100.0);
    assert(pop.agents[0].alive == 1);

    /* Add more agents */
    for (int i = 0; i < 9; i++) {
        idx = sfi_population_add_agent(&pop, i, i, i * 10.0);
        assert(idx == i + 1);
    }
    assert(pop.size == 10);
    /* Overflow */
    idx = sfi_population_add_agent(&pop, 0, 0, 0.0);
    assert(idx == -2);

    sfi_population_destroy(&pop);
    TEST_PASS();
}

void test_compact_dead(void) {
    sfi_population_t pop;
    sfi_population_init(&pop, 10, SFI_TOPO_FULL_MIXING, 0.01);
    for (int i = 0; i < 5; i++)
        sfi_population_add_agent(&pop, i, i, i * 10.0);

    /* Kill agents 1 and 3 */
    pop.agents[1].alive = 0;
    pop.agents[3].alive = 0;

    uint64_t dead = sfi_population_compact_dead(&pop);
    assert(dead == 2);
    assert(pop.size == 3);

    sfi_population_destroy(&pop);
    TEST_PASS();
}

void test_environment_init(void) {
    sfi_environment_t env;
    int rc = sfi_environment_init(&env, 10, 10, 0.1, 100.0);
    assert(rc == 0);
    assert(env.width == 10);
    assert(env.height == 10);
    assert(env.resources != NULL);
    assert(env.diffusion_buf != NULL);
    assert(env.torus == 1);
    assert(env.tick == 0);

    assert(sfi_environment_init(NULL, 10, 10, 0.1, 100.0) == -1);
    assert(sfi_environment_init(&env, 0, 10, 0.1, 100.0) == -1);

    sfi_environment_destroy(&env);
    TEST_PASS();
}

void test_environment_diffuse(void) {
    sfi_environment_t env;
    sfi_environment_init(&env, 4, 4, 0.0, 10.0);

    /* Set central cell high, rest zero */
    uint64_t cells = (uint64_t)env.width * (uint64_t)env.height;
    for (uint64_t i = 0; i < cells; i++) env.resources[i] = 0.0;
    env.resources[2 * 4 + 2] = 10.0;  /* Center (2,2) */

    sfi_environment_diffuse(&env, 0.5);
    /* Center should have decreased, neighbours increased */
    assert(env.resources[2 * 4 + 2] < 10.0);
    /* At least some resources moved */
    int moved = 0;
    for (uint64_t i = 0; i < cells; i++) {
        if (i != (uint64_t)(2 * 4 + 2) && env.resources[i] > 0.0)
            moved = 1;
    }
    assert(moved == 1);

    sfi_environment_destroy(&env);
    TEST_PASS();
}

void test_environment_regrow(void) {
    sfi_environment_t env;
    sfi_environment_init(&env, 4, 4, 0.5, 100.0);

    /* Deplete all resources */
    uint64_t cells = 16;
    for (uint64_t i = 0; i < cells; i++) env.resources[i] = 0.0;
    env.total_resource = 0.0;

    sfi_environment_regrow(&env);
    /* Each cell should have regrowth_rate * max_resource */
    for (uint64_t i = 0; i < cells; i++) {
        assert(env.resources[i] <= 100.0);
        assert(env.resources[i] > 0.0);
    }
    assert(env.total_resource > 0.0);

    sfi_environment_destroy(&env);
    TEST_PASS();
}

void test_environment_harvest(void) {
    sfi_environment_t env;
    sfi_environment_init(&env, 5, 5, 0.0, 100.0);

    env.resources[0] = 50.0;
    double got = sfi_environment_harvest(&env, 0, 0, 30.0);
    assert(fabs(got - 30.0) < 0.001);
    assert(fabs(env.resources[0] - 20.0) < 0.001);

    /* Try to harvest more than available */
    got = sfi_environment_harvest(&env, 0, 0, 100.0);
    assert(fabs(got - 20.0) < 0.001);
    assert(fabs(env.resources[0]) < 0.001);

    /* Out of bounds */
    got = sfi_environment_harvest(&env, -1, 0, 10.0);
    assert(fabs(got) < 0.001);

    sfi_environment_destroy(&env);
    TEST_PASS();
}

void test_topology_full_mixing(void) {
    sfi_population_t pop;
    sfi_population_init(&pop, 5, SFI_TOPO_FULL_MIXING, 0.01);
    for (int i = 0; i < 5; i++)
        sfi_population_add_agent(&pop, 0, 0, 100.0);

    int rc = sfi_topology_generate_full_mixing(&pop);
    assert(rc == 0);
    /* With 5 nodes, complete graph has 10 edges (undirected, stored once) */
    assert(pop.edge_count == 10);
    /* In directed edge storage (edges from i to j>i), the total degree sum equals 2*edges.
       With N=5: degrees are {4,3,2,1,0}, sum=10. */
    uint64_t sum_deg = 0;
    for (uint64_t i = 0; i < 5; i++)
        sum_deg += pop.degree[i];
    assert(sum_deg == 10);

    sfi_population_destroy(&pop);
    TEST_PASS();
}

void test_topology_small_world(void) {
    sfi_population_t pop;
    sfi_population_init(&pop, 20, SFI_TOPO_SMALL_WORLD, 0.01);
    for (int i = 0; i < 20; i++)
        sfi_population_add_agent(&pop, i, 0, 100.0);

    int rc = sfi_topology_generate_small_world(&pop, 4, 0.1);
    assert(rc == 0);
    assert(pop.edge_count > 0);
    /* Each node should have some connections */
    for (uint64_t i = 0; i < pop.size; i++)
        assert(pop.degree[i] > 0);

    sfi_population_destroy(&pop);
    TEST_PASS();
}

void test_macrostate_computation(void) {
    sfi_population_t pop;
    sfi_population_init(&pop, 5, SFI_TOPO_FULL_MIXING, 0.01);
    for (int i = 0; i < 5; i++)
        sfi_population_add_agent(&pop, i, i, (i + 1) * 10.0);

    sfi_topology_generate_full_mixing(&pop);

    sfi_environment_t env;
    sfi_environment_init(&env, 10, 10, 0.1, 100.0);

    sfi_macrostate_t ms;
    sfi_compute_macrostate(&pop, &env, &ms);

    assert(ms.population == 5.0);
    /* Mean wealth = (10+20+30+40+50)/5 = 30 */
    assert(fabs(ms.mean_wealth - 30.0) < 0.001);
    /* Gini index calculated */
    assert(ms.gini_index >= 0.0);

    sfi_population_destroy(&pop);
    sfi_environment_destroy(&env);
    TEST_PASS();
}

int main(void) {
    printf("=== SFI Macrosystems Tests ===\n");
    test_population_init_destroy();
    test_population_null();
    test_add_agent();
    test_compact_dead();
    test_environment_init();
    test_environment_diffuse();
    test_environment_regrow();
    test_environment_harvest();
    test_topology_full_mixing();
    test_topology_small_world();
    test_macrostate_computation();
    printf("\nAll macrosystems tests passed!\n");
    return 0;
}
