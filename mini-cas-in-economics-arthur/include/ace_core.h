#ifndef ACE_CORE_H
#define ACE_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* ============================================================================
 * Complex Adaptive Systems in Economics ? Core Definitions (L1)
 *
 * Based on the foundational works of W. Brian Arthur:
 *   "Increasing Returns and Path Dependence in the Economy" (1994)
 *   "Complexity and the Economy" (Science, 1999; Oxford, 2014)
 *   "The Nature of Technology" (2009)
 *   "Foundations of Complexity Economics" (Nature Reviews Physics, 2021)
 *
 * with contributions from:
 *   John H. Holland    (genetic algorithms, classifier systems)
 *   Stuart Kauffman    (NK fitness landscapes, origins of order)
 *   Kenneth Arrow      (general equilibrium, increasing returns)
 *   Paul David         (path dependence, QWERTY economics)
 *   Thomas Schelling   (micromotives and macrobehavior, tipping points)
 * ============================================================================ */

/* ============================================================================
 * L1: Core Definitions
 * ============================================================================ */

/* --- Market Regime Enumeration (Arthur 1999) --- */
typedef enum {
    ACE_EQUILIBRIUM_REGIME = 0,    /* Standard equilibrium: diminishing returns dominate */
    ACE_COMPLEX_REGIME     = 1,    /* Non-equilibrium: heterogeneous agents, emergent patterns */
    ACE_CHAOTIC_REGIME     = 2,    /* Highly volatile: speculative bubbles, crashes */
    ACE_EVOLUTIONARY_REGIME = 3    /* Structural evolution: new technologies, institutions emerge */
} ACEMarketRegime;

/* --- Reasoning Mode (Arthur 1994, El Farol problem) --- */
typedef enum {
    ACE_DEDUCTIVE_REASONING  = 0,  /* Deduce optimal action from known structure */
    ACE_INDUCTIVE_REASONING  = 1,  /* Form hypotheses from past patterns, test, discard */
    ACE_ABDUCTIVE_REASONING  = 2,  /* Infer best explanation; hypothesis generation */
    ACE_BOUNDED_RATIONALITY  = 3   /* Simon's satisficing; limited cognitive capacity */
} ACEReasoningMode;

/* --- Feedback Type (Arthur 1989, 1994) --- */
typedef enum {
    ACE_DIMINISHING_RETURNS = 0,   /* Negative feedback: convergence to unique equilibrium */
    ACE_CONSTANT_RETURNS    = 1,   /* Neutral feedback: path independent */
    ACE_INCREASING_RETURNS  = 2,   /* Positive feedback: self-reinforcement, multiple equilibria */
    ACE_NETWORK_EXTERNALITY = 3    /* Positive feedback through adoption network effects */
} ACEFeedbackType;

/* --- Lock-in State (Arthur 1989; David 1985) --- */
typedef enum {
    ACE_NOT_LOCKED    = 0,         /* Multiple technologies still viable */
    ACE_PARTIAL_LOCK  = 1,         /* One technology dominant but not exclusive */
    ACE_FULL_LOCK     = 2,         /* Irreversible adoption of one technology */
    ACE_LOCK_ESCAPE   = 3          /* Escape from lock-in via external shock or innovation */
} ACELockState;

/* --- Emergence Type --- */
typedef enum {
    ACE_WEAK_EMERGENCE    = 0,     /* Aggregate patterns derivable from micro-rules (in principle) */
    ACE_STRONG_EMERGENCE  = 1,     /* Novel properties not derivable even in principle */
    ACE_NOMOLOGICAL_EMERGENCE = 2  /* Law-like regularities emergent at macro scale */
} ACEEmergenceType;

/* --- Technology Type (Arthur 2009, "The Nature of Technology") --- */
typedef enum {
    ACE_PRIMITIVE_TECH   = 0,      /* Harnesses a single physical phenomenon */
    ACE_COMPOSITE_TECH   = 1,      /* Assembly of sub-technologies */
    ACE_PLATFORM_TECH    = 2,      /* Enables/constrains many derivative technologies */
    ACE_ENABLING_TECH    = 3,      /* Opens new technological domains (e.g., transistor, CRISPR) */
    ACE_AUTOCATALYTIC_TECH = 4    /* Self-catalyzing technology cluster */
} ACETechnologyType;

/* --- Agent Strategy Type --- */
typedef enum {
    ACE_ZERO_INTELLIGENCE = 0,     /* Random trader (Gode & Sunder 1993) */
    ACE_FUNDAMENTALIST     = 1,    /* Value-based; expects reversion to fundamental value */
    ACE_CHARTIST           = 2,    /* Technical trader; extrapolates trends */
    ACE_INDUCTIVE_LEARNER  = 3,    /* Arthur's inductive agent: generates/test hypotheses */
    ACE_GENETIC_LEARNER    = 4,    /* Holland's classifier system with genetic algorithm */
    ACE_REINFORCEMENT_LEARNER = 5  /* Q-learning / temporal difference learning */
} ACEAgentStrategy;

/* ============================================================================
 * L2: Core Concepts ? Data Structures
 * ============================================================================ */

/* --- Adoption Share State: tracks market share of competing technologies --- */
typedef struct {
    double* shares;          /* market share of each technology [0,1], sum=1 */
    int     n_technologies;  /* number of competing technologies */
    double  time;            /* current time stamp */
} ACEAdoptionState;

/* --- Agent: bounded-rational economic agent --- */
typedef struct {
    int     id;                    /* unique agent identifier */
    double  wealth;                /* current wealth/endowment */
    double  risk_aversion;         /* Arrow-Pratt coefficient */
    int     n_strategies;          /* number of active predictive strategies */
    double* strategy_weights;      /* weight/confidence in each strategy */
    double* strategy_predictions;  /* current prediction from each strategy */
    double  learning_rate;         /* exploration vs exploitation parameter */
    ACEAgentStrategy agent_type;   /* type of decision-making algorithm */
    ACEReasoningMode reasoning;    /* deductive / inductive / abductive */
} ACEAgent;

/* --- Prediction Strategy: a hypothesis about market behavior (Arthur 1994) --- */
typedef struct {
    int     id;                  /* strategy identifier */
    int     n_predictors;        /* number of predictors used */
    double* coefficients;        /* linear combination coefficients */
    int*    predictor_lags;      /* which lagged observations used */
    double  fitness;             /* accumulated prediction accuracy */
    int     times_activated;     /* how often this strategy was used */
    int     times_correct;       /* how often within tolerance */
} ACEStrategy;

/* --- Path History: records the stochastic path of an adoption process --- */
typedef struct {
    double* market_shares;       /* time series of market shares (flattened) */
    double* time_points;        /* sampled time points */
    int     n_steps;            /* number of recorded steps */
    int     n_technologies;     /* number of competing technologies */
    double  final_state;        /* terminal market share of dominant technology */
    bool    is_locked;          /* whether irreversible lock-in occurred */
} ACEPathHistory;

/* --- Polya Urn Configuration (Arthur, Ermoliev, Kaniovski 1987) --- */
typedef struct {
    int     n_colors;           /* number of ball colors (technology alternatives) */
    int*    initial_balls;      /* initial count per color */
    int*    current_balls;      /* current count per color */
    double* addition_function;  /* Arthur's urn function: f(proportion) balls added */
    int     total_balls;        /* running total */
    double  self_reinforcement; /* strength of increasing returns parameter */
    bool    use_generalized;    /* if true, use generalized Polya with arbitrary f() */
} ACEPolyaUrn;

/* --- Technology (Arthur 2009): a means to fulfill a human purpose --- */
typedef struct {
    int     tech_id;
    char    name[64];
    int     n_components;        /* number of sub-technologies combined */
    int*    component_ids;       /* IDs of component technologies */
    int     n_phenomena;         /* physical phenomena harnessed */
    char    phenomena[8][64];    /* e.g., "electron tunneling", "fluid dynamics" */
    double  fitness;             /* current performance measure */
    double  adoption_rate;       /* current market adoption */
    double  combinatorial_potential; /* potential for further combination */
    ACETechnologyType tech_type;
    bool    is_dominant_design;  /* Has this become the industry standard design? */
} ACETechnology;

/* --- Technology Ecosystem (Arthur & Polak 2006) --- */
typedef struct {
    int         n_technologies;
    ACETechnology* technologies;
    double**    complementarity;  /* nxn matrix: complementarity between tech i,j */
    double**    competition;      /* nxn matrix: substitution/competition */
    int*        cluster_ids;      /* emergent technology cluster membership */
    int         n_clusters;
    double      emergence_rate;   /* rate of new technology emergence */
} ACETechEcosystem;

/* ============================================================================
 * L3: Mathematical Structure Types
 * ============================================================================ */

/* --- Stochastic Process Descriptor --- */
typedef struct {
    int     state_dimension;
    double* state;               /* current state vector */
    double* drift;               /* deterministic drift component */
    double** diffusion;          /* stochastic diffusion matrix (nxn) */
    double  time_step;
    bool    is_markov;
    bool    is_ergodic;          /* true if long-run distribution independent of initial state */
    bool    is_path_dependent;   /* true if history matters for limiting outcome */
} ACEStochasticProcess;

/* --- Network Structure for agent interactions --- */
typedef struct {
    int     n_nodes;
    int**   adjacency;           /* adjacency matrix */
    int*    degree;              /* degree of each node */
    double  clustering_coeff;    /* average clustering coefficient */
    double  avg_path_length;     /* average shortest path length */
    bool    is_scale_free;       /* power-law degree distribution */
    bool    is_small_world;      /* Watts-Strogatz small world property */
} ACENetwork;

/* --- Time Series for economic indicators --- */
typedef struct {
    double* data;
    int     length;
    int     capacity;
    double  mean;
    double  variance;
    double  skewness;
    double  kurtosis;
    bool    is_stationary;
    double  hurst_exponent;      /* long-memory parameter */
} ACETimeSeries;

/* --- Distribution: empirical outcome distribution --- */
typedef struct {
    double* bins;
    double* probabilities;
    int     n_bins;
    double  range_min;
    double  range_max;
    double  bin_width;
    double  gini_coefficient;    /* inequality measure for economic outcomes */
    bool    is_heavy_tailed;     /* power-law tail detected */
    double  tail_exponent;       /* estimated Pareto exponent */
} ACEDistribution;

/* --- Macro State: aggregate economic observables --- */
typedef struct {
    double  gdp_growth;          /* economic growth rate */
    double  herfindahl_index;    /* market concentration */
    double  entropy;             /* economic diversity (Shannon entropy) */
    double  innovation_rate;     /* new product/technology introduction rate */
    double  inequality;          /* Gini coefficient */
    int     n_active_agents;     /* number of economically active agents */
    int     n_bankruptcies;
    double  market_volatility;
    bool    is_crisis;           /* systemic crisis flag */
} ACEMacroState;

/* ============================================================================
 * L4-L6: Core Algorithm and Problem Signatures
 * ============================================================================ */

/* --- El Farol Bar Problem Configuration --- */
typedef struct {
    int     n_agents;            /* number of potential bar-goers */
    int     capacity;            /* comfort capacity of the bar */
    int     n_strategies_per_agent;
    int     memory_length;       /* how many past weeks agents remember */
    int     n_weeks;             /* simulation duration */
    int*    attendance_history;  /* actual attendance each week */
    double  overcrowding_penalty;/* utility loss when bar is overcrowded */
    double  staying_home_utility;/* utility of staying home */
} ACEElFarolConfig;

/* --- Artificial Stock Market Configuration (Arthur et al. 1997) --- */
typedef struct {
    int     n_agents;
    int     n_strategies_pool;   /* total strategies in agent's pool */
    double  dividend_mean;       /* mean of dividend process */
    double  dividend_std;        /* std deviation of dividend */
    double  interest_rate;       /* risk-free rate */
    double  fundamental_value;   /* rational expectations equilibrium price */
    int     n_periods;           /* simulation periods */
    double  mutation_rate;       /* GA mutation rate for strategy evolution */
    double  crossover_rate;      /* GA crossover rate */
    double  tournament_size;     /* selection pressure */
} ACEStockMarketConfig;

/* --- NK Fitness Landscape (Kauffman 1993, applied to economics) --- */
typedef struct {
    int     N;                   /* number of technology/strategy components */
    int     K;                   /* epistatic interactions per component */
    double** fitness_table;      /* 2^N x N contribution table */
    double*  fitness_landscape;  /* full fitness values for all 2^N configurations */
    bool*   is_local_peak;       /* for each configuration: is it a local optimum? */
    int     n_peaks;             /* number of local optima */
    double  ruggedness;          /* landscape ruggedness measure */
} ACENKFitnessLandscape;

/* --- Combinatorial Evolution Configuration (Arthur 2009) --- */
typedef struct {
    int     n_initial_tech;      /* starting number of primitive technologies */
    int     max_components;      /* maximum components per composite technology */
    double  combination_rate;    /* probability of new combination per period */
    double  selection_pressure;  /* fitness advantage needed for adoption */
    int     n_periods;           /* evolution periods */
    bool    autocatalytic;       /* enable autocatalytic feedback */
    double  novelty_decay;       /* rate at which novelty advantage decays */
} ACECombinatorialConfig;

/* ============================================================================
 * API: Initialization / Allocation
 * ============================================================================ */

/* Allocate and initialize adoption state */
ACEAdoptionState* ace_adoption_state_alloc(int n_technologies);

/* Free adoption state */
void ace_adoption_state_free(ACEAdoptionState* state);

/* Allocate and initialize an agent */
ACEAgent* ace_agent_alloc(int id, double wealth, double risk_aversion,
                          int n_strategies, ACEAgentStrategy agent_type,
                          ACEReasoningMode reasoning);

/* Free agent */
void ace_agent_free(ACEAgent* agent);

/* Allocate a prediction strategy */
ACEStrategy* ace_strategy_alloc(int id, int n_predictors);

/* Free strategy */
void ace_strategy_free(ACEStrategy* strat);

/* Allocate path history */
ACEPathHistory* ace_path_history_alloc(int max_steps, int n_technologies);

/* Free path history */
void ace_path_history_free(ACEPathHistory* hist);

/* Allocate Polya urn */
ACEPolyaUrn* ace_polya_urn_alloc(int n_colors, const int* initial_counts,
                                 double self_reinforcement);

/* Free Polya urn */
void ace_polya_urn_free(ACEPolyaUrn* urn);

/* Allocate technology */
ACETechnology* ace_technology_alloc(int tech_id, const char* name,
                                    ACETechnologyType type);

/* Free technology */
void ace_technology_free(ACETechnology* tech);

/* Allocate technology ecosystem */
ACETechEcosystem* ace_tech_ecosystem_alloc(int n_technologies);

/* Free technology ecosystem */
void ace_tech_ecosystem_free(ACETechEcosystem* eco);

/* Allocate time series */
ACETimeSeries* ace_timeseries_alloc(int capacity);

/* Free time series */
void ace_timeseries_free(ACETimeSeries* ts);

/* Allocate network */
ACENetwork* ace_network_alloc(int n_nodes);

/* Free network */
void ace_network_free(ACENetwork* net);

/* Allocate distribution */
ACEDistribution* ace_distribution_alloc(int n_bins, double min, double max);

/* Free distribution */
void ace_distribution_free(ACEDistribution* dist);

/* Allocate macro state */
ACEMacroState* ace_macro_state_alloc(void);

/* Free macro state */
void ace_macro_state_free(ACEMacroState* ms);

/* Allocate stochastic process */
ACEStochasticProcess* ace_stochastic_process_alloc(int dim);

/* Free stochastic process */
void ace_stochastic_process_free(ACEStochasticProcess* sp);

/* Allocate El Farol config */
ACEElFarolConfig* ace_el_farol_config_alloc(int n_agents, int capacity,
                                             int n_strategies_per_agent,
                                             int memory_length, int n_weeks);

/* Free El Farol config */
void ace_el_farol_config_free(ACEElFarolConfig* cfg);

/* Allocate stock market config */
ACEStockMarketConfig* ace_stock_market_config_alloc(int n_agents,
                                                    int n_strategies_pool,
                                                    int n_periods);

/* Free stock market config */
void ace_stock_market_config_free(ACEStockMarketConfig* cfg);

/* Allocate NK landscape */
ACENKFitnessLandscape* ace_nk_landscape_alloc(int N, int K);

/* Free NK landscape */
void ace_nk_landscape_free(ACENKFitnessLandscape* nk);

/* Allocate combinatorial config */
ACECombinatorialConfig* ace_combinatorial_config_alloc(int n_initial_tech,
                                                        int max_components,
                                                        int n_periods);

/* Free combinatorial config */
void ace_combinatorial_config_free(ACECombinatorialConfig* cfg);

/* --- Utility Functions --- */

/* Compute Shannon entropy of a probability distribution over n categories */
double ace_shannon_entropy(const double* proportions, int n);

/* Compute Herfindahl-Hirschman Index (market concentration) */
double ace_herfindahl_index(const double* market_shares, int n_firms);

/* Generate a uniform random double in [0,1] using provided seed */
double ace_random_uniform(unsigned int* seed);

/* Generate a Gaussian random variable via Box-Muller (mu=0, sigma=1) */
double ace_random_gaussian(unsigned int* seed);

/* Softmax: convert scores to probability distribution (temperature parameter) */
void ace_softmax(const double* scores, int n, double temperature, double* out);

/* Linear congruential generator step (for reproducible stochastic processes) */
unsigned int ace_lcg_step(unsigned int* seed);

/* Check if two doubles are within tolerance */
bool ace_fequal(double a, double b, double tolerance);

#endif /* ACE_CORE_H */
