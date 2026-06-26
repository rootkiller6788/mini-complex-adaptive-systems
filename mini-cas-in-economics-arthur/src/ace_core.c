#include "ace_core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * ace_core.c ? Core implementations: memory management and utility functions
 *
 * Each function implements an independent knowledge point from the economic
 * complexity and complex adaptive systems literature.
 * ============================================================================ */

/* ---------------------------------------------------------------------------
 * L2: Adoption State Allocation / Deallocation
 * ---------------------------------------------------------------------------
 * Knowledge point: Market adoption state as a structured representation
 * of competing technologies' shares.
 * Complexity: O(n_technologies) space and time for allocation.
 */
ACEAdoptionState* ace_adoption_state_alloc(int n_technologies)
{
    if (n_technologies <= 0) return NULL;
    ACEAdoptionState* state = (ACEAdoptionState*)malloc(sizeof(ACEAdoptionState));
    if (!state) return NULL;
    state->shares = (double*)calloc((size_t)n_technologies, sizeof(double));
    if (!state->shares) { free(state); return NULL; }
    state->n_technologies = n_technologies;
    state->time = 0.0;
    /* Start with equal shares */
    double eq = 1.0 / (double)n_technologies;
    for (int i = 0; i < n_technologies; i++) state->shares[i] = eq;
    return state;
}

void ace_adoption_state_free(ACEAdoptionState* state)
{
    if (!state) return;
    free(state->shares);
    free(state);
}

/* ---------------------------------------------------------------------------
 * L2: Agent Allocation ? Bounded-Rational Economic Agent
 *
 * Knowledge point: The agent construct embodies Arthur's key insight that
 * economic agents use inductive reasoning with a portfolio of strategies,
 * not perfect deductive rationality. This is the micro-foundation of
 * complexity economics.
 *
 * Reference: Arthur (1994), "Inductive Reasoning and Bounded Rationality"
 *            (The El Farol Bar Problem), AEA Papers & Proceedings
 * Complexity: O(n_strategies) allocation time.
 */
ACEAgent* ace_agent_alloc(int id, double wealth, double risk_aversion,
                          int n_strategies, ACEAgentStrategy agent_type,
                          ACEReasoningMode reasoning)
{
    if (n_strategies < 0) return NULL;
    ACEAgent* agent = (ACEAgent*)malloc(sizeof(ACEAgent));
    if (!agent) return NULL;
    agent->id = id;
    agent->wealth = wealth;
    agent->risk_aversion = risk_aversion;
    agent->n_strategies = n_strategies;
    agent->agent_type = agent_type;
    agent->reasoning = reasoning;
    agent->learning_rate = 0.1;
    if (n_strategies > 0) {
        agent->strategy_weights = (double*)calloc((size_t)n_strategies, sizeof(double));
        agent->strategy_predictions = (double*)calloc((size_t)n_strategies, sizeof(double));
        if (!agent->strategy_weights || !agent->strategy_predictions) {
            free(agent->strategy_weights);
            free(agent->strategy_predictions);
            free(agent);
            return NULL;
        }
        /* Initialize weights uniformly */
        double init_w = 1.0 / (double)n_strategies;
        for (int i = 0; i < n_strategies; i++) agent->strategy_weights[i] = init_w;
    } else {
        agent->strategy_weights = NULL;
        agent->strategy_predictions = NULL;
    }
    return agent;
}

void ace_agent_free(ACEAgent* agent)
{
    if (!agent) return;
    free(agent->strategy_weights);
    free(agent->strategy_predictions);
    free(agent);
}

/* ---------------------------------------------------------------------------
 * L2: Prediction Strategy Allocation
 *
 * Knowledge point: A prediction strategy is a hypothesis about how the
 * world works ? a conditional forecasting rule. In Arthur's framework,
 * agents maintain a "bag of tricks" (multiple strategies) and switch
 * between them based on which has been most accurate recently.
 *
 * Reference: Arthur (1994), El Farol Bar Problem
 *            Holland, Holyoak, Nisbett, Thagard (1986),
 *            "Induction: Processes of Inference, Learning, and Discovery"
 */
ACEStrategy* ace_strategy_alloc(int id, int n_predictors)
{
    if (n_predictors <= 0) return NULL;
    ACEStrategy* strat = (ACEStrategy*)malloc(sizeof(ACEStrategy));
    if (!strat) return NULL;
    strat->id = id;
    strat->n_predictors = n_predictors;
    strat->coefficients = (double*)calloc((size_t)n_predictors, sizeof(double));
    strat->predictor_lags = (int*)calloc((size_t)n_predictors, sizeof(int));
    if (!strat->coefficients || !strat->predictor_lags) {
        free(strat->coefficients);
        free(strat->predictor_lags);
        free(strat);
        return NULL;
    }
    strat->fitness = 0.0;
    strat->times_activated = 0;
    strat->times_correct = 0;
    return strat;
}

void ace_strategy_free(ACEStrategy* strat)
{
    if (!strat) return;
    free(strat->coefficients);
    free(strat->predictor_lags);
    free(strat);
}

/* ---------------------------------------------------------------------------
 * L3: Path History Allocation
 *
 * Knowledge point: Path history records the stochastic trajectory of an
 * adoption process. In path-dependent systems, the whole history matters
 * for the final outcome ? convergence theorems (like the standard CLT)
 * that average out history do not apply.
 *
 * Reference: Arthur, Ermoliev, Kaniovski (1987), "Path-dependent processes"
 * Complexity: O(max_steps * n_technologies) space.
 */
ACEPathHistory* ace_path_history_alloc(int max_steps, int n_technologies)
{
    if (max_steps <= 0 || n_technologies <= 0) return NULL;
    ACEPathHistory* hist = (ACEPathHistory*)malloc(sizeof(ACEPathHistory));
    if (!hist) return NULL;
    int total_entries = max_steps * n_technologies;
    hist->market_shares = (double*)calloc((size_t)total_entries, sizeof(double));
    hist->time_points = (double*)calloc((size_t)max_steps, sizeof(double));
    if (!hist->market_shares || !hist->time_points) {
        free(hist->market_shares);
        free(hist->time_points);
        free(hist);
        return NULL;
    }
    hist->n_steps = 0;
    hist->n_technologies = n_technologies;
    hist->final_state = 0.0;
    hist->is_locked = false;
    return hist;
}

void ace_path_history_free(ACEPathHistory* hist)
{
    if (!hist) return;
    free(hist->market_shares);
    free(hist->time_points);
    free(hist);
}

/* ---------------------------------------------------------------------------
 * L3: Polya Urn Allocation
 *
 * Knowledge point: The Polya urn is the canonical mathematical model of
 * increasing returns. A ball is drawn, its color noted, and it is
 * returned along with additional balls of the same color. The proportion
 * converges almost surely to a random limit ? path dependence in pure form.
 *
 * Arthur generalized this: the number of added balls can be any function
 * f(proportion), leading to the generalized Polya urn.
 *
 * Reference: Arthur, Ermoliev, Kaniovski (1987)
 * Complexity: O(n_colors) allocation.
 */
ACEPolyaUrn* ace_polya_urn_alloc(int n_colors, const int* initial_counts,
                                 double self_reinforcement)
{
    if (n_colors <= 0 || !initial_counts) return NULL;
    ACEPolyaUrn* urn = (ACEPolyaUrn*)malloc(sizeof(ACEPolyaUrn));
    if (!urn) return NULL;
    urn->n_colors = n_colors;
    urn->initial_balls = (int*)malloc((size_t)n_colors * sizeof(int));
    urn->current_balls = (int*)malloc((size_t)n_colors * sizeof(int));
    urn->addition_function = (double*)calloc((size_t)n_colors, sizeof(double));
    if (!urn->initial_balls || !urn->current_balls || !urn->addition_function) {
        free(urn->initial_balls);
        free(urn->current_balls);
        free(urn->addition_function);
        free(urn);
        return NULL;
    }
    urn->total_balls = 0;
    for (int i = 0; i < n_colors; i++) {
        if (initial_counts[i] < 0) {
            ace_polya_urn_free(urn);
            return NULL;
        }
        urn->initial_balls[i] = initial_counts[i];
        urn->current_balls[i] = initial_counts[i];
        urn->total_balls += initial_counts[i];
    }
    urn->self_reinforcement = self_reinforcement;
    urn->use_generalized = false;
    return urn;
}

void ace_polya_urn_free(ACEPolyaUrn* urn)
{
    if (!urn) return;
    free(urn->initial_balls);
    free(urn->current_balls);
    free(urn->addition_function);
    free(urn);
}

/* ---------------------------------------------------------------------------
 * L1-L2: Technology Allocation (Arthur 2009)
 *
 * Knowledge point: A technology in Arthur's framework is "a means to
 * fulfill a human purpose" ? an assemblage of practices and components
 * that harnesses physical phenomena. Technologies are built from other
 * technologies (combinatorial evolution).
 * --------------------------------------------------------------------------- */
ACETechnology* ace_technology_alloc(int tech_id, const char* name,
                                    ACETechnologyType type)
{
    if (!name) return NULL;
    ACETechnology* tech = (ACETechnology*)malloc(sizeof(ACETechnology));
    if (!tech) return NULL;
    tech->tech_id = tech_id;
    strncpy(tech->name, name, 63);
    tech->name[63] = '\0';
    tech->n_components = 0;
    tech->component_ids = NULL;
    tech->n_phenomena = 0;
    for (int i = 0; i < 8; i++) tech->phenomena[i][0] = '\0';
    tech->fitness = 1.0;
    tech->adoption_rate = 0.0;
    tech->combinatorial_potential = 0.0;
    tech->tech_type = type;
    tech->is_dominant_design = false;
    return tech;
}

void ace_technology_free(ACETechnology* tech)
{
    if (!tech) return;
    free(tech->component_ids);
    free(tech);
}

/* ---------------------------------------------------------------------------
 * L3: Technology Ecosystem Allocation
 * --------------------------------------------------------------------------- */
ACETechEcosystem* ace_tech_ecosystem_alloc(int n_technologies)
{
    if (n_technologies <= 0) return NULL;
    ACETechEcosystem* eco = (ACETechEcosystem*)malloc(sizeof(ACETechEcosystem));
    if (!eco) return NULL;
    eco->n_technologies = 0;
    eco->technologies = (ACETechnology*)malloc((size_t)n_technologies * sizeof(ACETechnology));
    if (!eco->technologies) { free(eco); return NULL; }
    eco->complementarity = (double**)malloc((size_t)n_technologies * sizeof(double*));
    eco->competition = (double**)malloc((size_t)n_technologies * sizeof(double*));
    if (!eco->complementarity || !eco->competition) {
        free(eco->technologies);
        free(eco->complementarity);
        free(eco->competition);
        free(eco);
        return NULL;
    }
    for (int i = 0; i < n_technologies; i++) {
        eco->complementarity[i] = (double*)calloc((size_t)n_technologies, sizeof(double));
        eco->competition[i] = (double*)calloc((size_t)n_technologies, sizeof(double));
        if (!eco->complementarity[i] || !eco->competition[i]) {
            for (int j = 0; j <= i; j++) {
                free(eco->complementarity[j]);
                free(eco->competition[j]);
            }
            free(eco->complementarity);
            free(eco->competition);
            free(eco->technologies);
            free(eco);
            return NULL;
        }
    }
    eco->cluster_ids = (int*)malloc((size_t)n_technologies * sizeof(int));
    eco->n_clusters = 0;
    eco->emergence_rate = 0.0;
    if (!eco->cluster_ids) {
        ace_tech_ecosystem_free(eco);
        return NULL;
    }
    return eco;
}

void ace_tech_ecosystem_free(ACETechEcosystem* eco)
{
    if (!eco) return;
    int cap = eco->n_technologies; /* approximate capacity from allocated tech array */
    for (int i = 0; i < cap; i++) {
        free(eco->complementarity[i]);
        free(eco->competition[i]);
    }
    free(eco->complementarity);
    free(eco->competition);
    free(eco->cluster_ids);
    free(eco->technologies);
    free(eco);
}

/* ---------------------------------------------------------------------------
 * L3: Time Series Allocation
 *
 * Knowledge point: Economic time series are the empirical data that
 * complexity economics seeks to explain ? fat tails, volatility clustering,
 * long memory, and structural breaks are hallmarks of complex adaptive
 * economic dynamics.
 *
 * Reference: Mantegna & Stanley (2000), "Introduction to Econophysics"
 * Complexity: O(1) allocation, O(1) amortized append.
 * --------------------------------------------------------------------------- */
ACETimeSeries* ace_timeseries_alloc(int capacity)
{
    if (capacity <= 0) capacity = 256;
    ACETimeSeries* ts = (ACETimeSeries*)malloc(sizeof(ACETimeSeries));
    if (!ts) return NULL;
    ts->data = (double*)calloc((size_t)capacity, sizeof(double));
    if (!ts->data) { free(ts); return NULL; }
    ts->length = 0;
    ts->capacity = capacity;
    ts->mean = 0.0;
    ts->variance = 0.0;
    ts->skewness = 0.0;
    ts->kurtosis = 0.0;
    ts->is_stationary = false;
    ts->hurst_exponent = 0.5;
    return ts;
}

void ace_timeseries_free(ACETimeSeries* ts)
{
    if (!ts) return;
    free(ts->data);
    free(ts);
}

/* ---------------------------------------------------------------------------
 * L3: Network Allocation
 *
 * Knowledge point: Agent interaction networks determine how information,
 * influence, and externalities propagate through an economic system.
 * Scale-free and small-world properties strongly affect market dynamics,
 * cascade behavior, and the speed of innovation diffusion.
 *
 * Reference: Jackson (2008), "Social and Economic Networks"
 *            Watts & Strogatz (1998), "Collective dynamics of small-world networks"
 * --------------------------------------------------------------------------- */
ACENetwork* ace_network_alloc(int n_nodes)
{
    if (n_nodes <= 0) return NULL;
    ACENetwork* net = (ACENetwork*)malloc(sizeof(ACENetwork));
    if (!net) return NULL;
    net->n_nodes = n_nodes;
    net->adjacency = (int**)malloc((size_t)n_nodes * sizeof(int*));
    net->degree = (int*)calloc((size_t)n_nodes, sizeof(int));
    if (!net->adjacency || !net->degree) {
        free(net->adjacency);
        free(net->degree);
        free(net);
        return NULL;
    }
    for (int i = 0; i < n_nodes; i++) {
        net->adjacency[i] = (int*)calloc((size_t)n_nodes, sizeof(int));
        if (!net->adjacency[i]) {
            for (int j = 0; j < i; j++) free(net->adjacency[j]);
            free(net->adjacency);
            free(net->degree);
            free(net);
            return NULL;
        }
    }
    net->clustering_coeff = 0.0;
    net->avg_path_length = 0.0;
    net->is_scale_free = false;
    net->is_small_world = false;
    return net;
}

void ace_network_free(ACENetwork* net)
{
    if (!net) return;
    for (int i = 0; i < net->n_nodes; i++) free(net->adjacency[i]);
    free(net->adjacency);
    free(net->degree);
    free(net);
}

/* ---------------------------------------------------------------------------
 * L3: Distribution Allocation
 * --------------------------------------------------------------------------- */
ACEDistribution* ace_distribution_alloc(int n_bins, double min, double max)
{
    if (n_bins <= 0 || min >= max) return NULL;
    ACEDistribution* dist = (ACEDistribution*)malloc(sizeof(ACEDistribution));
    if (!dist) return NULL;
    dist->n_bins = n_bins;
    dist->range_min = min;
    dist->range_max = max;
    dist->bin_width = (max - min) / (double)n_bins;
    dist->bins = (double*)calloc((size_t)n_bins, sizeof(double));
    dist->probabilities = (double*)calloc((size_t)n_bins, sizeof(double));
    if (!dist->bins || !dist->probabilities) {
        free(dist->bins);
        free(dist->probabilities);
        free(dist);
        return NULL;
    }
    for (int i = 0; i < n_bins; i++) {
        dist->bins[i] = min + (i + 0.5) * dist->bin_width;
    }
    dist->gini_coefficient = 0.0;
    dist->is_heavy_tailed = false;
    dist->tail_exponent = 0.0;
    return dist;
}

void ace_distribution_free(ACEDistribution* dist)
{
    if (!dist) return;
    free(dist->bins);
    free(dist->probabilities);
    free(dist);
}

/* ---------------------------------------------------------------------------
 * L3: Macro State Allocation
 * --------------------------------------------------------------------------- */
ACEMacroState* ace_macro_state_alloc(void)
{
    ACEMacroState* ms = (ACEMacroState*)malloc(sizeof(ACEMacroState));
    if (!ms) return NULL;
    ms->gdp_growth = 0.0;
    ms->herfindahl_index = 0.0;
    ms->entropy = 0.0;
    ms->innovation_rate = 0.0;
    ms->inequality = 0.0;
    ms->n_active_agents = 0;
    ms->n_bankruptcies = 0;
    ms->market_volatility = 0.0;
    ms->is_crisis = false;
    return ms;
}

void ace_macro_state_free(ACEMacroState* ms)
{
    free(ms);
}

/* ---------------------------------------------------------------------------
 * L3: Stochastic Process Allocation
 * --------------------------------------------------------------------------- */
ACEStochasticProcess* ace_stochastic_process_alloc(int dim)
{
    if (dim <= 0) return NULL;
    ACEStochasticProcess* sp = (ACEStochasticProcess*)malloc(sizeof(ACEStochasticProcess));
    if (!sp) return NULL;
    sp->state_dimension = dim;
    sp->state = (double*)calloc((size_t)dim, sizeof(double));
    sp->drift = (double*)calloc((size_t)dim, sizeof(double));
    sp->diffusion = (double**)malloc((size_t)dim * sizeof(double*));
    if (!sp->state || !sp->drift || !sp->diffusion) {
        free(sp->state); free(sp->drift); free(sp->diffusion); free(sp);
        return NULL;
    }
    for (int i = 0; i < dim; i++) {
        sp->diffusion[i] = (double*)calloc((size_t)dim, sizeof(double));
        if (!sp->diffusion[i]) {
            for (int j = 0; j < i; j++) free(sp->diffusion[j]);
            free(sp->diffusion); free(sp->state); free(sp->drift); free(sp);
            return NULL;
        }
    }
    sp->time_step = 0.01;
    sp->is_markov = true;
    sp->is_ergodic = false;
    sp->is_path_dependent = false;
    return sp;
}

void ace_stochastic_process_free(ACEStochasticProcess* sp)
{
    if (!sp) return;
    for (int i = 0; i < sp->state_dimension; i++) free(sp->diffusion[i]);
    free(sp->diffusion);
    free(sp->state);
    free(sp->drift);
    free(sp);
}

/* ---------------------------------------------------------------------------
 * L6: El Farol Config Allocation
 * --------------------------------------------------------------------------- */
ACEElFarolConfig* ace_el_farol_config_alloc(int n_agents, int capacity,
                                             int n_strategies_per_agent,
                                             int memory_length, int n_weeks)
{
    if (n_agents <= 0 || capacity <= 0 || n_weeks <= 0) return NULL;
    ACEElFarolConfig* cfg = (ACEElFarolConfig*)malloc(sizeof(ACEElFarolConfig));
    if (!cfg) return NULL;
    cfg->n_agents = n_agents;
    cfg->capacity = capacity;
    cfg->n_strategies_per_agent = n_strategies_per_agent;
    cfg->memory_length = memory_length;
    cfg->n_weeks = n_weeks;
    cfg->attendance_history = (int*)calloc((size_t)n_weeks, sizeof(int));
    if (!cfg->attendance_history) { free(cfg); return NULL; }
    cfg->overcrowding_penalty = 1.0;
    cfg->staying_home_utility = 0.0;
    return cfg;
}

void ace_el_farol_config_free(ACEElFarolConfig* cfg)
{
    if (!cfg) return;
    free(cfg->attendance_history);
    free(cfg);
}

/* ---------------------------------------------------------------------------
 * L6: Stock Market Config Allocation
 * --------------------------------------------------------------------------- */
ACEStockMarketConfig* ace_stock_market_config_alloc(int n_agents,
                                                    int n_strategies_pool,
                                                    int n_periods)
{
    if (n_agents <= 0 || n_periods <= 0) return NULL;
    ACEStockMarketConfig* cfg = (ACEStockMarketConfig*)malloc(sizeof(ACEStockMarketConfig));
    if (!cfg) return NULL;
    cfg->n_agents = n_agents;
    cfg->n_strategies_pool = n_strategies_pool;
    cfg->dividend_mean = 10.0;
    cfg->dividend_std = 0.5;
    cfg->interest_rate = 0.05;
    cfg->fundamental_value = 200.0;
    cfg->n_periods = n_periods;
    cfg->mutation_rate = 0.01;
    cfg->crossover_rate = 0.7;
    cfg->tournament_size = 4.0;
    return cfg;
}

void ace_stock_market_config_free(ACEStockMarketConfig* cfg)
{
    free(cfg);
}

/* ---------------------------------------------------------------------------
 * L5-L8: NK Landscape Allocation
 * --------------------------------------------------------------------------- */
ACENKFitnessLandscape* ace_nk_landscape_alloc(int N, int K)
{
    if (N <= 0 || K < 0 || K >= N) return NULL;
    ACENKFitnessLandscape* nk = (ACENKFitnessLandscape*)malloc(sizeof(ACENKFitnessLandscape));
    if (!nk) return NULL;
    nk->N = N;
    nk->K = K;
    int configs = 1 << N;
    nk->fitness_table = (double**)malloc((size_t)configs * sizeof(double*));
    nk->fitness_landscape = (double*)malloc((size_t)configs * sizeof(double));
    nk->is_local_peak = (bool*)calloc((size_t)configs, sizeof(bool));
    if (!nk->fitness_table || !nk->fitness_landscape || !nk->is_local_peak) {
        free(nk->fitness_table); free(nk->fitness_landscape);
        free(nk->is_local_peak); free(nk);
        return NULL;
    }
    for (int i = 0; i < configs; i++) {
        nk->fitness_table[i] = (double*)calloc((size_t)N, sizeof(double));
        if (!nk->fitness_table[i]) {
            for (int j = 0; j < i; j++) free(nk->fitness_table[j]);
            free(nk->fitness_table); free(nk->fitness_landscape);
            free(nk->is_local_peak); free(nk);
            return NULL;
        }
    }
    nk->n_peaks = 0;
    nk->ruggedness = 0.0;
    return nk;
}

void ace_nk_landscape_free(ACENKFitnessLandscape* nk)
{
    if (!nk) return;
    int configs = 1 << nk->N;
    for (int i = 0; i < configs; i++) free(nk->fitness_table[i]);
    free(nk->fitness_table);
    free(nk->fitness_landscape);
    free(nk->is_local_peak);
    free(nk);
}

/* ---------------------------------------------------------------------------
 * L5: Combinatorial Config Allocation
 * --------------------------------------------------------------------------- */
ACECombinatorialConfig* ace_combinatorial_config_alloc(int n_initial_tech,
                                                        int max_components,
                                                        int n_periods)
{
    if (n_initial_tech <= 0 || n_periods <= 0) return NULL;
    ACECombinatorialConfig* cfg = (ACECombinatorialConfig*)malloc(sizeof(ACECombinatorialConfig));
    if (!cfg) return NULL;
    cfg->n_initial_tech = n_initial_tech;
    cfg->max_components = max_components;
    cfg->combination_rate = 0.1;
    cfg->selection_pressure = 1.1;
    cfg->n_periods = n_periods;
    cfg->autocatalytic = true;
    cfg->novelty_decay = 0.05;
    return cfg;
}

void ace_combinatorial_config_free(ACECombinatorialConfig* cfg)
{
    free(cfg);
}

/* ============================================================================
 * Mathematical Utility Functions
 * ============================================================================ */

/* ---------------------------------------------------------------------------
 * Shannon Entropy
 *
 * Knowledge point: H = -sum(p_i * log(p_i)). In Arthur's complexity economics,
 * entropy measures economic diversity ? the variety of technologies,
 * strategies, or market positions in the system. High entropy = diverse,
 * resilient system. Low entropy = concentrated, fragile system.
 *
 * Reference: Shannon (1948); applied to economics in Arthur (1999)
 * Complexity: O(n) time, O(1) auxiliary space.
 * --------------------------------------------------------------------------- */
double ace_shannon_entropy(const double* proportions, int n)
{
    if (!proportions || n <= 0) return 0.0;
    double entropy = 0.0;
    double total = 0.0;
    for (int i = 0; i < n; i++) {
        if (proportions[i] < 0.0) return -1.0; /* invalid input */
        total += proportions[i];
    }
    if (total <= 0.0) return 0.0;
    for (int i = 0; i < n; i++) {
        double p = proportions[i] / total;
        if (p > 1e-15) entropy -= p * log(p);
    }
    return entropy;
}

/* ---------------------------------------------------------------------------
 * Herfindahl-Hirschman Index (HHI)
 *
 * Knowledge point: HHI = sum(s_i^2) where s_i is market share of firm i.
 * HHI ? [1/n, 1]. HHI > 0.25 indicates high concentration (DOJ threshold).
 * In complexity economics, HHI tracks the emergence of dominant designs
 * and winner-take-all dynamics under increasing returns.
 *
 * Reference: Herfindahl (1950); Hirschman (1964); Arthur (1996)
 * Complexity: O(n) time.
 * --------------------------------------------------------------------------- */
double ace_herfindahl_index(const double* market_shares, int n_firms)
{
    if (!market_shares || n_firms <= 0) return 0.0;
    double hhi = 0.0;
    double total = 0.0;
    for (int i = 0; i < n_firms; i++) {
        if (market_shares[i] < 0.0) return -1.0;
        total += market_shares[i];
    }
    if (total <= 0.0) return 0.0;
    for (int i = 0; i < n_firms; i++) {
        double s = market_shares[i] / total;
        hhi += s * s;
    }
    return hhi;
}

/* ---------------------------------------------------------------------------
 * Linear Congruential Generator (LCG)
 *
 * Knowledge point: Pseudo-random number generation for reproducible
 * stochastic simulations. LCG parameters are from Numerical Recipes
 * (a=1664525, c=1013904223, m=2^32).
 *
 * Reference: Knuth (1997), "The Art of Computer Programming", Vol. 2
 * Complexity: O(1).
 * --------------------------------------------------------------------------- */
unsigned int ace_lcg_step(unsigned int* seed)
{
    if (!seed) return 0;
    *seed = (*seed) * 1664525u + 1013904223u;
    return *seed;
}

/* ---------------------------------------------------------------------------
 * Uniform Random Number in [0,1]
 *
 * Knowledge point: Converts LCG output to uniform [0,1] by dividing by 2^32.
 * Uses high 16 bits for better uniformity (low bits of LCG are less random).
 * Complexity: O(1).
 * --------------------------------------------------------------------------- */
double ace_random_uniform(unsigned int* seed)
{
    if (!seed) return 0.0;
    unsigned int r = ace_lcg_step(seed);
    return (double)(r >> 16) / 65536.0;
}

/* ---------------------------------------------------------------------------
 * Box-Muller Gaussian Random Variable
 *
 * Knowledge point: Transform two independent uniform [0,1] variates into
 * two independent standard normal variates. Used for simulating stochastic
 * processes with Gaussian innovations.
 *
 * Reference: Box & Muller (1958), "A Note on the Generation of Random
 *            Normal Deviates", Annals of Mathematical Statistics
 * Complexity: O(1).
 * --------------------------------------------------------------------------- */
double ace_random_gaussian(unsigned int* seed)
{
    if (!seed) return 0.0;
    double u1 = ace_random_uniform(seed);
    double u2 = ace_random_uniform(seed);
    /* Avoid log(0) */
    if (u1 < 1e-15) u1 = 1e-15;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979323846 * u2);
}

/* ---------------------------------------------------------------------------
 * Softmax Function
 *
 * Knowledge point: The softmax converts a vector of scores/utilities into
 * a probability distribution. Temperature T controls the randomness of
 * choice: T->0 = pure maximization (deterministic), T->infinity = uniform
 * random. In economic agent models, softmax models bounded rationality ?
 * agents choose better options more often but sometimes explore.
 *
 * Reference: Bridle (1990); used in Arthur's agent decision models
 * Complexity: O(n) time.
 * --------------------------------------------------------------------------- */
void ace_softmax(const double* scores, int n, double temperature, double* out)
{
    if (!scores || !out || n <= 0) return;
    if (temperature <= 0.0) {
        /* Argmax: deterministic choice */
        int best = 0;
        for (int i = 1; i < n; i++)
            if (scores[i] > scores[best]) best = i;
        for (int i = 0; i < n; i++) out[i] = (i == best) ? 1.0 : 0.0;
        return;
    }
    /* Find max for numerical stability */
    double max_score = scores[0];
    for (int i = 1; i < n; i++)
        if (scores[i] > max_score) max_score = scores[i];
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        out[i] = exp((scores[i] - max_score) / temperature);
        sum += out[i];
    }
    if (sum > 1e-15) {
        for (int i = 0; i < n; i++) out[i] /= sum;
    } else {
        /* Numerical underflow: uniform fallback */
        for (int i = 0; i < n; i++) out[i] = 1.0 / (double)n;
    }
}

/* ---------------------------------------------------------------------------
 * Floating-Point Approximate Equality
 *
 * Knowledge point: Due to finite precision, floating-point comparisons
 * must use tolerance. Essential for testing convergence, detecting
 * equilibria, and comparing simulation outputs.
 *
 * Complexity: O(1).
 * --------------------------------------------------------------------------- */
bool ace_fequal(double a, double b, double tolerance)
{
    if (tolerance <= 0.0) tolerance = 1e-10;
    double diff = a - b;
    if (diff < 0.0) diff = -diff;
    return diff < tolerance;
}
