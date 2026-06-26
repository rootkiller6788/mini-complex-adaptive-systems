#ifndef SWARM_CONSENSUS_H
#define SWARM_CONSENSUS_H

#include "swarm_types.h"

/* ============================================================================
 * Distributed Consensus & Collective Decision Making
 *
 * Olfati-Saber, Fax & Murray (2007): "Consensus and Cooperation in Networked
 *   Multi-Agent Systems" — Comprehensive framework for distributed consensus.
 *
 * Ren & Beard (2008): "Distributed Consensus in Multi-vehicle Cooperative Control"
 *
 * Tsitsiklis (1984): "Problems in Decentralized Decision Making and Computation"
 *
 * Consensus Protocol (continuous-time):
 *   ẋ_i(t) = Σ_{j∈N_i} a_{ij} (x_j(t) - x_i(t))
 *
 * Consensus Protocol (discrete-time):
 *   x_i(k+1) = x_i(k) + ε Σ_{j∈N_i} a_{ij} (x_j(k) - x_i(k))
 *   = Σ_j w_{ij} x_j(k)   where W is row-stochastic
 *
 * Convergence condition (Olfati-Saber & Murray 2004):
 *   If the communication graph is connected (or has a spanning tree for
 *   directed graphs), then average consensus is achieved:
 *     lim_{t→∞} x_i(t) = (1/N) Σ_i x_i(0)  for all i
 *
 * Key applications:
 *   - Distributed averaging / sensor fusion
 *   - Clock synchronization in networks
 *   - Flocking (alignment → velocity consensus)
 *   - Distributed optimization
 *   - Collective decision-making (quorum sensing)
 * ============================================================================ */

/* ── Consensus Protocols ── */

/* Initialize consensus state */
ConsensusState* swarm_consensus_create(int n_agents, const double* initial_values);
void            swarm_consensus_free(ConsensusState* state);
void            swarm_consensus_reset(ConsensusState* state, const double* initial_values);

/* Build weight matrices */
void swarm_consensus_weights_max_degree(double** W, int n, const double** adjacency);
void swarm_consensus_weights_metropolis(double** W, int n, const double** adjacency);
void swarm_consensus_weights_laplacian(double** W, int n, const double** adjacency, double epsilon);

/* Iterate consensus one step */
void swarm_consensus_iterate(ConsensusState* state, double epsilon);

/* Run consensus until convergence (returns iterations) */
int  swarm_consensus_run(ConsensusState* state, int max_iterations);

/* Check if consensus reached: max |x_i - x_j| < threshold */
bool swarm_consensus_is_reached(const ConsensusState* state, double threshold);

/* Compute convergence rate (second largest eigenvalue modulus of W) */
double swarm_consensus_convergence_rate(const double** W, int n);

/* ── Collective Decision Making ── */

/* Majority voting: each agent adopts majority opinion of neighbors */
void swarm_decision_majority_vote(double* opinions, const int* neighbor_lists,
           const int* neighbor_counts, int n_agents, int max_neighbors);

/* Voter model: randomly adopt a neighbor's opinion */
void swarm_decision_voter_model(double* opinions, const int* neighbor_lists,
           const int* neighbor_counts, int n_agents, int max_neighbors,
           int steps_per_agent);

/* Threshold model (Granovetter 1978): adopt if fraction of adopters > threshold */
void swarm_decision_threshold_model(double* opinions, double* thresholds,
           const int* neighbor_lists, const int* neighbor_counts,
           int n_agents, int max_neighbors);

/* Quorum sensing: collective switches when quorum proportion reached */
bool swarm_decision_quorum_sensed(const double* opinions, int n_agents, 
           double quorum_threshold, int* quorum_count);

/* Weighted majority: each agent has a weight (e.g., confidence) */
double swarm_decision_weighted_majority(const double* opinions, 
           const double* weights, int n_agents);

/* ── Gossip Algorithms ── */

/* Pairwise gossip (Boyd et al. 2006): randomly selected pair averages */
void swarm_gossip_pairwise(double* values, int n_agents, int n_exchanges);

/* Broadcast gossip: random agent broadcasts, neighbors update */
void swarm_gossip_broadcast(double* values, int n_agents, 
           const int* neighbor_lists, const int* neighbor_counts,
           int max_neighbors, int n_broadcasts);

/* Geographic gossip: route to random agent, then average (Dimakis et al. 2008) */
void swarm_gossip_geographic(double* values, int n_agents, 
           const int* neighbor_lists, const int* neighbor_counts,
           int max_neighbors, int n_gossips);

/* ── Rendezvous & Flocking Control ── */

/* Circumcenter law (Ando et al. 1999): move to circumcenter of neighbors */
void swarm_rendezvous_circumcenter(double* positions, int n_agents, int dim,
           const int* neighbor_lists, const int* neighbor_counts);

/* Consensus-based formation control */
void swarm_formation_consensus(double* positions, int n_agents, int dim,
           const double** desired_offsets, double step_size, int n_steps);

/* ── Information Cascades ── */

/* Bikhchandani-Hirshleifer-Welch (1992) information cascade model */
void swarm_cascade_update(double* beliefs, const int* decisions, 
           const double* private_signals, int n_agents, double signal_accuracy);

/* ── Analysis ── */

/* Algebraic connectivity (Fiedler value λ₂ of Laplacian) */
double swarm_consensus_algebraic_connectivity(const double** Laplacian, int n);

/* Spectral gap = 1 - |λ₂(W)|; convergence speed */
double swarm_consensus_spectral_gap(const double** W, int n);

/* Estimate time to ε-consensus: t_ε ≈ log(ε/||x(0)||) / log(λ₂) */
double swarm_consensus_epsilon_time(const double** W, int n, double epsilon, 
           double initial_disagreement);

#endif /* SWARM_CONSENSUS_H */
