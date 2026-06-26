#ifndef ACE_MARKET_H
#define ACE_MARKET_H

#include "ace_core.h"

/* ============================================================================
 * Complex Adaptive Systems in Economics ? Market Models (L5-L6)
 *
 * Implements the El Farol Bar Problem (Arthur 1994) and the Santa Fe
 * Artificial Stock Market (Arthur, Holland, LeBaron, Palmer, Tayler 1997).
 *
 * Key insight: When agents are heterogeneous and inductively rational,
 * markets can self-organize into complex regimes that are neither fully
 * efficient nor fully chaotic. The market ecology evolves endogenously.
 * ============================================================================ */

/* ============================================================================
 * L6: El Farol Bar Problem (Arthur 1994)
 *
 * N agents independently decide each week whether to attend the bar.
 * If fewer than Capacity attend, those who went enjoy themselves (+1 utility).
 * If more than Capacity attend, it's overcrowded (-1 utility to attendees).
 * Those who stay home get 0 utility.
 *
 * Each agent has k predictive strategies (hypotheses about attendance).
 * Agents use their most accurate strategy each week.
 * The attendance fluctuates around the capacity ? a self-organized pattern.
 * ============================================================================ */

/* Initialize the El Farol simulation */
void ace_el_farol_init(ACEElFarolConfig* cfg, unsigned int* seed);

/* Generate random initial strategies for an agent */
void ace_el_farol_generate_strategies(ACEStrategy* strategies,
                                       int n_strategies, int memory_length,
                                       unsigned int* seed);

/* Predict attendance from a strategy given attendance history */
double ace_el_farol_predict(const ACEStrategy* strat,
                             const int* history, int memory_length,
                             int current_week);

/* Run one week of the El Farol problem:
   - Each agent predicts attendance using best strategy
   - Each agent decides to go or stay based on prediction vs capacity
   - Actual attendance is realized
   - Strategy fitness values are updated */
void ace_el_farol_step(ACEElFarolConfig* cfg, ACEAgent* agents,
                        ACEStrategy** agent_strategies,
                        int current_week, unsigned int* seed);

/* Run full El Farol simulation for n_weeks */
void ace_el_farol_run(ACEElFarolConfig* cfg, ACEAgent* agents,
                       ACEStrategy** agent_strategies,
                       unsigned int* seed);

/* Compute the mean attendance and its variance over the simulation */
void ace_el_farol_statistics(const ACEElFarolConfig* cfg,
                              double* mean_attendance,
                              double* variance);

/* ============================================================================
 * L5-L6: Minority Game (Challet & Zhang 1997, extension of El Farol)
 *
 * Generalization of El Farol: N agents choose side A or B each round.
 * The minority side wins. Agents have S strategies mapping past M outcomes
 * to a choice. Best-performing strategy determines each agent's action.
 * ============================================================================ */

/* Minority Game configuration embedded in El Farol structure */

/* Predict minority side from a binary strategy */
int ace_minority_game_predict(const ACEStrategy* strat,
                               const int* history, int memory_length);

/* Update strategy fitness based on whether prediction was correct */
void ace_minority_game_update_fitness(ACEStrategy* strat, bool was_correct);

/* Run one round of the minority game */
void ace_minority_game_step(ACEAgent* agents, ACEStrategy** strategies,
                             int n_agents, int* history, int memory_length,
                             int round, unsigned int* seed);

/* ============================================================================
 * L6: Santa Fe Artificial Stock Market (Arthur et al. 1997)
 *
 * Agents choose between a risk-free bond (return r) and a risky stock
 * paying stochastic dividend d_t. Agents form price expectations using
 * a pool of forecasting strategies (classifier systems).
 *
 * Market price emerges from agent bids/offers. Strategy fitness evolves
 * via a genetic algorithm. The market self-organizes into:
 *   - Rational expectations regime (all fundamentalists)
 *   - Complex regime (mix of fundamentalists and chartists)
 *   - Bubble/crash regime (chartists dominate)
 * ============================================================================ */

/* Initialize the artificial stock market simulation */
void ace_stock_market_init(ACEStockMarketConfig* cfg,
                            ACEAgent* agents, ACEStrategy** agent_strategies,
                            unsigned int* seed);

/* Generate a dividend from the stochastic dividend process */
double ace_dividend_generate(double mean, double std, unsigned int* seed);

/* Agent forms a price expectation using its best strategy */
double ace_agent_price_expectation(const ACEAgent* agent,
                                    const ACEStrategy* strategies,
                                    const double* price_history,
                                    const double* dividend_history,
                                    int history_length,
                                    int current_period);

/* Compute agent's demand for the risky asset given price expectation.
   Uses CARA (constant absolute risk aversion) utility:
   demand = (E[p_{t+1} + d_{t+1}] - (1+r)*p_t) / (lambda * sigma^2) */
double ace_agent_demand(double price_expectation, double current_price,
                         double expected_dividend, double interest_rate,
                         double risk_aversion, double variance_estimate);

/* Market clearing: find price that equates total demand to supply */
double ace_market_clearing(const double* demands, int n_agents,
                            double supply, double min_price, double max_price,
                            double precision);

/* Update strategy fitness using prediction error.
   fitness_new = fitness_old - eta * |predicted - actual| / actual */
void ace_update_strategy_fitness(ACEStrategy* strategies,
                                  int n_strategies,
                                  double predicted_price,
                                  double actual_price,
                                  double learning_rate);

/* Genetic algorithm step: select, crossover, and mutate strategies.
   Selection: tournament selection of size k.
   Crossover: uniform crossover with probability p_c.
   Mutation: each coefficient mutates with probability p_m. */
void ace_genetic_algorithm_step(ACEStrategy* strategy_pool,
                                 int pool_size,
                                 int n_predictors,
                                 double mutation_rate,
                                 double crossover_rate,
                                 int tournament_size,
                                 unsigned int* seed);

/* Run one period of the artificial stock market */
void ace_stock_market_period(ACEStockMarketConfig* cfg,
                              ACEAgent* agents,
                              ACEStrategy** agent_strategies,
                              double* price_history,
                              double* dividend_history,
                              int current_period,
                              unsigned int* seed);

/* Run full stock market simulation for n_periods */
void ace_stock_market_run(ACEStockMarketConfig* cfg,
                           ACEAgent* agents,
                           ACEStrategy** agent_strategies,
                           double* price_history,
                           double* dividend_history,
                           unsigned int* seed);

/* Detect the market regime: equilibrium / complex / chaotic */
ACEMarketRegime ace_detect_market_regime(const double* price_history,
                                          int n_periods,
                                          double fundamental_value);

/* ============================================================================
 * L5: Classifier System (Holland 1975, 1986; Arthur et al 1997)
 *
 * A classifier is an IF-THEN rule: IF (condition on market state) THEN (prediction).
 * Conditions are ternary strings over {0, 1, #} where # is "don't care".
 * The bucket brigade algorithm distributes credit among classifier chains.
 * ============================================================================ */

/* Maximum length of a classifier condition string */
#define ACE_CLASSIFIER_MAX_COND 32

/* A single classifier rule */
typedef struct {
    char    condition[ACE_CLASSIFIER_MAX_COND + 1];  /* ternary string {0,1,#} */
    char    action[ACE_CLASSIFIER_MAX_COND + 1];     /* predicted price direction */
    double  strength;          /* classifier's strength (fitness) */
    double  specificity;       /* fraction of non-# positions */
    int     age;               /* iterations since creation */
    bool    is_active;         /* whether this classifier fired */
} ACEClassifier;

/* Classifier system: a population of rules */
typedef struct {
    ACEClassifier* rules;
    int     n_rules;
    int     max_rules;
    double  bid_fraction;      /* fraction of strength bid by active classifiers */
    double  tax_rate;          /* per-iteration strength tax */
    double  ga_threshold;      /* minimum time between GA invocations */
    int     ga_counter;
} ACEClassifierSystem;

/* Initialize a classifier system */
ACEClassifierSystem* ace_classifier_system_alloc(int max_rules,
                                                  double bid_fraction,
                                                  double tax_rate);

/* Free classifier system */
void ace_classifier_system_free(ACEClassifierSystem* cs);

/* Match classifiers whose conditions match the current market state */
int ace_classifier_match(const ACEClassifierSystem* cs,
                          const char* market_state,
                          int* matched_indices,
                          int max_matches);

/* Generate a random classifier condition */
void ace_classifier_random_condition(char* condition, int length,
                                      double specificity_target,
                                      unsigned int* seed);

/* Add a new classifier to the system */
bool ace_classifier_add(ACEClassifierSystem* cs, const char* condition,
                         const char* action, unsigned int* seed);

/* Bucket brigade credit assignment: winning classifiers pay bid to
   previously active classifiers, receive reward from environment */
void ace_bucket_brigade(ACEClassifierSystem* cs,
                         const int* active_indices, int n_active,
                         const int* prev_active_indices, int n_prev_active,
                         double environmental_reward);

/* Classifier system genetic algorithm: select, crossover, mutate */
void ace_classifier_ga(ACEClassifierSystem* cs,
                        double crossover_rate, double mutation_rate,
                        unsigned int* seed);

#endif /* ACE_MARKET_H */
