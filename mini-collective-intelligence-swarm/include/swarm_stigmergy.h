#ifndef SWARM_STIGMERGY_H
#define SWARM_STIGMERGY_H

#include "swarm_types.h"

/* ============================================================================
 * Stigmergy & Self-Organization
 *
 * Grassé (1959): "La Reconstruction du Nid et les Coordinations Interindividuelles
 *   chez Bellicositermes Natalensis et Cubitermes sp." — Coined "stigmergy"
 *   (stigma = sign, ergon = work): coordination via environmental traces.
 *
 * Deneubourg et al. (1990): "The Blind Leading the Blind: Modeling Chemically
 *   Mediated Army Ant Raid Patterns" — Threshold-based trail following.
 *
 * Theraulaz & Bonabeau (1999): "A Brief History of Stigmergy"
 *
 * Camazine et al. (2001): "Self-Organization in Biological Systems"
 *
 * Stigmergy is indirect coordination: agents modify the environment, and the
 * modified environment influences subsequent agent behavior. Unlike direct
 * communication, stigmergy is:
 *   - Asynchronous (no need for simultaneous presence)
 *   - Scalable (environment mediates unbounded number of agents)
 *   - Robust (individual failures don't destroy the signal)
 *
 * Types:
 *   - Sematectonic: through physical traces (building, digging)
 *   - Sign-based: through signals (pheromones, markers)
 *
 * Key mechanisms:
 *   - Positive feedback (amplification): more pheromone → more followers
 *   - Negative feedback (regulation): evaporation, saturation, depletion
 *   - Randomness: probabilistic decisions enable exploration
 *   - Multiple interactions: many agents modify the shared medium
 * ============================================================================ */

/* ── Stigmergy Environment ── */
StigmergyEnvironment* swarm_stigmergy_create(int width, int height, 
                         double diffusion_rate, double decay_rate);
void                  swarm_stigmergy_free(StigmergyEnvironment* env);
void                  swarm_stigmergy_reset(StigmergyEnvironment* env);

/* Diffusion step: ∂τ/∂t = D·∇²τ */
void swarm_stigmergy_diffuse(StigmergyEnvironment* env, double dt);

/* Decay step: τ ← τ · exp(-λ·dt) */
void swarm_stigmergy_decay(StigmergyEnvironment* env, double dt);

/* Deposit pheromone at position (x,y) */
void swarm_stigmergy_deposit(StigmergyEnvironment* env, double x, double y, 
                              double amount, double radius);

/* Sample pheromone at position (x,y) using bilinear interpolation */
double swarm_stigmergy_sample(const StigmergyEnvironment* env, double x, double y);

/* Compute gradient at (x,y) for chemotaxis */
void swarm_stigmergy_gradient(const StigmergyEnvironment* env, double x, double y,
                               double* grad_x, double* grad_y);

/* Full environment update step: decay + diffuse */
void swarm_stigmergy_update(StigmergyEnvironment* env, double dt);

/* ── Pheromone Trail Models ── */

/* Trail following: agent moves along pheromone gradient */
void swarm_trail_follow(double* position, const StigmergyEnvironment* env,
           double speed, double noise, double* new_position);

/* Deneubourg threshold model: probability to follow trail = τ²/(τ²+K²) */
double swarm_denebourg_response(double pheromone, double threshold_K);

/* Binary bridge experiment (Deneubourg et al. 1990): two paths, 
   pheromone-driven symmetry breaking */
typedef struct {
    double pheromone_short;   /* Pheromone on short branch */
    double pheromone_long;    /* Pheromone on long branch */
    double ratio_short;       /* Fraction choosing short branch */
    double elapsed_time;
    bool   broken;            /* Has symmetry broken? */
} BridgeExperiment;

void swarm_bridge_experiment_init(BridgeExperiment* exp);
void swarm_bridge_experiment_step(BridgeExperiment* exp, int n_ants_per_step,
           double evaporation_rate, double deposit_amount,
           double threshold_K, double path_ratio);

/* ── Self-Organization Patterns ── */

/* Cellular automaton for pattern formation (Slime mold aggregation) */
void swarm_ca_slime_mold(double** grid, int width, int height,
           double camp_diffusion, double camp_decay, double camp_production,
           double dt, int n_steps);

/* Turing pattern reaction-diffusion (activator-inhibitor model) */
void swarm_turing_pattern(double** activator, double** inhibitor,
           int width, int height, double a_diff, double i_diff,
           double feed_rate, double kill_rate, double dt, int n_steps);

/* ── Ant Foraging Simulation ── */
typedef struct {
    double x, y;              /* Position */
    double heading;           /* Direction (radians) */
    bool   has_food;          /* Carrying food? */
    bool   returning;         /* Returning to nest? */
    double wander_angle;      /* Random walk angle */
} ForagingAnt;

typedef struct {
    ForagingAnt* ants;
    StigmergyEnvironment* food_pheromone;
    StigmergyEnvironment* home_pheromone;
    int n_ants;
    double nest_x, nest_y;
    double** food_sources;    /* [n_food][2] positions */
    double*  food_amounts;    /* Remaining food per source */
    int n_food_sources;
    double ant_speed;
    double deposit_rate;
    double sensing_radius;
    double wander_strength;
} ForagingSimulation;

ForagingSimulation* swarm_foraging_create(int n_ants, double nest_x, double nest_y,
                      int world_width, int world_height);
void swarm_foraging_free(ForagingSimulation* sim);
void swarm_foraging_update(ForagingSimulation* sim);
void swarm_foraging_add_food(ForagingSimulation* sim, double x, double y, double amount);
int  swarm_foraging_food_collected(const ForagingSimulation* sim);

/* ── Nest Building Simulation ── */
typedef struct {
    double x, y;
    bool   carrying;          /* Carrying building material? */
    double material_type;     /* Type discrimination */
} BuilderAnt;

/* Cemetery clustering (ants sort items by type via local rules) */
void swarm_cemetery_clustering(int** grid, int width, int height, 
           int n_ants, int n_steps, double pick_prob, double drop_prob);

/* ── Digital Pheromone (for robotics / UAV coordination) ── */
typedef struct {
    double*  x_coords;
    double*  y_coords;
    double*  intensities;
    double*  decay_rates;
    int      n_markers;
    int      capacity;
} DigitalPheromone;

DigitalPheromone* swarm_digital_pheromone_create(int capacity);
void              swarm_digital_pheromone_free(DigitalPheromone* dp);
void              swarm_digital_pheromone_deposit(DigitalPheromone* dp,
                      double x, double y, double intensity, double decay_rate);
double            swarm_digital_pheromone_query(const DigitalPheromone* dp,
                      double x, double y, double radius);
void              swarm_digital_pheromone_decay(DigitalPheromone* dp, double dt);
int               swarm_digital_pheromone_prune(DigitalPheromone* dp, double min_intensity);

#endif /* SWARM_STIGMERGY_H */
