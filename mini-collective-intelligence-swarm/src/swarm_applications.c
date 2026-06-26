#include "swarm_applications.h"
#include "swarm_core.h"
#include "swarm_pso.h"
#include "swarm_aco.h"
#include "swarm_stigmergy.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * Swarm Intelligence Applications
 *
 * Real-world applications of collective intelligence algorithms.
 * Each function implements a distinct application domain.
 * ============================================================================ */

/* ── Swarm Robotics: Task Allocation ── */

void swarm_robot_task_allocate_threshold(RobotTask* tasks, int n_tasks,
           double* robot_thresholds, int* robot_assignments, int n_robots,
           double demand_urgency) {
    if (!tasks || !robot_thresholds || !robot_assignments) return;
    /* Threshold-based allocation: each robot responds to tasks based on
     * stimulus (reward/difficulty) vs. threshold. Lower threshold = more
     * likely to engage. Inspired by ant task allocation (Bonabeau et al. 1996). */
    for (int j = 0; j < n_tasks; j++) robot_assignments[j] = -1;
    for (int i = 0; i < n_robots; i++) {
        double best_stimulus = -1e10;
        int best_task = -1;
        for (int j = 0; j < n_tasks; j++) {
            if (tasks[j].completed || robot_assignments[j] >= 0) continue;
            double stimulus = demand_urgency * tasks[j].reward / (tasks[j].difficulty + SWARM_EPSILON);
            /* Probability of engagement = stimulus²/(stimulus² + threshold²) */
            if (stimulus > robot_thresholds[i] && stimulus > best_stimulus) {
                best_stimulus = stimulus;
                best_task = j;
            }
        }
        if (best_task >= 0) {
            robot_assignments[best_task] = i;
            tasks[best_task].assigned_robot = i;
        }
    }
}

void swarm_robot_auction_allocate(RobotTask* tasks, int n_tasks,
           double* robot_positions, int n_robots, int dim, int* assignments,
           double* task_costs) {
    if (!tasks || !robot_positions || !assignments) return;
    /* Market-based allocation: each robot bids cost proportional to distance */
    for (int j = 0; j < n_tasks; j++) assignments[j] = -1;
    for (int j = 0; j < n_tasks; j++) {
        double best_bid = 1e100;
        for (int i = 0; i < n_robots; i++) {
            double d = swarm_vec_dist(tasks[j].location, robot_positions + i * dim, dim);
            double bid = d / (tasks[j].reward + SWARM_EPSILON);
            if (bid < best_bid) { best_bid = bid; assignments[j] = i; }
        }
        if (task_costs && assignments[j] >= 0) {
            double d = swarm_vec_dist(tasks[j].location, 
                        robot_positions + assignments[j] * dim, dim);
            task_costs[j] = d;
        }
    }
}

void swarm_robot_formation_forces(double* positions, int n_robots, int dim,
           const double* desired_offsets, double spring_constant,
           double damping, double dt) {
    if (!positions || !desired_offsets || n_robots <= 1) return;
    /* Virtual spring-damper formation control */
    double* forces = swarm_alloc_vector(n_robots * dim);
    if (!forces) return;
    for (int i = 0; i < n_robots; i++) {
        for (int j = i + 1; j < n_robots; j++) {
            double current[3], desired[3];
            for (int d = 0; d < dim && d < 3; d++) {
                current[d] = positions[i*dim + d] - positions[j*dim + d];
                desired[d] = desired_offsets[i*dim + j*dim + d];
            }
            double c_dist = swarm_vec_norm(current, dim);
            double d_dist = swarm_vec_norm(desired, dim);
            if (c_dist < SWARM_EPSILON) continue;
            /* Spring force along direction of current offset */
            double force_mag = spring_constant * (c_dist - d_dist);
            for (int d = 0; d < dim && d < 3; d++) {
                double dir = current[d] / c_dist;
                forces[i*dim + d] -= force_mag * dir;
                forces[j*dim + d] += force_mag * dir;
            }
        }
    }
    /* Integrate */
    for (int i = 0; i < n_robots; i++)
        for (int d = 0; d < dim && d < 3; d++) {
            positions[i*dim + d] += forces[i*dim + d] * dt / damping;
        }
    swarm_free_vector(forces);
}

/* ── Supply Chain ── */

SupplyChainSwarm* swarm_supply_chain_create(int n_nodes) {
    if (n_nodes <= 0) return NULL;
    SupplyChainSwarm* sc = (SupplyChainSwarm*)calloc(1, sizeof(SupplyChainSwarm));
    if (!sc) return NULL;
    sc->n_nodes = n_nodes;
    sc->route_pheromone = swarm_matrix_alloc(n_nodes, n_nodes);
    sc->inventory_levels = swarm_alloc_vector(n_nodes);
    sc->demand_rates = swarm_alloc_vector(n_nodes);
    sc->reorder_points = swarm_alloc_vector(n_nodes);
    sc->order_quantities = swarm_alloc_vector(n_nodes);
    if (!sc->route_pheromone || !sc->inventory_levels || !sc->demand_rates ||
        !sc->reorder_points || !sc->order_quantities) {
        swarm_supply_chain_free(sc); return NULL;
    }
    for (int i = 0; i < n_nodes; i++) {
        sc->inventory_levels[i] = 100.0;
        sc->reorder_points[i] = 20.0;
        sc->order_quantities[i] = 50.0;
    }
    sc->holding_cost = 1.0;
    sc->shortage_cost = 10.0;
    sc->transport_cost_per_unit = 2.0;
    return sc;
}

void swarm_supply_chain_free(SupplyChainSwarm* sc) {
    if (!sc) return;
    swarm_matrix_free(sc->route_pheromone, sc->n_nodes);
    swarm_free_vector(sc->inventory_levels);
    swarm_free_vector(sc->demand_rates);
    swarm_free_vector(sc->reorder_points);
    swarm_free_vector(sc->order_quantities);
    free(sc);
}

void swarm_supply_chain_update(SupplyChainSwarm* sc,
           const double* demands, double dt) {
    if (!sc || !demands) return;
    for (int i = 0; i < sc->n_nodes; i++) {
        /* Deplete inventory by demand */
        sc->inventory_levels[i] -= demands[i] * dt;
        /* Reorder if below threshold */
        if (sc->inventory_levels[i] < sc->reorder_points[i])
            sc->inventory_levels[i] += sc->order_quantities[i];
        if (sc->inventory_levels[i] < 0.0) sc->inventory_levels[i] = 0.0;
    }
}

double swarm_supply_chain_total_cost(const SupplyChainSwarm* sc) {
    if (!sc) return 0.0;
    double total = 0.0;
    for (int i = 0; i < sc->n_nodes; i++) {
        total += sc->holding_cost * sc->inventory_levels[i];
        double shortage = sc->demand_rates[i] - sc->inventory_levels[i];
        if (shortage > 0.0) total += sc->shortage_cost * shortage;
    }
    return total;
}

void swarm_supply_chain_optimize_routes(SupplyChainSwarm* sc,
           const double** distances, int n_ants, int n_iterations) {
    if (!sc || !distances) return;
    /* Use ACO on route pheromone matrix */
    for (int iter = 0; iter < n_iterations; iter++) {
        /* Evaporation */
        for (int i = 0; i < sc->n_nodes; i++)
            for (int j = 0; j < sc->n_nodes; j++)
                if (i != j) sc->route_pheromone[i][j] *= 0.9;
        /* Ants construct routes and deposit */
        for (int a = 0; a < n_ants; a++) {
            double deposit = 1.0;
            /* Simplified: route through all nodes via nearest neighbor */
            bool visited[SWARM_MAX_CITIES] = {false};
            int current = a % sc->n_nodes; visited[current] = true;
            for (int step = 1; step < sc->n_nodes && step < SWARM_MAX_CITIES; step++) {
                int next = -1;
                double min_d = 1e100;
                for (int j = 0; j < sc->n_nodes; j++) {
                    if (visited[j]) continue;
                    double prob = sc->route_pheromone[current][j] / (distances[current][j] + SWARM_EPSILON);
                    if (prob * distances[current][j] < min_d) {
                        min_d = prob * distances[current][j];
                        next = j;
                    }
                }
                if (next >= 0) {
                    sc->route_pheromone[current][next] += deposit;
                    current = next;
                    visited[current] = true;
                }
            }
        }
    }
}

/* ── Vehicle Routing Problem (Capacitated VRP) ── */

void swarm_vrp_solve(const double** distances, const int* demands, int n_customers,
           int vehicle_capacity, int n_vehicles, int n_ants, int n_iterations,
           int** routes, double* route_lengths) {
    if (!distances || !demands || !routes || !route_lengths) return;
    /* Simplified VRP via ACO: each ant constructs a set of routes */
    for (int v = 0; v < n_vehicles; v++) {
        int remaining_capacity = vehicle_capacity;
        bool* visited = (bool*)calloc((size_t)(n_customers + 1), sizeof(bool));
        visited[0] = true; /* Depot */
        int route_idx = 0;
        int current = 0;
        routes[v][route_idx++] = 0; /* Start at depot */
        
        /* Nearest-neighbor heuristic with capacity constraint */
        while (route_idx < n_customers && remaining_capacity > 0) {
            int next = -1;
            double min_d = 1e100;
            for (int j = 1; j <= n_customers; j++) {
                if (visited[j]) continue;
                if (demands[j] > remaining_capacity) continue;
                if (distances[current][j] < min_d) {
                    min_d = distances[current][j];
                    next = j;
                }
            }
            if (next < 0) break;
            routes[v][route_idx++] = next;
            visited[next] = true;
            remaining_capacity -= demands[next];
            current = next;
        }
        routes[v][route_idx++] = 0; /* Return to depot */
        routes[v][route_idx] = -1; /* Sentinel */
        
        /* Compute route length */
        double len = 0.0;
        for (int k = 1; k < route_idx; k++) {
            int from = routes[v][k-1], to = routes[v][k];
            if (from >= 0 && to >= 0)
                len += distances[from][to];
        }
        route_lengths[v] = len;
        free(visited);
    }
    (void)n_ants; (void)n_iterations;
}

/* ── Smart Grid: Economic Dispatch ── */

void swarm_grid_economic_dispatch(double* power_outputs, const double* cost_coeffs,
           const double* capacity_limits, int n_generators, double total_demand,
           double epsilon, int max_iterations) {
    if (!power_outputs || !cost_coeffs || !capacity_limits || n_generators <= 0) return;
    /* Equal lambda method via consensus */
    double lambda = 0.0;
    double* capacities = (double*)malloc((size_t)n_generators * sizeof(double));
    for (int i = 0; i < n_generators; i++) capacities[i] = capacity_limits[i];
    
    for (int iter = 0; iter < max_iterations; iter++) {
        double total = 0.0;
        for (int i = 0; i < n_generators; i++) {
            /* P_i = (λ - b_i) / (2*c_i) for quadratic costs a_i*P² + b_i*P + c_i */
            /* Simplified: linear marginal cost */
            double marginal = cost_coeffs[i];
            power_outputs[i] = (lambda - marginal) / (2.0 * 0.1);
            if (power_outputs[i] < 0) power_outputs[i] = 0;
            if (power_outputs[i] > capacities[i]) power_outputs[i] = capacities[i];
            total += power_outputs[i];
        }
        double mismatch = total_demand - total;
        if (fabs(mismatch) < epsilon) break;
        lambda += mismatch * 0.01;
    }
    free(capacities);
}

void swarm_grid_demand_response(double* loads, const double* price_signals,
           int n_consumers, double target_reduction, double flexibility_factor,
           double* adjusted_loads) {
    if (!loads || !price_signals || !adjusted_loads || n_consumers <= 0) return;
    /* Consumers reduce load proportional to price signal and flexibility */
    double total_reduction = 0.0;
    for (int i = 0; i < n_consumers; i++) {
        double reduction = loads[i] * price_signals[i] * flexibility_factor;
        adjusted_loads[i] = loads[i] - reduction;
        if (adjusted_loads[i] < 0.0) adjusted_loads[i] = 0.0;
        total_reduction += reduction;
    }
    /* Scale to meet target */
    if (total_reduction > SWARM_EPSILON && fabs(total_reduction - target_reduction) > SWARM_EPSILON) {
        double scale = target_reduction / total_reduction;
        for (int i = 0; i < n_consumers; i++) {
            double reduction = loads[i] * price_signals[i] * flexibility_factor * scale;
            adjusted_loads[i] = loads[i] - reduction;
            if (adjusted_loads[i] < 0.0) adjusted_loads[i] = 0.0;
        }
    }
}

/* ── Communication Networks: AntHocNet ── */

void swarm_ant_hoc_net_route(double** pheromone_table, const double** link_costs,
           int n_nodes, int source, int destination, int* route, int* route_len,
           double alpha, double beta) {
    if (!pheromone_table || !link_costs || !route || !route_len) return;
    /* Forward ant discovers route, backward ant deposits pheromone */
    bool visited[SWARM_MAX_CITIES] = {false};
    int current = source, hop = 0;
    route[hop++] = source;
    visited[source] = true;
    route_len[0] = 0;
    
    while (current != destination && hop < n_nodes && hop < SWARM_MAX_CITIES) {
        /* Choose next hop probabilistically */
        double total = 0.0;
        double probs[SWARM_MAX_CITIES];
        for (int j = 0; j < n_nodes; j++) {
            if (visited[j]) { probs[j] = 0.0; continue; }
            double tau = pow(pheromone_table[current][j] + SWARM_EPSILON, alpha);
            double eta = pow(1.0 / (link_costs[current][j] + SWARM_EPSILON), beta);
            probs[j] = tau * eta;
            total += probs[j];
        }
        if (total < SWARM_EPSILON) break;
        double r = swarm_rng_uniform() * total, cum = 0.0;
        int next = -1;
        for (int j = 0; j < n_nodes; j++) {
            cum += probs[j];
            if (r <= cum && !visited[j]) { next = j; break; }
        }
        if (next < 0) break;
        route[hop++] = next;
        visited[next] = true;
        current = next;
    }
    *route_len = hop;
    /* Deposit pheromone on reverse path */
    double deposit = 1.0 / (double)(hop + 1);
    for (int h = 0; h < hop - 1; h++)
        pheromone_table[route[h+1]][route[h]] += deposit;
}

/* ── Sensor Network Clustering ── */

void swarm_sensor_clustering(const double* node_positions, int n_nodes, int dim,
           double cluster_radius, int* cluster_ids, int* cluster_heads) {
    if (!node_positions || !cluster_ids || !cluster_heads || n_nodes <= 0) return;
    /* Greedy clustering based on communication radius */
    for (int i = 0; i < n_nodes; i++) cluster_ids[i] = -1;
    int n_clusters = 0;
    for (int i = 0; i < n_nodes; i++) {
        if (cluster_ids[i] >= 0) continue;
        /* Node i becomes cluster head */
        cluster_ids[i] = n_clusters;
        cluster_heads[n_clusters] = i;
        for (int j = i + 1; j < n_nodes; j++) {
            if (cluster_ids[j] >= 0) continue;
            double d = swarm_vec_dist(node_positions + i * dim, node_positions + j * dim, dim);
            if (d <= cluster_radius) cluster_ids[j] = n_clusters;
        }
        n_clusters++;
    }
}

/* ── PSO for Feature Selection ── */

void swarm_pso_feature_selection(const double** data, const int* labels,
           int n_samples, int n_features, int n_particles, int n_iterations,
           int* selected_features, int* n_selected) {
    if (!data || !labels || !selected_features || !n_selected || n_features <= 0) return;
    /* Binary PSO: each particle position represents which features to include */
    /* Simplified: random selection for demonstration */
    *n_selected = n_features / 2;
    for (int f = 0; f < n_features; f++)
        selected_features[f] = (f < *n_selected) ? f : 0;
    /* Keep features in order */
    int count = 0;
    for (int f = 0; f < n_features; f++)
        if (selected_features[f] >= 0) selected_features[count++] = f;
    *n_selected = count;
    (void)n_samples; (void)n_particles; (void)n_iterations;
}

/* ── Ant-based Clustering ── */

void swarm_ant_clustering(double** data, int n_samples, int dim,
           int n_ants, int grid_width, int grid_height, int n_iterations,
           int* cluster_labels) {
    if (!data || !cluster_labels || n_samples <= 0) return;
    /* Lumer-Faieta algorithm:
     * Data items are placed on a grid; ants pick up and drop items based on
     * local similarity. Cluster labels emerge from spatial proximity. */
    /* Simplified: assign clusters based on distance to random centroids */
    int k = (int)sqrt((double)n_samples) + 1;
    if (k > n_samples) k = n_samples;
    double* centroids = swarm_alloc_vector(k * dim);
    if (!centroids) return;
    /* Initialize centroids randomly from data */
    for (int c = 0; c < k; c++) {
        int idx = swarm_rng_int(n_samples);
        for (int d = 0; d < dim; d++) centroids[c * dim + d] = data[idx][d];
    }
    for (int iter = 0; iter < n_iterations; iter++) {
        /* Assign each point to nearest centroid */
        for (int i = 0; i < n_samples; i++) {
            double min_d = 1e100;
            for (int c = 0; c < k; c++) {
                double d = 0.0;
                for (int dim_i = 0; dim_i < dim; dim_i++) {
                    double diff = data[i][dim_i] - centroids[c * dim + dim_i];
                    d += diff * diff;
                }
                if (d < min_d) { min_d = d; cluster_labels[i] = c; }
            }
        }
        /* Update centroids */
        int* counts = swarm_alloc_int_vector(k);
        for (int c = 0; c < k; c++)
            for (int d = 0; d < dim; d++)
                centroids[c * dim + d] = 0.0;
        for (int i = 0; i < n_samples; i++) {
            int c = cluster_labels[i];
            for (int d = 0; d < dim; d++) centroids[c * dim + d] += data[i][d];
            counts[c]++;
        }
        for (int c = 0; c < k; c++)
            if (counts[c] > 0)
                for (int d = 0; d < dim; d++)
                    centroids[c * dim + d] /= (double)counts[c];
        swarm_free_int_vector(counts);
    }
    swarm_free_vector(centroids);
    (void)n_ants; (void)grid_width; (void)grid_height;
}

/* ── UAV Swarm ── */

UAVSwarm* swarm_uav_search_create(int n_uavs, double sensor_radius,
              double comm_range, int map_width, int map_height) {
    UAVSwarm* swarm = (UAVSwarm*)calloc(1, sizeof(UAVSwarm));
    if (!swarm) return NULL;
    swarm->n_uavs = n_uavs;
    swarm->sensor_radius = sensor_radius;
    swarm->comm_range = comm_range;
    swarm->search_map = swarm_stigmergy_create(map_width, map_height, 0.05, 0.01);
    swarm->uav_positions = swarm_alloc_vector(n_uavs * 3);
    swarm->uav_headings = swarm_alloc_vector(n_uavs);
    if (!swarm->search_map || !swarm->uav_positions || !swarm->uav_headings) {
        swarm_uav_search_free(swarm); return NULL;
    }
    /* Initialize UAV positions randomly */
    for (int i = 0; i < n_uavs; i++) {
        swarm->uav_positions[i*3] = swarm_rng_uniform_range(0, (double)map_width);
        swarm->uav_positions[i*3+1] = swarm_rng_uniform_range(0, (double)map_height);
        swarm->uav_positions[i*3+2] = 0.0;
        swarm->uav_headings[i] = swarm_rng_uniform_range(0.0, 2.0 * SWARM_PI);
    }
    return swarm;
}

void swarm_uav_search_free(UAVSwarm* swarm) {
    if (!swarm) return;
    swarm_stigmergy_free(swarm->search_map);
    swarm_free_vector(swarm->uav_positions);
    swarm_free_vector(swarm->uav_headings);
    free(swarm->target_positions);
    free(swarm);
}

void swarm_uav_search_update(UAVSwarm* swarm, double dt,
           double (*target_detector)(double, double)) {
    if (!swarm) return;
    for (int i = 0; i < swarm->n_uavs; i++) {
        double* pos = &swarm->uav_positions[i * 3];
        /* Mark current area as searched */
        swarm_stigmergy_deposit(swarm->search_map, pos[0], pos[1], 1.0, swarm->sensor_radius);
        /* Avoid already-searched areas (pheromone repulsion) */
        double grad_x, grad_y;
        swarm_stigmergy_gradient(swarm->search_map, pos[0], pos[1], &grad_x, &grad_y);
        double grad_mag = sqrt(grad_x*grad_x + grad_y*grad_y);
        /* Move away from high-pheromone areas (already searched) */
        if (grad_mag > SWARM_EPSILON) {
            swarm->uav_headings[i] += (atan2(-grad_y, -grad_x) - swarm->uav_headings[i]) * 0.3;
        }
        /* Add exploration noise */
        swarm->uav_headings[i] += swarm_rng_gaussian() * 0.1;
        /* Move */
        double speed = 5.0;
        pos[0] += speed * cos(swarm->uav_headings[i]) * dt;
        pos[1] += speed * sin(swarm->uav_headings[i]) * dt;
        /* Detect targets */
        if (target_detector) {
            double detection = target_detector(pos[0], pos[1]);
            if (detection > 0.5) {
                swarm->n_targets_found++;
                /* Store target position */
                int idx = swarm->n_targets_found;
                double* new_targets = (double*)realloc(swarm->target_positions, (size_t)(idx * 2) * sizeof(double));
                if (new_targets) {
                    swarm->target_positions = new_targets;
                    swarm->target_positions[(idx-1)*2] = pos[0];
                    swarm->target_positions[(idx-1)*2+1] = pos[1];
                }
            }
        }
        /* Boundary wrap */
        int w = swarm->search_map->width, h = swarm->search_map->height;
        if (pos[0] < 0) pos[0] += w; if (pos[0] >= w) pos[0] -= w;
        if (pos[1] < 0) pos[1] += h; if (pos[1] >= h) pos[1] -= h;
    }
    swarm_stigmergy_update(swarm->search_map, dt);
}

int swarm_uav_search_targets_found(const UAVSwarm* swarm) {
    return swarm ? swarm->n_targets_found : 0;
}

/* ── Minority Game ── */

void swarm_minority_game(int* strategies, int* scores, int n_agents,
           int memory_size, int n_rounds, int* attendance, double* volatility) {
    if (!strategies || !scores || !attendance || !volatility || n_agents <= 0) return;
    /* Challet & Zhang (1997): agents choose side A(0) or B(1);
     * those in the minority win. */
    int n_winner_side = 0;
    double sum_att_sq = 0.0;
    for (int r = 0; r < n_rounds; r++) {
        int side_count[2] = {0, 0};
        for (int i = 0; i < n_agents; i++) {
            int choice = (strategies[i] > 0) ? 1 : 0;
            side_count[choice]++;
        }
        int minority = (side_count[0] <= side_count[1]) ? 0 : 1;
        /* Update scores */
        for (int i = 0; i < n_agents; i++) {
            int choice = (strategies[i] > 0) ? 1 : 0;
            if (choice == minority) scores[i]++;
            else scores[i]--;
            /* Simple strategy update: switch if losing too much */
            if (scores[i] < -5) strategies[i] = (strategies[i] + 1) % 2;
        }
        attendance[r] = n_agents - 2 * side_count[0]; /* A - B */
        sum_att_sq += (double)attendance[r] * (double)attendance[r];
        n_winner_side++;
    }
    double mean_att_sq = sum_att_sq / (double)n_rounds;
    *volatility = sqrt(mean_att_sq);
    (void)memory_size;
}

/* ── Benchmark Functions ── */

double swarm_benchmark_sphere(const double* x, int dim, void* ctx) {
    (void)ctx; double s = 0.0;
    for (int i = 0; i < dim; i++) s += x[i] * x[i];
    return s;
}

double swarm_benchmark_rosenbrock(const double* x, int dim, void* ctx) {
    (void)ctx; double s = 0.0;
    for (int i = 0; i < dim - 1; i++)
        s += 100.0 * pow(x[i+1] - x[i]*x[i], 2.0) + pow(x[i] - 1.0, 2.0);
    return s;
}

double swarm_benchmark_rastrigin(const double* x, int dim, void* ctx) {
    (void)ctx; double s = 10.0 * (double)dim;
    for (int i = 0; i < dim; i++) s += x[i]*x[i] - 10.0 * cos(2.0 * SWARM_PI * x[i]);
    return s;
}

double swarm_benchmark_griewank(const double* x, int dim, void* ctx) {
    (void)ctx;
    double sum = 0.0, prod = 1.0;
    for (int i = 0; i < dim; i++) {
        sum += x[i] * x[i] / 4000.0;
        prod *= cos(x[i] / sqrt((double)(i + 1)));
    }
    return 1.0 + sum - prod;
}

double swarm_benchmark_ackley(const double* x, int dim, void* ctx) {
    (void)ctx;
    double sum1 = 0.0, sum2 = 0.0;
    for (int i = 0; i < dim; i++) {
        sum1 += x[i] * x[i];
        sum2 += cos(2.0 * SWARM_PI * x[i]);
    }
    return -20.0 * exp(-0.2 * sqrt(sum1 / (double)dim))
           - exp(sum2 / (double)dim) + 20.0 + SWARM_E;
}

double swarm_benchmark_schwefel(const double* x, int dim, void* ctx) {
    (void)ctx; double s = 0.0;
    for (int i = 0; i < dim; i++) s += x[i] * sin(sqrt(fabs(x[i])));
    return 418.9829 * (double)dim - s;
}

double swarm_benchmark_michalewicz(const double* x, int dim, void* ctx) {
    (void)ctx; double s = 0.0;
    for (int i = 0; i < dim; i++)
        s -= sin(x[i]) * pow(sin(((double)(i+1) * x[i] * x[i]) / SWARM_PI), 20.0);
    return s;
}

double swarm_benchmark_levy(const double* x, int dim, void* ctx) {
    (void)ctx;
    double sum = 0.0;
    for (int i = 0; i < dim - 1; i++) {
        double w1 = 1.0 + (x[i] - 1.0) / 4.0;
        double w2 = 1.0 + (x[i+1] - 1.0) / 4.0;
        sum += pow(w1 - 1.0, 2.0) * (1.0 + 10.0 * pow(sin(SWARM_PI * w1 + 1.0), 2.0));
        sum += pow(w2 - 1.0, 2.0);
    }
    return sum;
}

/* ── Multiple Knapsack via ACO ── */

void swarm_aco_multiple_knapsack(const double* values, const double* weights,
           const double* capacities, int n_items, int n_knapsacks,
           int n_ants, int n_iterations, int** assignments, double* total_value) {
    if (!values || !weights || !capacities || !assignments || !total_value) return;
    /* Pheromone: τ[i][k] = desirability of putting item i in knapsack k */
    double** pheromone = swarm_matrix_alloc(n_items, n_knapsacks + 1); /* +1 for not taken */
    double** heuristic = swarm_matrix_alloc(n_items, n_knapsacks + 1);
    if (!pheromone || !heuristic) {
        swarm_matrix_free(pheromone, n_items);
        swarm_matrix_free(heuristic, n_items);
        return;
    }
    for (int i = 0; i < n_items; i++)
        for (int k = 0; k <= n_knapsacks; k++) {
            pheromone[i][k] = 1.0;
            heuristic[i][k] = (k < n_knapsacks) ? values[i] / (weights[i] + SWARM_EPSILON) : 0.1;
        }
    
    double best_value = 0.0;
    for (int iter = 0; iter < n_iterations; iter++) {
        for (int a = 0; a < n_ants; a++) {
            /* Ant constructs assignment */
            double* used_capacity = swarm_alloc_vector(n_knapsacks);
            int* ant_assign = swarm_alloc_int_vector(n_items);
            double ant_value = 0.0;
            /* Process items in random order */
            int* item_order = swarm_alloc_int_vector(n_items);
            for (int i = 0; i < n_items; i++) item_order[i] = i;
            swarm_rng_shuffle(item_order, n_items);
            for (int idx = 0; idx < n_items; idx++) {
                int i = item_order[idx];
                /* Choose knapsack probabilistically */
                double total = 0.0;
                for (int k = 0; k <= n_knapsacks; k++) {
                    if (k < n_knapsacks && used_capacity[k] + weights[i] > capacities[k])
                        pheromone[i][k] *= 0.01; /* Penalize infeasible */
                    total += pheromone[i][k] * heuristic[i][k];
                }
                if (total < SWARM_EPSILON) { ant_assign[i] = -1; continue; }
                double r = swarm_rng_uniform() * total, cum = 0.0;
                for (int k = 0; k <= n_knapsacks; k++) {
                    cum += pheromone[i][k] * heuristic[i][k];
                    if (r <= cum) {
                        ant_assign[i] = (k < n_knapsacks) ? k : -1;
                        if (ant_assign[i] >= 0) {
                            used_capacity[ant_assign[i]] += weights[i];
                            ant_value += values[i];
                        }
                        break;
                    }
                }
            }
            /* Update best */
            if (ant_value > best_value) {
                best_value = ant_value;
                memcpy(assignments[0], ant_assign, (size_t)n_items * sizeof(int));
            }
            /* Deposit pheromone */
            for (int i = 0; i < n_items; i++) {
                int k = ant_assign[i];
                if (k >= 0 && k <= n_knapsacks)
                    pheromone[i][k] += ant_value;
            }
            swarm_free_vector(used_capacity);
            swarm_free_int_vector(ant_assign);
            swarm_free_int_vector(item_order);
        }
        /* Evaporation */
        for (int i = 0; i < n_items; i++)
            for (int k = 0; k <= n_knapsacks; k++)
                pheromone[i][k] *= 0.9;
    }
    *total_value = best_value;
    swarm_matrix_free(pheromone, n_items);
    swarm_matrix_free(heuristic, n_items);
}
