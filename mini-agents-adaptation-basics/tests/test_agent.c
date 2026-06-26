#include "agent.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("=== test_agent ===\n");

    /* Test agent_create */
    Agent *a = agent_create(1, "TestAgent", 0.1);
    assert(a != NULL);
    assert(a->agent_id == 1);
    assert(fabs(a->adaptation_rate - 0.1) < 1e-6);
    printf("  agent_create: PASS\n");

    /* Test agent_fitness */
    assert(fabs(agent_fitness(a) - 0.0) < 1e-9);
    printf("  agent_fitness (initial zero): PASS\n");

    /* Test agent_step */
    Percept p;
    p.dim = 3;
    p.values[0] = 0.5;
    p.values[1] = 0.3;
    p.values[2] = 0.9;
    p.timestamp = 0;

    Action act = agent_step(a, &p);
    assert(a->age == 1);
    printf("  agent_step: PASS\n");

    /* Test agent_learn_from_experience */
    agent_learn_from_experience(a, &p, &act, 1.0);
    assert(a->cumulative_reward == 1.0);
    assert(fabs(a->fitness - 1.0) < 1e-6);
    printf("  agent_learn_from_experience: PASS\n");

    /* Test epsilon_greedy */
    double qvals[] = {0.0, 0.5, 1.0, 0.3};
    int n_actions = 4;
    int greedy_choice = agent_epsilon_greedy(qvals, n_actions, 0.0);
    assert(greedy_choice == 2); /* best value is 1.0 at index 2 */
    printf("  agent_epsilon_greedy (greedy): PASS\n");

    /* Test boltzmann */
    int bchoice = agent_boltzmann_selection(qvals, n_actions, 0.1);
    assert(bchoice >= 0 && bchoice < 4);
    printf("  agent_boltzmann_selection: PASS\n");

    /* Test policy entropy */
    double probs[] = {0.25, 0.25, 0.25, 0.25};
    double ent = agent_policy_entropy(probs, 4);
    assert(ent > 1.0); /* uniform should have high entropy */
    printf("  agent_policy_entropy: PASS\n");

    /* Test state similarity */
    AgentState s1, s2;
    memset(&s1, 0, sizeof(s1));
    memset(&s2, 0, sizeof(s2));
    s1.state_dim = 3;
    s2.state_dim = 3;
    s1.belief_vec[0] = 1.0;
    s2.belief_vec[0] = 1.0;
    double sim = agent_state_similarity(&s1, &s2);
    assert(fabs(sim - 1.0) < 0.01);
    printf("  agent_state_similarity (identical): PASS\n");

    /* Test state merge */
    AgentState merged = agent_state_merge(&s1, &s2, 0.5);
    assert(merged.state_dim == 3);
    printf("  agent_state_merge: PASS\n");

    /* Test discretize */
    Percept dp;
    dp.dim = 2;
    dp.values[0] = 0.2;
    dp.values[1] = 0.7;
    int disc = agent_discretize_percept(&dp, 5);
    assert(disc >= 0);
    printf("  agent_discretize_percept: PASS\n");

    /* Test satisficing */
    (void)agent_satisfice(a, &p, 0.3, 5);
    printf("  agent_satisfice: PASS\n");

    /* Test aspiration adaptation */
    double hist[] = {0.5, 0.6, 0.4, 0.7, 0.5};
    double asp = agent_adapt_aspiration(a, hist, 5);
    assert(asp > 0.0);
    printf("  agent_adapt_aspiration: PASS\n");

    /* Test message passing */
    Agent *b = agent_create(2, "Receiver", 0.05);
    agent_send_message(a, b, "hello", 0.5);
    assert(b->memory_size == 1);
    printf("  agent_send_message: PASS\n");

    /* Test peer observation */
    a->fitness = 0.8;
    b->fitness = 0.5;
    agent_observe_peer(b, a, &p, &act);
    printf("  agent_observe_peer: PASS\n");

    /* Test imitation */
    agent_imitate(b, a, 0.3);
    printf("  agent_imitate: PASS\n");

    agent_destroy(a);
    agent_destroy(b);

    printf("=== test_agent: ALL PASS ===\n");
    return 0;
}
