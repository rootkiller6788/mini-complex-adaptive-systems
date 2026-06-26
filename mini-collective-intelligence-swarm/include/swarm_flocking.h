#ifndef SWARM_FLOCKING_H
#define SWARM_FLOCKING_H

#include "swarm_types.h"

/* ============================================================================
 * Flocking & Collective Motion
 *
 * Reynolds (1987): "Flocks, Herds, and Schools: A Distributed Behavioral Model"
 *   — The seminal boids model with three rules: separation, alignment, cohesion.
 *
 * Vicsek et al. (1995): "Novel Type of Phase Transition in a System of
 *   Self-Driven Particles" — Phase transition from disorder to ordered motion.
 *
 * Cucker & Smale (2007): "Emergent Behavior in Flocks" — Mathematical proof
 *   of convergence to common heading under hierarchical leadership.
 *
 * Couzin et al. (2005): "Effective Leadership and Decision-Making in Animal
 *   Groups" — How a small informed minority can guide group movement.
 *
 * Three rules (Reynolds):
 *   1. Separation: s_i = -Σ_{j∈N_i} (x_j - x_i) / d_ij²
 *      Steer to avoid crowding local flockmates.
 *   2. Alignment: a_i = (Σ_{j∈N_i} v_j / |N_i|) - v_i
 *      Steer toward the average heading of local flockmates.
 *   3. Cohesion: c_i = (Σ_{j∈N_i} x_j / |N_i|) - x_i
 *      Steer toward the average position of local flockmates.
 *
 * Total steering: F_i = w_s·s_i + w_a·a_i + w_c·c_i
 *
 * Vicsek Model (discrete-time alignment with noise):
 *   v_i(t+1) = v₀ · R_η[ Σ_{j∈N_i} v_j(t) / |Σ_{j∈N_i} v_j(t)| ]
 *   where R_η is a random rotation within angle η (noise).
 *   Order parameter: φ = |Σ_i v_i| / (N·v₀) ∈ [0,1]
 *   Phase transition at critical noise η_c.
 *
 * Cucker-Smale Model:
 *   dv_i/dt = Σ_j a_{ij} (v_j - v_i),  a_{ij} = K / (1 + ||x_i - x_j||²)^β
 *   Theorem: unconditional convergence if β < 1/2
 *            conditional convergence if β ≥ 1/2 (depends on initial data)
 * ============================================================================ */

/* ── Boid Initialization & Memory ── */
Boid*  swarm_boid_create(int id);
void   swarm_boid_free(Boid* boid);
void   swarm_boid_init_random(Boid* boid, int id, double world_size, 
                              double max_speed);
void   swarm_boid_init_grid(Boid* boid, int id, int grid_x, int grid_y,
                            double spacing, double max_speed);
Boid** swarm_flock_create(const FlockConfig* config);
void   swarm_flock_free(Boid** flock, int n_boids);

/* ── Spatial Hashing for Fast Neighbor Lookup ── */
typedef struct {
    int*   cell_boids;        /* Flat array: boid indices per cell              */
    int*   cell_counts;       /* Number of boids per cell                       */
    int*   cell_capacity;     /* Capacity per cell                              */
    int    grid_x;            /* Number of cells in x                           */
    int    grid_y;            /* Number of cells in y                           */
    int    grid_z;            /* Number of cells in z                           */
    double cell_size;         /* Size of each cell (≥ perception_radius)        */
    double world_size;        /* World half-extent                              */
} SpatialHash;

SpatialHash* swarm_spatial_hash_create(double world_size, double cell_size);
void         swarm_spatial_hash_free(SpatialHash* sh);
void         swarm_spatial_hash_clear(SpatialHash* sh);
void         swarm_spatial_hash_insert(SpatialHash* sh, const double pos[3], int boid_idx);
void         swarm_spatial_hash_query(const SpatialHash* sh, const double pos[3],
                   double radius, int* neighbors, int* n_neighbors, int max_neighbors,
                   const Boid* boids);

/* ── Reynolds Rules ── */
void swarm_flocking_separation(const Boid* boid, const Boid* flock, int n_boids,
           double separation_distance, double weight, double force[3]);
void swarm_flocking_alignment(const Boid* boid, const Boid* flock, int n_boids,
           double radius, double weight, double force[3]);
void swarm_flocking_cohesion(const Boid* boid, const Boid* flock, int n_boids,
           double radius, double weight, double force[3]);
void swarm_flocking_accumulate_forces(Boid* boid, const Boid* flock, int n_boids,
           const FlockConfig* config);
void swarm_flocking_boundary_force(Boid* boid, double world_size, double margin,
           double force[3], bool wrap_around);

/* ── Flocking Simulation Step ── */
void swarm_flock_update(Boid* flock, int n_boids, const FlockConfig* config);
void swarm_flock_update_with_spatial_hash(Boid* flock, int n_boids, 
           const FlockConfig* config, SpatialHash* sh);

/* ── Flocking Analysis ── */

/* Vicsek order parameter: φ = |Σv_i| / (N·v₀) ∈ [0, 1] */
double swarm_flock_order_parameter(const Boid* flock, int n_boids);

/* Group centroid */
void swarm_flock_centroid(const Boid* flock, int n_boids, double centroid[3]);

/* Group extent (max distance from centroid) */
double swarm_flock_extent(const Boid* flock, int n_boids);

/* Average distance to nearest neighbor */
double swarm_flock_nearest_neighbor_distance(const Boid* flock, int n_boids);

/* Detect number of subgroups (connected components by perception radius) */
int  swarm_flock_subgroup_count(const Boid* flock, int n_boids, double radius);

/* ── Predator-Prey Dynamics ── */
void swarm_flocking_predator_chase(Boid* predator, const Boid* flock, int n_boids,
           double chase_weight, double force[3]);
void swarm_flocking_prey_evade(const Boid* prey, const Boid* predator, 
           double evade_weight, double force[3]);

/* ── Leader-Follower Models ── */
void swarm_flocking_leader_goal(Boid* leader, const double goal[3], 
                                 double goal_weight, double force[3]);
void swarm_flocking_follow_leader(Boid* follower, const Boid* leader,
           double follow_weight, double force[3]);

/* ── Obstacle Avoidance ── */
typedef struct {
    double position[3];
    double radius;
} Obstacle;

void swarm_flocking_obstacle_avoidance(Boid* boid, const Obstacle* obstacles,
           int n_obstacles, double weight, double force[3]);

/* ── Migratory / Environmental Bias ── */
void swarm_flocking_wind(const Boid* flock, int n_boids, const double wind[3], 
                         double weight);

/* ── Vicsek Model ── */
void swarm_vicsek_init(Boid* flock, int n_boids, double world_size, 
                        double speed, double noise_eta);
void swarm_vicsek_update(Boid* flock, int n_boids, double speed, double radius,
                          double noise_eta, double world_size);
double swarm_vicsek_order_parameter(const Boid* flock, int n_boids, double speed);

/* ── Cucker-Smale Model ── */
void swarm_cucker_smale_init(Boid* flock, int n_boids, double world_size);
void swarm_cucker_smale_update(Boid* flock, int n_boids, double K, double beta,
                                double dt);
bool swarm_cucker_smale_has_converged(const Boid* flock, int n_boids, double epsilon);

#endif /* SWARM_FLOCKING_H */
