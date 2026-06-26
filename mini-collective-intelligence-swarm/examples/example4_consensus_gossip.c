#include "swarm_consensus.h"
#include "swarm_core.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Example 4: Distributed Consensus and Gossip Algorithms.
 * Demonstrates average consensus on a 5-node ring, pairwise gossip,
 * broadcast gossip, and threshold-based collective decision making.
 * Verifies convergence to the average value and measures convergence rate.
 */

int main(void) {
    printf("\n=== Distributed Consensus & Gossip ===\n\n");
    swarm_rng_seed(42);
    
    /* ── Average Consensus on a Ring ── */
    printf("[1] Average Consensus on 5-node Ring\n");
    double init_values[] = {1.0, 3.0, 5.0, 7.0, 9.0};
    double avg_true = (1.0 + 3.0 + 5.0 + 7.0 + 9.0) / 5.0;
    printf("  True average = %.4f\n", avg_true);
    
    ConsensusState* cs = swarm_consensus_create(5, init_values);
    double** adj = swarm_matrix_alloc(5, 5);
    for (int i = 0; i < 5; i++) {
        adj[i][(i+1)%5] = 1.0;
        adj[i][(i+4)%5] = 1.0;
    }
    swarm_consensus_weights_metropolis(cs->weights, 5, (const double**)adj);
    int iters = swarm_consensus_run(cs, 1000);
    
    printf("  Iterations to converge: %d\n", iters);
    printf("  Final values: [");
    for (int i = 0; i < 5; i++) printf("%.4f%s", cs->values[i], i < 4 ? ", " : "");
    printf("]\n");
    printf("  Disagreement: %.2e\n", cs->disagreement);
    
    double spectral_gap = swarm_consensus_spectral_gap((const double**)cs->weights, 5);
    double t_eps = swarm_consensus_epsilon_time((const double**)cs->weights, 5, 0.01, 8.0);
    printf("  Spectral gap: %.4f\n", spectral_gap);
    printf("  Estimated ε-time (ε=0.01): %.1f iterations\n", t_eps);
    
    swarm_matrix_free(adj, 5);
    
    /* ── Pairwise Gossip ── */
    printf("\n[2] Pairwise Gossip (10 agents, 500 exchanges)\n");
    double gossip_vals[10];
    for (int i = 0; i < 10; i++) gossip_vals[i] = (double)i;
    double gossip_avg = (0.0 + 1.0 + 2.0 + 3.0 + 4.0 + 5.0 + 6.0 + 7.0 + 8.0 + 9.0) / 10.0;
    swarm_gossip_pairwise(gossip_vals, 10, 500);
    printf("  True average = %.4f\n", gossip_avg);
    printf("  Gossip values: [");
    for (int i = 0; i < 10; i++) printf("%.4f%s", gossip_vals[i], i < 9 ? ", " : "");
    printf("]\n");
    double gossip_err = 0.0;
    for (int i = 0; i < 10; i++) gossip_err += fabs(gossip_vals[i] - gossip_avg);
    printf("  Total absolute error: %.4f\n", gossip_err);
    
    /* ── Collective Decision Making ── */
    printf("\n[3] Collective Decision Making (Majority Vote)\n");
    double opinions[10] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0}; /* 3 yes, 7 no */
    /* Build simple ring neighborhood */
    int neighbor_lists[10 * 4], neighbor_counts[10];
    for (int i = 0; i < 10; i++) {
        neighbor_counts[i] = 2;
        neighbor_lists[i*4] = (i+1)%10;
        neighbor_lists[i*4+1] = (i+9)%10;
        neighbor_lists[i*4+2] = -1;
        neighbor_lists[i*4+3] = -1;
    }
    printf("  Initial: 3 yes, 7 no\n");
    for (int round = 0; round < 5; round++) {
        swarm_decision_majority_vote(opinions, neighbor_lists, neighbor_counts, 10, 4);
        int yes = 0;
        for (int i = 0; i < 10; i++) if (opinions[i] > 0.5) yes++;
        printf("  Round %d: %d yes, %d no\n", round + 1, yes, 10 - yes);
    }
    
    /* ── Quorum Sensing ── */
    printf("\n[4] Quorum Sensing\n");
    double quorum_opinions[10] = {1, 1, 1, 1, 1, 1, 1, 0, 0, 0};
    int q_count;
    bool quorum = swarm_decision_quorum_sensed(quorum_opinions, 10, 0.7, &q_count);
    printf("  7/10 adopters, quorum at 0.7: %s (count=%d)\n",
        quorum ? "REACHED" : "NOT REACHED", q_count);
    
    swarm_consensus_free(cs);
    
    printf("\nConsensus and gossip demonstration complete.\n");
    return 0;
}
