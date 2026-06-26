#include "swarm_flocking.h"
#include "swarm_core.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Example 3: Reynolds Flocking Simulation.
 * Simulates 50 boids in a bounded world with separation, alignment, and cohesion.
 * Reports order parameter, group centroid, extent, and nearest-neighbor distance
 * over time, demonstrating emergent collective motion from local rules.
 */

int main(void) {
    printf("\n=== Reynolds Flocking Simulation ===\n\n");
    swarm_rng_seed(99);
    
    FlockConfig config;
    memset(&config, 0, sizeof(config));
    config.n_boids = 50;
    config.world_size = 100.0;
    config.max_speed = 3.0;
    config.max_force = 0.15;
    config.perception_radius = 10.0;
    config.separation_weight = 1.8;
    config.alignment_weight = 1.0;
    config.cohesion_weight = 0.8;
    config.separation_distance = 4.0;
    config.dt = 0.05;
    config.wrap_around = true;
    
    Boid** flock = swarm_flock_create(&config);
    if (!flock) {
        printf("Failed to create flock.\n");
        return 1;
    }
    
    printf("Initial state (t=0):\n");
    printf("  Order parameter: %.4f\n", swarm_flock_order_parameter(flock, config.n_boids));
    printf("  Extent: %.2f\n", swarm_flock_extent(flock, config.n_boids));
    printf("  NN distance: %.2f\n", swarm_flock_nearest_neighbor_distance(flock, config.n_boids));
    
    /* Simulate 200 steps */
    printf("\nSimulating 200 steps...\n");
    printf("%-6s %-12s %-10s %-10s %-10s\n", "Step", "Order_φ", "Extent", "NN_Dist", "Subgroups");
    printf("------ ---------- ---------- ---------- ----------\n");
    
    for (int step = 0; step <= 200; step++) {
        if (step % 20 == 0) {
            double phi = swarm_flock_order_parameter(flock, config.n_boids);
            double extent = swarm_flock_extent(flock, config.n_boids);
            double nn_dist = swarm_flock_nearest_neighbor_distance(flock, config.n_boids);
            int subgroups = swarm_flock_subgroup_count(flock, config.n_boids, config.perception_radius);
            printf("%-6d %-12.4f %-10.2f %-10.2f %-10d\n",
                step, phi, extent, nn_dist, subgroups);
        }
        if (step < 200)
            swarm_flock_update(flock, config.n_boids, &config);
    }
    
    /* Centroid position */
    double centroid[3];
    swarm_flock_centroid(flock, config.n_boids, centroid);
    printf("\nFinal centroid: (%.2f, %.2f, %.2f)\n",
        centroid[0], centroid[1], centroid[2]);
    
    swarm_flock_free(flock, config.n_boids);
    
    printf("\nFlocking simulation complete.\n");
    printf("The order parameter φ → 1 indicates alignment convergence.\n");
    printf("Subgroup count decreasing indicates merging into a single flock.\n");
    
    return 0;
}
