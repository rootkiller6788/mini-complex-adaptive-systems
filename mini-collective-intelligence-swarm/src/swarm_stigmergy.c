#include "swarm_stigmergy.h"
#include "swarm_core.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Stigmergy & Self-Organization — Core Implementation
 *
 * References:
 *   Grassé (1959)            — Stigmergy concept
 *   Deneubourg et al. (1990) — Threshold-based trail following
 *   Camazine et al. (2001)   — Self-Organization in Biological Systems
 *   Theraulaz & Bonabeau (1999) — History of stigmergy
 * ============================================================================ */

/* ── Stigmergy Environment ── */
StigmergyEnvironment* swarm_stigmergy_create(int width, int height,
                         double diffusion_rate, double decay_rate) {
    if (width <= 0 || height <= 0) return NULL;
    StigmergyEnvironment* env = (StigmergyEnvironment*)calloc(1, sizeof(StigmergyEnvironment));
    if (!env) return NULL;
    env->width = width; env->height = height;
    env->diffusion_rate = diffusion_rate;
    env->decay_rate = decay_rate;
    env->cells = swarm_matrix_alloc(height, width);
    env->gradient_x = swarm_matrix_alloc(height, width);
    env->gradient_y = swarm_matrix_alloc(height, width);
    if (!env->cells || !env->gradient_x || !env->gradient_y) {
        swarm_stigmergy_free(env); return NULL;
    }
    return env;
}

void swarm_stigmergy_free(StigmergyEnvironment* env) {
    if (!env) return;
    swarm_matrix_free(env->cells, env->height);
    swarm_matrix_free(env->gradient_x, env->height);
    swarm_matrix_free(env->gradient_y, env->height);
    free(env);
}

void swarm_stigmergy_reset(StigmergyEnvironment* env) {
    if (!env) return;
    for (int y = 0; y < env->height; y++)
        for (int x = 0; x < env->width; x++)
            env->cells[y][x] = 0.0;
}

/* ── Diffusion: ∂τ/∂t = D·∇²τ (5-point Laplacian, explicit Euler) ── */
void swarm_stigmergy_diffuse(StigmergyEnvironment* env, double dt) {
    if (!env) return;
    int w = env->width, h = env->height;
    double D = env->diffusion_rate;
    /* Stability condition: D·dt < 0.25 for unit grid */
    double* new_data = (double*)calloc((size_t)(w * h), sizeof(double));
    if (!new_data) return;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            double laplacian = -4.0 * env->cells[y][x];
            if (y > 0) laplacian += env->cells[y-1][x];
            else if (env->toroidal) laplacian += env->cells[h-1][x];
            if (y < h-1) laplacian += env->cells[y+1][x];
            else if (env->toroidal) laplacian += env->cells[0][x];
            if (x > 0) laplacian += env->cells[y][x-1];
            else if (env->toroidal) laplacian += env->cells[y][w-1];
            if (x < w-1) laplacian += env->cells[y][x+1];
            else if (env->toroidal) laplacian += env->cells[y][0];
            new_data[y * w + x] = env->cells[y][x] + D * laplacian * dt;
            if (new_data[y * w + x] < 0.0) new_data[y * w + x] = 0.0;
        }
    }
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            env->cells[y][x] = new_data[y * w + x];
    free(new_data);
}

/* ── Decay: τ ← τ·exp(-λ·dt) ── */
void swarm_stigmergy_decay(StigmergyEnvironment* env, double dt) {
    if (!env) return;
    double factor = exp(-env->decay_rate * dt);
    for (int y = 0; y < env->height; y++)
        for (int x = 0; x < env->width; x++)
            env->cells[y][x] *= factor;
}

/* ── Deposit pheromone at position with Gaussian spread ── */
void swarm_stigmergy_deposit(StigmergyEnvironment* env, double x, double y,
                              double amount, double radius) {
    if (!env) return;
    int cx = (int)floor(x), cy = (int)floor(y);
    int r = (int)ceil(radius);
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int gx = cx + dx, gy = cy + dy;
            if (gx < 0 || gx >= env->width || gy < 0 || gy >= env->height) continue;
            double dist = sqrt((double)(dx*dx + dy*dy));
            if (dist > radius) continue;
            /* Gaussian falloff */
            double contrib = amount * exp(-dist*dist / (2.0 * radius * radius));
            env->cells[gy][gx] += contrib;
        }
    }
}

/* ── Bilinear sample at continuous position ── */
double swarm_stigmergy_sample(const StigmergyEnvironment* env, double x, double y) {
    if (!env) return 0.0;
    int x0 = (int)floor(x), y0 = (int)floor(y);
    int x1 = x0 + 1, y1 = y0 + 1;
    /* Clamp to bounds */
    if (x0 < 0) x0 = 0; if (x0 >= env->width) x0 = env->width - 1;
    if (x1 < 0) x1 = 0; if (x1 >= env->width) x1 = env->width - 1;
    if (y0 < 0) y0 = 0; if (y0 >= env->height) y0 = env->height - 1;
    if (y1 < 0) y1 = 0; if (y1 >= env->height) y1 = env->height - 1;
    double fx = x - floor(x), fy = y - floor(y);
    double v00 = env->cells[y0][x0], v10 = env->cells[y1][x0];
    double v01 = env->cells[y0][x1], v11 = env->cells[y1][x1];
    return (1.0-fy)*((1.0-fx)*v00 + fx*v01) + fy*((1.0-fx)*v10 + fx*v11);
}

/* ── Gradient computation ── */
void swarm_stigmergy_gradient(const StigmergyEnvironment* env, double x, double y,
                               double* grad_x, double* grad_y) {
    if (!env || !grad_x || !grad_y) return;
    double dx = 0.5; /* Finite difference step */
    *grad_x = (swarm_stigmergy_sample(env, x + dx, y) - 
               swarm_stigmergy_sample(env, x - dx, y)) / (2.0 * dx);
    *grad_y = (swarm_stigmergy_sample(env, x, y + dx) - 
               swarm_stigmergy_sample(env, x, y - dx)) / (2.0 * dx);
}

void swarm_stigmergy_update(StigmergyEnvironment* env, double dt) {
    if (!env) return;
    swarm_stigmergy_decay(env, dt);
    swarm_stigmergy_diffuse(env, dt);
}

/* ── Trail Following ── */
void swarm_trail_follow(double* position, const StigmergyEnvironment* env,
           double speed, double noise, double* new_position) {
    if (!position || !env || !new_position) return;
    double grad_x, grad_y;
    swarm_stigmergy_gradient(env, position[0], position[1], &grad_x, &grad_y);
    double mag = sqrt(grad_x*grad_x + grad_y*grad_y);
    double dx, dy;
    if (mag > SWARM_EPSILON) {
        dx = (grad_x / mag) * speed;
        dy = (grad_y / mag) * speed;
    } else {
        /* Random walk */
        double angle = swarm_rng_uniform_range(0.0, 2.0 * SWARM_PI);
        dx = speed * cos(angle);
        dy = speed * sin(angle);
    }
    /* Add noise */
    dx += swarm_rng_gaussian() * noise;
    dy += swarm_rng_gaussian() * noise;
    new_position[0] = position[0] + dx;
    new_position[1] = position[1] + dy;
    /* Clamp */
    if (new_position[0] < 0) new_position[0] = 0;
    if (new_position[0] >= env->width) new_position[0] = env->width - 1;
    if (new_position[1] < 0) new_position[1] = 0;
    if (new_position[1] >= env->height) new_position[1] = env->height - 1;
}

/* ── Deneubourg Response Function ── */
double swarm_denebourg_response(double pheromone, double threshold_K) {
    /* P(follow) = τ² / (τ² + K²) */
    double t2 = pheromone * pheromone;
    double K2 = threshold_K * threshold_K;
    return t2 / (t2 + K2 + SWARM_EPSILON);
}

/* ── Bridge Experiment ── */
void swarm_bridge_experiment_init(BridgeExperiment* exp) {
    if (!exp) return;
    memset(exp, 0, sizeof(BridgeExperiment));
    exp->pheromone_short = 1.0;
    exp->pheromone_long = 1.0;
    exp->ratio_short = 0.5;
}

void swarm_bridge_experiment_step(BridgeExperiment* exp, int n_ants_per_step,
           double evaporation_rate, double deposit_amount,
           double threshold_K, double path_ratio) {
    if (!exp) return;
    /* Evaporation */
    exp->pheromone_short *= (1.0 - evaporation_rate);
    exp->pheromone_long  *= (1.0 - evaporation_rate);
    /* Ants choose path based on Deneubourg function */
    double p_short = swarm_denebourg_response(exp->pheromone_short, threshold_K);
    double p_long  = swarm_denebourg_response(exp->pheromone_long, threshold_K);
    double total = p_short + p_long + SWARM_EPSILON;
    p_short /= total;
    /* Stochastic allocation */
    int n_short = 0;
    for (int i = 0; i < n_ants_per_step; i++)
        if (swarm_rng_uniform() < p_short) n_short++;
    int n_long = n_ants_per_step - n_short;
    /* Deposit pheromone */
    exp->pheromone_short += n_short * deposit_amount;
    exp->pheromone_long  += n_long * deposit_amount * path_ratio;
    exp->ratio_short = (double)n_short / (double)n_ants_per_step;
    exp->elapsed_time += 1.0;
    /* Symmetry breaking detection */
    if (exp->ratio_short > 0.8 || exp->ratio_short < 0.2) exp->broken = true;
}

/* ── Cellular Automaton Slime Mold ── */
void swarm_ca_slime_mold(double** grid, int width, int height,
           double camp_diffusion, double camp_decay, double camp_production,
           double dt, int n_steps) {
    if (!grid) return;
    for (int step = 0; step < n_steps; step++) {
        /* Diffusion + decay + production of cAMP */
        double* new_row = (double*)calloc((size_t)width, sizeof(double));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                double laplacian = -4.0 * grid[y][x];
                if (y > 0) laplacian += grid[y-1][x];
                if (y < height-1) laplacian += grid[y+1][x];
                if (x > 0) laplacian += grid[y][x-1];
                if (x < width-1) laplacian += grid[y][x+1];
                new_row[x] = grid[y][x] + (camp_diffusion * laplacian - 
                             camp_decay * grid[y][x] + camp_production * grid[y][x]) * dt;
                if (new_row[x] < 0.0) new_row[x] = 0.0;
            }
            memcpy(grid[y], new_row, (size_t)width * sizeof(double));
        }
        free(new_row);
        new_row = (double*)calloc((size_t)width, sizeof(double));
    }
}

/* ── Turing Pattern ── */
void swarm_turing_pattern(double** activator, double** inhibitor,
           int width, int height, double a_diff, double i_diff,
           double feed_rate, double kill_rate, double dt, int n_steps) {
    if (!activator || !inhibitor) return;
    for (int step = 0; step < n_steps; step++) {
        /* Compute Laplacians (periodic boundaries) */
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                /* Laplacian for activator */
                double lap_a = -4.0 * activator[y][x];
                int ym = (y > 0) ? y-1 : height-1, yp = (y < height-1) ? y+1 : 0;
                int xm = (x > 0) ? x-1 : width-1, xp = (x < width-1) ? x+1 : 0;
                lap_a += activator[ym][x] + activator[yp][x] + activator[y][xm] + activator[y][xp];
                /* Laplacian for inhibitor */
                double lap_i = -4.0 * inhibitor[y][x];
                lap_i += inhibitor[ym][x] + inhibitor[yp][x] + inhibitor[y][xm] + inhibitor[y][xp];
                /* Gray-Scott reaction-diffusion */
                double a = activator[y][x], i = inhibitor[y][x];
                double da = a_diff * lap_a - a*i*i + feed_rate*(1.0 - a);
                double di = i_diff * lap_i + a*i*i - (feed_rate + kill_rate)*i;
                activator[y][x] += da * dt;
                inhibitor[y][x] += di * dt;
                if (activator[y][x] < 0.0) activator[y][x] = 0.0;
                if (inhibitor[y][x] < 0.0) inhibitor[y][x] = 0.0;
            }
        }
    }
}

/* ── Foraging Simulation ── */
ForagingSimulation* swarm_foraging_create(int n_ants, double nest_x, double nest_y,
                      int world_width, int world_height) {
    ForagingSimulation* sim = (ForagingSimulation*)calloc(1, sizeof(ForagingSimulation));
    if (!sim) return NULL;
    sim->ants = (ForagingAnt*)calloc((size_t)n_ants, sizeof(ForagingAnt));
    if (!sim->ants) { free(sim); return NULL; }
    sim->n_ants = n_ants;
    sim->nest_x = nest_x; sim->nest_y = nest_y;
    sim->food_pheromone = swarm_stigmergy_create(world_width, world_height, 0.1, 0.05);
    sim->home_pheromone = swarm_stigmergy_create(world_width, world_height, 0.1, 0.05);
    sim->ant_speed = 1.0;
    sim->deposit_rate = 1.0;
    sim->sensing_radius = 2.0;
    sim->wander_strength = 0.3;
    /* Initialize ants at nest */
    for (int i = 0; i < n_ants; i++) {
        sim->ants[i].x = nest_x;
        sim->ants[i].y = nest_y;
        sim->ants[i].heading = swarm_rng_uniform_range(0.0, 2.0 * SWARM_PI);
        sim->ants[i].has_food = false;
        sim->ants[i].returning = false;
    }
    return sim;
}

void swarm_foraging_free(ForagingSimulation* sim) {
    if (!sim) return;
    free(sim->ants);
    swarm_stigmergy_free(sim->food_pheromone);
    swarm_stigmergy_free(sim->home_pheromone);
    if (sim->food_sources) { free(sim->food_sources[0]); free(sim->food_sources); }
    free(sim->food_amounts);
    free(sim);
}

void swarm_foraging_add_food(ForagingSimulation* sim, double x, double y, double amount) {
    if (!sim) return;
    int n = sim->n_food_sources + 1;
    double** new_sources = (double**)malloc((size_t)n * sizeof(double*));
    new_sources[0] = (double*)malloc((size_t)(n * 2) * sizeof(double));
    for (int i = 1; i < n; i++) new_sources[i] = new_sources[0] + i * 2;
    double* new_amounts = (double*)realloc(sim->food_amounts, (size_t)n * sizeof(double));
    if (!new_sources || !new_sources[0] || !new_amounts) {
        if (new_sources) { free(new_sources[0]); free(new_sources); }
        free(new_amounts); return;
    }
    /* Copy existing */
    for (int i = 0; i < sim->n_food_sources; i++) {
        new_sources[i][0] = sim->food_sources[i][0];
        new_sources[i][1] = sim->food_sources[i][1];
        new_amounts[i] = sim->food_amounts[i];
    }
    new_sources[sim->n_food_sources][0] = x;
    new_sources[sim->n_food_sources][1] = y;
    new_amounts[sim->n_food_sources] = amount;
    if (sim->food_sources) { free(sim->food_sources[0]); free(sim->food_sources); }
    sim->food_sources = new_sources;
    sim->food_amounts = new_amounts;
    sim->n_food_sources = n;
}

void swarm_foraging_update(ForagingSimulation* sim) {
    if (!sim) return;
    for (int i = 0; i < sim->n_ants; i++) {
        ForagingAnt* ant = &sim->ants[i];
        double pheromone;
        if (!ant->has_food) {
            /* Searching: follow food pheromone */
            pheromone = swarm_stigmergy_sample(sim->food_pheromone, ant->x, ant->y);
            /* Also check for nearby food */
            for (int f = 0; f < sim->n_food_sources; f++) {
                double dx = ant->x - sim->food_sources[f][0];
                double dy = ant->y - sim->food_sources[f][1];
                if (sqrt(dx*dx + dy*dy) < 1.5 && sim->food_amounts[f] > 0.0) {
                    ant->has_food = true;
                    ant->returning = true;
                    sim->food_amounts[f] -= 1.0;
                    break;
                }
            }
        } else {
            /* Returning: follow home pheromone */
            pheromone = swarm_stigmergy_sample(sim->home_pheromone, ant->x, ant->y);
            double dx_home = sim->nest_x - ant->x;
            double dy_home = sim->nest_y - ant->y;
            if (sqrt(dx_home*dx_home + dy_home*dy_home) < 2.0) {
                ant->has_food = false;
                ant->returning = false;
                ant->heading = swarm_rng_uniform_range(0.0, 2.0 * SWARM_PI);
            }
        }
        /* Move: biased random walk toward pheromone gradient */
        double grad_x, grad_y;
        StigmergyEnvironment* env = ant->has_food ? sim->home_pheromone : sim->food_pheromone;
        swarm_stigmergy_gradient(env, ant->x, ant->y, &grad_x, &grad_y);
        double gmag = sqrt(grad_x*grad_x + grad_y*grad_y);
        if (gmag > SWARM_EPSILON) {
            ant->heading = 0.8 * atan2(grad_y, grad_x) + 0.2 * ant->heading;
        }
        ant->heading += swarm_rng_gaussian() * sim->wander_strength;
        ant->x += sim->ant_speed * cos(ant->heading);
        ant->y += sim->ant_speed * sin(ant->heading);
        /* Deposit pheromone */
        if (ant->has_food) {
            swarm_stigmergy_deposit(sim->food_pheromone, ant->x, ant->y, sim->deposit_rate, 1.0);
        } else {
            swarm_stigmergy_deposit(sim->home_pheromone, ant->x, ant->y, sim->deposit_rate, 1.0);
        }
        /* Boundary clamp */
        if (ant->x < 0) ant->x = 0;
        if (ant->x >= sim->food_pheromone->width) ant->x = sim->food_pheromone->width - 1;
        if (ant->y < 0) ant->y = 0;
        if (ant->y >= sim->food_pheromone->height) ant->y = sim->food_pheromone->height - 1;
    }
    /* Update environment */
    swarm_stigmergy_update(sim->food_pheromone, 0.1);
    swarm_stigmergy_update(sim->home_pheromone, 0.1);
}

int swarm_foraging_food_collected(const ForagingSimulation* sim) {
    if (!sim) return 0;
    int collected = 0;
    for (int f = 0; f < sim->n_food_sources; f++)
        collected += (int)((sim->food_sources ? 0 : 0));
    /* Count ants carrying food returning to nest */
    for (int i = 0; i < sim->n_ants; i++)
        if (sim->ants[i].has_food && sim->ants[i].returning) {
            double dx = sim->nest_x - sim->ants[i].x;
            double dy = sim->nest_y - sim->ants[i].y;
            if (sqrt(dx*dx + dy*dy) < 2.0) collected++;
        }
    return collected;
}

/* ── Cemetery Clustering (Lumer & Faieta 1994) ──
 *
 * Ants move on a grid, picking up and dropping items based on local
 * similarity. Over time, items of the same type cluster together.
 *
 * Pick probability: p_pick = (k_p / (k_p + f))^2
 *   where f = local density of similar items in neighborhood.
 * Drop probability: p_drop = (f / (k_d + f))^2
 *   where f = local density of similar items at current location.
 *
 * grid[y][x]: item type at (x,y), 0 = empty, >0 = item type
 */
void swarm_cemetery_clustering(int** grid, int width, int height,
           int n_ants, int n_steps, double pick_prob, double drop_prob) {
    if (!grid || width <= 0 || height <= 0 || n_ants <= 0) return;

    /* Initialize ant positions */
    int* ant_x = swarm_alloc_int_vector(n_ants);
    int* ant_y = swarm_alloc_int_vector(n_ants);
    int* ant_carrying = swarm_alloc_int_vector(n_ants); /* 0=nothing, >0=item type */
    if (!ant_x || !ant_y || !ant_carrying) {
        swarm_free_int_vector(ant_x); swarm_free_int_vector(ant_y);
        swarm_free_int_vector(ant_carrying); return;
    }
    for (int a = 0; a < n_ants; a++) {
        ant_x[a] = swarm_rng_int(width);
        ant_y[a] = swarm_rng_int(height);
        ant_carrying[a] = 0;
    }

    double k_p = 0.1, k_d = 0.3; /* Threshold constants */
    int neighborhood = 3; /* 3x3 neighborhood for similarity */

    for (int step = 0; step < n_steps; step++) {
        for (int a = 0; a < n_ants; a++) {
            int cx = ant_x[a], cy = ant_y[a];

            if (ant_carrying[a] == 0) {
                /* Not carrying: try to pick up */
                if (cx >= 0 && cx < width && cy >= 0 && cy < height && grid[cy][cx] != 0) {
                    int item_type = grid[cy][cx];
                    /* Compute local density of similar items */
                    int similar = 0, total = 0;
                    for (int dy = -neighborhood; dy <= neighborhood; dy++) {
                        for (int dx = -neighborhood; dx <= neighborhood; dx++) {
                            int nx = cx + dx, ny = cy + dy;
                            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                            if (grid[ny][nx] != 0) {
                                total++;
                                if (grid[ny][nx] == item_type) similar++;
                            }
                        }
                    }
                    double density = (total > 0) ? (double)similar / (double)total : 0.0;
                    double p_pick = pick_prob * pow(k_p / (k_p + density + SWARM_EPSILON), 2.0);
                    if (swarm_rng_uniform() < p_pick) {
                        ant_carrying[a] = item_type;
                        grid[cy][cx] = 0;
                    }
                }
            } else {
                /* Carrying: try to drop */
                if (cx >= 0 && cx < width && cy >= 0 && cy < height && grid[cy][cx] == 0) {
                    int item_type = ant_carrying[a];
                    /* Compute local density of similar items in perception */
                    int similar = 0, total = 0;
                    for (int dy = -neighborhood; dy <= neighborhood; dy++) {
                        for (int dx = -neighborhood; dx <= neighborhood; dx++) {
                            int nx = cx + dx, ny = cy + dy;
                            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                            if (grid[ny][nx] != 0) {
                                total++;
                                if (grid[ny][nx] == item_type) similar++;
                            }
                        }
                    }
                    double density = (total > 0) ? (double)similar / (double)total : 0.0;
                    double p_drop = drop_prob * pow(density / (k_d + density + SWARM_EPSILON), 2.0);
                    if (swarm_rng_uniform() < p_drop) {
                        grid[cy][cx] = item_type;
                        ant_carrying[a] = 0;
                    }
                }
            }

            /* Move ant randomly */
            ant_x[a] += swarm_rng_int(3) - 1; /* -1, 0, or 1 */
            ant_y[a] += swarm_rng_int(3) - 1;
            /* Wrap around */
            if (ant_x[a] < 0) ant_x[a] += width;
            if (ant_x[a] >= width) ant_x[a] -= width;
            if (ant_y[a] < 0) ant_y[a] += height;
            if (ant_y[a] >= height) ant_y[a] -= height;
        }
    }

    swarm_free_int_vector(ant_x);
    swarm_free_int_vector(ant_y);
    swarm_free_int_vector(ant_carrying);
}

/* ── Digital Pheromone (for UAV/robot coordination) ── */
DigitalPheromone* swarm_digital_pheromone_create(int capacity) {
    DigitalPheromone* dp = (DigitalPheromone*)calloc(1, sizeof(DigitalPheromone));
    if (!dp) return NULL;
    dp->capacity = capacity;
    dp->x_coords = swarm_alloc_vector(capacity);
    dp->y_coords = swarm_alloc_vector(capacity);
    dp->intensities = swarm_alloc_vector(capacity);
    dp->decay_rates = swarm_alloc_vector(capacity);
    if (!dp->x_coords || !dp->y_coords || !dp->intensities || !dp->decay_rates) {
        swarm_digital_pheromone_free(dp); return NULL;
    }
    dp->n_markers = 0;
    return dp;
}

void swarm_digital_pheromone_free(DigitalPheromone* dp) {
    if (!dp) return;
    swarm_free_vector(dp->x_coords);
    swarm_free_vector(dp->y_coords);
    swarm_free_vector(dp->intensities);
    swarm_free_vector(dp->decay_rates);
    free(dp);
}

void swarm_digital_pheromone_deposit(DigitalPheromone* dp,
           double x, double y, double intensity, double decay_rate) {
    if (!dp || dp->n_markers >= dp->capacity) return;
    int idx = dp->n_markers++;
    dp->x_coords[idx] = x;
    dp->y_coords[idx] = y;
    dp->intensities[idx] = intensity;
    dp->decay_rates[idx] = decay_rate;
}

double swarm_digital_pheromone_query(const DigitalPheromone* dp,
           double x, double y, double radius) {
    if (!dp) return 0.0;
    double total = 0.0;
    double rsq = radius * radius;
    for (int i = 0; i < dp->n_markers; i++) {
        double dx = x - dp->x_coords[i], dy = y - dp->y_coords[i];
        double dsq = dx*dx + dy*dy;
        if (dsq <= rsq) total += dp->intensities[i];
    }
    return total;
}

void swarm_digital_pheromone_decay(DigitalPheromone* dp, double dt) {
    if (!dp) return;
    for (int i = 0; i < dp->n_markers; i++)
        dp->intensities[i] *= exp(-dp->decay_rates[i] * dt);
}

int swarm_digital_pheromone_prune(DigitalPheromone* dp, double min_intensity) {
    if (!dp) return 0;
    int removed = 0;
    for (int i = 0; i < dp->n_markers; ) {
        if (dp->intensities[i] < min_intensity) {
            /* Remove by swapping with last */
            if (i < dp->n_markers - 1) {
                dp->x_coords[i] = dp->x_coords[dp->n_markers - 1];
                dp->y_coords[i] = dp->y_coords[dp->n_markers - 1];
                dp->intensities[i] = dp->intensities[dp->n_markers - 1];
                dp->decay_rates[i] = dp->decay_rates[dp->n_markers - 1];
            }
            dp->n_markers--;
            removed++;
        } else {
            i++;
        }
    }
    return removed;
}
