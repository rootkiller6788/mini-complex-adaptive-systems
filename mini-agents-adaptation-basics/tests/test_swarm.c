#include "swarm.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("=== test_swarm ===\n");

    /* PSO test on Rastrigin */
    double lb[] = {-5.12, -5.12};
    double ub[] = {5.12, 5.12};
    PSO_Swarm *pso = pso_create(20, 2, lb, ub, 0.7, 1.5, 1.5);
    assert(pso != NULL);
    pso_random_init(pso);
    pso_run(pso, 50, swarm_test_rastrigin, NULL, false);
    double best_r = pso->global_best_value;
    printf("  PSO Rastrigin: best=%.4f\n", best_r);
    assert(best_r < 100.0); /* should improve from random */
    pso_destroy(pso);
    printf("  PSO: PASS\n");

    /* ACO test on small TSP */
    int n_cities = 5;
    double dists[] = {
        0, 2, 3, 4, 5,
        2, 0, 4, 5, 3,
        3, 4, 0, 1, 6,
        4, 5, 1, 0, 2,
        5, 3, 6, 2, 0
    };
    ACO_Colony *aco = aco_create(10, n_cities, dists, 1.0, 2.0, 0.1);
    assert(aco != NULL);
    aco_init_pheromone(aco, 1.0);
    aco_run(aco, 30, false);
    assert(aco->best_tour_length > 0.0);
    double lb_tsp = aco_lower_bound(aco);
    printf("  ACO TSP: best=%.4f, lower_bound=%.4f\n", aco->best_tour_length, lb_tsp);
    aco_destroy(aco);
    printf("  ACO: PASS\n");

    /* Flocking test */
    Flock *flock = flock_create(10, 2, 3.0, 1.0, 1.5, 1.0, 2.0);
    assert(flock != NULL);
    double fmin[] = {-10, -10}, fmax[] = {10, 10};
    flock_random_init(flock, fmin, fmax);
    double center[2];
    flock_center_of_mass(flock, center);
    double polar = flock_polarization(flock);
    printf("  Flock: center=(%.2f,%.2f), polarization=%.4f\n",
           center[0], center[1], polar);
    flock_run(flock, 5, 0.1);
    polar = flock_polarization(flock);
    printf("  Flock after 5 steps: polarization=%.4f\n", polar);
    flock_destroy(flock);
    printf("  Flocking: PASS\n");

    /* Firefly test */
    FireflySwarm *fs = firefly_create(15, 2, lb, ub, 0.2, 1.0, 1.0);
    assert(fs != NULL);
    firefly_random_init(fs);
    firefly_run(fs, 30, swarm_test_rosenbrock, NULL, false);
    const Firefly *best = firefly_get_best(fs);
    printf("  Firefly Rosenbrock: best=%.4f\n", best ? best->brightness : -1.0);
    firefly_destroy(fs);
    printf("  Firefly: PASS\n");

    /* Test functions */
    double x1[] = {0.0, 0.0};
    double rast = swarm_test_rastrigin(x1, 2, NULL);
    assert(fabs(rast) < 1e-9); /* global min at (0,0) */
    printf("  Rastrigin(0,0)=%.4f: PASS\n", rast);

    double x2[] = {1.0, 1.0};
    double rosen = swarm_test_rosenbrock(x2, 2, NULL);
    assert(fabs(rosen) < 1e-9);
    printf("  Rosenbrock(1,1)=%.4f: PASS\n", rosen);

    printf("=== test_swarm: ALL PASS ===\n");
    return 0;
}
