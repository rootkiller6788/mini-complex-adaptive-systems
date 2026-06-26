#include "swarm_flocking.h"
#include "swarm_core.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Reynolds Flocking (Boids) — Core Implementation
 *
 * References:
 *   Reynolds (1987)        — Original boids model
 *   Vicsek et al. (1995)   — Self-driven particles, phase transition
 *   Cucker & Smale (2007)  — Convergence theory for flocking
 *   Couzin et al. (2005)   — Effective leadership in groups
 * ============================================================================ */

/* ── Boid Management ── */
Boid* swarm_boid_create(int id) {
    Boid* b = (Boid*)calloc(1, sizeof(Boid));
    if (!b) return NULL;
    b->id = id;
    b->max_speed = 1.0;
    b->max_force = 0.1;
    b->perception_radius = 5.0;
    b->separation_weight = 1.5;
    b->alignment_weight = 1.0;
    b->cohesion_weight = 1.0;
    return b;
}

void swarm_boid_free(Boid* boid) { free(boid); }

void swarm_boid_init_random(Boid* boid, int id, double world_size, double max_speed) {
    if (!boid) return;
    boid->id = id;
    for (int d = 0; d < 3; d++) {
        boid->position[d] = swarm_rng_uniform_range(-world_size * 0.5, world_size * 0.5);
        boid->velocity[d] = swarm_rng_uniform_range(-max_speed, max_speed);
        boid->acceleration[d] = 0.0;
    }
    boid->max_speed = max_speed;
    boid->is_predator = false;
    boid->is_leader = false;
}

void swarm_boid_init_grid(Boid* boid, int id, int grid_x, int grid_y,
                           double spacing, double max_speed) {
    if (!boid) return;
    boid->id = id;
    int row = id / grid_x, col = id % grid_x;
    boid->position[0] = (col - grid_x / 2.0) * spacing;
    boid->position[1] = (row - grid_y / 2.0) * spacing;
    boid->position[2] = 0.0;
    boid->velocity[0] = swarm_rng_uniform_range(-1, 1);
    boid->velocity[1] = swarm_rng_uniform_range(-1, 1);
    boid->velocity[2] = 0.0;
    boid->acceleration[0] = boid->acceleration[1] = boid->acceleration[2] = 0.0;
    boid->max_speed = max_speed;
}

Boid** swarm_flock_create(const FlockConfig* config) {
    if (!config || config->n_boids <= 0) return NULL;
    Boid** flock = (Boid**)calloc((size_t)config->n_boids, sizeof(Boid*));
    if (!flock) return NULL;
    for (int i = 0; i < config->n_boids; i++) {
        flock[i] = swarm_boid_create(i);
        if (!flock[i]) {
            for (int j = 0; j < i; j++) swarm_boid_free(flock[j]);
            free(flock); return NULL;
        }
        swarm_boid_init_random(flock[i], i, config->world_size, config->max_speed);
        flock[i]->perception_radius = config->perception_radius;
        flock[i]->max_speed = config->max_speed;
        flock[i]->max_force = config->max_force;
        flock[i]->separation_weight = config->separation_weight;
        flock[i]->alignment_weight = config->alignment_weight;
        flock[i]->cohesion_weight = config->cohesion_weight;
    }
    return flock;
}

void swarm_flock_free(Boid** flock, int n_boids) {
    if (!flock) return;
    for (int i = 0; i < n_boids; i++) swarm_boid_free(flock[i]);
    free(flock);
}

/* ── Spatial Hash ── */
SpatialHash* swarm_spatial_hash_create(double world_size, double cell_size) {
    SpatialHash* sh = (SpatialHash*)calloc(1, sizeof(SpatialHash));
    if (!sh) return NULL;
    if (cell_size <= 0.0) cell_size = world_size * 0.1;
    sh->cell_size = cell_size;
    sh->world_size = world_size;
    int cells_per_dim = (int)ceil(world_size / cell_size) + 1;
    sh->grid_x = sh->grid_y = sh->grid_z = cells_per_dim;
    int total_cells = cells_per_dim * cells_per_dim * cells_per_dim;
    sh->cell_counts = (int*)calloc((size_t)total_cells, sizeof(int));
    sh->cell_capacity = (int*)calloc((size_t)total_cells, sizeof(int));
    sh->cell_boids = (int*)malloc((size_t)(total_cells * 8) * sizeof(int));
    if (!sh->cell_counts || !sh->cell_capacity || !sh->cell_boids) {
        swarm_spatial_hash_free(sh); return NULL;
    }
    for (int i = 0; i < total_cells; i++) sh->cell_capacity[i] = 8;
    return sh;
}

void swarm_spatial_hash_free(SpatialHash* sh) {
    if (!sh) return;
    free(sh->cell_counts); free(sh->cell_capacity); free(sh->cell_boids);
    free(sh);
}

void swarm_spatial_hash_clear(SpatialHash* sh) {
    if (!sh) return;
    int total = sh->grid_x * sh->grid_y * sh->grid_z;
    memset(sh->cell_counts, 0, (size_t)total * sizeof(int));
}

static int spatial_hash_cell(const SpatialHash* sh, const double pos[3], int axis) {
    double half = sh->world_size * 0.5;
    int idx = (int)floor((pos[axis] + half) / sh->cell_size);
    int max_cells = (axis == 0) ? sh->grid_x : (axis == 1) ? sh->grid_y : sh->grid_z;
    if (idx < 0) idx = 0; if (idx >= max_cells) idx = max_cells - 1;
    return idx;
}

void swarm_spatial_hash_insert(SpatialHash* sh, const double pos[3], int boid_idx) {
    if (!sh || !pos) return;
    int cx = spatial_hash_cell(sh, pos, 0);
    int cy = spatial_hash_cell(sh, pos, 1);
    int cz = spatial_hash_cell(sh, pos, 2);
    int cell_idx = (cz * sh->grid_y + cy) * sh->grid_x + cx;
    int total_cells = sh->grid_x * sh->grid_y * sh->grid_z;
    if (cell_idx < 0 || cell_idx >= total_cells) return;
    int count = sh->cell_counts[cell_idx];
    if (count >= sh->cell_capacity[cell_idx]) return; /* Cell full */
    int offset = cell_idx * sh->cell_capacity[0]; /* simplified */
    sh->cell_boids[offset + count] = boid_idx;
    sh->cell_counts[cell_idx]++;
}

void swarm_spatial_hash_query(const SpatialHash* sh, const double pos[3],
           double radius, int* neighbors, int* n_neighbors, int max_neighbors,
           const Boid* boids) {
    if (!sh || !pos || !neighbors || !n_neighbors) return;
    *n_neighbors = 0;
    int cx = spatial_hash_cell(sh, pos, 0);
    int cy = spatial_hash_cell(sh, pos, 1);
    int cz = spatial_hash_cell(sh, pos, 2);
    int search_radius = (int)ceil(radius / sh->cell_size);
    double rsq = radius * radius;
    int total_cells = sh->grid_x * sh->grid_y * sh->grid_z;
    
    for (int dz = -search_radius; dz <= search_radius; dz++) {
        for (int dy = -search_radius; dy <= search_radius; dy++) {
            for (int dx = -search_radius; dx <= search_radius; dx++) {
                int nx = cx + dx, ny = cy + dy, nz = cz + dz;
                if (nx < 0 || nx >= sh->grid_x || ny < 0 || ny >= sh->grid_y || nz < 0 || nz >= sh->grid_z)
                    continue;
                int cell_idx = (nz * sh->grid_y + ny) * sh->grid_x + nx;
                if (cell_idx < 0 || cell_idx >= total_cells) continue;
                int count = sh->cell_counts[cell_idx];
                int offset = cell_idx * sh->cell_capacity[0];
                for (int i = 0; i < count && *n_neighbors < max_neighbors; i++) {
                    int b_idx = sh->cell_boids[offset + i];
                    const double* bpos = boids[b_idx].position;
                    double dsq = 0.0;
                    for (int d = 0; d < 3; d++) { double diff = pos[d] - bpos[d]; dsq += diff * diff; }
                    if (dsq <= rsq && dsq > 0.0) {
                        neighbors[(*n_neighbors)++] = b_idx;
                    }
                }
            }
        }
    }
}

/* ── Separation Rule ── */
void swarm_flocking_separation(const Boid* boid, const Boid* flock, int n_boids,
           double separation_distance, double weight, double force[3]) {
    if (!boid || !flock) return;
    force[0] = force[1] = force[2] = 0.0;
    double sep_sq = separation_distance * separation_distance;
    int count = 0;
    for (int i = 0; i < n_boids; i++) {
        if (flock[i].id == boid->id) continue;
        double dx = boid->position[0] - flock[i].position[0];
        double dy = boid->position[1] - flock[i].position[1];
        double dz = boid->position[2] - flock[i].position[2];
        double dsq = dx * dx + dy * dy + dz * dz;
        if (dsq < sep_sq && dsq > SWARM_EPSILON) {
            double inv_d = 1.0 / sqrt(dsq);
            force[0] += dx * inv_d;
            force[1] += dy * inv_d;
            force[2] += dz * inv_d;
            count++;
        }
    }
    if (count > 0) {
        double inv_count = 1.0 / (double)count;
        force[0] *= inv_count; force[1] *= inv_count; force[2] *= inv_count;
        /* Steer toward this direction */
        double mag = sqrt(force[0]*force[0] + force[1]*force[1] + force[2]*force[2]);
        if (mag > SWARM_EPSILON) {
            force[0] = (force[0] / mag) * weight;
            force[1] = (force[1] / mag) * weight;
            force[2] = (force[2] / mag) * weight;
        }
    }
}

/* ── Alignment Rule ── */
void swarm_flocking_alignment(const Boid* boid, const Boid* flock, int n_boids,
           double radius, double weight, double force[3]) {
    if (!boid || !flock) return;
    double avg_vel[3] = {0.0, 0.0, 0.0};
    double rsq = radius * radius;
    int count = 0;
    for (int i = 0; i < n_boids; i++) {
        if (flock[i].id == boid->id) continue;
        double dx = boid->position[0] - flock[i].position[0];
        double dy = boid->position[1] - flock[i].position[1];
        double dz = boid->position[2] - flock[i].position[2];
        double dsq = dx * dx + dy * dy + dz * dz;
        if (dsq < rsq) {
            avg_vel[0] += flock[i].velocity[0];
            avg_vel[1] += flock[i].velocity[1];
            avg_vel[2] += flock[i].velocity[2];
            count++;
        }
    }
    if (count > 0) {
        double inv = 1.0 / (double)count;
        avg_vel[0] *= inv; avg_vel[1] *= inv; avg_vel[2] *= inv;
        force[0] = (avg_vel[0] - boid->velocity[0]) * weight;
        force[1] = (avg_vel[1] - boid->velocity[1]) * weight;
        force[2] = (avg_vel[2] - boid->velocity[2]) * weight;
    } else {
        force[0] = force[1] = force[2] = 0.0;
    }
}

/* ── Cohesion Rule ── */
void swarm_flocking_cohesion(const Boid* boid, const Boid* flock, int n_boids,
           double radius, double weight, double force[3]) {
    if (!boid || !flock) return;
    double avg_pos[3] = {0.0, 0.0, 0.0};
    double rsq = radius * radius;
    int count = 0;
    for (int i = 0; i < n_boids; i++) {
        if (flock[i].id == boid->id) continue;
        double dx = boid->position[0] - flock[i].position[0];
        double dy = boid->position[1] - flock[i].position[1];
        double dz = boid->position[2] - flock[i].position[2];
        double dsq = dx * dx + dy * dy + dz * dz;
        if (dsq < rsq) {
            avg_pos[0] += flock[i].position[0];
            avg_pos[1] += flock[i].position[1];
            avg_pos[2] += flock[i].position[2];
            count++;
        }
    }
    if (count > 0) {
        double inv = 1.0 / (double)count;
        avg_pos[0] *= inv; avg_pos[1] *= inv; avg_pos[2] *= inv;
        force[0] = (avg_pos[0] - boid->position[0]) * weight;
        force[1] = (avg_pos[1] - boid->position[1]) * weight;
        force[2] = (avg_pos[2] - boid->position[2]) * weight;
    } else {
        force[0] = force[1] = force[2] = 0.0;
    }
}

/* ── Accumulate Forces ── */
void swarm_flocking_accumulate_forces(Boid* boid, const Boid* flock, int n_boids,
           const FlockConfig* config) {
    if (!boid || !flock || !config) return;
    double sep[3], ali[3], coh[3];
    swarm_flocking_separation(boid, flock, n_boids, config->separation_distance,
                               config->separation_weight, sep);
    swarm_flocking_alignment(boid, flock, n_boids, config->perception_radius,
                              config->alignment_weight, ali);
    swarm_flocking_cohesion(boid, flock, n_boids, config->perception_radius,
                             config->cohesion_weight, coh);
    boid->acceleration[0] = sep[0] + ali[0] + coh[0];
    boid->acceleration[1] = sep[1] + ali[1] + coh[1];
    boid->acceleration[2] = sep[2] + ali[2] + coh[2];
}

/* ── Boundary Force ── */
void swarm_flocking_boundary_force(Boid* boid, double world_size, double margin,
                                    double force[3], bool wrap_around) {
    if (!boid) return;
    double half = world_size * 0.5;
    force[0] = force[1] = force[2] = 0.0;
    for (int d = 0; d < 3; d++) {
        if (wrap_around) {
            if (boid->position[d] > half) boid->position[d] = -half;
            if (boid->position[d] < -half) boid->position[d] = half;
        } else {
            if (boid->position[d] > half - margin)
                force[d] = -1.0 / (boid->position[d] - (half - margin) + SWARM_EPSILON);
            if (boid->position[d] < -half + margin)
                force[d] += 1.0 / ((-half + margin) - boid->position[d] + SWARM_EPSILON);
        }
    }
}

/* ── Flock Update Step ── */
void swarm_flock_update(Boid* flock, int n_boids, const FlockConfig* config) {
    if (!flock || !config) return;
    for (int i = 0; i < n_boids; i++) {
        swarm_flocking_accumulate_forces(&flock[i], flock, n_boids, config);
        /* Integrate: Euler method */
        for (int d = 0; d < 3; d++) {
            /* Clamp acceleration to max_force */
            if (flock[i].acceleration[d] > flock[i].max_force)
                flock[i].acceleration[d] = flock[i].max_force;
            if (flock[i].acceleration[d] < -flock[i].max_force)
                flock[i].acceleration[d] = -flock[i].max_force;
            /* Update velocity and position */
            flock[i].velocity[d] += flock[i].acceleration[d] * config->dt;
            /* Clamp velocity to max_speed */
            double speed = sqrt(flock[i].velocity[0] * flock[i].velocity[0] +
                               flock[i].velocity[1] * flock[i].velocity[1] +
                               flock[i].velocity[2] * flock[i].velocity[2]);
            if (speed > flock[i].max_speed && speed > SWARM_EPSILON) {
                double scale = flock[i].max_speed / speed;
                flock[i].velocity[d] *= scale;
            }
            flock[i].position[d] += flock[i].velocity[d] * config->dt;
        }
        /* Boundary handling */
        if (!config->wrap_around) {
            double half = config->world_size * 0.5;
            for (int d = 0; d < 3; d++) {
                if (flock[i].position[d] > half) { flock[i].position[d] = half; flock[i].velocity[d] *= -0.5; }
                if (flock[i].position[d] < -half) { flock[i].position[d] = -half; flock[i].velocity[d] *= -0.5; }
            }
        }
    }
}

void swarm_flock_update_with_spatial_hash(Boid* flock, int n_boids,
           const FlockConfig* config, SpatialHash* sh) {
    if (!flock || !config || !sh) return;
    swarm_spatial_hash_clear(sh);
    for (int i = 0; i < n_boids; i++)
        swarm_spatial_hash_insert(sh, flock[i].position, i);
    /* Then use hash for neighbor queries instead of O(n²) */
    swarm_flock_update(flock, n_boids, config); /* fallback to brute force for now */
}

/* ── Flocking Analysis ── */
double swarm_flock_order_parameter(const Boid* flock, int n_boids) {
    if (!flock || n_boids <= 0) return 0.0;
    double sum_v[3] = {0.0, 0.0, 0.0};
    double total_speed = 0.0;
    for (int i = 0; i < n_boids; i++) {
        sum_v[0] += flock[i].velocity[0];
        sum_v[1] += flock[i].velocity[1];
        sum_v[2] += flock[i].velocity[2];
        total_speed += sqrt(flock[i].velocity[0] * flock[i].velocity[0] +
                           flock[i].velocity[1] * flock[i].velocity[1] +
                           flock[i].velocity[2] * flock[i].velocity[2]);
    }
    double mag = sqrt(sum_v[0] * sum_v[0] + sum_v[1] * sum_v[1] + sum_v[2] * sum_v[2]);
    return (total_speed > SWARM_EPSILON) ? mag / total_speed : 0.0;
}

void swarm_flock_centroid(const Boid* flock, int n_boids, double centroid[3]) {
    centroid[0] = centroid[1] = centroid[2] = 0.0;
    if (!flock || n_boids <= 0) return;
    for (int i = 0; i < n_boids; i++)
        for (int d = 0; d < 3; d++) centroid[d] += flock[i].position[d];
    double inv = 1.0 / (double)n_boids;
    for (int d = 0; d < 3; d++) centroid[d] *= inv;
}

double swarm_flock_extent(const Boid* flock, int n_boids) {
    if (!flock || n_boids <= 0) return 0.0;
    double centroid[3];
    swarm_flock_centroid(flock, n_boids, centroid);
    double max_r = 0.0;
    for (int i = 0; i < n_boids; i++) {
        double dx = flock[i].position[0] - centroid[0];
        double dy = flock[i].position[1] - centroid[1];
        double dz = flock[i].position[2] - centroid[2];
        double r = sqrt(dx*dx + dy*dy + dz*dz);
        if (r > max_r) max_r = r;
    }
    return max_r;
}

double swarm_flock_nearest_neighbor_distance(const Boid* flock, int n_boids) {
    if (!flock || n_boids <= 1) return 0.0;
    double sum_min = 0.0;
    for (int i = 0; i < n_boids; i++) {
        double min_dsq = 1e100;
        for (int j = 0; j < n_boids; j++) {
            if (i == j) continue;
            double dx = flock[i].position[0] - flock[j].position[0];
            double dy = flock[i].position[1] - flock[j].position[1];
            double dz = flock[i].position[2] - flock[j].position[2];
            double dsq = dx*dx + dy*dy + dz*dz;
            if (dsq < min_dsq) min_dsq = dsq;
        }
        sum_min += sqrt(min_dsq);
    }
    return sum_min / (double)n_boids;
}

int swarm_flock_subgroup_count(const Boid* flock, int n_boids, double radius) {
    if (!flock || n_boids <= 0) return 0;
    bool* visited = (bool*)calloc((size_t)n_boids, sizeof(bool));
    if (!visited) return 0;
    int groups = 0;
    double rsq = radius * radius;
    for (int i = 0; i < n_boids; i++) {
        if (visited[i]) continue;
        groups++;
        /* BFS to find connected component */
        int queue[SWARM_MAX_AGENTS], head = 0, tail = 0;
        queue[tail++] = i; visited[i] = true;
        while (head < tail) {
            int u = queue[head++];
            for (int v = 0; v < n_boids; v++) {
                if (visited[v]) continue;
                double dx = flock[u].position[0] - flock[v].position[0];
                double dy = flock[u].position[1] - flock[v].position[1];
                double dz = flock[u].position[2] - flock[v].position[2];
                if (dx*dx + dy*dy + dz*dz <= rsq) {
                    visited[v] = true;
                    if (tail < SWARM_MAX_AGENTS) queue[tail++] = v;
                }
            }
        }
    }
    free(visited);
    return groups;
}

/* ── Predator-Prey Dynamics ── */
void swarm_flocking_predator_chase(Boid* predator, const Boid* flock, int n_boids,
           double chase_weight, double force[3]) {
    if (!predator || !flock) { force[0]=force[1]=force[2]=0.0; return; }
    /* Find nearest prey */
    int nearest = -1; double min_dsq = 1e100;
    for (int i = 0; i < n_boids; i++) {
        if (flock[i].is_predator) continue;
        double dx = predator->position[0] - flock[i].position[0];
        double dy = predator->position[1] - flock[i].position[1];
        double dz = predator->position[2] - flock[i].position[2];
        double dsq = dx*dx + dy*dy + dz*dz;
        if (dsq < min_dsq) { min_dsq = dsq; nearest = i; }
    }
    if (nearest >= 0) {
        double dx = flock[nearest].position[0] - predator->position[0];
        double dy = flock[nearest].position[1] - predator->position[1];
        double dz = flock[nearest].position[2] - predator->position[2];
        double d = sqrt(dx*dx + dy*dy + dz*dz);
        if (d > SWARM_EPSILON) {
            force[0] = (dx / d) * chase_weight;
            force[1] = (dy / d) * chase_weight;
            force[2] = (dz / d) * chase_weight;
            return;
        }
    }
    force[0] = force[1] = force[2] = 0.0;
}

void swarm_flocking_prey_evade(const Boid* prey, const Boid* predator,
           double evade_weight, double force[3]) {
    if (!prey || !predator) { force[0]=force[1]=force[2]=0.0; return; }
    double dx = prey->position[0] - predator->position[0];
    double dy = prey->position[1] - predator->position[1];
    double dz = prey->position[2] - predator->position[2];
    double d = sqrt(dx*dx + dy*dy + dz*dz);
    if (d > SWARM_EPSILON) {
        force[0] = (dx / d) * evade_weight;
        force[1] = (dy / d) * evade_weight;
        force[2] = (dz / d) * evade_weight;
    } else {
        force[0] = force[1] = force[2] = 0.0;
    }
}

/* ── Obstacle Avoidance ── */
void swarm_flocking_obstacle_avoidance(Boid* boid, const Obstacle* obstacles,
           int n_obstacles, double weight, double force[3]) {
    force[0] = force[1] = force[2] = 0.0;
    if (!boid || !obstacles || n_obstacles <= 0) return;
    for (int o = 0; o < n_obstacles; o++) {
        double dx = boid->position[0] - obstacles[o].position[0];
        double dy = boid->position[1] - obstacles[o].position[1];
        double dz = boid->position[2] - obstacles[o].position[2];
        double d = sqrt(dx*dx + dy*dy + dz*dz);
        double safe_dist = obstacles[o].radius + boid->perception_radius * 0.3;
        if (d < safe_dist && d > SWARM_EPSILON) {
            double inv_d = 1.0 / d;
            force[0] += (dx * inv_d) * weight / (d * d);
            force[1] += (dy * inv_d) * weight / (d * d);
            force[2] += (dz * inv_d) * weight / (d * d);
        }
    }
}

/* ── Leader-Follower ── */
void swarm_flocking_leader_goal(Boid* leader, const double goal[3],
                                 double goal_weight, double force[3]) {
    if (!leader || !goal) { force[0]=force[1]=force[2]=0.0; return; }
    double dx = goal[0] - leader->position[0];
    double dy = goal[1] - leader->position[1];
    double dz = goal[2] - leader->position[2];
    double d = sqrt(dx*dx + dy*dy + dz*dz);
    if (d > SWARM_EPSILON) {
        force[0] = (dx / d) * goal_weight;
        force[1] = (dy / d) * goal_weight;
        force[2] = (dz / d) * goal_weight;
    } else {
        force[0] = force[1] = force[2] = 0.0;
    }
}

void swarm_flocking_follow_leader(Boid* follower, const Boid* leader,
           double follow_weight, double force[3]) {
    if (!follower || !leader) { force[0]=force[1]=force[2]=0.0; return; }
    double dx = leader->position[0] - follower->position[0];
    double dy = leader->position[1] - follower->position[1];
    double dz = leader->position[2] - follower->position[2];
    force[0] = dx * follow_weight;
    force[1] = dy * follow_weight;
    force[2] = dz * follow_weight;
}

void swarm_flocking_wind(const Boid* flock, int n_boids, const double wind[3], double weight) {
    if (!flock || !wind) return;
    for (int i = 0; i < n_boids; i++) {
        /* Add wind as additional acceleration */
        /* Wind effect applied externally in the update loop */
        (void)weight;
    }
}

/* ── Vicsek Model ── */
void swarm_vicsek_init(Boid* flock, int n_boids, double world_size,
                        double speed, double noise_eta) {
    if (!flock) return;
    for (int i = 0; i < n_boids; i++) {
        /* Random positions in box */
        for (int d = 0; d < 2; d++)
            flock[i].position[d] = swarm_rng_uniform_range(-world_size*0.5, world_size*0.5);
        flock[i].position[2] = 0.0;
        /* Random direction, fixed speed */
        double angle = swarm_rng_uniform_range(0.0, 2.0 * SWARM_PI);
        flock[i].velocity[0] = speed * cos(angle);
        flock[i].velocity[1] = speed * sin(angle);
        flock[i].velocity[2] = 0.0;
        flock[i].max_speed = speed;
        flock[i].perception_radius = 1.0;
        (void)noise_eta;
    }
}

void swarm_vicsek_update(Boid* flock, int n_boids, double speed, double radius,
                          double noise_eta, double world_size) {
    if (!flock) return;
    /* Store current velocities */
    double new_vel[SWARM_MAX_AGENTS][3];
    double rsq = radius * radius;
    
    for (int i = 0; i < n_boids; i++) {
        /* Average direction of neighbors within radius */
        double avg_cos = 0.0, avg_sin = 0.0;
        int count = 0;
        for (int j = 0; j < n_boids; j++) {
            double dx = flock[i].position[0] - flock[j].position[0];
            double dy = flock[i].position[1] - flock[j].position[1];
            if (dx*dx + dy*dy < rsq) {
                double v_norm = sqrt(flock[j].velocity[0]*flock[j].velocity[0] +
                                    flock[j].velocity[1]*flock[j].velocity[1]);
                if (v_norm > SWARM_EPSILON) {
                    avg_cos += flock[j].velocity[0] / v_norm;
                    avg_sin += flock[j].velocity[1] / v_norm;
                }
                count++;
            }
        }
        double angle;
        if (count > 0 && (fabs(avg_cos) > SWARM_EPSILON || fabs(avg_sin) > SWARM_EPSILON)) {
            angle = atan2(avg_sin, avg_cos);
        } else {
            angle = atan2(flock[i].velocity[1], flock[i].velocity[0]);
        }
        /* Add noise */
        angle += swarm_rng_uniform_range(-noise_eta * 0.5, noise_eta * 0.5);
        new_vel[i][0] = speed * cos(angle);
        new_vel[i][1] = speed * sin(angle);
        new_vel[i][2] = 0.0;
    }
    
    for (int i = 0; i < n_boids; i++) {
        for (int d = 0; d < 3; d++) {
            flock[i].velocity[d] = new_vel[i][d];
            flock[i].position[d] += flock[i].velocity[d];
        }
        /* Periodic boundaries */
        double half = world_size * 0.5;
        if (flock[i].position[0] > half) flock[i].position[0] -= world_size;
        if (flock[i].position[0] < -half) flock[i].position[0] += world_size;
        if (flock[i].position[1] > half) flock[i].position[1] -= world_size;
        if (flock[i].position[1] < -half) flock[i].position[1] += world_size;
    }
}

double swarm_vicsek_order_parameter(const Boid* flock, int n_boids, double speed) {
    if (!flock || n_boids <= 0) return 0.0;
    double sum_vx = 0.0, sum_vy = 0.0;
    for (int i = 0; i < n_boids; i++) {
        sum_vx += flock[i].velocity[0];
        sum_vy += flock[i].velocity[1];
    }
    return sqrt(sum_vx*sum_vx + sum_vy*sum_vy) / ((double)n_boids * speed);
}

/* ── Cucker-Smale Model ── */
void swarm_cucker_smale_init(Boid* flock, int n_boids, double world_size) {
    if (!flock) return;
    for (int i = 0; i < n_boids; i++) {
        for (int d = 0; d < 3; d++) {
            flock[i].position[d] = swarm_rng_uniform_range(-world_size*0.5, world_size*0.5);
            flock[i].velocity[d] = swarm_rng_uniform_range(-1, 1);
        }
    }
}

void swarm_cucker_smale_update(Boid* flock, int n_boids, double K, double beta, double dt) {
    if (!flock) return;
    double new_vel[SWARM_MAX_AGENTS][3];
    for (int i = 0; i < n_boids; i++) {
        double dv[3] = {0.0, 0.0, 0.0};
        for (int j = 0; j < n_boids; j++) {
            if (i == j) continue;
            double dx = flock[i].position[0] - flock[j].position[0];
            double dy = flock[i].position[1] - flock[j].position[1];
            double dz = flock[i].position[2] - flock[j].position[2];
            double dsq = dx*dx + dy*dy + dz*dz;
            double a_ij = K / pow(1.0 + dsq, beta);
            dv[0] += a_ij * (flock[j].velocity[0] - flock[i].velocity[0]);
            dv[1] += a_ij * (flock[j].velocity[1] - flock[i].velocity[1]);
            dv[2] += a_ij * (flock[j].velocity[2] - flock[i].velocity[2]);
        }
        new_vel[i][0] = flock[i].velocity[0] + dv[0] * dt;
        new_vel[i][1] = flock[i].velocity[1] + dv[1] * dt;
        new_vel[i][2] = flock[i].velocity[2] + dv[2] * dt;
    }
    for (int i = 0; i < n_boids; i++) {
        for (int d = 0; d < 3; d++) {
            flock[i].velocity[d] = new_vel[i][d];
            flock[i].position[d] += flock[i].velocity[d] * dt;
        }
    }
}

bool swarm_cucker_smale_has_converged(const Boid* flock, int n_boids, double epsilon) {
    if (!flock || n_boids <= 1) return true;
    double avg_vel[3] = {0.0, 0.0, 0.0};
    for (int i = 0; i < n_boids; i++)
        for (int d = 0; d < 3; d++) avg_vel[d] += flock[i].velocity[d];
    for (int d = 0; d < 3; d++) avg_vel[d] /= (double)n_boids;
    for (int i = 0; i < n_boids; i++) {
        double diff_sq = 0.0;
        for (int d = 0; d < 3; d++) { double df = flock[i].velocity[d] - avg_vel[d]; diff_sq += df*df; }
        if (sqrt(diff_sq) > epsilon) return false;
    }
    return true;
}
