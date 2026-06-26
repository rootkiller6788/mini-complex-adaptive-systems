#ifndef ACE_EVOLUTION_H
#define ACE_EVOLUTION_H

#include "ace_core.h"

/* ============================================================================
 * Complex Adaptive Systems in Economics ? Evolution Models (L5-L8)
 *
 * Based on W. Brian Arthur's "The Nature of Technology" (2009):
 *   - Technologies evolve by combination of existing technologies
 *   - The set of technologies is autocatalytic: more technologies
 *     enable more combinations
 *   - Combinatorial evolution explains exponential technological growth
 *
 * Based on Kauffman's NK fitness landscapes (1993):
 *   - Economic organizations/technologies have N interdependent components
 *   - K epistatic interactions per component
 *   - Adaptive walks on rugged fitness landscapes
 *   - Complexity catastrophe at high K
 *
 * Reference: Arthur (2009), "The Nature of Technology"
 * Reference: Arthur & Polak (2006), "The Evolution of Technology
 *            within a Simple Computer Model", Complexity
 * ============================================================================ */

/* ============================================================================
 * L5: Technology Evolution ? Combinatorial Model
 * ============================================================================ */

/* Initialize a technology from primitive phenomena */
void ace_technology_init_primitive(ACETechnology* tech, int id,
                                    const char* name,
                                    const char* phenomenon);

/* Create a new composite technology by combining two parent technologies */
ACETechnology* ace_technology_combine(const ACETechnology* parent1,
                                       const ACETechnology* parent2,
                                       int new_id, const char* new_name,
                                       unsigned int* seed);

/* Compute the novelty of a technology relative to the existing ecosystem.
   Higher novelty = more unique combinations of phenomena. */
double ace_technology_novelty(const ACETechnology* tech,
                               const ACETechEcosystem* ecosystem);

/* Check if two technologies are complementary (can be fruitfully combined) */
bool ace_technology_are_complementary(const ACETechnology* tech1,
                                       const ACETechnology* tech2,
                                       double threshold);

/* Compute the combinatorial potential: how many new technologies
   can this technology combine with */
double ace_technology_combinatorial_potential(const ACETechnology* tech,
                                               const ACETechEcosystem* eco);

/* --- Technology Ecosystem Dynamics --- */

/* Add a new technology to the ecosystem */
void ace_ecosystem_add_technology(ACETechEcosystem* eco,
                                   const ACETechnology* tech);

/* Compute all pairwise complementarities in the ecosystem */
void ace_ecosystem_compute_complementarities(ACETechEcosystem* eco);

/* Detect technology clusters using community detection.
   Returns the number of clusters found. */
int ace_ecosystem_detect_clusters(ACETechEcosystem* eco,
                                   double modularity_threshold);

/* Compute the ecosystem's combinatorial expansion potential:
   expected number of new technologies from combinations */
double ace_ecosystem_expansion_potential(const ACETechEcosystem* eco);

/* ============================================================================
 * L5: Combinatorial Evolution Simulation
 * ============================================================================ */

/* One step of combinatorial evolution:
   1. Select pairs of technologies to combine
   2. Form new composite technologies
   3. Evaluate fitness of new technologies
   4. Remove unfit technologies (selection)
   5. Update ecosystem state (cluster structure, complementarities) */
void ace_combinatorial_evolution_step(ACETechEcosystem* eco,
                                       ACECombinatorialConfig* cfg,
                                       int period, unsigned int* seed);

/* Run full combinatorial evolution simulation */
void ace_combinatorial_evolution_run(ACETechEcosystem* eco,
                                      ACECombinatorialConfig* cfg,
                                      unsigned int* seed);

/* Record ecosystem statistics over time */
void ace_combinatorial_evolution_stats(const ACETechEcosystem* eco,
                                        int period,
                                        int* n_techs_out,
                                        int* n_clusters_out,
                                        double* avg_fitness_out,
                                        double* diversity_out);

/* ============================================================================
 * L5-L8: NK Fitness Landscapes for Economics
 *
 * An NK landscape models a system with N components where each component's
 * contribution to total fitness depends on K other components (epistasis).
 *
 * Key results (Kauffman 1993):
 *   - K=0: single smooth peak, easy optimization (decomposable system)
 *   - K=N-1: maximally rugged, many local optima (fully integrated system)
 *   - Complexity catastrophe: at intermediate K, adaptation gets stuck
 *     on mediocre local peaks
 *
 * Economics interpretation (Arthur 2009, Levinthal 1997):
 *   - N = number of design decisions in a technology
 *   - K = degree of interdependence between decisions
 *   - High K technologies are harder to improve but more defensible
 * ============================================================================ */

/* Generate an NK fitness landscape with random fitness contributions */
void ace_nk_generate(ACENKFitnessLandscape* nk, unsigned int* seed);

/* Generate fitness contributions with specified ruggedness (autocorrelated) */
void ace_nk_generate_correlated(ACENKFitnessLandscape* nk,
                                 double correlation,
                                 unsigned int* seed);

/* Evaluate fitness of a binary configuration (bit string encoding) */
double ace_nk_evaluate(const ACENKFitnessLandscape* nk,
                        const bool* configuration);

/* Encode a binary configuration as an integer index */
int ace_nk_config_index(const bool* config, int N);

/* Decode an integer index to a binary configuration */
void ace_nk_index_to_config(int index, int N, bool* config);

/* Find all local peaks in the landscape (configurations with no fitter neighbor) */
int ace_nk_find_peaks(ACENKFitnessLandscape* nk);

/* Compute landscape ruggedness: correlation of fitness between neighbors */
double ace_nk_ruggedness(const ACENKFitnessLandscape* nk);

/* ============================================================================
 * L5: Adaptive Walk on NK Landscape
 *
 * An adaptive walk simulates an organization/technology improving through
 * incremental changes. At each step, it evaluates all 1-mutant neighbors
 * and moves to the first that improves fitness, or the best neighbor.
 * ============================================================================ */

/* Perform one step of a greedy adaptive walk (move to first improving neighbor) */
bool ace_adaptive_walk_step_greedy(const ACENKFitnessLandscape* nk,
                                    bool* current_config,
                                    double* current_fitness);

/* Perform one step of gradient-ascent walk (move to best neighbor) */
bool ace_adaptive_walk_step_best(const ACENKFitnessLandscape* nk,
                                  bool* current_config,
                                  double* current_fitness);

/* Perform one step of a temperature-based walk (simulated annealing style).
   Accept worse moves with probability exp(-delta_E / T).
   Temperature parameter: higher T = more exploration. */
bool ace_adaptive_walk_step_thermal(const ACENKFitnessLandscape* nk,
                                     bool* current_config,
                                     double* current_fitness,
                                     double temperature,
                                     unsigned int* seed);

/* Run a full adaptive walk to convergence (local peak) */
int ace_adaptive_walk_run(const ACENKFitnessLandscape* nk,
                           bool* start_config,
                           double* final_fitness,
                           int max_steps);

/* ============================================================================
 * L8: Advanced ? Autocatalytic Sets and Economic Growth
 *
 * An autocatalytic set (ACS) in an economic context is a set of technologies
 * or firms where each member's production is catalyzed by other members.
 *
 * The emergence of an ACS can drive a phase transition from slow, linear
 * growth to rapid, exponential growth (Arthur 2009, Ch. 9).
 *
 * Kauffman's theory: Life/economy emerges when a diversity threshold is
 * crossed and an autocatalytic set forms spontaneously.
 * ============================================================================ */

/* Detect autocatalytic sets in a technology ecosystem.
   Returns the size of the largest autocatalytic set found. */
int ace_detect_autocatalytic_set(const ACETechEcosystem* eco,
                                  double catalysis_threshold,
                                  int* acs_members);

/* Check if a given subset of technologies forms an autocatalytic set */
bool ace_is_autocatalytic(const ACETechEcosystem* eco,
                           const int* subset, int subset_size,
                           double catalysis_threshold);

/* Compute the catalytic closure: for a seed set, find the minimal ACS
   that contains it (iteratively add technologies catalyzed by members) */
int ace_catalytic_closure(const ACETechEcosystem* eco,
                           const int* seed, int seed_size,
                           double threshold, int* closure_out);

/* ============================================================================
 * L8: Advanced ? Agent-Based Computational Economics (ACE)
 *
 * Full agent-based model with heterogeneous boundedly-rational agents,
 * endogenous interaction networks, and emergent macro patterns.
 * ============================================================================ */

/* Maximum number of agents in the full ACE model */
#define ACE_MAX_AGENTS 1024

/* Full ACE simulation state */
typedef struct {
    ACEAgent*       agents;
    int             n_agents;
    ACENetwork*     network;           /* agent interaction network */
    ACEStockMarketConfig* market_cfg;  /* financial market component */
    ACEElFarolConfig*    bar_cfg;      /* coordination game component */
    ACETechEcosystem*    tech_eco;     /* technology evolution component */
    ACEMacroState   macro;             /* aggregate state */
    ACETimeSeries*  gdp_series;        /* macro time series */
    int             current_period;
    unsigned int    seed;
} ACEFullModel;

/* Initialize the full ACE model */
ACEFullModel* ace_full_model_alloc(int n_agents,
                                    int n_strategies_per_agent,
                                    int n_periods,
                                    unsigned int* seed);

/* Free the full ACE model */
void ace_full_model_free(ACEFullModel* model);

/* Run one period of the full ACE model:
   - Agents make production/consumption decisions
   - Market clearing (prices, quantities)
   - Strategy updating (learning/evolution)
   - Network rewiring (partner selection)
   - Technology innovation (combinatorial)
   - Macro state recording */
void ace_full_model_period(ACEFullModel* model);

/* Run the full ACE model for all periods */
void ace_full_model_run(ACEFullModel* model);

/* Extract a summary report from the model run */
void ace_full_model_report(const ACEFullModel* model,
                            double* avg_gdp_growth,
                            double* avg_volatility,
                            double* avg_inequality,
                            int* n_crises,
                            int* n_innovations);

#endif /* ACE_EVOLUTION_H */
