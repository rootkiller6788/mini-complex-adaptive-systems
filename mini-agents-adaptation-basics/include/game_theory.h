/**
 * game_theory.h — Game Theory and Evolutionary Dynamics
 *
 * Strategic interactions between self-interested agents. Covers normal-form
 * games, Nash equilibrium, evolutionary game theory, replicator dynamics,
 * and iterated Prisoner's Dilemma tournaments.
 *
 * Reference: Nash (1951), Maynard Smith & Price (1973), Taylor & Jonker (1978)
 *            Axelrod (1984), Hofbauer & Sigmund (1998), Nowak (2006)
 *
 * Knowledge coverage: L1 (Definitions), L2 (Core Concepts), L4 (Nash, ESS),
 *                     L5 (Equilibrium computation), L6 (PD canon), L7 (Tournaments)
 */

#ifndef GAME_THEORY_H
#define GAME_THEORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

/* L1: Normal-Form Game */
typedef struct {
    int      rows;
    int      cols;
    double **payoff_p1;
    double **payoff_p2;
    char   **row_names;
    char   **col_names;
    bool     is_symmetric;
} NormalFormGame;

typedef struct {
    int      num_strategies;
    double  *probs;
} MixedStrategy;

typedef struct {
    MixedStrategy p1;
    MixedStrategy p2;
} StrategyProfile;

NormalFormGame* nfg_create(int rows, int cols, bool symmetric);
void   nfg_set_payoff(NormalFormGame *game, int row, int col,
                       double p1_payoff, double p2_payoff);
double nfg_expected_payoff_p1(const NormalFormGame *game,
                               const MixedStrategy *s1, const MixedStrategy *s2);
double nfg_expected_payoff_p2(const NormalFormGame *game,
                               const MixedStrategy *s1, const MixedStrategy *s2);
double nfg_pure_vs_mixed_p1(const NormalFormGame *game, int pure_row,
                              const MixedStrategy *s2);
int    nfg_best_response_p1(const NormalFormGame *game, const MixedStrategy *s2);
void   nfg_best_response_values_p1(const NormalFormGame *game,
                                    const MixedStrategy *s2, double *values_out);
bool   nfg_is_nash(const NormalFormGame *game, const StrategyProfile *profile,
                    double tolerance);
void   nfg_destroy(NormalFormGame *game);

/* L4+L5: Nash equilibrium computation */
StrategyProfile* nfg_lemke_howson(const NormalFormGame *game);
int nfg_enumeration_find_all(const NormalFormGame *game,
                              StrategyProfile **profiles_out, int max_profiles);

/* L6: Canonical games */
NormalFormGame* nfg_prisoners_dilemma(double T, double R, double P, double S);
NormalFormGame* nfg_battle_of_sexes(void);
NormalFormGame* nfg_stag_hunt(double payoff_stag, double payoff_hare);
NormalFormGame* nfg_hawk_dove(double V, double C);
NormalFormGame* nfg_rock_paper_scissors(void);
NormalFormGame* nfg_matching_pennies(void);

/* L4: Replicator Dynamics (Evolutionary Game Theory) */
typedef struct {
    int      num_strategies;
    double  *frequencies;
    double  *payoffs;
} ReplicatorState;

ReplicatorState* replicator_create(int num_strategies,
                                    const double *payoff_matrix,
                                    const double *initial_freqs);
void   replicator_step(ReplicatorState *rs, double dt);
void   replicator_run(ReplicatorState *rs, int n_steps, double dt,
                       double *history_out, double *avg_fitness_history);
bool   replicator_is_ess(const ReplicatorState *rs, int strategy_idx,
                          double epsilon);
int    replicator_find_fixed_points(const ReplicatorState *rs,
                                     double *fixed_points_out, int max_points,
                                     double tolerance);
double replicator_strategy_fitness(const ReplicatorState *rs, int strategy);
double replicator_average_fitness(const ReplicatorState *rs);
void   replicator_jacobian(const ReplicatorState *rs, double *J_out);
void   replicator_destroy(ReplicatorState *rs);

/* L6+L7: Iterated Prisoner's Dilemma strategies (Axelrod, 1984) */
typedef enum {
    PD_COOPERATE = 0,
    PD_DEFECT    = 1
} PDAction;

typedef struct {
    int        max_history;
    int        length;
    PDAction  *my_moves;
    PDAction  *opponent_moves;
} PDHistory;

typedef PDAction (*PDStrategy)(const PDHistory *history);

PDAction pd_tit_for_tat(const PDHistory *h);
PDAction pd_always_defect(const PDHistory *h);
PDAction pd_always_cooperate(const PDHistory *h);
PDAction pd_grim_trigger(const PDHistory *h);
PDAction pd_generous_tit_for_tat(const PDHistory *h);
PDAction pd_pavlov(const PDHistory *h);
PDAction pd_random_strategy(const PDHistory *h);
PDAction pd_two_tits_for_tat(const PDHistory *h);

void pd_play_round(const NormalFormGame *pd_game,
                    PDStrategy s1, PDStrategy s2,
                    PDHistory *h1, PDHistory *h2,
                    PDAction *a1_out, PDAction *a2_out,
                    double *payoff1_out, double *payoff2_out);
void pd_tournament(const NormalFormGame *pd_game,
                    PDStrategy s1, PDStrategy s2,
                    int n_rounds, double *total_p1, double *total_p2);
PDHistory* pd_history_create(int max_history);
void       pd_history_record(PDHistory *h, PDAction my, PDAction opp);
void       pd_history_destroy(PDHistory *h);
void pd_round_robin(const NormalFormGame *pd_game,
                     PDStrategy *strategies, const char **names, int n_strategies,
                     int n_rounds, double *scores_out);

/* L8: Correlated Equilibrium (Aumann, 1974) */
typedef struct {
    int      rows;
    int      cols;
    double **joint_probs;
} CorrelatedEquilibrium;

bool   ce_is_valid(const NormalFormGame *game, const CorrelatedEquilibrium *ce,
                    double tolerance);
double ce_expected_payoff_p1(const NormalFormGame *game,
                              const CorrelatedEquilibrium *ce);
double ce_expected_payoff_p2(const NormalFormGame *game,
                              const CorrelatedEquilibrium *ce);
void   ce_destroy(CorrelatedEquilibrium *ce);

#endif /* GAME_THEORY_H */
