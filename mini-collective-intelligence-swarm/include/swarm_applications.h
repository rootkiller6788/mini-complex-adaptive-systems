#ifndef SWARM_APPLICATIONS_H
#define SWARM_APPLICATIONS_H

#include "swarm_types.h"

/* ============================================================================
 * Swarm Intelligence Applications
 *
 * Real-world deployments of collective intelligence algorithms across
 * robotics, logistics, networks, and engineering optimization.
 *
 * Bonabeau & Meyer (2001): "Swarm Intelligence: A Whole New Way to Think
 *   About Business" — Harvard Business Review.
 *
 * Kube & Zhang (1993): "Collective Robotics: From Social Insects to Robots"
 *   — First application of swarm principles to multi-robot systems.
 *
 * Dorigo et al. (2004): "Swarm Robotics" — Special issue of Autonomous Robots.
 * ============================================================================ */

/* ── Swarm Robotics ── */

/* Multi-robot task allocation (market-based) */
typedef struct {
    int     task_id;
    double* location;         /* Task location [dim]                            */
    double  reward;           /* Reward for completing task                     */
    double  difficulty;       /* Task difficulty [0,1]                          */
    double  deadline;         /* Time by which task must be done                */
    bool    completed;
    int     assigned_robot;
    bool    requires_collaboration; /* Multiple robots needed?                  */
    int     min_robots;       /* Minimum robots for collaboration               */
} RobotTask;

/* Task allocation via threshold-based response (inspired by ants) */
void swarm_robot_task_allocate_threshold(RobotTask* tasks, int n_tasks,
           double* robot_thresholds, int* robot_assignments, int n_robots,
           double demand_urgency);

/* Market-based auction allocation */
void swarm_robot_auction_allocate(RobotTask* tasks, int n_tasks,
           double* robot_positions, int n_robots, int dim, int* assignments,
           double* task_costs);

/* Formation control via virtual forces */
void swarm_robot_formation_forces(double* positions, int n_robots, int dim,
           const double* desired_offsets, double spring_constant, 
           double damping, double dt);

/* ── Supply Chain & Logistics ── */

/* Inventory management with ant-inspired routing */
typedef struct {
    double** route_pheromone;  /* Desirability of routes */
    double*  inventory_levels; /* Current stock levels */
    double*  demand_rates;     /* Demand per node */
    double*  reorder_points;   /* Reorder threshold */
    double*  order_quantities; /* Economic order quantity */
    int      n_nodes;
    double   holding_cost;
    double   shortage_cost;
    double   transport_cost_per_unit;
} SupplyChainSwarm;

SupplyChainSwarm* swarm_supply_chain_create(int n_nodes);
void              swarm_supply_chain_free(SupplyChainSwarm* sc);
void              swarm_supply_chain_update(SupplyChainSwarm* sc, 
                      const double* demands, double dt);
double            swarm_supply_chain_total_cost(const SupplyChainSwarm* sc);
void              swarm_supply_chain_optimize_routes(SupplyChainSwarm* sc, 
                      const double** distances, int n_ants, int n_iterations);

/* Vehicle routing with ACO (capacitated VRP) */
void swarm_vrp_solve(const double** distances, const int* demands, int n_customers,
           int vehicle_capacity, int n_vehicles, int n_ants, int n_iterations,
           int** routes, double* route_lengths);

/* ── Smart Grid / Energy Systems ── */

/* Distributed economic dispatch via consensus */
void swarm_grid_economic_dispatch(double* power_outputs, const double* cost_coeffs,
           const double* capacity_limits, int n_generators, double total_demand,
           double epsilon, int max_iterations);

/* Demand response coordination */
void swarm_grid_demand_response(double* loads, const double* price_signals,
           int n_consumers, double target_reduction, double flexibility_factor,
           double* adjusted_loads);

/* ── Communication Networks ── */

/* AntHocNet: ACO-based routing for mobile ad-hoc networks (MANETs) */
void swarm_ant_hoc_net_route(double** pheromone_table, const double** link_costs,
           int n_nodes, int source, int destination, int* route, int* route_len,
           double alpha, double beta);

/* Distributed clustering of sensor nodes */
void swarm_sensor_clustering(const double* node_positions, int n_nodes, int dim,
           double cluster_radius, int* cluster_ids, int* cluster_heads);

/* ── Data Mining / Machine Learning ── */

/* PSO for feature selection */
void swarm_pso_feature_selection(const double** data, const int* labels,
           int n_samples, int n_features, int n_particles, int n_iterations,
           int* selected_features, int* n_selected);

/* Ant-based clustering (Lumer & Faieta 1994) */
void swarm_ant_clustering(double** data, int n_samples, int dim,
           int n_ants, int grid_width, int grid_height, int n_iterations,
           int* cluster_labels);

/* ── Aerospace / UAV Swarms ── */

/* UAV search and surveillance with pheromone maps */
typedef struct {
    StigmergyEnvironment* search_map;  /* Pheromone map for covered areas */
    double* uav_positions;             /* UAV positions [n_uavs * 3] */
    double* uav_headings;
    double  sensor_radius;
    double  comm_range;
    int     n_uavs;
    int     n_targets_found;
    double* target_positions;          /* Discovered target positions */
} UAVSwarm;

UAVSwarm* swarm_uav_search_create(int n_uavs, double sensor_radius, 
              double comm_range, int map_width, int map_height);
void      swarm_uav_search_free(UAVSwarm* swarm);
void      swarm_uav_search_update(UAVSwarm* swarm, double dt,
              double (*target_detector)(double, double));
int       swarm_uav_search_targets_found(const UAVSwarm* swarm);

/* ── Finance / Market Models ── */

/* Minority game (Challet & Zhang 1997): agents choose between two sides */
void swarm_minority_game(int* strategies, int* scores, int n_agents, 
           int memory_size, int n_rounds, int* attendance, double* volatility);

/* ── Bio-inspired Optimization Benchmarking ── */

/* Standard benchmark functions for swarm optimization */
double swarm_benchmark_sphere(const double* x, int dim, void* ctx);
double swarm_benchmark_rosenbrock(const double* x, int dim, void* ctx);
double swarm_benchmark_rastrigin(const double* x, int dim, void* ctx);
double swarm_benchmark_griewank(const double* x, int dim, void* ctx);
double swarm_benchmark_ackley(const double* x, int dim, void* ctx);
double swarm_benchmark_schwefel(const double* x, int dim, void* ctx);
double swarm_benchmark_michalewicz(const double* x, int dim, void* ctx);
double swarm_benchmark_levy(const double* x, int dim, void* ctx);

/* Multiple knapsack via ACO */
void swarm_aco_multiple_knapsack(const double* values, const double* weights,
           const double* capacities, int n_items, int n_knapsacks,
           int n_ants, int n_iterations, int** assignments, double* total_value);

#endif /* SWARM_APPLICATIONS_H */
