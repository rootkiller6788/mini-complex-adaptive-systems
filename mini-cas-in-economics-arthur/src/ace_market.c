#include "ace_market.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "ace_market.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * ace_market.c - Market Models (L5-L6)
 *
 * El Farol Bar Problem, Minority Game, Santa Fe Artificial Stock Market,
 * and Classifier Systems from W. Brian Arthur's complexity economics program.
 * ============================================================================ */

/* ---- M0: El Farol Initialization (L6) ---- */
void ace_el_farol_init(ACEElFarolConfig* cfg, unsigned int* seed)
{
    if (!cfg || !seed) return;
    for (int i = 0; i < cfg->n_weeks; i++) {
        cfg->attendance_history[i] = 0;
    }
    /* Seed first few weeks with random attendance */
    for (int i = 0; i < cfg->memory_length && i < cfg->n_weeks; i++) {
        cfg->attendance_history[i] = cfg->capacity / 2
            + (int)(20.0 * ace_random_gaussian(seed));
        if (cfg->attendance_history[i] < 0) cfg->attendance_history[i] = 0;
        if (cfg->attendance_history[i] > cfg->n_agents)
            cfg->attendance_history[i] = cfg->n_agents;
    }
}

/* ---- M1: Generate Agent Strategies (L6) ---- */
void ace_el_farol_generate_strategies(ACEStrategy* strategies,
                                       int n_strategies, int memory_length,
                                       unsigned int* seed)
{
    if (!strategies || !seed || n_strategies <= 0 || memory_length <= 0)
        return;
    for (int s = 0; s < n_strategies; s++) {
        strategies[s].id = s;
        strategies[s].n_predictors = memory_length;
        strategies[s].fitness = 0.0;
        strategies[s].times_activated = 0;
        strategies[s].times_correct = 0;
        for (int p = 0; p < memory_length; p++) {
            strategies[s].coefficients[p] =
                -1.0 + 2.0 * ace_random_uniform(seed);
            strategies[s].predictor_lags[p] = p;
        }
        /* Normalize coefficients to sum to approximately 1 */
        double sum = 0.0;
        for (int p = 0; p < memory_length; p++) {
            if (strategies[s].coefficients[p] > 0.0)
                sum += strategies[s].coefficients[p];
        }
        if (sum > 1e-10) {
            for (int p = 0; p < memory_length; p++) {
                if (strategies[s].coefficients[p] > 0.0)
                    strategies[s].coefficients[p] /= sum;
            }
        }
    }
}


/* ---- M2: El Farol Predict (L6) ---- */
double ace_el_farol_predict(const ACEStrategy* strat,
                             const int* history, int memory_length,
                             int current_week)
{
    if (!strat || !history || memory_length <= 0) return 0.0;
    double prediction = 0.0;
    int available = (current_week < memory_length) ? current_week : memory_length;
    for (int p = 0; p < strat->n_predictors && p < available; p++) {
        int lag = strat->predictor_lags[p] + 1;
        if (lag <= current_week) {
            prediction += strat->coefficients[p]
                        * (double)history[current_week - lag];
        }
    }
    return prediction;
}

/* ---- M3: El Farol Step (L6) ---- */
void ace_el_farol_step(ACEElFarolConfig* cfg, ACEAgent* agents,
                        ACEStrategy** agent_strategies,
                        int current_week, unsigned int* seed)
{
    if (!cfg || !agents || !agent_strategies || !seed) return;
    int n_agents = cfg->n_agents;
    int attendance = 0;
    for (int a = 0; a < n_agents; a++) {
        /* Find best strategy by fitness */
        int best_s = 0;
        double best_fit = agent_strategies[a][0].fitness;
        for (int s = 1; s < cfg->n_strategies_per_agent; s++) {
            if (agent_strategies[a][s].fitness > best_fit) {
                best_fit = agent_strategies[a][s].fitness;
                best_s = s;
            }
        }
        /* Predict attendance using best strategy */
        double pred = ace_el_farol_predict(&agent_strategies[a][best_s],
            cfg->attendance_history, cfg->memory_length, current_week);
        /* Decide: go if prediction <= capacity, else stay home */
        bool go = (pred <= (double)cfg->capacity);
        if (go) attendance++;
        /* Update strategy fitness after outcome is known (done after loop) */
        agents[a].strategy_predictions[best_s] = pred;
        agents[a].strategy_weights[best_s] = (double)(go ? 1 : 0);
    }
    /* Record attendance */
    cfg->attendance_history[current_week] = attendance;
    /* Update strategy fitness based on actual attendance */
    bool overcrowded = (attendance > cfg->capacity);
    for (int a = 0; a < n_agents; a++) {
        for (int s = 0; s < cfg->n_strategies_per_agent; s++) {
            double pred = ace_el_farol_predict(&agent_strategies[a][s],
                cfg->attendance_history, cfg->memory_length, current_week);
            /* Strategy was correct if it predicted the right side of capacity */
            bool pred_go = (pred <= (double)cfg->capacity);
            /* Actual good outcome: go when not overcrowded, or stay when overcrowded */
            bool strategy_correct;
            if (overcrowded) {
                strategy_correct = !pred_go;
            } else {
                strategy_correct = pred_go;
            }
            agent_strategies[a][s].times_activated++;
            if (strategy_correct) {
                agent_strategies[a][s].times_correct++;
                agent_strategies[a][s].fitness += 1.0;
            }
        }
    }
}

/* ---- M4: El Farol Run (L6) ---- */
void ace_el_farol_run(ACEElFarolConfig* cfg, ACEAgent* agents,
                       ACEStrategy** agent_strategies, unsigned int* seed)
{
    if (!cfg || !agents || !agent_strategies || !seed) return;
    for (int w = cfg->memory_length; w < cfg->n_weeks; w++) {
        ace_el_farol_step(cfg, agents, agent_strategies, w, seed);
    }
}

/* ---- M5: El Farol Statistics (L6) ---- */
void ace_el_farol_statistics(const ACEElFarolConfig* cfg,
                              double* mean_attendance, double* variance)
{
    if (!cfg || !mean_attendance || !variance) return;
    double sum = 0.0, sum_sq = 0.0;
    int n = cfg->n_weeks - cfg->memory_length;
    if (n <= 0) { *mean_attendance = 0.0; *variance = 0.0; return; }
    for (int w = cfg->memory_length; w < cfg->n_weeks; w++) {
        double a = (double)cfg->attendance_history[w];
        sum += a;
        sum_sq += a * a;
    }
    *mean_attendance = sum / (double)n;
    *variance = sum_sq / (double)n - (*mean_attendance) * (*mean_attendance);
    if (*variance < 0.0) *variance = 0.0;
}


int ace_minority_game_predict(const ACEStrategy* strat,
                               const int* history, int memory_length)
{
    if (!strat || !history || memory_length <= 0) return 0;
    double signal = 0.0;
    for (int p = 0; p < strat->n_predictors && p < memory_length; p++) {
        int lag = strat->predictor_lags[p];
        if (lag < memory_length)
            signal += strat->coefficients[p] * (double)history[lag];
    }
    return (signal >= 0.0) ? 1 : -1;
}

void ace_minority_game_update_fitness(ACEStrategy* strat, bool was_correct)
{
    if (!strat) return;
    strat->times_activated++;
    if (was_correct) { strat->times_correct++; strat->fitness += 1.0; }
    else strat->fitness -= 0.5;
}

void ace_minority_game_step(ACEAgent* agents, ACEStrategy** strategies,
                             int n_agents, int* history, int memory_length,
                             int round, unsigned int* seed)
{
    if (!agents || !strategies || !history || !seed || n_agents <= 0) return;
    int side_a = 0, side_b = 0;
    int* choices = (int*)malloc((size_t)n_agents * sizeof(int));
    if (!choices) return;
    for (int a = 0; a < n_agents; a++) {
        int best_s = 0;
        double best_fit = strategies[a][0].fitness;
        for (int s = 1; s < agents[a].n_strategies; s++)
            if (strategies[a][s].fitness > best_fit) {
                best_fit = strategies[a][s].fitness; best_s = s;
            }
        choices[a] = ace_minority_game_predict(
            &strategies[a][best_s], history, memory_length);
        if (choices[a] >= 1) side_a++; else side_b++;
    }
    int minority_side = (side_a < side_b) ? 1 : -1;
    for (int a = 0; a < n_agents; a++) {
        bool correct = (choices[a] == minority_side);
        for (int s = 0; s < agents[a].n_strategies; s++)
            ace_minority_game_update_fitness(&strategies[a][s], correct);
    }
    history[round % memory_length] = minority_side;
    free(choices);
}


double ace_dividend_generate(double mean, double std, unsigned int* seed)
{
    if (!seed) return mean;
    return mean + std * ace_random_gaussian(seed);
}

double ace_agent_price_expectation(const ACEAgent* agent,
    const ACEStrategy* strategies, const double* price_history,
    const double* dividend_history, int history_length, int current_period)
{
    if (!agent || !strategies || !price_history || !dividend_history)
        return 0.0;
    int best_s = 0;
    double best_fit = strategies[0].fitness;
    for (int s = 1; s < agent->n_strategies; s++)
        if (strategies[s].fitness > best_fit) {
            best_fit = strategies[s].fitness; best_s = s;
        }
    const ACEStrategy* strat = &strategies[best_s];
    double expectation = 0.0;
    for (int p = 0; p < strat->n_predictors && p < history_length; p++) {
        int lag = strat->predictor_lags[p];
        if (lag < current_period && current_period - lag - 1 >= 0)
            expectation += strat->coefficients[p]
                * price_history[current_period - lag - 1];
    }
    return expectation;
}

double ace_agent_demand(double price_expectation, double current_price,
    double expected_dividend, double interest_rate,
    double risk_aversion, double variance_estimate)
{
    double R = 1.0 + interest_rate;
    double numer = price_expectation + expected_dividend - R * current_price;
    double denom = risk_aversion * variance_estimate;
    if (denom < 1e-10) return 0.0;
    return numer / denom;
}

double ace_market_clearing(const double* demands, int n_agents,
    double supply, double min_price, double max_price, double precision)
{
    if (!demands || n_agents <= 0) return (min_price + max_price) / 2.0;
    double lo = min_price, hi = max_price;
    for (int iter = 0; iter < 100; iter++) {
        double mid = (lo + hi) / 2.0;
        double total_demand = 0.0;
        for (int i = 0; i < n_agents; i++) total_demand += demands[i];
        double avg_demand = total_demand / (double)n_agents;
        if (fabs(avg_demand - supply) < precision) return mid;
        if (avg_demand > supply) lo = mid; else hi = mid;
        if (hi - lo < precision) return mid;
    }
    return (lo + hi) / 2.0;
}

void ace_update_strategy_fitness(ACEStrategy* strategies, int n_strategies,
    double predicted_price, double actual_price, double learning_rate)
{
    if (!strategies) return;
    for (int i = 0; i < n_strategies; i++) {
        double error = fabs(predicted_price - actual_price)
            / (fabs(actual_price) + 1e-10);
        strategies[i].fitness -= learning_rate * error;
        strategies[i].times_activated++;
    }
}


void ace_genetic_algorithm_step(ACEStrategy* pool, int pool_size,
    int n_pred, double mut_rate, double cross_rate, int tourn_size,
    unsigned int* seed)
{
    if (!pool || !seed || pool_size < 4) return;
    ACEStrategy* np = (ACEStrategy*)malloc((size_t)pool_size * sizeof(ACEStrategy));
    if (!np) return;
    int best = 0;
    for (int i = 1; i < pool_size; i++)
        if (pool[i].fitness > pool[best].fitness) best = i;
    memcpy(&np[0], &pool[best], sizeof(ACEStrategy));
    for (int i = 1; i < pool_size; i++) {
        int p1 = 0, p2 = 1;
        double bs1 = -1e100, bs2 = -1e100;
        for (int t = 0; t < tourn_size; t++) {
            int idx = (int)(ace_random_uniform(seed) * pool_size);
            if (idx >= pool_size) idx = pool_size - 1;
            if (pool[idx].fitness > bs1) { bs1 = pool[idx].fitness; p1 = idx; }
        }
        for (int t = 0; t < tourn_size; t++) {
            int idx = (int)(ace_random_uniform(seed) * pool_size);
            if (idx >= pool_size) idx = pool_size - 1;
            if (idx != p1 && pool[idx].fitness > bs2) {
                bs2 = pool[idx].fitness; p2 = idx;
            }
        }
        if (p2 == p1) p2 = (p1 + 1) % pool_size;
        np[i] = pool[p1];
        np[i].id = i; np[i].fitness = 0.0;
        np[i].times_activated = 0; np[i].times_correct = 0;
        if (ace_random_uniform(seed) < cross_rate) {
            int cp = 1 + (int)(ace_random_uniform(seed) * (n_pred - 1));
            if (cp >= n_pred) cp = n_pred - 1;
            for (int j = cp; j < n_pred; j++) {
                np[i].coefficients[j] = pool[p2].coefficients[j];
                np[i].predictor_lags[j] = pool[p2].predictor_lags[j];
            }
        }
        for (int j = 0; j < n_pred; j++) {
            if (ace_random_uniform(seed) < mut_rate) {
                np[i].coefficients[j] += 0.1 * ace_random_gaussian(seed);
                if (ace_random_uniform(seed) < 0.1)
                    np[i].predictor_lags[j] = (int)(ace_random_uniform(seed) * n_pred);
            }
        }
    }
    memcpy(pool, np, (size_t)pool_size * sizeof(ACEStrategy));
    free(np);
}

void ace_stock_market_init(ACEStockMarketConfig* cfg, ACEAgent* agents,
    ACEStrategy** agent_strategies, unsigned int* seed)
{
    if (!cfg || !agents || !agent_strategies || !seed) return;
    for (int a = 0; a < cfg->n_agents; a++) {
        agents[a].id = a;
        agents[a].wealth = 10000.0;
        agents[a].risk_aversion = 0.5 + ace_random_uniform(seed);
        agents[a].n_strategies = cfg->n_strategies_pool;
        agents[a].learning_rate = 0.05;
        agents[a].agent_type = ACE_INDUCTIVE_LEARNER;
        agents[a].reasoning = ACE_INDUCTIVE_REASONING;
        for (int s = 0; s < cfg->n_strategies_pool; s++) {
            agent_strategies[a][s].id = s;
            agent_strategies[a][s].n_predictors = 10;
            agent_strategies[a][s].fitness = 100.0 * ace_random_uniform(seed);
            agent_strategies[a][s].times_activated = 0;
            agent_strategies[a][s].times_correct = 0;
            for (int p = 0; p < 10; p++) {
                agent_strategies[a][s].coefficients[p] =
                    -1.0 + 2.0 * ace_random_uniform(seed);
                agent_strategies[a][s].predictor_lags[p] =
                    (int)(ace_random_uniform(seed) * 10);
            }
        }
    }
}


void ace_stock_market_period(ACEStockMarketConfig* cfg, ACEAgent* agents,
    ACEStrategy** agent_strategies, double* price_history,
    double* dividend_history, int t, unsigned int* seed)
{
    if (!cfg || !agents || !agent_strategies || !price_history
        || !dividend_history || !seed) return;
    double dividend = ace_dividend_generate(
        cfg->dividend_mean, cfg->dividend_std, seed);
    dividend_history[t] = dividend;
    double prev_price = (t > 0) ? price_history[t-1] : cfg->fundamental_value;
    double* demands = (double*)malloc((size_t)cfg->n_agents * sizeof(double));
    if (!demands) return;
    double var_est = 4.0;
    if (t > 10) {
        double sum_p = 0.0, sum_p2 = 0.0;
        for (int k = t-10; k < t; k++) {
            sum_p += price_history[k]; sum_p2 += price_history[k]*price_history[k];
        }
        var_est = sum_p2/10.0 - (sum_p/10.0)*(sum_p/10.0);
        if (var_est < 1e-6) var_est = 1e-6;
    }
    double total_demand = 0.0;
    for (int a = 0; a < cfg->n_agents; a++) {
        double pe = ace_agent_price_expectation(&agents[a],
            agent_strategies[a], price_history, dividend_history, t, t);
        demands[a] = ace_agent_demand(pe, prev_price, dividend,
            cfg->interest_rate, agents[a].risk_aversion, var_est);
        total_demand += demands[a];
    }
    double clearing_price = prev_price + 0.1 * (total_demand / cfg->n_agents);
    if (clearing_price < 0.01) clearing_price = 0.01;
    price_history[t] = clearing_price;
    for (int a = 0; a < cfg->n_agents; a++) {
        ace_update_strategy_fitness(agent_strategies[a],
            cfg->n_strategies_pool, prev_price, clearing_price,
            agents[a].learning_rate);
    }
    if (t > 0 && t % 50 == 0) {
        for (int a = 0; a < cfg->n_agents; a++) {
            ace_genetic_algorithm_step(agent_strategies[a],
                cfg->n_strategies_pool, 10,
                cfg->mutation_rate, cfg->crossover_rate,
                (int)cfg->tournament_size, seed);
        }
    }
    free(demands);
}

void ace_stock_market_run(ACEStockMarketConfig* cfg, ACEAgent* agents,
    ACEStrategy** agent_strategies, double* price_history,
    double* dividend_history, unsigned int* seed)
{
    if (!cfg || !agents || !agent_strategies || !price_history
        || !dividend_history || !seed) return;
    price_history[0] = cfg->fundamental_value;
    dividend_history[0] = cfg->dividend_mean;
    for (int t = 1; t < cfg->n_periods; t++) {
        ace_stock_market_period(cfg, agents, agent_strategies,
            price_history, dividend_history, t, seed);
    }
}

ACEMarketRegime ace_detect_market_regime(const double* price_history,
    int n_periods, double fundamental_value)
{
    if (!price_history || n_periods < 10) return ACE_EQUILIBRIUM_REGIME;
    double mean_dev = 0.0, max_dev = 0.0, var = 0.0;
    for (int t = 0; t < n_periods; t++) {
        double dev = fabs(price_history[t] - fundamental_value)
            / fundamental_value;
        mean_dev += dev;
        if (dev > max_dev) max_dev = dev;
    }
    mean_dev /= (double)n_periods;
    for (int t = 0; t < n_periods; t++) {
        double d = (price_history[t] - fundamental_value) / fundamental_value;
        var += d * d;
    }
    var /= (double)n_periods;
    if (max_dev < 0.05 && mean_dev < 0.02)
        return ACE_EQUILIBRIUM_REGIME;
    if (max_dev > 1.0 || var > 1.0)
        return ACE_CHAOTIC_REGIME;
    if (mean_dev > 0.1 && mean_dev < 0.5)
        return ACE_COMPLEX_REGIME;
    return ACE_EVOLUTIONARY_REGIME;
}


ACEClassifierSystem* ace_classifier_system_alloc(int max_rules,
    double bid_fraction, double tax_rate)
{
    if (max_rules <= 0) return NULL;
    ACEClassifierSystem* cs = (ACEClassifierSystem*)malloc(
        sizeof(ACEClassifierSystem));
    if (!cs) return NULL;
    cs->rules = (ACEClassifier*)calloc((size_t)max_rules, sizeof(ACEClassifier));
    if (!cs->rules) { free(cs); return NULL; }
    cs->n_rules = 0;
    cs->max_rules = max_rules;
    cs->bid_fraction = bid_fraction;
    cs->tax_rate = tax_rate;
    cs->ga_threshold = 25.0;
    cs->ga_counter = 0;
    return cs;
}

void ace_classifier_system_free(ACEClassifierSystem* cs)
{
    if (!cs) return;
    free(cs->rules);
    free(cs);
}

int ace_classifier_match(const ACEClassifierSystem* cs,
    const char* market_state, int* matched_indices, int max_matches)
{
    if (!cs || !market_state || !matched_indices) return 0;
    int count = 0;
    int state_len = (int)strlen(market_state);
    for (int i = 0; i < cs->n_rules && count < max_matches; i++) {
        bool matches = true;
        for (int j = 0; j < state_len && j < ACE_CLASSIFIER_MAX_COND; j++) {
            char cond = cs->rules[i].condition[j];
            if (cond == '#') continue;
            if (cond != market_state[j]) { matches = false; break; }
        }
        if (matches) {
            matched_indices[count++] = i;
        }
    }
    return count;
}

void ace_classifier_random_condition(char* condition, int length,
    double specificity_target, unsigned int* seed)
{
    if (!condition || !seed || length <= 0) return;
    for (int i = 0; i < length && i < ACE_CLASSIFIER_MAX_COND; i++) {
        double r = ace_random_uniform(seed);
        if (r < specificity_target) {
            condition[i] = (ace_random_uniform(seed) < 0.5) ? '0' : '1';
        } else {
            condition[i] = '#';
        }
    }
    condition[(length < ACE_CLASSIFIER_MAX_COND) ? length : ACE_CLASSIFIER_MAX_COND] = '\0';
}

bool ace_classifier_add(ACEClassifierSystem* cs, const char* condition,
    const char* action, unsigned int* seed)
{
    if (!cs || !condition || !action) return false;
    if (cs->n_rules >= cs->max_rules) return false;
    ACEClassifier* rule = &cs->rules[cs->n_rules];
    strncpy(rule->condition, condition, ACE_CLASSIFIER_MAX_COND);
    rule->condition[ACE_CLASSIFIER_MAX_COND] = '\0';
    strncpy(rule->action, action, ACE_CLASSIFIER_MAX_COND);
    rule->action[ACE_CLASSIFIER_MAX_COND] = '\0';
    rule->strength = 100.0;
    rule->specificity = 0.0;
    int len = (int)strlen(condition);
    int specific = 0;
    for (int i = 0; i < len; i++)
        if (condition[i] != '#') specific++;
    rule->specificity = (len > 0) ? (double)specific / (double)len : 0.0;
    rule->age = 0;
    rule->is_active = false;
    cs->n_rules++;
    (void)seed;
    return true;
}

void ace_bucket_brigade(ACEClassifierSystem* cs,
    const int* active_indices, int n_active,
    const int* prev_active_indices, int n_prev_active,
    double environmental_reward)
{
    if (!cs) return;
    for (int i = 0; i < cs->n_rules; i++) {
        cs->rules[i].strength *= (1.0 - cs->tax_rate);
        cs->rules[i].age++;
        cs->rules[i].is_active = false;
    }
    for (int i = 0; i < n_active; i++) {
        int idx = active_indices[i];
        if (idx >= 0 && idx < cs->n_rules) {
            cs->rules[idx].is_active = true;
            double bid = cs->rules[idx].strength * cs->bid_fraction
                * cs->rules[idx].specificity;
            cs->rules[idx].strength -= bid;
            if (n_prev_active > 0) {
                double share = bid / (double)n_prev_active;
                for (int j = 0; j < n_prev_active; j++) {
                    int pidx = prev_active_indices[j];
                    if (pidx >= 0 && pidx < cs->n_rules)
                        cs->rules[pidx].strength += share;
                }
            }
        }
    }
    if (n_active > 0) {
        double share = environmental_reward / (double)n_active;
        for (int i = 0; i < n_active; i++) {
            int idx = active_indices[i];
            if (idx >= 0 && idx < cs->n_rules)
                cs->rules[idx].strength += share;
        }
    }
}

void ace_classifier_ga(ACEClassifierSystem* cs,
    double crossover_rate, double mutation_rate, unsigned int* seed)
{
    if (!cs || !seed || cs->n_rules < 4) return;
    cs->ga_counter++;
    if (cs->ga_counter < (int)cs->ga_threshold) return;
    cs->ga_counter = 0;
    int p1 = 0, p2 = 1;
    double s1 = -1e100, s2 = -1e100;
    for (int i = 0; i < cs->n_rules; i++) {
        if (cs->rules[i].strength > s1) { s2 = s1; p2 = p1; s1 = cs->rules[i].strength; p1 = i; }
        else if (cs->rules[i].strength > s2 && i != p1) { s2 = cs->rules[i].strength; p2 = i; }
    }
    if (cs->n_rules >= cs->max_rules) {
        int worst = 0;
        double ws = cs->rules[0].strength;
        for (int i = 1; i < cs->n_rules; i++)
            if (cs->rules[i].strength < ws) { ws = cs->rules[i].strength; worst = i; }
        ACEClassifier* child = &cs->rules[worst];
        strncpy(child->condition, cs->rules[p1].condition, ACE_CLASSIFIER_MAX_COND);
        child->condition[ACE_CLASSIFIER_MAX_COND] = '\0';
        int len = (int)strlen(child->condition);
        if (ace_random_uniform(seed) < crossover_rate) {
            int cp = (int)(ace_random_uniform(seed) * len);
            for (int i = cp; i < len; i++)
                child->condition[i] = cs->rules[p2].condition[i];
        }
        if (ace_random_uniform(seed) < mutation_rate) {
            int mp = (int)(ace_random_uniform(seed) * len);
            child->condition[mp] = (ace_random_uniform(seed) < 0.5) ? '0' : '1';
        }
        child->strength = (cs->rules[p1].strength + cs->rules[p2].strength) / 2.0;
        child->age = 0;
    }
}
