#include "ace_core.h"
#include "ace_market.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    unsigned int seed = 42;
    int n_agents = 100, capacity = 60, n_strat = 5, memory = 10, n_weeks = 60;
    printf("=== El Farol Bar Problem (Arthur 1994) ===
");
    printf("Agents: %d, Capacity: %d, Strategies/Agent: %d, Weeks: %d

",
           n_agents, capacity, n_strat, n_weeks);
    ACEElFarolConfig* cfg = ace_el_farol_config_alloc(
        n_agents, capacity, n_strat, memory, n_weeks);
    ACEAgent* agents = (ACEAgent*)malloc((size_t)n_agents * sizeof(ACEAgent));
    ACEStrategy** strats = (ACEStrategy**)malloc((size_t)n_agents * sizeof(ACEStrategy*));
    for (int a = 0; a < n_agents; a++) {
        agents[a].id = a; agents[a].wealth = 100.0;
        agents[a].risk_aversion = 0.5; agents[a].n_strategies = n_strat;
        agents[a].strategy_weights = (double*)calloc((size_t)n_strat, sizeof(double));
        agents[a].strategy_predictions = (double*)calloc((size_t)n_strat, sizeof(double));
        agents[a].learning_rate = 0.1;
        agents[a].agent_type = ACE_INDUCTIVE_LEARNER;
        agents[a].reasoning = ACE_INDUCTIVE_REASONING;
        strats[a] = (ACEStrategy*)malloc((size_t)n_strat * sizeof(ACEStrategy));
        ace_el_farol_generate_strategies(strats[a], n_strat, memory, &seed);
    }
    ace_el_farol_init(cfg, &seed);
    ace_el_farol_run(cfg, agents, strats, &seed);
    double mean_att, var;
    ace_el_farol_statistics(cfg, &mean_att, &var);
    printf("Results (weeks %d-%d):
", memory, n_weeks);
    printf("  Mean attendance: %.1f
", mean_att);
    printf("  Std deviation:   %.1f
", sqrt(var));
    printf("  Capacity:        %d
", capacity);
    printf("  Efficiency:      %.1f%%
",
           100.0 * (1.0 - fabs(mean_att - capacity) / capacity));
    printf("
Attendance time series:
");
    for (int w = memory; w < n_weeks; w++) {
        printf("  Week %2d: %3d", w, cfg->attendance_history[w]);
        if (cfg->attendance_history[w] > capacity) printf(" OVERCROWDED");
        printf("
");
    }
    for (int a = 0; a < n_agents; a++) {
        free(agents[a].strategy_weights);
        free(agents[a].strategy_predictions);
        free(strats[a]);
    }
    free(agents); free(strats);
    ace_el_farol_config_free(cfg);
    return 0;
}
