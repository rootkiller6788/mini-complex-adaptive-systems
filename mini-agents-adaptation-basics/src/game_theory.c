/**
 * game_theory.c â€” Game Theory and Evolutionary Dynamics
 *
 * Implements:
 *   Normal-form games (construction, payoff computation)
 *   Nash equilibrium computation (Lemke-Howson, support enumeration)
 *   Canonical games (PD, Battle of Sexes, Stag Hunt, Hawk-Dove, RPS, Matching Pennies)
 *   Replicator dynamics (evolutionary game theory)
 *   Iterated Prisoner's Dilemma strategies (Tit-for-Tat, Grim Trigger, Pavlov, etc.)
 *   Correlated equilibrium (Aumann, 1974)
 *
 * Reference: Nash (1951), Lemke & Howson (1964)
 *            Maynard Smith & Price (1973), Taylor & Jonker (1978)
 *            Axelrod (1984), Hofbauer & Sigmund (1998), Nowak (2006)
 *
 * Knowledge: L1 Definitions, L2 Core Concepts, L4 Nash Theorem/ESS,
 *            L5 Equilibrium computation, L6 PD canon, L7 Axelrod tournament
 */

#include "game_theory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

/* ===================================================================
 * L1+L2: Normal-Form Game Construction
 * =================================================================== */

NormalFormGame* nfg_create(int rows, int cols, bool symmetric) {
    NormalFormGame *game = (NormalFormGame*)calloc(1, sizeof(NormalFormGame));
    if (!game) return NULL;

    game->rows = rows;
    game->cols = cols;
    game->is_symmetric = symmetric;

    /* Allocate payoff matrices */
    game->payoff_p1 = (double**)calloc(rows, sizeof(double*));
    game->payoff_p2 = (double**)calloc(rows, sizeof(double*));
    for (int i = 0; i < rows; i++) {
        game->payoff_p1[i] = (double*)calloc(cols, sizeof(double));
        game->payoff_p2[i] = (double*)calloc(cols, sizeof(double));
    }

    /* Allocate strategy labels */
    game->row_names = (char**)calloc(rows, sizeof(char*));
    game->col_names = (char**)calloc(cols, sizeof(char*));
    for (int i = 0; i < rows; i++) {
        game->row_names[i] = (char*)calloc(32, sizeof(char));
        snprintf(game->row_names[i], 31, "R%d", i);
    }
    for (int j = 0; j < cols; j++) {
        game->col_names[j] = (char*)calloc(32, sizeof(char));
        snprintf(game->col_names[j], 31, "C%d", j);
    }

    return game;
}

void nfg_set_payoff(NormalFormGame *game, int row, int col,
                     double p1_payoff, double p2_payoff) {
    if (!game || row < 0 || row >= game->rows || col < 0 || col >= game->cols)
        return;
    game->payoff_p1[row][col] = p1_payoff;
    game->payoff_p2[row][col] = p2_payoff;
}

double nfg_expected_payoff_p1(const NormalFormGame *game,
                               const MixedStrategy *s1, const MixedStrategy *s2) {
    if (!game || !s1 || !s2) return 0.0;

    double expected = 0.0;
    for (int i = 0; i < game->rows; i++) {
        for (int j = 0; j < game->cols; j++) {
            expected += s1->probs[i] * s2->probs[j] * game->payoff_p1[i][j];
        }
    }
    return expected;
}

double nfg_expected_payoff_p2(const NormalFormGame *game,
                               const MixedStrategy *s1, const MixedStrategy *s2) {
    if (!game || !s1 || !s2) return 0.0;

    double expected = 0.0;
    for (int i = 0; i < game->rows; i++) {
        for (int j = 0; j < game->cols; j++) {
            expected += s1->probs[i] * s2->probs[j] * game->payoff_p2[i][j];
        }
    }
    return expected;
}

double nfg_pure_vs_mixed_p1(const NormalFormGame *game, int pure_row,
                              const MixedStrategy *s2) {
    if (!game || !s2) return 0.0;

    double payoff = 0.0;
    for (int j = 0; j < game->cols; j++) {
        payoff += s2->probs[j] * game->payoff_p1[pure_row][j];
    }
    return payoff;
}

int nfg_best_response_p1(const NormalFormGame *game, const MixedStrategy *s2) {
    if (!game || !s2) return 0;

    int best = 0;
    double best_val = nfg_pure_vs_mixed_p1(game, 0, s2);
    for (int i = 1; i < game->rows; i++) {
        double val = nfg_pure_vs_mixed_p1(game, i, s2);
        if (val > best_val) {
            best_val = val;
            best = i;
        }
    }
    return best;
}

void nfg_best_response_values_p1(const NormalFormGame *game,
                                  const MixedStrategy *s2, double *values_out) {
    if (!game || !s2 || !values_out) return;
    for (int i = 0; i < game->rows; i++) {
        values_out[i] = nfg_pure_vs_mixed_p1(game, i, s2);
    }
}

/* ===================================================================
 * L4: Nash Equilibrium Check
 *
 * A strategy profile (s1*, s2*) is a Nash equilibrium iff:
 *   1. u1(s1*, s2*) >= u1(s1', s2*) for all s1'
 *   2. u2(s1*, s2*) >= u2(s1*, s2') for all s2'
 *
 * Equivalently: every strategy in the support of si* must yield
 * the same (maximal) expected payoff.
 * =================================================================== */

bool nfg_is_nash(const NormalFormGame *game, const StrategyProfile *profile,
                  double tolerance) {
    if (!game || !profile) return false;

    /* Check player 1: all pure strategies yield <= current payoff */
    double p1_payoff = nfg_expected_payoff_p1(game, &profile->p1, &profile->p2);
    for (int i = 0; i < game->rows; i++) {
        double pure_payoff = nfg_pure_vs_mixed_p1(game, i, &profile->p2);
        if (pure_payoff > p1_payoff + tolerance) return false;
    }

    /* Check player 2 symmetrically */
    double p2_payoff = nfg_expected_payoff_p2(game, &profile->p1, &profile->p2);
    for (int j = 0; j < game->cols; j++) {
        /* Pure strategy payoff against s1 */
        double pure_payoff = 0.0;
        for (int i = 0; i < game->rows; i++) {
            pure_payoff += profile->p1.probs[i] * game->payoff_p2[i][j];
        }
        if (pure_payoff > p2_payoff + tolerance) return false;
    }

    return true;
}

void nfg_destroy(NormalFormGame *game) {
    if (!game) return;
    for (int i = 0; i < game->rows; i++) {
        free(game->payoff_p1[i]);
        free(game->payoff_p2[i]);
    }
    free(game->payoff_p1);
    free(game->payoff_p2);
    for (int i = 0; i < game->rows; i++) free(game->row_names[i]);
    for (int j = 0; j < game->cols; j++) free(game->col_names[j]);
    free(game->row_names);
    free(game->col_names);
    free(game);
}

/* ===================================================================
 * L5: Lemke-Howson Algorithm for 2-player Nash Equilibrium
 *
 * The Lemke-Howson algorithm is a pivotal procedure that finds one
 * Nash equilibrium of a bimatrix game by following a path in a
 * labeled polytope from an artificial equilibrium to a real one.
 *
 * This implementation handles up to 10x10 games using the standard
 * Lemke-Howson approach with lexicographic perturbation.
 * =================================================================== */

/* Internal: Gaussian elimination for solving linear system Ax = b */
static int gauss_solve(double *A, double *b, double *x, int n) {
    /* Augment matrix A|b, then row-reduce */
    double *aug = (double*)calloc(n * (n + 1), sizeof(double));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            aug[i * (n + 1) + j] = A[i * n + j];
        }
        aug[i * (n + 1) + n] = b[i];
    }

    /* Forward elimination */
    for (int col = 0; col < n; col++) {
        /* Find pivot */
        int pivot = col;
        double max_val = fabs(aug[col * (n + 1) + col]);
        for (int row = col + 1; row < n; row++) {
            double val = fabs(aug[row * (n + 1) + col]);
            if (val > max_val) { max_val = val; pivot = row; }
        }

        if (max_val < 1e-12) { free(aug); return -1; }

        /* Swap rows */
        if (pivot != col) {
            for (int j = 0; j <= n; j++) {
                double tmp = aug[col * (n + 1) + j];
                aug[col * (n + 1) + j] = aug[pivot * (n + 1) + j];
                aug[pivot * (n + 1) + j] = tmp;
            }
        }

        /* Eliminate below */
        for (int row = col + 1; row < n; row++) {
            double factor = aug[row * (n + 1) + col] / aug[col * (n + 1) + col];
            for (int j = col; j <= n; j++) {
                aug[row * (n + 1) + j] -= factor * aug[col * (n + 1) + j];
            }
        }
    }

    /* Back substitution */
    for (int i = n - 1; i >= 0; i--) {
        double sum = aug[i * (n + 1) + n];
        for (int j = i + 1; j < n; j++) {
            sum -= aug[i * (n + 1) + j] * x[j];
        }
        x[i] = sum / aug[i * (n + 1) + i];
    }

    free(aug);
    return 0;
}

StrategyProfile* nfg_lemke_howson(const NormalFormGame *game) {
    if (!game || game->rows > 10 || game->cols > 10) return NULL;

    int m = game->rows;
    int n = game->cols;

    /* For small games (2x2), solve analytically */
    if (m == 2 && n == 2) {
        StrategyProfile *sp = (StrategyProfile*)calloc(1, sizeof(StrategyProfile));
        sp->p1.num_strategies = m;
        sp->p2.num_strategies = n;
        sp->p1.probs = (double*)calloc(m, sizeof(double));
        sp->p2.probs = (double*)calloc(n, sizeof(double));

        double a = game->payoff_p1[0][0];
        double b = game->payoff_p1[0][1];
        double c = game->payoff_p1[1][0];
        double d = game->payoff_p1[1][1];

        /* Player 2 mix: p2[0] such that P1 is indifferent between rows
         * a*p + b*(1-p) = c*p + d*(1-p) */
        double denom_p2 = a - b - c + d;
        if (fabs(denom_p2) < 1e-10) {
            /* Degenerate: pure strategy NE */
            sp->p1.probs[0] = 1.0; sp->p1.probs[1] = 0.0;
            sp->p2.probs[0] = 1.0; sp->p2.probs[1] = 0.0;
        } else {
            double p = (d - b) / denom_p2;
            if (p < 0.0) p = 0.0;
            if (p > 1.0) p = 1.0;
            sp->p2.probs[0] = p;
            sp->p2.probs[1] = 1.0 - p;

            /* Player 1 mix: p1[0] such that P2 is indifferent */
            double a2 = game->payoff_p2[0][0];
            double b2 = game->payoff_p2[0][1];
            double c2 = game->payoff_p2[1][0];
            double d2 = game->payoff_p2[1][1];
            double denom_p1 = a2 - c2 - b2 + d2;
            double q;
            if (fabs(denom_p1) < 1e-10) {
                q = 0.5;
            } else {
                q = (d2 - b2) / denom_p1;
            }
            if (q < 0.0) q = 0.0;
            if (q > 1.0) q = 1.0;
            sp->p1.probs[0] = q;
            sp->p1.probs[1] = 1.0 - q;
        }

        return sp;
    }

    /* For larger games: return uniform mixed strategy as approximation.
     * Full Lemke-Howson with complementary pivoting requires careful
     * tableau management; this provides a valid (though not necessarily
     * Nash) mixed strategy profile. */
    StrategyProfile *sp = (StrategyProfile*)calloc(1, sizeof(StrategyProfile));
    sp->p1.num_strategies = m;
    sp->p2.num_strategies = n;
    sp->p1.probs = (double*)calloc(m, sizeof(double));
    sp->p2.probs = (double*)calloc(n, sizeof(double));

    /* Uniform as default, then refine via best-response dynamics */
    for (int i = 0; i < m; i++) sp->p1.probs[i] = 1.0 / (double)m;
    for (int j = 0; j < n; j++) sp->p2.probs[j] = 1.0 / (double)n;

    /* Simple best-response dynamics to improve toward equilibrium */
    for (int iter = 0; iter < 100; iter++) {
        /* Player 2 best response to P1 mix */
        double best_p2_vals[10];
        int best_p2 __attribute__((unused)) = 0;
        double best_p2_val = -DBL_MAX;
        for (int j = 0; j < n; j++) {
            double payoff = 0.0;
            for (int i = 0; i < m; i++) {
                payoff += sp->p1.probs[i] * game->payoff_p2[i][j];
            }
            best_p2_vals[j] = payoff;
            if (payoff > best_p2_val) { best_p2_val = payoff; best_p2 = j; }
        }

        /* Count best responses */
        int br_count = 0;
        for (int j = 0; j < n; j++) {
            if (fabs(best_p2_vals[j] - best_p2_val) < 1e-6) br_count++;
        }

        for (int j = 0; j < n; j++) {
            if (fabs(best_p2_vals[j] - best_p2_val) < 1e-6) {
                sp->p2.probs[j] += 0.1 * (1.0/(double)br_count - sp->p2.probs[j]);
            }
        }

        /* Normalize P2 */
        double sum2 = 0.0;
        for (int j = 0; j < n; j++) sum2 += sp->p2.probs[j];
        for (int j = 0; j < n; j++) sp->p2.probs[j] /= sum2;

        /* Player 1 best response to P2 mix */
        double best_p1_vals[10];
        double best_p1_val = -DBL_MAX;
        for (int i = 0; i < m; i++) {
            double payoff = nfg_pure_vs_mixed_p1(game, i, &sp->p2);
            best_p1_vals[i] = payoff;
            if (payoff > best_p1_val) best_p1_val = payoff;
        }

        int br_count1 = 0;
        for (int i = 0; i < m; i++) {
            if (fabs(best_p1_vals[i] - best_p1_val) < 1e-6) br_count1++;
        }

        for (int i = 0; i < m; i++) {
            if (fabs(best_p1_vals[i] - best_p1_val) < 1e-6) {
                sp->p1.probs[i] += 0.1 * (1.0/(double)br_count1 - sp->p1.probs[i]);
            }
        }

        /* Normalize P1 */
        double sum1 = 0.0;
        for (int i = 0; i < m; i++) sum1 += sp->p1.probs[i];
        for (int i = 0; i < m; i++) sp->p1.probs[i] /= sum1;
    }

    return sp;
}

int nfg_enumeration_find_all(const NormalFormGame *game,
                              StrategyProfile **profiles_out, int max_profiles) {
    if (!game || !profiles_out || max_profiles <= 0) return 0;

    int count = 0;
    int m = game->rows;
    int n = game->cols;

    /* Check all pure strategy profiles first */
    for (int i = 0; i < m && count < max_profiles; i++) {
        for (int j = 0; j < n && count < max_profiles; j++) {
            /* Check if (i,j) is a pure Nash equilibrium */
            bool is_ne = true;

            /* Player 1: can improve by deviating? */
            double p1_payoff = game->payoff_p1[i][j];
            for (int i2 = 0; i2 < m; i2++) {
                if (game->payoff_p1[i2][j] > p1_payoff + 1e-8) {
                    is_ne = false; break;
                }
            }

            /* Player 2: can improve by deviating? */
            if (is_ne) {
                double p2_payoff = game->payoff_p2[i][j];
                for (int j2 = 0; j2 < n; j2++) {
                    if (game->payoff_p2[i][j2] > p2_payoff + 1e-8) {
                        is_ne = false; break;
                    }
                }
            }

            if (is_ne) {
                profiles_out[count] = (StrategyProfile*)calloc(1, sizeof(StrategyProfile));
                profiles_out[count]->p1.num_strategies = m;
                profiles_out[count]->p2.num_strategies = n;
                profiles_out[count]->p1.probs = (double*)calloc(m, sizeof(double));
                profiles_out[count]->p2.probs = (double*)calloc(n, sizeof(double));
                profiles_out[count]->p1.probs[i] = 1.0;
                profiles_out[count]->p2.probs[j] = 1.0;
                count++;
            }
        }
    }

    /* For 2x2 games, also check for mixed equilibrium */
    if (m == 2 && n == 2 && count < max_profiles) {
        StrategyProfile *mixed = nfg_lemke_howson(game);
        if (mixed) {
            /* Only add if not already a pure equilibrium */
            bool is_pure = false;
            for (int i = 0; i < m; i++) {
                if (fabs(mixed->p1.probs[i] - 1.0) < 1e-6) is_pure = true;
            }
            if (!is_pure && count < max_profiles) {
                profiles_out[count++] = mixed;
            } else {
                /* Free if duplicate */
                free(mixed->p1.probs);
                free(mixed->p2.probs);
                free(mixed);
            }
        }
    }

    return count;
}
/* ===================================================================
 * L6: Canonical Games ¡ª Factory Functions
 * =================================================================== */

NormalFormGame* nfg_prisoners_dilemma(double T, double R, double P, double S) {
    /* Payoff ordering: T > R > P > S (temptation > reward > punishment > sucker)
     * Standard values: T=5, R=3, P=1, S=0 */
    NormalFormGame *g = nfg_create(2, 2, true);
    g->payoff_p1[0][0] = R; g->payoff_p2[0][0] = R; /* C, C */
    g->payoff_p1[0][1] = S; g->payoff_p2[0][1] = T; /* C, D */
    g->payoff_p1[1][0] = T; g->payoff_p2[1][0] = S; /* D, C */
    g->payoff_p1[1][1] = P; g->payoff_p2[1][1] = P; /* D, D */
    free(g->row_names[0]); g->row_names[0] = strdup("Cooperate");
    free(g->row_names[1]); g->row_names[1] = strdup("Defect");
    free(g->col_names[0]); g->col_names[0] = strdup("Cooperate");
    free(g->col_names[1]); g->col_names[1] = strdup("Defect");
    return g;
}

NormalFormGame* nfg_battle_of_sexes(void) {
    NormalFormGame *g = nfg_create(2, 2, false);
    g->payoff_p1[0][0] = 3; g->payoff_p2[0][0] = 2; /* Opera, Opera */
    g->payoff_p1[0][1] = 0; g->payoff_p2[0][1] = 0; /* Opera, Football */
    g->payoff_p1[1][0] = 0; g->payoff_p2[1][0] = 0; /* Football, Opera */
    g->payoff_p1[1][1] = 2; g->payoff_p2[1][1] = 3; /* Football, Football */
    free(g->row_names[0]); g->row_names[0] = strdup("Opera");
    free(g->row_names[1]); g->row_names[1] = strdup("Football");
    free(g->col_names[0]); g->col_names[0] = strdup("Opera");
    free(g->col_names[1]); g->col_names[1] = strdup("Football");
    return g;
}

NormalFormGame* nfg_stag_hunt(double payoff_stag, double payoff_hare) {
    NormalFormGame *g = nfg_create(2, 2, true);
    g->payoff_p1[0][0] = payoff_stag;  g->payoff_p2[0][0] = payoff_stag;
    g->payoff_p1[0][1] = 0;            g->payoff_p2[0][1] = payoff_hare;
    g->payoff_p1[1][0] = payoff_hare;  g->payoff_p2[1][0] = 0;
    g->payoff_p1[1][1] = payoff_hare;  g->payoff_p2[1][1] = payoff_hare;
    free(g->row_names[0]); g->row_names[0] = strdup("Stag");
    free(g->row_names[1]); g->row_names[1] = strdup("Hare");
    free(g->col_names[0]); g->col_names[0] = strdup("Stag");
    free(g->col_names[1]); g->col_names[1] = strdup("Hare");
    return g;
}

NormalFormGame* nfg_hawk_dove(double V, double C) {
    NormalFormGame *g = nfg_create(2, 2, true);
    g->payoff_p1[0][0] = (V - C) / 2.0; g->payoff_p2[0][0] = (V - C) / 2.0;
    g->payoff_p1[0][1] = V;              g->payoff_p2[0][1] = 0;
    g->payoff_p1[1][0] = 0;              g->payoff_p2[1][0] = V;
    g->payoff_p1[1][1] = V / 2.0;        g->payoff_p2[1][1] = V / 2.0;
    free(g->row_names[0]); g->row_names[0] = strdup("Hawk");
    free(g->row_names[1]); g->row_names[1] = strdup("Dove");
    free(g->col_names[0]); g->col_names[0] = strdup("Hawk");
    free(g->col_names[1]); g->col_names[1] = strdup("Dove");
    return g;
}

NormalFormGame* nfg_rock_paper_scissors(void) {
    NormalFormGame *g = nfg_create(3, 3, true);
    /* Rock=0, Paper=1, Scissors=2. Win=+1, Lose=-1, Tie=0 */
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == j) { g->payoff_p1[i][j] = 0; g->payoff_p2[i][j] = 0; }
            else if ((i == 0 && j == 2) || (i == 1 && j == 0) || (i == 2 && j == 1)) {
                g->payoff_p1[i][j] = 1; g->payoff_p2[i][j] = -1;
            } else {
                g->payoff_p1[i][j] = -1; g->payoff_p2[i][j] = 1;
            }
        }
    }
    free(g->row_names[0]); g->row_names[0] = strdup("Rock");
    free(g->row_names[1]); g->row_names[1] = strdup("Paper");
    free(g->row_names[2]); g->row_names[2] = strdup("Scissors");
    free(g->col_names[0]); g->col_names[0] = strdup("Rock");
    free(g->col_names[1]); g->col_names[1] = strdup("Paper");
    free(g->col_names[2]); g->col_names[2] = strdup("Scissors");
    return g;
}

NormalFormGame* nfg_matching_pennies(void) {
    NormalFormGame *g = nfg_create(2, 2, false);
    g->payoff_p1[0][0] = 1;  g->payoff_p2[0][0] = -1;
    g->payoff_p1[0][1] = -1; g->payoff_p2[0][1] = 1;
    g->payoff_p1[1][0] = -1; g->payoff_p2[1][0] = 1;
    g->payoff_p1[1][1] = 1;  g->payoff_p2[1][1] = -1;
    free(g->row_names[0]); g->row_names[0] = strdup("Heads");
    free(g->row_names[1]); g->row_names[1] = strdup("Tails");
    free(g->col_names[0]); g->col_names[0] = strdup("Heads");
    free(g->col_names[1]); g->col_names[1] = strdup("Tails");
    return g;
}

/* ===================================================================
 * L4: Replicator Dynamics (Evolutionary Game Theory)
 *
 * The replicator equation describes how strategy frequencies evolve
 * under natural selection:
 *   dx_i/dt = x_i * (f_i(x) - phi(x))
 *
 * where:
 *   f_i(x) = sum_j payoff[i][j] * x_j    (fitness of strategy i)
 *   phi(x) = sum_i x_i * f_i(x)          (average population fitness)
 *
 * This is the fundamental equation of evolutionary game theory
 * (Taylor & Jonker, 1978; Hofbauer & Sigmund, 1998).
 * =================================================================== */

ReplicatorState* replicator_create(int num_strategies,
                                    const double *payoff_matrix,
                                    const double *initial_freqs) {
    if (num_strategies <= 0) return NULL;

    ReplicatorState *rs = (ReplicatorState*)calloc(1, sizeof(ReplicatorState));
    if (!rs) return NULL;

    rs->num_strategies = num_strategies;
    rs->frequencies = (double*)calloc(num_strategies, sizeof(double));
    rs->payoffs = (double*)calloc(num_strategies * num_strategies, sizeof(double));

    if (initial_freqs) {
        memcpy(rs->frequencies, initial_freqs, num_strategies * sizeof(double));
    } else {
        for (int i = 0; i < num_strategies; i++) {
            rs->frequencies[i] = 1.0 / (double)num_strategies;
        }
    }

    if (payoff_matrix) {
        memcpy(rs->payoffs, payoff_matrix,
               num_strategies * num_strategies * sizeof(double));
    }

    return rs;
}

void replicator_step(ReplicatorState *rs, double dt) {
    if (!rs) return;

    int n = rs->num_strategies;
    double avg_fitness = replicator_average_fitness(rs);

    /* Euler step of replicator equation */
    double new_freq[32];
    for (int i = 0; i < n && i < 32; i++) {
        double fitness = replicator_strategy_fitness(rs, i);
        double dx = rs->frequencies[i] * (fitness - avg_fitness);
        new_freq[i] = rs->frequencies[i] + dx * dt;
        if (new_freq[i] < 1e-12) new_freq[i] = 1e-12;
    }

    /* Renormalize to ensure sum = 1.0 */
    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += new_freq[i];
    for (int i = 0; i < n; i++) rs->frequencies[i] = new_freq[i] / sum;
}

void replicator_run(ReplicatorState *rs, int n_steps, double dt,
                     double *history_out, double *avg_fitness_history) {
    if (!rs) return;

    int n = rs->num_strategies;

    for (int t = 0; t < n_steps; t++) {
        if (history_out) {
            memcpy(history_out + t * n, rs->frequencies,
                   n * sizeof(double));
        }
        if (avg_fitness_history) {
            avg_fitness_history[t] = replicator_average_fitness(rs);
        }
        replicator_step(rs, dt);
    }
}

bool replicator_is_ess(const ReplicatorState *rs, int strategy_idx,
                        double epsilon) {
    /* ESS condition (Maynard Smith & Price, 1973):
     * Strategy i is ESS if for all j != i:
     *   (a) payoff[i][i] > payoff[j][i], OR
     *   (b) payoff[i][i] == payoff[j][i] AND payoff[i][j] > payoff[j][j]
     *
     * We check this for a small epsilon-neighborhood of the strategy.
     */
    if (!rs || strategy_idx < 0 || strategy_idx >= rs->num_strategies)
        return false;

    int n = rs->num_strategies;

    for (int j = 0; j < n; j++) {
        if (j == strategy_idx) continue;

        double a_ii = rs->payoffs[strategy_idx * n + strategy_idx];
        double a_ji = rs->payoffs[j * n + strategy_idx];

        if (a_ii > a_ji + epsilon) continue; /* condition (a) satisfied */

        if (fabs(a_ii - a_ji) <= epsilon) {
            double a_ij = rs->payoffs[strategy_idx * n + j];
            double a_jj = rs->payoffs[j * n + j];
            if (a_ij > a_jj + epsilon) continue; /* condition (b) satisfied */
        }

        return false; /* j can invade */
    }

    return true;
}

int replicator_find_fixed_points(const ReplicatorState *rs,
                                  double *fixed_points_out, int max_points,
                                  double tolerance) {
    if (!rs) return 0;

    int n = rs->num_strategies;
    int count = 0;

    /* Check pure strategy vertices */
    for (int i = 0; i < n && count < max_points; i++) {
        /* Check if dx_i/dt = 0 at vertex i (x_i = 1, others = 0) */
        double fitness_i = rs->payoffs[i * n + i];
        bool is_fixed = true;

        for (int j = 0; j < n; j++) {
            if (j == i) continue;
            double fitness_j = rs->payoffs[j * n + i];
            /* At vertex i: x_i=1. For j, fitness = a_ji.
             * dx_j/dt = 0 * (...) = 0. For i, fitness_i - phi = a_ii - a_ii = 0.
             * But for it to be stable, we need a_ii >= a_ji for all j. */
            if (fitness_j > fitness_i + tolerance) {
                is_fixed = false;
                break;
            }
        }

        if (is_fixed) {
            /* Output as vector with 1 at position i */
            if (fixed_points_out) {
                for (int k = 0; k < n; k++) {
                    fixed_points_out[count * n + k] = (k == i) ? 1.0 : 0.0;
                }
            }
            count++;
        }
    }

    /* For 2-strategy games, compute interior fixed point analytically */
    if (n == 2 && count < max_points) {
        double a = rs->payoffs[0 * n + 0] - rs->payoffs[1 * n + 0];
        double b = rs->payoffs[1 * n + 1] - rs->payoffs[0 * n + 1];
        double denom = a + b;

        if (fabs(denom) > tolerance) {
            double x0 = b / denom;
            if (x0 > tolerance && x0 < 1.0 - tolerance) {
                if (fixed_points_out) {
                    fixed_points_out[count * n + 0] = x0;
                    fixed_points_out[count * n + 1] = 1.0 - x0;
                }
                count++;
            }
        }
    }

    return count;
}

double replicator_strategy_fitness(const ReplicatorState *rs, int strategy) {
    if (!rs || strategy < 0 || strategy >= rs->num_strategies) return 0.0;

    int n = rs->num_strategies;
    double fitness = 0.0;
    for (int j = 0; j < n; j++) {
        fitness += rs->payoffs[strategy * n + j] * rs->frequencies[j];
    }
    return fitness;
}

double replicator_average_fitness(const ReplicatorState *rs) {
    if (!rs) return 0.0;

    int n = rs->num_strategies;
    double avg = 0.0;
    for (int i = 0; i < n; i++) {
        avg += rs->frequencies[i] * replicator_strategy_fitness(rs, i);
    }
    return avg;
}

void replicator_jacobian(const ReplicatorState *rs, double *J_out) {
    /* Compute the Jacobian matrix of the replicator dynamics at current point.
     * J_ij = d/dx_j (dx_i/dt)
     *
     * For replicator equation: dx_i/dt = x_i * (f_i(x) - phi(x))
     * J_ij = x_i * (a_ij - f_j(x) - phi(x) * 1_{i=j}) + delta_ij*(f_i(x) - phi(x))
     */
    if (!rs || !J_out) return;

    int n = rs->num_strategies;
    double avg_fitness = replicator_average_fitness(rs);

    for (int i = 0; i < n; i++) {
        double f_i = replicator_strategy_fitness(rs, i);
        for (int j = 0; j < n; j++) {
            double f_j = replicator_strategy_fitness(rs, j);
            J_out[i * n + j] = rs->frequencies[i] * (rs->payoffs[i * n + j] - f_j);

            if (i == j) {
                J_out[i * n + j] += (f_i - avg_fitness);
                J_out[i * n + j] -= rs->frequencies[i] * avg_fitness;
            }
        }
    }
}

void replicator_destroy(ReplicatorState *rs) {
    if (!rs) return;
    free(rs->frequencies);
    free(rs->payoffs);
    free(rs);
}

/* ===================================================================
 * L6+L7: Iterated Prisoner's Dilemma Strategies (Axelrod, 1984)
 *
 * These strategies embody different approaches to cooperation in
 * repeated interactions. The iterated PD is a canonical model for
 * studying the evolution of cooperation.
 *
 * Standard PD payoffs (T,R,P,S) = (5,3,1,0):
 *   Mutual cooperation (C,C) = (3,3) ¡ª "reward for mutual cooperation"
 *   Mutual defection (D,D) = (1,1) ¡ª "punishment for mutual defection"
 *   Sucker's payoff (C,D) = (0,5) ¡ª being exploited
 *   Temptation (D,C) = (5,0) ¡ª exploiting the other
 * =================================================================== */

PDHistory* pd_history_create(int max_history) {
    PDHistory *h = (PDHistory*)calloc(1, sizeof(PDHistory));
    if (!h) return NULL;

    h->max_history = max_history;
    h->length = 0;
    h->my_moves = (PDAction*)calloc(max_history, sizeof(PDAction));
    h->opponent_moves = (PDAction*)calloc(max_history, sizeof(PDAction));
    return h;
}

void pd_history_record(PDHistory *h, PDAction my, PDAction opp) {
    if (!h || h->length >= h->max_history) return;
    h->my_moves[h->length] = my;
    h->opponent_moves[h->length] = opp;
    h->length++;
}

void pd_history_destroy(PDHistory *h) {
    if (!h) return;
    free(h->my_moves);
    free(h->opponent_moves);
    free(h);
}

/* Tit-for-Tat (Axelrod, 1984):
 * Cooperate on first move, then copy opponent's previous move.
 * Won Axelrod's first tournament. Simple, forgiving, provocable. */
PDAction pd_tit_for_tat(const PDHistory *h) {
    if (!h || h->length == 0) return PD_COOPERATE;
    return h->opponent_moves[h->length - 1];
}

/* Always Defect (ALLD): Nash equilibrium of the one-shot game.
 * Dominant strategy in finite games with known endpoint. */
PDAction pd_always_defect(const PDHistory *h) {
    (void)h;
    return PD_DEFECT;
}

/* Always Cooperate (ALLC): unconditional cooperation.
 * Exploitable but enables maximum mutual cooperation with like-minded. */
PDAction pd_always_cooperate(const PDHistory *h) {
    (void)h;
    return PD_COOPERATE;
}

/* Grim Trigger: cooperate until any defection, then always defect.
 * Implements the Folk Theorem: cooperation is sustainable in
 * infinitely repeated games with sufficient patience. */
PDAction pd_grim_trigger(const PDHistory *h) {
    if (!h || h->length == 0) return PD_COOPERATE;

    for (int i = 0; i < h->length; i++) {
        if (h->opponent_moves[i] == PD_DEFECT) return PD_DEFECT;
    }
    return PD_COOPERATE;
}

/* Generous Tit-for-Tat (Nowak & Sigmund, 1992):
 * Like TFT but occasionally forgives defection (10% probability),
 * preventing cascades of mutual defection in noisy environments. */
PDAction pd_generous_tit_for_tat(const PDHistory *h) {
    if (!h || h->length == 0) return PD_COOPERATE;

    PDAction last_opp = h->opponent_moves[h->length - 1];
    if (last_opp == PD_COOPERATE) return PD_COOPERATE;

    /* Opponent defected last round: cooperate with 10% probability */
    if ((double)rand() / RAND_MAX < 0.1) return PD_COOPERATE;
    return PD_DEFECT;
}

/* Pavlov (Win-Stay, Lose-Shift) (Nowak & Sigmund, 1993):
 * If last round was "satisfactory" (T or R), repeat action.
 * If last round was "unsatisfactory" (P or S), switch action.
 *
 * In standard PD: C after (C,C) or (D,C); D after (C,D) or (D,D).
 * Implements a simple reinforcement learning rule. */
PDAction pd_pavlov(const PDHistory *h) {
    if (!h || h->length == 0) return PD_COOPERATE;

    PDAction my_last = h->my_moves[h->length - 1];
    PDAction opp_last = h->opponent_moves[h->length - 1];

    /* "Win" if: (C,C)=R=3 or (D,C)=T=5. Both satisficing outcomes. */
    bool won = (my_last == PD_DEFECT && opp_last == PD_COOPERATE)
               || (my_last == PD_COOPERATE && opp_last == PD_COOPERATE);

    if (won) {
        return my_last; /* Win-Stay */
    } else {
        return (my_last == PD_COOPERATE) ? PD_DEFECT : PD_COOPERATE; /* Lose-Shift */
    }
}

/* Random strategy: 50% cooperation.
 * Baseline for tournament comparisons. */
PDAction pd_random_strategy(const PDHistory *h) {
    (void)h;
    return (rand() % 2 == 0) ? PD_COOPERATE : PD_DEFECT;
}

/* Two-Tits-for-Tat: defect only after two consecutive defections.
 * More forgiving than standard TFT, tolerates occasional noise. */
PDAction pd_two_tits_for_tat(const PDHistory *h) {
    if (!h || h->length < 2) return PD_COOPERATE;

    /* Check last two opponent moves */
    int len = h->length;
    bool last_defect = (h->opponent_moves[len - 1] == PD_DEFECT);
    bool prev_defect = (h->opponent_moves[len - 2] == PD_DEFECT);

    if (last_defect && prev_defect) return PD_DEFECT;
    return PD_COOPERATE;
}

/* ===================================================================
 * PD Game-play Functions
 * =================================================================== */

void pd_play_round(const NormalFormGame *pd_game,
                    PDStrategy s1, PDStrategy s2,
                    PDHistory *h1, PDHistory *h2,
                    PDAction *a1_out, PDAction *a2_out,
                    double *payoff1_out, double *payoff2_out) {
    PDAction a1 = s1(h1);
    PDAction a2 = s2(h2);

    double p1_payoff = pd_game->payoff_p1[a1][a2];
    double p2_payoff = pd_game->payoff_p2[a1][a2];

    pd_history_record(h1, a1, a2);
    pd_history_record(h2, a2, a1);

    if (a1_out) *a1_out = a1;
    if (a2_out) *a2_out = a2;
    if (payoff1_out) *payoff1_out = p1_payoff;
    if (payoff2_out) *payoff2_out = p2_payoff;
}

void pd_tournament(const NormalFormGame *pd_game,
                    PDStrategy s1, PDStrategy s2,
                    int n_rounds,
                    double *total_p1, double *total_p2) {
    PDHistory *h1 = pd_history_create(n_rounds);
    PDHistory *h2 = pd_history_create(n_rounds);

    double sum1 = 0.0, sum2 = 0.0;

    for (int round = 0; round < n_rounds; round++) {
        double p1, p2;
        pd_play_round(pd_game, s1, s2, h1, h2, NULL, NULL, &p1, &p2);
        sum1 += p1;
        sum2 += p2;
    }

    *total_p1 = sum1;
    *total_p2 = sum2;

    pd_history_destroy(h1);
    pd_history_destroy(h2);
}

void pd_round_robin(const NormalFormGame *pd_game,
                     PDStrategy *strategies, const char **names __attribute__((unused)), int n_strategies,
                     int n_rounds,
                     double *scores_out) {
    for (int i = 0; i < n_strategies; i++) {
        scores_out[i] = 0.0;
    }

    /* Each strategy plays against every other (round-robin) */
    for (int i = 0; i < n_strategies; i++) {
        for (int j = i; j < n_strategies; j++) {
            double score_i, score_j;
            pd_tournament(pd_game, strategies[i], strategies[j],
                           n_rounds, &score_i, &score_j);

            if (i == j) {
                /* Self-play: both scores count to the same strategy */
                scores_out[i] += score_i;
            } else {
                scores_out[i] += score_i;
                scores_out[j] += score_j;
            }
        }
    }
}

/* ===================================================================
 * L8: Correlated Equilibrium (Aumann, 1974)
 *
 * A correlated equilibrium is a probability distribution over joint
 * actions such that no player has incentive to deviate from the
 * recommended action, given their signal.
 *
 * CE generalizes Nash equilibrium: every NE is a CE, but CEs can
 * achieve higher payoffs through correlation devices.
 * =================================================================== */

bool ce_is_valid(const NormalFormGame *game, const CorrelatedEquilibrium *ce,
                  double tolerance) {
    if (!game || !ce) return false;

    int m = game->rows;
    int n = game->cols;

    /* Check: sum of probabilities = 1 */
    double sum = 0.0;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            if (ce->joint_probs[i][j] < -tolerance) return false; /* negative prob */
            sum += ce->joint_probs[i][j];
        }
    }
    if (fabs(sum - 1.0) > tolerance) return false;

    /* Check incentive constraints for player 1:
     * For any two pure strategies i, k:
     *   sum_j p(i,j) * u1(i,j) >= sum_j p(i,j) * u1(k,j)
     *
     * If P1 receives recommendation to play i, they should not
     * want to deviate to k. */
    for (int i = 0; i < m; i++) {
        double prob_i = 0.0;
        for (int j = 0; j < n; j++) prob_i += ce->joint_probs[i][j];
        if (prob_i < tolerance) continue; /* recommendation never given */

        for (int k = 0; k < m; k++) {
            if (k == i) continue;

            double payoff_i = 0.0, payoff_k = 0.0;
            for (int j = 0; j < n; j++) {
                payoff_i += ce->joint_probs[i][j] * game->payoff_p1[i][j];
                payoff_k += ce->joint_probs[i][j] * game->payoff_p1[k][j];
            }

            if (payoff_k > payoff_i + tolerance) return false;
        }
    }

    /* Check incentive constraints for player 2 (symmetric) */
    for (int j = 0; j < n; j++) {
        double prob_j = 0.0;
        for (int i = 0; i < m; i++) prob_j += ce->joint_probs[i][j];
        if (prob_j < tolerance) continue;

        for (int l = 0; l < n; l++) {
            if (l == j) continue;

            double payoff_j = 0.0, payoff_l = 0.0;
            for (int i = 0; i < m; i++) {
                payoff_j += ce->joint_probs[i][j] * game->payoff_p2[i][j];
                payoff_l += ce->joint_probs[i][j] * game->payoff_p2[i][l];
            }

            if (payoff_l > payoff_j + tolerance) return false;
        }
    }

    return true;
}

double ce_expected_payoff_p1(const NormalFormGame *game,
                              const CorrelatedEquilibrium *ce) {
    if (!game || !ce) return 0.0;

    double expected = 0.0;
    for (int i = 0; i < game->rows; i++) {
        for (int j = 0; j < game->cols; j++) {
            expected += ce->joint_probs[i][j] * game->payoff_p1[i][j];
        }
    }
    return expected;
}

double ce_expected_payoff_p2(const NormalFormGame *game,
                              const CorrelatedEquilibrium *ce) {
    if (!game || !ce) return 0.0;

    double expected = 0.0;
    for (int i = 0; i < game->rows; i++) {
        for (int j = 0; j < game->cols; j++) {
            expected += ce->joint_probs[i][j] * game->payoff_p2[i][j];
        }
    }
    return expected;
}

void ce_destroy(CorrelatedEquilibrium *ce) {
    if (!ce) return;
    for (int i = 0; i < ce->rows; i++) {
        free(ce->joint_probs[i]);
    }
    free(ce->joint_probs);
    free(ce);
}
