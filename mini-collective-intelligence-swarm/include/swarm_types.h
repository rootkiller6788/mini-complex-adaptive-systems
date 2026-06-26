#ifndef SWARM_TYPES_H
#define SWARM_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Swarm Types — Collective Intelligence & Swarm Intelligence
 *
 * Based on:
 *   Bonabeau, Dorigo & Theraulaz (1999) — Swarm Intelligence: From Natural to Artificial Systems
 *   Kennedy & Eberhart (1995) — Particle Swarm Optimization
 *   Dorigo & Stützle (2004) — Ant Colony Optimization
 *   Reynolds (1987) — Flocks, Herds, and Schools: A Distributed Behavioral Model
 *   Camazine et al. (2001) — Self-Organization in Biological Systems
 *   Couzin et al. (2005) — Effective Leadership and Decision-Making in Animal Groups
 *   Olfati-Saber, Fax & Murray (2007) — Consensus and Cooperation in Networked Multi-Agent Systems
 *
 * Collective intelligence emerges when groups of simple agents, following local
 * rules, produce globally intelligent behavior. Key mechanisms:
 *
 *   Stigmergy: indirect coordination via environment modification (pheromones)
 *   Positive feedback: amplification of successful behaviors
 *   Negative feedback: stabilization and exploration
 *   Multiple interactions: many agents communicate locally
 *   Self-organization: global patterns from local rules without central control
 *
 * Swarm Intelligence = the computational paradigm inspired by social insects
 * and animal collectives. Core algorithms: PSO, ACO, ABC (Artificial Bee Colony).
 * ============================================================================ */

/* ── Maximum Configuration Constants ── */
#define SWARM_MAX_AGENTS       1024    /* Maximum agents in a swarm            */
#define SWARM_MAX_DIMENSIONS     64    /* Maximum search space dimensions       */
#define SWARM_MAX_NEIGHBORS      32    /* Maximum neighbors per agent           */
#define SWARM_MAX_CITIES        512    /* Maximum cities for TSP (ACO)          */
#define SWARM_MAX_ITERATIONS  10000    /* Maximum iterations                    */

/* ── Swarm Agent: the fundamental unit of collective computation ──
 *
 * Each agent operates on local information only — it does not have global
 * knowledge of the swarm state. This locality is what makes swarm intelligence
 * scalable, robust, and decentralized.
 *
 * position: current location in search space (D-dimensional)
 * velocity: current velocity vector (PSO) or direction of motion
 * fitness: quality of current position (lower = better for minimization)
 * best_position: personal best position found (PSO)
 * best_fitness: fitness at personal best
 * id: unique agent identifier
 * state: behavioral state (exploring, exploiting, recruiting, etc.)
 * neighbors[]: indices of neighbor agents (topology-dependent)
 * neighbor_count: number of neighbors
 */
typedef struct {
    double*  position;           /* Current position [dim]                     */
    double*  velocity;           /* Current velocity [dim]                     */
    double*  best_position;      /* Personal best position [dim]               */
    double   fitness;            /* Current fitness value                      */
    double   best_fitness;       /* Personal best fitness                      */
    int      id;                 /* Unique agent ID                            */
    int      dim;                /* Problem dimensionality                     */
    int      neighbors[SWARM_MAX_NEIGHBORS]; /* Neighbor indices               */
    int      neighbor_count;     /* Active neighbor count                      */
    uint8_t  state;              /* Behavioral state                           */
    bool     is_active;          /* Agent currently active?                    */
} SwarmAgent;

/* Agent behavioral states */
#define SWARM_STATE_EXPLORING    0   /* Wide search, high velocity              */
#define SWARM_STATE_EXPLOITING   1   /* Local refinement, low velocity          */
#define SWARM_STATE_RECRUITING   2   /* Broadcasting good solution              */
#define SWARM_STATE_RESTING      3   /* Inactive / waiting                     */
#define SWARM_STATE_DEAD         4   /* Removed from population                */

/* ── Neighbor Topology: defines who communicates with whom ──
 *
 * GLOBAL (gbest): every agent is neighbor to every other. Fast convergence
 *   but prone to premature convergence (loses diversity).
 * RING: each agent connected to k nearest indices. Slower convergence but
 *   maintains diversity longer (less prone to local optima).
 * VON_NEUMANN: 2D grid with 4 neighbors (up, down, left, right).
 *   Moderate information flow.
 * RANDOM: each agent has k randomly selected neighbors, rewired periodically.
 *   Balances exploration and exploitation (small-world property).
 * STAR: one hub agent connected to all; peripheral agents only to hub.
 *   Centralized influence, hub acts as leader.
 * DYNAMIC: neighbors change based on distance/performance.
 */
typedef enum {
    SWARM_TOPOLOGY_GLOBAL       = 0,
    SWARM_TOPOLOGY_RING         = 1,
    SWARM_TOPOLOGY_VON_NEUMANN  = 2,
    SWARM_TOPOLOGY_RANDOM       = 3,
    SWARM_TOPOLOGY_STAR         = 4,
    SWARM_TOPOLOGY_DYNAMIC      = 5
} SwarmTopology;

/* ── Pheromone structure (for ACO / stigmergy) ──
 *
 * Pheromones encode collective memory in the environment. Agents deposit
 * pheromone on paths/solutions they traverse, and other agents are biased
 * toward higher pheromone concentrations. Evaporation prevents stagnation.
 *
 * tau[i][j]: pheromone level on edge (i,j) or at site (i,j)
 * heuristic[i][j]: a priori desirability (distance^-1 for TSP)
 * rows, cols: dimensions of pheromone matrix
 */
typedef struct {
    double** tau;               /* Pheromone concentration matrix              */
    double** heuristic;         /* Heuristic information matrix                */
    int      rows;              /* Number of rows (cities for TSP)             */
    int      cols;              /* Number of columns                           */
    double   evaporation_rate;  /* ρ ∈ (0,1): evaporation per step             */
    double   deposit_amount;    /* Q: pheromone deposited per ant              */
    double   tau_min;           /* Lower bound (MMAS)                          */
    double   tau_max;           /* Upper bound (MMAS)                          */
    double   alpha;             /* Pheromone weight exponent                   */
    double   beta;              /* Heuristic weight exponent                   */
} PheromoneMatrix;

/* ── Swarm Configuration: global parameters governing the swarm ──
 *
 * n_agents: number of agents in the swarm
 * dimensions: search space dimensionality
 * topology: neighbor topology type
 * topology_param: topology-specific parameter (e.g., ring size k)
 * inertia_weight: ω — controls momentum (PSO)
 * cognitive_weight: c1 — attraction to personal best (PSO)
 * social_weight: c2 — attraction to global/neighborhood best (PSO)
 * max_velocity: v_max — velocity clamping bound
 * bounds_low[bounds_high]: search space boundaries per dimension
 * fitness_function: pointer to objective function f(x) -> R
 * minimize: true = minimize, false = maximize
 * seed: RNG seed for reproducibility
 */
typedef struct {
    int      n_agents;          /* Number of agents                           */
    int      dimensions;        /* Problem dimensionality                     */
    SwarmTopology topology;     /* Neighbor topology                          */
    int      topology_param;    /* Topology parameter (k)                     */
    
    /* PSO-specific parameters */
    double   inertia_weight;    /* ω: inertia / momentum                      */
    double   cognitive_weight;  /* c1: personal best weight                   */
    double   social_weight;     /* c2: social best weight                     */
    double   max_velocity;      /* v_max for velocity clamping                */
    bool     use_constriction;  /* Use Clerc-Kennedy constriction?            */
    
    /* ACO-specific parameters */
    double   evaporation_rate;  /* ρ: pheromone evaporation                   */
    double   alpha;             /* Pheromone importance                       */
    double   beta;              /* Heuristic importance                       */
    double   q0;                /* Exploitation probability threshold         */
    
    /* Search space bounds */
    double*  bounds_low;        /* Lower bounds [dim]                         */
    double*  bounds_high;       /* Upper bounds [dim]                         */
    
    /* Fitness function */
    double (*fitness_function)(const double* position, int dim, void* context);
    void*    fitness_context;   /* User-provided context data                 */
    bool     minimize;          /* Minimize (true) or maximize (false)        */
    
    /* Execution */
    int      max_iterations;    /* Maximum iterations                         */
    double   target_fitness;    /* Stop if reached                            */
    bool     verbose;           /* Print progress                             */
    uint64_t seed;              /* RNG seed                                   */
} SwarmConfig;

/* ── Swarm Result: output of a swarm optimization run ──
 *
 * best_position: best solution found
 * best_fitness: fitness of best solution
 * convergence_iteration: iteration where convergence was reached
 * history: best fitness per iteration (length = iterations_run)
 * iterations_run: actual number of iterations executed
 * n_evaluations: total function evaluations
 * diversity: average pairwise distance at end (population diversity measure)
 */
typedef struct {
    double*  best_position;     /* Best solution [dim]                         */
    double   best_fitness;      /* Best fitness value                          */
    int      convergence_iter;  /* Iteration of convergence                    */
    double*  history;           /* Best fitness history [iters]                */
    int      iterations_run;    /* Actual iterations                           */
    int      n_evaluations;     /* Total fitness evaluations                   */
    double   diversity;         /* Final population diversity                  */
    bool     converged;         /* Did the swarm converge?                     */
    double   time_seconds;      /* Wall-clock time                             */
} SwarmResult;

/* ── Boid (Bird-oid): Reynolds flocking agent ──
 *
 * Reynolds' three rules of flocking (1987):
 *   1. Separation: steer to avoid crowding local flockmates
 *   2. Alignment: steer toward the average heading of local flockmates
 *   3. Cohesion: steer toward the average position of local flockmates
 *
 * Each rule produces a steering vector; the weighted sum determines
 * the boid's acceleration. Only neighbors within perception_radius are considered.
 */
typedef struct {
    double position[3];         /* Position (x, y, z) in world space          */
    double velocity[3];         /* Velocity vector                            */
    double acceleration[3];     /* Accumulated steering force                 */
    double max_speed;           /* Speed limit                                 */
    double max_force;           /* Steering force limit                       */
    double perception_radius;   /* Visual range for neighbors                  */
    double separation_weight;   /* Weight for separation rule                  */
    double alignment_weight;    /* Weight for alignment rule                   */
    double cohesion_weight;     /* Weight for cohesion rule                    */
    int    id;                  /* Boid identifier                             */
    bool   is_predator;         /* Predator mode (chases boids)                */
    bool   is_leader;           /* Leader with goal bias                       */
    double goal[3];             /* Goal position (for leader)                  */
} Boid;

/* ── Flock Configuration ── */
typedef struct {
    int    n_boids;             /* Number of boids                             */
    double world_size;          /* World bounding box half-width               */
    double max_speed;           /* Maximum speed                               */
    double max_force;           /* Maximum steering force                      */
    double perception_radius;   /* Perception radius                           */
    double separation_weight;   /* Separation rule weight                      */
    double alignment_weight;    /* Alignment rule weight                       */
    double cohesion_weight;     /* Cohesion rule weight                        */
    double separation_distance; /* Minimum desired separation                  */
    double dt;                  /* Time step for simulation                    */
    bool   wrap_around;         /* Toroidal world?                             */
    bool   has_predator;        /* Include predator?                           */
} FlockConfig;

/* ── Consensus State for Multi-Agent Agreement ──
 *
 * Models distributed consensus where agents iteratively update their
 * local value to reach agreement without a central coordinator.
 *
 * Consensus iteration: x_i(t+1) = Σ_j w_{ij} x_j(t)
 * where W = [w_{ij}] is the weight matrix (row-stochastic).
 *
 * Applications: distributed averaging, clock synchronization, flock heading,
 * sensor fusion, distributed optimization.
 */
typedef struct {
    double*  values;            /* Agent values [n_agents]                     */
    double*  values_new;        /* Next iteration values [n_agents]            */
    double** weights;           /* Weight matrix [n_agents][n_agents]          */
    int      n_agents;          /* Number of agents                            */
    double   epsilon;           /* Convergence threshold                       */
    bool     converged;         /* Consensus reached?                          */
    int      iterations;        /* Iterations to convergence                   */
    double   disagreement;      /* max|x_i - x_j|                              */
    double   average;           /* Average value (for averaging consensus)     */
} ConsensusState;

/* ── Stigmergy Environment ──
 *
 * A shared environment that agents modify, creating feedback loops.
 * Key to ant colony behavior: ants lay pheromone trails that guide
 * subsequent ants. The environment becomes a distributed memory.
 *
 * cells[r][c]: pheromone concentration at each cell
 * gradient[r][c][2]: ∇ pheromone (dx, dy) for chemotaxis
 * width, height: grid dimensions
 * diffusion_rate: spatial pheromone spread
 * decay_rate: temporal pheromone decay
 */
typedef struct {
    double** cells;             /* Pheromone grid [height][width]              */
    double** gradient_x;        /* Gradient x-component [height][width]        */
    double** gradient_y;        /* Gradient y-component [height][width]        */
    int      width;             /* Grid width                                  */
    int      height;            /* Grid height                                 */
    double   diffusion_rate;    /* Spatial diffusion coefficient               */
    double   decay_rate;        /* Temporal decay rate                         */
    double   deposit_threshold; /* Min pheromone to trigger following          */
    bool     toroidal;          /* Wrap-around boundaries?                     */
} StigmergyEnvironment;

/* ── Multi-Swarm Configuration for cooperative coevolution ──
 *
 * Multiple sub-swarms each optimize a subset of dimensions or different
 * objectives, sharing information through a blackboard mechanism.
 * Used for large-scale optimization (CCPSO) and multi-objective problems.
 */
typedef struct {
    int      n_subswarms;       /* Number of sub-swarms                        */
    int*     dims_per_swarm;    /* Dimensions assigned to each sub-swarm       */
    double** global_best;       /* Shared best solution pool                   */
    int      n_global_best;     /* Number of solutions in pool                 */
    double   exchange_rate;     /* Probability of inter-swarm communication   */
    int      exchange_interval; /* Iterations between exchanges                */
    bool     competitive;       /* Competitive (vs cooperative) coevolution    */
} MultiSwarmConfig;

/* ── Fitness Landscape Characterization ──
 *
 * Analyzes the structure of the fitness landscape to diagnose
 * problem difficulty and guide algorithm parameter selection.
 *
 * rugosity: landscape roughness (high rugosity = many local optima)
 * correlation_length: distance over which fitness values remain correlated
 * basins_count: estimated number of attraction basins
 * neutrality_ratio: fraction of neutral (flat) regions
 * deceptiveness: degree to which gradient leads away from global optimum
 */
typedef struct {
    double rugosity;            /* Landscape roughness measure                 */
    double correlation_length;  /* Fitness-distance correlation length         */
    int    basins_count;        /* Estimated number of local optima basins     */
    double neutrality_ratio;    /* Fraction of neutral mutations               */
    double deceptiveness;       /* Deceptiveness index [0,1]                   */
    double fitness_variance;    /* Variance of sampled fitness values          */
    int    n_local_optima;      /* Number of local optima found                */
    double* local_optima_values;/* Fitness values of local optima             */
} FitnessLandscape;

/* ── Convergence Diagnostics ──
 *
 * Monitors swarm behavior to detect convergence, stagnation, or
 * premature convergence, enabling adaptive parameter control.
 */
typedef struct {
    double   swarm_radius;      /* Max distance from swarm center              */
    double   velocity_mean;     /* Mean velocity magnitude                     */
    double   velocity_variance; /* Velocity variance                           */
    double   fitness_improvement_rate; /* Δ best fitness / iteration           */
    bool     is_stagnant;       /* No improvement for threshold iterations?    */
    bool     is_premature;      /* Converged to non-global optimum?            */
    bool     has_diverged;      /* Agents outside bounds?                      */
    double   exploration_ratio; /* Fraction of agents exploring vs exploiting  */
    int      stagnation_count;  /* Consecutive iterations without improvement  */
    double   entropy;           /* Positional entropy (diversity measure)      */
} ConvergenceDiagnostics;

/* ── Agent-Based Model Configuration ──
 *
 * General framework for agent-based modeling where heterogeneous agents
 * interact according to behavioral rules in a spatial environment.
 */
typedef struct {
    int      n_agents;          /* Total agents                                */
    int      n_types;           /* Number of agent types                       */
    int*     type_counts;       /* Count per type [n_types]                    */
    double   interaction_radius;/* Interaction range                           */
    double   noise_level;       /* Behavioral stochasticity                    */
    bool     spatial_grid;      /* Use spatial grid for neighbor lookup       */
    int      grid_width;        /* Grid dimensions (if spatial)                */
    int      grid_height;
    double   dt;                /* Time step                                    */
    int      max_steps;         /* Maximum simulation steps                    */
} ABMConfig;

#endif /* SWARM_TYPES_H */
