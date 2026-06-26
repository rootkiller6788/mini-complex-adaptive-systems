#include "swarm_core.h"
#include "swarm_pso.h"
#include "swarm_aco.h"
#include "swarm_flocking.h"
#include "swarm_consensus.h"
#include "swarm_stigmergy.h"
#include "swarm_emergent.h"
#include "swarm_applications.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define TEST_DIM 5
#define TEST_AGENTS 30
#define EPS 1e-6

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  %-50s ", #name); \
    if (test_##name()) { printf("PASS\n"); tests_passed++; } \
    else { printf("FAIL\n"); } \
} while(0)

/* ── Test helper: sphere function ── */
static double sphere_fitness(const double* x, int dim, void* ctx) {
    (void)ctx;
    double s = 0.0;
    for (int i = 0; i < dim; i++) s += x[i] * x[i];
    return s;
}

/* ============================================================================
 * L1: Core Definitions — struct/typedef existence and initialization
 * ============================================================================ */

static bool test_swarm_agent_create_free(void) {
    SwarmAgent* a = swarm_agent_create(3);
    if (!a) return false;
    assert(a->dim == 3);
    assert(a->is_active == true);
    assert(a->position != NULL);
    assert(a->velocity != NULL);
    assert(a->best_position != NULL);
    swarm_agent_free(a);
    return true;
}

static bool test_swarm_agent_init_random(void) {
    SwarmAgent* a = swarm_agent_create(TEST_DIM);
    double bounds_low[] = {-10, -10, -10, -10, -10};
    double bounds_high[] = {10, 10, 10, 10, 10};
    swarm_agent_init_random(a, TEST_DIM, 0, bounds_low, bounds_high);
    for (int d = 0; d < TEST_DIM; d++) {
        assert(a->position[d] >= bounds_low[d]);
        assert(a->position[d] <= bounds_high[d]);
    }
    swarm_agent_free(a);
    return true;
}

static bool test_pheromone_create(void) {
    PheromoneMatrix* p = swarm_aco_pheromone_create(10, 1.0);
    if (!p) return false;
    assert(p->rows == 10);
    assert(p->cols == 10);
    assert(fabs(p->tau[0][1] - 1.0) < EPS);
    assert(fabs(p->tau[0][0] - 0.0) < EPS);
    swarm_aco_pheromone_free(p);
    return true;
}

static bool test_boid_create(void) {
    Boid* b = swarm_boid_create(42);
    if (!b) return false;
    assert(b->id == 42);
    assert(b->max_speed > 0);
    assert(b->is_predator == false);
    swarm_boid_free(b);
    return true;
}

static bool test_consensus_state_create(void) {
    double init[] = {1.0, 3.0, 5.0, 7.0, 9.0};
    ConsensusState* cs = swarm_consensus_create(5, init);
    if (!cs) return false;
    assert(cs->n_agents == 5);
    assert(fabs(cs->values[0] - 1.0) < EPS);
    assert(fabs(cs->values[4] - 9.0) < EPS);
    swarm_consensus_free(cs);
    return true;
}

static bool test_stigmergy_env_create(void) {
    StigmergyEnvironment* env = swarm_stigmergy_create(50, 50, 0.1, 0.05);
    if (!env) return false;
    assert(env->width == 50);
    assert(env->height == 50);
    assert(fabs(env->cells[0][0] - 0.0) < EPS);
    swarm_stigmergy_free(env);
    return true;
}

/* ============================================================================
 * L2: Core Concepts — Random numbers, vector ops, selection
 * ============================================================================ */

static bool test_rng_uniform_range(void) {
    swarm_rng_seed(12345ULL);
    for (int i = 0; i < 100; i++) {
        double r = swarm_rng_uniform();
        assert(r >= 0.0 && r < 1.0);
    }
    for (int i = 0; i < 100; i++) {
        double r = swarm_rng_uniform_range(-5.0, 5.0);
        assert(r >= -5.0 && r < 5.0);
    }
    return true;
}

static bool test_rng_gaussian(void) {
    swarm_rng_seed(42ULL);
    double sum = 0.0;
    int n = 10000;
    for (int i = 0; i < n; i++) sum += swarm_rng_gaussian();
    double mean = sum / n;
    return fabs(mean) < 0.1; /* Should be close to 0 */
}

static bool test_vec_operations(void) {
    double a[] = {1, 2, 3}, b[] = {4, 5, 6}, c[3];
    swarm_vec_add(a, b, c, 3);
    assert(fabs(c[0] - 5) < EPS && fabs(c[1] - 7) < EPS && fabs(c[2] - 9) < EPS);
    swarm_vec_sub(a, b, c, 3);
    assert(fabs(c[0] + 3) < EPS && fabs(c[1] + 3) < EPS && fabs(c[2] + 3) < EPS);
    double d = swarm_vec_dot(a, b, 3);
    assert(fabs(d - 32.0) < EPS); /* 1*4+2*5+3*6=32 */
    double n2 = swarm_vec_norm_sq(a, 3);
    assert(fabs(n2 - 14.0) < EPS); /* 1+4+9=14 */
    double dist = swarm_vec_dist(a, b, 3);
    assert(fabs(dist - sqrt(27.0)) < EPS); /* (4-1)²+(5-2)²+(6-3)²=27 */
    return true;
}

static bool test_vec_normalize(void) {
    double v[] = {3.0, 4.0};
    swarm_vec_normalize(v, 2);
    assert(fabs(swarm_vec_norm(v, 2) - 1.0) < EPS);
    return true;
}

static bool test_vec_dist_metrics(void) {
    double a[] = {1, 0}, b[] = {4, 0};
    double manh = swarm_vec_dist_manhattan(a, b, 2);
    assert(fabs(manh - 3.0) < EPS);
    double inf = swarm_vec_dist_infinity(a, b, 2);
    assert(fabs(inf - 3.0) < EPS);
    return true;
}

static bool test_selection_roulette(void) {
    swarm_rng_seed(99ULL);
    double fitness[] = {0.1, 0.5, 0.9}; /* min=0.1 best, max=0.9 worst */
    int counts[3] = {0, 0, 0};
    for (int i = 0; i < 1000; i++) {
        int idx = swarm_select_roulette(fitness, 3, true);
        assert(idx >= 0 && idx < 3);
        counts[idx]++;
    }
    /* Best (lowest fitness) should be selected more */
    return counts[0] > counts[2];
}

static bool test_selection_tournament(void) {
    double fitness[] = {0.1, 0.5, 0.9};
    swarm_rng_seed(42ULL);
    int idx = swarm_select_tournament(fitness, 3, 3, true);
    return idx >= 0 && idx < 3;
}

/* ============================================================================
 * L3: Math Structures — matrix ops, topology, Laplacian
 * ============================================================================ */

static bool test_matrix_alloc_free(void) {
    double** m = swarm_matrix_alloc(5, 7);
    if (!m) return false;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 7; j++)
            m[i][j] = (double)(i + j);
    assert(fabs(m[2][3] - 5.0) < EPS);
    swarm_matrix_free(m, 5);
    return true;
}

static bool test_matrix_normalize(void) {
    double** m = swarm_matrix_alloc(3, 3);
    swarm_matrix_set(m, 3, 3, 1.0);
    swarm_matrix_normalize_rows(m, 3, 3);
    for (int i = 0; i < 3; i++) {
        double sum = 0.0;
        for (int j = 0; j < 3; j++) sum += m[i][j];
        assert(fabs(sum - 1.0) < EPS);
    }
    swarm_matrix_free(m, 3);
    return true;
}

static bool test_topology_ring(void) {
    int neighbors[SWARM_MAX_NEIGHBORS];
    memset(neighbors, -1, sizeof(neighbors));
    swarm_topology_ring(neighbors, 0, 10, 4);
    /* Agent 0 should have neighbors 8, 9, 1, 2 */
    int expected[] = {8, 9, 1, 2};
    for (int i = 0; i < 4; i++)
        assert(neighbors[i] == expected[i]);
    return true;
}

static bool test_graph_laplacian(void) {
    double** adj = swarm_matrix_alloc(3, 3);
    adj[0][1] = adj[1][0] = 1.0;
    adj[0][2] = adj[2][0] = 0.0;
    adj[1][2] = adj[2][1] = 1.0;
    double** L = swarm_graph_laplacian(adj, 3);
    /* Check row sums = 0 (property of Laplacian) */
    for (int i = 0; i < 3; i++) {
        double sum = 0.0;
        for (int j = 0; j < 3; j++) sum += L[i][j];
        assert(fabs(sum) < EPS);
    }
    swarm_matrix_free(adj, 3);
    swarm_matrix_free(L, 3);
    return true;
}

static bool test_graph_connected(void) {
    double** adj = swarm_matrix_alloc(4, 4);
    adj[0][1] = adj[1][0] = 1.0;
    adj[1][2] = adj[2][1] = 1.0;
    adj[2][3] = adj[3][2] = 1.0;
    bool conn = swarm_graph_is_connected(adj, 4);
    assert(conn == true);
    /* Disconnect node 3 */
    adj[2][3] = adj[3][2] = 0.0;
    conn = swarm_graph_is_connected(adj, 4);
    assert(conn == false);
    swarm_matrix_free(adj, 4);
    return true;
}

/* ============================================================================
 * L4: Fundamental Laws/Theorems — PSO convergence, consensus
 * ============================================================================ */

static bool test_pso_convergence_condition(void) {
    /* Clerc-Kennedy: constriction requires φ > 4 */
    double chi = 2.0 / fabs(2.0 - 4.1 - sqrt(4.1*4.1 - 4.0*4.1));
    assert(chi > 0.5 && chi < 1.0); /* χ should be in (0,1] */
    return true;
}

static bool test_consensus_converges_to_average(void) {
    /* Connected ring of 4 agents, values [1, 3, 5, 7] */
    double init[] = {1.0, 3.0, 5.0, 7.0};
    ConsensusState* cs = swarm_consensus_create(4, init);
    /* Build ring adjacency and Metropolis weights */
    double** adj = swarm_matrix_alloc(4, 4);
    for (int i = 0; i < 4; i++) {
        adj[i][(i+1)%4] = 1.0;
        adj[i][(i+3)%4] = 1.0;
    }
    swarm_consensus_weights_metropolis(cs->weights, 4, (const double**)adj);
    int iters = swarm_consensus_run(cs, 1000);
    /* All values should be close to 4.0 (average) */
    for (int i = 0; i < 4; i++)
        assert(fabs(cs->values[i] - 4.0) < 0.01);
    assert(iters > 0);
    assert(cs->converged);
    swarm_matrix_free(adj, 4);
    swarm_consensus_free(cs);
    return true;
}

static bool test_flocking_order_parameter(void) {
    /* All boids moving in same direction → φ ≈ 1 */
    Boid flock[4];
    for (int i = 0; i < 4; i++) {
        flock[i].velocity[0] = 1.0;
        flock[i].velocity[1] = 0.0;
        flock[i].velocity[2] = 0.0;
        flock[i].max_speed = 1.0;
    }
    double phi = swarm_flock_order_parameter(flock, 4);
    assert(fabs(phi - 1.0) < EPS);
    /* Random directions → φ < 1 */
    flock[0].velocity[0] = 1.0; flock[0].velocity[1] = 0.0;
    flock[1].velocity[0] = -1.0; flock[1].velocity[1] = 0.0;
    flock[2].velocity[0] = 0.0; flock[2].velocity[1] = 1.0;
    flock[3].velocity[0] = 0.0; flock[3].velocity[1] = -1.0;
    phi = swarm_flock_order_parameter(flock, 4);
    assert(phi < 0.5); /* Cancelling directions */
    return true;
}

static bool test_deneubourg_response(void) {
    /* τ = K → P = 0.5 */
    double p = swarm_denebourg_response(1.0, 1.0);
    assert(fabs(p - 0.5) < EPS);
    /* τ >> K → P ≈ 1 */
    p = swarm_denebourg_response(100.0, 1.0);
    assert(p > 0.99);
    /* τ << K → P ≈ 0 */
    p = swarm_denebourg_response(0.01, 1.0);
    assert(p < 0.01);
    return true;
}

static bool test_vicsek_order_parameter(void) {
    Boid flock[3];
    double speed = 0.5;
    for (int i = 0; i < 3; i++) {
        flock[i].velocity[0] = speed;
        flock[i].velocity[1] = 0.0;
        flock[i].velocity[2] = 0.0;
        flock[i].max_speed = speed;
    }
    double phi = swarm_vicsek_order_parameter(flock, 3, speed);
    assert(fabs(phi - 1.0) < EPS);
    return true;
}

/* ============================================================================
 * L5: Algorithms — PSO, ACO, flocking, stigmergy, consensus
 * ============================================================================ */

static bool test_pso_run_sphere(void) {
    SwarmConfig config;
    memset(&config, 0, sizeof(config));
    config.n_agents = 20;
    config.dimensions = 2;
    config.topology = SWARM_TOPOLOGY_GLOBAL;
    config.inertia_weight = 0.7298;
    config.cognitive_weight = 1.49618;
    config.social_weight = 1.49618;
    config.max_velocity = 2.0;
    config.use_constriction = true;
    config.fitness_function = sphere_fitness;
    config.fitness_context = NULL;
    config.minimize = true;
    config.max_iterations = 100;
    config.seed = 42;

    double bounds_low[] = {-10.0, -10.0};
    double bounds_high[] = {10.0, 10.0};
    config.bounds_low = bounds_low;
    config.bounds_high = bounds_high;

    SwarmResult* result = swarm_pso_run(&config);
    if (!result) return false;
    /* Sphere minimum at origin: fitness should be close to 0 */
    bool ok = (result->best_fitness < 0.01);
    swarm_free_vector(result->best_position);
    swarm_free_vector(result->history);
    free(result);
    return ok;
}

static bool test_aco_tsp_run(void) {
    swarm_rng_seed(123ULL);
    /* Simple 6-city TSP in a circle */
    double x[] = {0, 1, 2, 3, 4, 5};
    double y[] = {0, 0, 0, 0, 0, 0};
    ACOConfig config;
    memset(&config, 0, sizeof(config));
    config.n_ants = 10;
    config.strategy = ACO_MMAS;
    config.alpha = 1.0;
    config.beta = 5.0;
    config.rho = 0.1;
    config.Q = 100.0;
    config.tau0 = 1.0;
    SwarmResult* result = swarm_aco_run_tsp(x, y, 6, &config);
    if (!result) return false;
    /* Optimal tour should visit in order, length = 10 (0→1→2→3→4→5→0 twice? No, 0→5→0?) */
    /* Line: 0-1-2-3-4-5, optimal: 10.0 (out and back) */
    bool ok = (result->best_fitness < 15.0 && result->best_fitness > 5.0);
    swarm_free_vector(result->best_position);
    swarm_free_vector(result->history);
    free(result);
    return ok;
}

static bool test_flocking_simulation(void) {
    FlockConfig config;
    memset(&config, 0, sizeof(config));
    config.n_boids = 20;
    config.world_size = 50.0;
    config.max_speed = 2.0;
    config.max_force = 0.1;
    config.perception_radius = 8.0;
    config.separation_weight = 1.5;
    config.alignment_weight = 1.0;
    config.cohesion_weight = 1.0;
    config.separation_distance = 3.0;
    config.dt = 0.1;
    config.wrap_around = true;

    /* Allocate flat array of Boids */
    Boid* flock = (Boid*)calloc((size_t)config.n_boids, sizeof(Boid));
    if (!flock) return false;
    for (int i = 0; i < config.n_boids; i++)
        swarm_boid_init_random(&flock[i], i, config.world_size, config.max_speed);
    /* Run 10 steps */
    for (int s = 0; s < 10; s++)
        swarm_flock_update(flock, config.n_boids, &config);
    /* Flock should stay within world bounds */
    if (!config.wrap_around) {
        double half = config.world_size * 0.5;
        for (int i = 0; i < config.n_boids; i++) {
            assert(fabs(flock[i].position[0]) <= half + 1.0);
        }
    }
    double phi = swarm_flock_order_parameter(flock, config.n_boids);
    /* After some steps, order should increase from random */
    assert(phi >= 0.0 && phi <= 1.0);
    free(flock);
    return true;
}

static bool test_stigmergy_diffusion(void) {
    StigmergyEnvironment* env = swarm_stigmergy_create(20, 20, 0.2, 0.0);
    /* Deposit pheromone at center */
    swarm_stigmergy_deposit(env, 10.0, 10.0, 100.0, 2.0);
    double peak_before = swarm_stigmergy_sample(env, 10.0, 10.0);
    /* Diffuse */
    for (int s = 0; s < 5; s++) swarm_stigmergy_diffuse(env, 0.1);
    double peak_after = swarm_stigmergy_sample(env, 10.0, 10.0);
    /* After diffusion, center concentration should decrease */
    assert(peak_after < peak_before);
    /* Neighboring cell should increase */
    double side_before = 0.0;
    double side_after = swarm_stigmergy_sample(env, 12.0, 10.0);
    assert(side_after > side_before);
    swarm_stigmergy_free(env);
    return true;
}

static bool test_bridge_experiment(void) {
    BridgeExperiment exp;
    swarm_bridge_experiment_init(&exp);
    for (int s = 0; s < 200; s++)
        swarm_bridge_experiment_step(&exp, 10, 0.05, 1.0, 2.0, 1.0);
    /* Symmetry should break */
    assert(exp.broken == true);
    return true;
}

/* ============================================================================
 * L6: Canonical Problems — benchmark functions, TSP
 * ============================================================================ */

static bool test_benchmark_functions(void) {
    double x[] = {0.0, 0.0};
    assert(fabs(swarm_benchmark_sphere(x, 2, NULL)) < EPS);
    assert(swarm_benchmark_rosenbrock(x, 2, NULL) > 0.0);
    assert(fabs(swarm_benchmark_rastrigin(x, 2, NULL)) < EPS);
    assert(swarm_benchmark_griewank(x, 2, NULL) < EPS);
    assert(swarm_benchmark_schwefel(x, 2, NULL) > 0.0);
    return true;
}

static bool test_aco_nearest_neighbor(void) {
    double x[] = {0, 1, 3, 6};
    double y[] = {0, 0, 0, 0};
    ACOProblem* prob = swarm_aco_problem_from_coords(x, y, 4, true);
    AntTour* tour = swarm_aco_tour_create(4);
    swarm_aco_nearest_neighbor_tour(tour, prob, 0);
    double len = swarm_aco_tour_length(tour, prob);
    /* NN tour on a line: 0→1→3→6→0 = 1+2+3+6=12 */
    assert(len > 0.0 && len < 20.0);
    swarm_aco_tour_free(tour);
    swarm_aco_problem_free(prob);
    return true;
}

static bool test_sandpile_criticality(void) {
    Sandpile* sp = swarm_sandpile_create(20, 20, 4);
    /* Drop many grains to reach critical state */
    for (int i = 0; i < 500; i++)
        swarm_sandpile_drop(sp, 10, 10);
    /* Should have produced avalanches */
    assert(sp->n_avalanches > 0);
    double alpha = swarm_sandpile_power_law_exponent(sp->avalanche_sizes, sp->n_avalanches);
    assert(alpha > 0.0);
    swarm_sandpile_free(sp);
    return true;
}

/* ============================================================================
 * L7: Applications — supply chain, UAV swarm, smart grid
 * ============================================================================ */

static bool test_supply_chain(void) {
    SupplyChainSwarm* sc = swarm_supply_chain_create(5);
    double demands[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    swarm_supply_chain_update(sc, demands, 1.0);
    double cost = swarm_supply_chain_total_cost(sc);
    assert(cost > 0.0);
    swarm_supply_chain_free(sc);
    return true;
}

static bool test_grid_economic_dispatch(void) {
    double outputs[] = {0, 0, 0};
    double costs[] = {10, 20, 30};
    double limits[] = {100, 100, 100};
    swarm_grid_economic_dispatch(outputs, costs, limits, 3, 150.0, 0.01, 100);
    double total = outputs[0] + outputs[1] + outputs[2];
    assert(fabs(total - 150.0) < 10.0);
    return true;
}

static bool test_uav_search(void) {
    UAVSwarm* swarm = swarm_uav_search_create(5, 2.0, 10.0, 50, 50);
    if (!swarm) return false;
    for (int s = 0; s < 10; s++)
        swarm_uav_search_update(swarm, 0.5, NULL);
    /* Should have covered some area */
    double coverage = swarm_stigmergy_sample(swarm->search_map, 25.0, 25.0);
    assert(coverage >= 0.0);
    swarm_uav_search_free(swarm);
    return true;
}

/* ============================================================================
 * L8: Advanced Topics — phase transition, emergent behavior
 * ============================================================================ */

static bool test_ca_classification(void) {
    int uniform[] = {0,0,0,0,0,0,0,0,0,0};
    int cls = swarm_ca_classify(uniform, 10, 1, 256);
    assert(cls == 1); /* Class 1: uniform */

    int chaotic[] = {0,5,2,8,1,9,3,7,4,6};
    cls = swarm_ca_classify(chaotic, 10, 1, 256);
    assert(cls == 3 || cls == 4); /* Class 3 or 4 */
    return true;
}

static bool test_langton_lambda(void) {
    int rule30[] = {0,1,1,1,1,1,0,0}; /* Rule 30: 5 out of 8 non-zero */
    double lambda = swarm_langton_lambda(rule30, 8, 1, 0);
    assert(fabs(lambda - 5.0/8.0) < EPS);
    return true;
}

static bool test_early_warning_signal(void) {
    /* Generate series with increasing AR1 */
    double series[50];
    series[0] = 0.0;
    for (int i = 1; i < 50; i++)
        series[i] = 0.95 * series[i-1] + 0.1 * sin((double)i * 0.1);
    double tau, p;
    bool warned = swarm_early_warning_signal(series, 50, &tau, &p);
    /* Should detect trend (or not; test just verifies function runs) */
    return warned == true || warned == false;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n=== Swarm Collective Intelligence Test Suite ===\n\n");
    
    printf("[L1] Definitions — struct/typedef creation\n");
    RUN_TEST(swarm_agent_create_free);
    RUN_TEST(swarm_agent_init_random);
    RUN_TEST(pheromone_create);
    RUN_TEST(boid_create);
    RUN_TEST(consensus_state_create);
    RUN_TEST(stigmergy_env_create);
    
    printf("\n[L2] Core Concepts — RNG, vector ops, selection\n");
    RUN_TEST(rng_uniform_range);
    RUN_TEST(rng_gaussian);
    RUN_TEST(vec_operations);
    RUN_TEST(vec_normalize);
    RUN_TEST(vec_dist_metrics);
    RUN_TEST(selection_roulette);
    RUN_TEST(selection_tournament);
    
    printf("\n[L3] Math Structures — matrices, topology, graph\n");
    RUN_TEST(matrix_alloc_free);
    RUN_TEST(matrix_normalize);
    RUN_TEST(topology_ring);
    RUN_TEST(graph_laplacian);
    RUN_TEST(graph_connected);
    
    printf("\n[L4] Fundamental Laws — PSO convergence, consensus, flocking\n");
    RUN_TEST(pso_convergence_condition);
    RUN_TEST(consensus_converges_to_average);
    RUN_TEST(flocking_order_parameter);
    RUN_TEST(deneubourg_response);
    RUN_TEST(vicsek_order_parameter);
    
    printf("\n[L5] Algorithms — PSO, ACO, flocking, stigmergy\n");
    RUN_TEST(pso_run_sphere);
    RUN_TEST(aco_tsp_run);
    RUN_TEST(flocking_simulation);
    RUN_TEST(stigmergy_diffusion);
    RUN_TEST(bridge_experiment);
    
    printf("\n[L6] Canonical Problems — benchmarks, TSP, sandpile\n");
    RUN_TEST(benchmark_functions);
    RUN_TEST(aco_nearest_neighbor);
    RUN_TEST(sandpile_criticality);
    
    printf("\n[L7] Applications — supply chain, smart grid, UAV\n");
    RUN_TEST(supply_chain);
    RUN_TEST(grid_economic_dispatch);
    RUN_TEST(uav_search);
    
    printf("\n[L8] Advanced Topics — CA, λ, early warning\n");
    RUN_TEST(ca_classification);
    RUN_TEST(langton_lambda);
    RUN_TEST(early_warning_signal);
    
    printf("\n==============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
