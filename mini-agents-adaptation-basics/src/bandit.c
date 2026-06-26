/**
 * bandit.c â€” Multi-Armed Bandit Implementation
 *
 * Implements core MAB algorithms:
 *   epsilon-Greedy, UCB1, Thompson Sampling (Beta/Gaussian),
 *   Softmax, epsilon-Decreasing, Optimistic Initialization,
 *   LinUCB (contextual bandit), EXP3 (adversarial bandit),
 *   and regret analysis (Lai-Robbins bound, UCB1 finite-time bound).
 *
 * Reference: Robbins (1952), Lai & Robbins (1985),
 *            Auer et al. (2002), Thompson (1933),
 *            Li et al. (2010) â€” LinUCB, Bubeck & Cesa-Bianchi (2012)
 *
 * Knowledge: L1 Definitions, L2 Explore/Exploit, L4 Regret bounds,
 *            L5 Algorithms, L6 Canonical Problem, L7 Applications
 */

#include "bandit.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===================================================================
 * L1+L2: Bandit Construction and Core Operations
 * =================================================================== */

MultiArmedBandit* bandit_create(int K, const double *true_means,
                                 const double *true_stds, int horizon) {
    if (K <= 0) return NULL;

    MultiArmedBandit *mab = (MultiArmedBandit*)calloc(1, sizeof(MultiArmedBandit));
    if (!mab) return NULL;

    mab->num_arms = K;
    mab->arms = (BanditArm*)calloc(K, sizeof(BanditArm));
    mab->horizon = horizon;
    mab->total_pulls = 0;
    mab->cumulative_regret = 0.0;

    /* Identify best arm */
    mab->best_arm_index = 0;
    mab->best_arm_mean = true_means ? true_means[0] : 0.0;

    for (int i = 0; i < K; i++) {
        mab->arms[i].arm_id = i;
        snprintf(mab->arms[i].name, 31, "arm_%d", i);
        mab->arms[i].true_mean = true_means ? true_means[i] : 0.5;
        mab->arms[i].true_std  = true_stds  ? true_stds[i]  : 0.2;
        mab->arms[i].empirical_mean = 0.0;
        mab->arms[i].sum_rewards = 0.0;
        mab->arms[i].pull_count = 0;
        mab->arms[i].sum_sq_rewards = 0.0;
        /* Beta(1,1) = Uniform prior for Thompson Sampling */
        mab->arms[i].beta_alpha = 1.0;
        mab->arms[i].beta_beta  = 1.0;

        if (mab->arms[i].true_mean > mab->best_arm_mean) {
            mab->best_arm_mean = mab->arms[i].true_mean;
            mab->best_arm_index = i;
        }
    }

    return mab;
}

double bandit_pull(MultiArmedBandit *mab, int arm_index) {
    if (!mab || arm_index < 0 || arm_index >= mab->num_arms) return 0.0;

    BanditArm *arm = &mab->arms[arm_index];

    /* Generate reward: Gaussian(mean, std) truncated to [0, 1] for Beta-Bernoulli */
    double u1 = (double)rand() / (double)RAND_MAX;
    double u2 = (double)rand() / (double)RAND_MAX;
    if (u1 < 1e-12) u1 = 1e-12;
    double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    double reward = arm->true_mean + arm->true_std * z;

    /* Clamp to [0, 1] for compatibility with Beta-Bernoulli model */
    if (reward < 0.0) reward = 0.0;
    if (reward > 1.0) reward = 1.0;

    /* Update bandit state */
    bandit_update_arm(arm, reward);

    /* Update regret */
    mab->cumulative_regret += bandit_instantaneous_regret(mab, arm_index);
    mab->total_pulls++;

    return reward;
}

void bandit_update_arm(BanditArm *arm, double reward) {
    if (!arm) return;
    arm->pull_count++;
    arm->sum_rewards += reward;
    arm->sum_sq_rewards += reward * reward;
    arm->empirical_mean = arm->sum_rewards / (double)arm->pull_count;

    /* Update Beta posterior for Thompson Sampling */
    /* Treat reward as pseudo-count for Beta(alpha, beta) */
    arm->beta_alpha += reward;
    arm->beta_beta += (1.0 - reward);
}

double bandit_instantaneous_regret(const MultiArmedBandit *mab, int arm_chosen) {
    if (!mab || arm_chosen < 0 || arm_chosen >= mab->num_arms) return 0.0;
    return mab->best_arm_mean - mab->arms[arm_chosen].true_mean;
}

void bandit_reset(MultiArmedBandit *mab) {
    if (!mab) return;
    for (int i = 0; i < mab->num_arms; i++) {
        mab->arms[i].empirical_mean = 0.0;
        mab->arms[i].sum_rewards = 0.0;
        mab->arms[i].pull_count = 0;
        mab->arms[i].sum_sq_rewards = 0.0;
        mab->arms[i].beta_alpha = 1.0;
        mab->arms[i].beta_beta = 1.0;
    }
    mab->total_pulls = 0;
    mab->cumulative_regret = 0.0;
}

double bandit_expected_regret(const MultiArmedBandit *mab, int arm_index) {
    return bandit_gap(mab, arm_index);
}

void bandit_destroy(MultiArmedBandit *mab) {
    if (!mab) return;
    free(mab->arms);
    free(mab);
}

/* ===================================================================
 * L5: Bandit Algorithms
 * =================================================================== */

/* epsilon-Greedy: simplest non-trivial strategy */
int bandit_epsilon_greedy(const MultiArmedBandit *mab, double epsilon) {
    if (!mab || mab->num_arms <= 0) return -1;
    if (mab->num_arms == 1) return 0;

    if ((double)rand() / RAND_MAX < epsilon) {
        return rand() % mab->num_arms;
    }

    /* Exploit: pick arm with highest empirical mean */
    int best = 0;
    double best_mean = mab->arms[0].empirical_mean;
    for (int i = 1; i < mab->num_arms; i++) {
        if (mab->arms[i].empirical_mean > best_mean) {
            best_mean = mab->arms[i].empirical_mean;
            best = i;
        }
    }
    return best;
}

/* UCB1 (Auer et al., 2002): logarithmic regret O(log T) */
int bandit_ucb1(const MultiArmedBandit *mab) {
    return bandit_ucb(mab, sqrt(2.0));
}

int bandit_ucb(const MultiArmedBandit *mab, double c) {
    if (!mab || mab->num_arms <= 0) return -1;
    if (mab->num_arms == 1) return 0;

    int t = mab->total_pulls;

    /* Pull each arm at least once (initialization phase) */
    for (int i = 0; i < mab->num_arms; i++) {
        if (mab->arms[i].pull_count == 0) return i;
    }

    /* UCB: argmax_i [ mu_hat_i + c * sqrt(ln(t) / n_i) ] */
    int best = 0;
    double best_ucb = -DBL_MAX;
    double ln_t = log((double)t);

    for (int i = 0; i < mab->num_arms; i++) {
        double bonus = c * sqrt(ln_t / (double)mab->arms[i].pull_count);
        double ucb_value = mab->arms[i].empirical_mean + bonus;
        if (ucb_value > best_ucb) {
            best_ucb = ucb_value;
            best = i;
        }
    }
    return best;
}

/* Helper: generate Beta-distributed random variable via Gamma method */
static double rand_beta(double alpha, double beta) {
    if (alpha <= 0.0) alpha = 0.01;
    if (beta <= 0.0) beta = 0.01;

    /* Marsaglia-Tsang method for Gamma(alpha, 1) */
    double x = 0.0;

    if (alpha < 1.0) {
        /* Use Johnk generator for alpha < 1 */
        double u, v;
        do {
            u = pow((double)rand() / RAND_MAX, 1.0 / alpha);
            v = pow((double)rand() / RAND_MAX, 1.0 / beta);
        } while (u + v > 1.0);
        return u / (u + v);
    }

    /* Cheng rejection sampling for Gamma */
    double d = alpha - 1.0/3.0;
    double c_inner = 1.0 / sqrt(9.0 * d);
    for (;;) {
        double z;
        do {
            z = 0.0;
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            z = sqrt(-2.0 * log(u1 < 1e-12 ? 1e-12 : u1))
                * cos(2.0 * M_PI * u2);
        } while (z < -1.0 / c_inner);

        double v = (1.0 + c_inner * z);
        v = v * v * v;
        double check = 0.5 * z * z + d - d * v + d * log(v);
        double u_check = log((double)rand() / RAND_MAX + 1e-12);
        if (u_check < check) {
            x = d * v;
            break;
        }
    }

    /* Need second Gamma for beta */
    double d2 = beta - 1.0/3.0;
    double c2 = 1.0 / sqrt(9.0 * d2);
    double y;
    for (;;) {
        double z;
        do {
            z = 0.0;
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            z = sqrt(-2.0 * log(u1 < 1e-12 ? 1e-12 : u1))
                * cos(2.0 * M_PI * u2);
        } while (z < -1.0 / c2);

        double v = (1.0 + c2 * z);
        v = v * v * v;
        double check = 0.5 * z * z + d2 - d2 * v + d2 * log(v);
        double u_check = log((double)rand() / RAND_MAX + 1e-12);
        if (u_check < check) {
            y = d2 * v;
            break;
        }
    }

    return x / (x + y);
}

int bandit_thompson_sampling_beta(MultiArmedBandit *mab) {
    if (!mab || mab->num_arms <= 0) return -1;
    if (mab->num_arms == 1) return 0;

    int best = 0;
    double best_sample = -1.0;

    for (int i = 0; i < mab->num_arms; i++) {
        /* Sample theta_i from Beta(alpha_i, beta_i) */
        double sample = rand_beta(mab->arms[i].beta_alpha,
                                   mab->arms[i].beta_beta);
        if (sample > best_sample) {
            best_sample = sample;
            best = i;
        }
    }
    return best;
}

int bandit_thompson_sampling_gaussian(const MultiArmedBandit *mab) {
    if (!mab || mab->num_arms <= 0) return -1;
    if (mab->num_arms == 1) return 0;

    int best = 0;
    double best_sample = -DBL_MAX;

    for (int i = 0; i < mab->num_arms; i++) {
        /* Sample from Gaussian posterior: N(empirical_mean, sigma^2 / n_i) */
        double n = (double)mab->arms[i].pull_count;
        if (n < 1.0) {
            /* Prior: N(0, 1) */
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            double sample = sqrt(-2.0 * log(u1 < 1e-12 ? 1e-12 : u1))
                            * cos(2.0 * M_PI * u2);
            if (sample > best_sample) { best_sample = sample; best = i; }
            continue;
        }

        double empirical_var = mab->arms[i].sum_sq_rewards / n
                               - mab->arms[i].empirical_mean * mab->arms[i].empirical_mean;
        if (empirical_var < 1e-4) empirical_var = 1.0; /* use known variance = 1 */

        double posterior_std = sqrt(1.0 / n);
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        double z = sqrt(-2.0 * log(u1 < 1e-12 ? 1e-12 : u1))
                   * cos(2.0 * M_PI * u2);
        double sample = mab->arms[i].empirical_mean + posterior_std * z;

        if (sample > best_sample) {
            best_sample = sample;
            best = i;
        }
    }
    return best;
}

int bandit_softmax(const MultiArmedBandit *mab, double temperature) {
    if (!mab || mab->num_arms <= 0) return -1;
    if (mab->num_arms == 1) return 0;

    double max_mean = mab->arms[0].empirical_mean;
    for (int i = 1; i < mab->num_arms; i++) {
        if (mab->arms[i].empirical_mean > max_mean)
            max_mean = mab->arms[i].empirical_mean;
    }

    double sum = 0.0;
    double prob[64];
    double tau = (temperature < 1e-10) ? 1e-10 : temperature;

    for (int i = 0; i < mab->num_arms && i < 64; i++) {
        prob[i] = exp((mab->arms[i].empirical_mean - max_mean) / tau);
        sum += prob[i];
    }

    double r = (double)rand() / RAND_MAX;
    double cum = 0.0;
    for (int i = 0; i < mab->num_arms && i < 64; i++) {
        cum += prob[i] / sum;
        if (r <= cum) return i;
    }
    return mab->num_arms - 1;
}

int bandit_epsilon_decreasing(MultiArmedBandit *mab, double epsilon_0) {
    if (!mab || mab->num_arms <= 0) return -1;
    if (mab->num_arms == 1) return 0;

    int t = mab->total_pulls + 1;
    double epsilon = epsilon_0 / (double)t;
    if (epsilon > 1.0) epsilon = 1.0;
    if (epsilon < 0.01) epsilon = 0.01;

    return bandit_epsilon_greedy(mab, epsilon);
}

int bandit_optimistic_init(MultiArmedBandit *mab, double optimistic_value) {
    if (!mab || mab->num_arms <= 0) return -1;
    if (mab->num_arms == 1) return 0;

    /* On first call, initialize all arms with optimistic value */
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < mab->num_arms; i++) {
            mab->arms[i].empirical_mean = optimistic_value;
            mab->arms[i].sum_rewards = optimistic_value;
            mab->arms[i].pull_count = 1;
        }
        initialized = 1;
    }

    /* Greedy selection (optimism drives exploration) */
    int best = 0;
    double best_mean = mab->arms[0].empirical_mean;
    for (int i = 1; i < mab->num_arms; i++) {
        if (mab->arms[i].empirical_mean > best_mean) {
            best_mean = mab->arms[i].empirical_mean;
            best = i;
        }
    }
    return best;
}

/* ===================================================================
 * L4: Regret Analysis
 * =================================================================== */

double bandit_cumulative_regret_sim(MultiArmedBandit *mab, int T,
                                     int (*strategy_fn)(const MultiArmedBandit*)) {
    if (!mab || !strategy_fn) return 0.0;

    double regret = 0.0;
    for (int t = 0; t < T; t++) {
        int arm = strategy_fn(mab);
        (void)bandit_pull(mab, arm);
        regret += mab->best_arm_mean - mab->arms[arm].true_mean;
    }
    return regret;
}

double bandit_lai_robbins_bound(const MultiArmedBandit *mab) {
    /* Lai-Robbins asymptotic lower bound:
     * lim inf R_T / ln(T) >= sum_{i != i*} Delta_i / KL(mu_i || mu*)
     *
     * For Gaussian rewards with variance sigma^2:
     * KL(mu_i || mu*) = (mu* - mu_i)^2 / (2 * sigma^2)
     */
    if (!mab) return 0.0;

    double bound = 0.0;
    double mu_star = mab->best_arm_mean;
    double sigma_sq = 0.04; /* default variance */

    for (int i = 0; i < mab->num_arms; i++) {
        if (i == mab->best_arm_index) continue;
        double delta = mu_star - mab->arms[i].true_mean;
        if (delta <= 0.0) continue;
        double kl = (delta * delta) / (2.0 * sigma_sq);
        if (kl > 0.0) {
            bound += delta / kl;
        }
    }
    return bound;
}

double bandit_ucb1_regret_bound(const MultiArmedBandit *mab, int T) {
    /* UCB1 finite-time regret bound (Auer et al., 2002):
     * R_T <= 8 * sum_{i != i*} (ln T / Delta_i) + (1 + pi^2/3) * sum_i Delta_i
     */
    if (!mab || T <= 0) return 0.0;

    double bound = 0.0;
    double sum_delta = 0.0;
    double ln_T = log((double)T);

    for (int i = 0; i < mab->num_arms; i++) {
        double delta = bandit_gap(mab, i);
        sum_delta += delta;
        if (i != mab->best_arm_index && delta > 0.0) {
            bound += 8.0 * ln_T / delta;
        }
    }

    bound += (1.0 + M_PI * M_PI / 3.0) * sum_delta;
    return bound;
}

double bandit_kl_divergence(double p, double q) {
    /* KL divergence for Bernoulli distributions:
     * KL(p || q) = p ln(p/q) + (1-p) ln((1-p)/(1-q))
     */
    if (p <= 0.0) p = 1e-12;
    if (p >= 1.0) p = 1.0 - 1e-12;
    if (q <= 0.0) q = 1e-12;
    if (q >= 1.0) q = 1.0 - 1e-12;

    return p * log(p / q) + (1.0 - p) * log((1.0 - p) / (1.0 - q));
}

double bandit_gap(const MultiArmedBandit *mab, int arm_index) {
    if (!mab || arm_index < 0 || arm_index >= mab->num_arms) return 0.0;
    return mab->best_arm_mean - mab->arms[arm_index].true_mean;
}
/* ===================================================================
 * L6: Contextual Bandit ¡ª LinUCB (Li et al., 2010)
 *
 * Each round presents a context vector x_t. The expected reward for
 * arm a is theta_a^T x_t. The algorithm maintains a ridge regression
 * estimate of theta_a and uses a confidence ellipsoid for exploration.
 *
 * This models personalized recommendation (e.g. news articles), where
 * context encodes user features and article features.
 * =================================================================== */

ContextualBandit* cbandit_create(int num_arms, int context_dim,
                                  double **true_params, double noise_std) {
    ContextualBandit *cb = (ContextualBandit*)calloc(1, sizeof(ContextualBandit));
    if (!cb) return NULL;

    cb->num_arms = num_arms;
    cb->context_dim = context_dim;
    cb->noise_std = noise_std;

    cb->true_params = (double**)calloc(num_arms, sizeof(double*));
    for (int i = 0; i < num_arms; i++) {
        cb->true_params[i] = (double*)calloc(context_dim, sizeof(double));
        if (true_params && true_params[i]) {
            memcpy(cb->true_params[i], true_params[i],
                   context_dim * sizeof(double));
        }
    }

    cb->current_context = (double*)calloc(context_dim, sizeof(double));
    return cb;
}

void cbandit_set_context(ContextualBandit *cb, const double *context) {
    if (!cb || !context) return;
    memcpy(cb->current_context, context, cb->context_dim * sizeof(double));
}

double cbandit_pull(ContextualBandit *cb, int arm) {
    if (!cb || arm < 0 || arm >= cb->num_arms) return 0.0;

    /* True reward: theta_a^T x + noise */
    double mean = 0.0;
    for (int d = 0; d < cb->context_dim; d++) {
        mean += cb->true_params[arm][d] * cb->current_context[d];
    }

    /* Add Gaussian noise */
    double u1 = (double)rand() / RAND_MAX;
    double u2 = (double)rand() / RAND_MAX;
    double z = sqrt(-2.0 * log(u1 < 1e-12 ? 1e-12 : u1))
               * cos(2.0 * M_PI * u2);
    double reward = mean + cb->noise_std * z;

    if (reward < 0.0) reward = 0.0;
    if (reward > 1.0) reward = 1.0;
    return reward;
}

/* Matrix inverse for positive definite d x d matrix using Cholesky decomposition.
 * A = L * L^T, then solve L*y = b, L^T*x = y.
 * In-place: A is overwritten with L. Returns 0 on success, -1 on failure. */
static int cholesky_decompose(double *A, int d) {
    for (int i = 0; i < d; i++) {
        for (int j = i; j < d; j++) {
            double sum = A[i * d + j];
            for (int k = 0; k < i; k++) {
                sum -= A[i * d + k] * A[j * d + k];
            }
            if (i == j) {
                if (sum <= 0.0) return -1;
                A[i * d + i] = sqrt(sum);
            } else {
                A[j * d + i] = sum / A[i * d + i];
            }
        }
    }
    return 0;
}

/* Solve L * x = b where L is lower triangular (d x d) */
static void forward_substitution(const double *L, int d,
                                  const double *b, double *x) {
    for (int i = 0; i < d; i++) {
        double sum = b[i];
        for (int j = 0; j < i; j++) {
            sum -= L[i * d + j] * x[j];
        }
        x[i] = sum / L[i * d + i];
    }
}

/* Solve L^T * x = b where L is lower triangular */
static void backward_substitution(const double *L, int d,
                                   const double *b, double *x) {
    for (int i = d - 1; i >= 0; i--) {
        double sum = b[i];
        for (int j = i + 1; j < d; j++) {
            sum -= L[j * d + i] * x[j];
        }
        x[i] = sum / L[i * d + i];
    }
}

/* Solve A * x = b using stored Cholesky factorization */
static void cholesky_solve(double *A_chol, int d,
                            const double *b, double *x) {
    double *y = (double*)calloc(d, sizeof(double));
    forward_substitution(A_chol, d, b, y);
    backward_substitution(A_chol, d, y, x);
    free(y);
}

/* Sherman-Morrison rank-1 update: (A + u*u^T)^{-1} = A^{-1} - (A^{-1}u)(u^T A^{-1})/(1+u^T A^{-1}u)
 * Updates A_inv in-place given vector u of length d. */
static void sherman_morrison_update(double *A_inv, const double *u, int d) {
    double *Au = (double*)calloc(d, sizeof(double));
    /* Compute A_inv * u */
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < d; j++) {
            Au[i] += A_inv[i * d + j] * u[j];
        }
    }

    /* Compute denominator: 1 + u^T * A_inv * u */
    double denom = 1.0;
    for (int i = 0; i < d; i++) {
        denom += u[i] * Au[i];
    }

    /* Rank-1 update: A_inv -= (Au * Au^T) / denom */
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < d; j++) {
            A_inv[i * d + j] -= Au[i] * Au[j] / denom;
        }
    }
    free(Au);
}

int cbandit_linucb_select(ContextualBandit *cb, double alpha,
                            double **A_inv, double **b, double **theta_hat) {
    if (!cb) return -1;

    int d = cb->context_dim;
    double *x = cb->current_context;
    int best_arm = 0;
    double best_ucb = -DBL_MAX;

    for (int arm = 0; arm < cb->num_arms; arm++) {
        /* Phase 1: if A_inv not initialized, pull arm to get data */
        if (A_inv[arm][0] < 0.0) {
            /* Not initialized yet ¡ª pull this arm */
            return arm;
        }

        /* Compute theta_hat from A_inv and b: theta = A^{-1} * b */
        double *A_copy = (double*)calloc(d * d, sizeof(double));
        memcpy(A_copy, A_inv[arm], d * d * sizeof(double));

        /* Invert A_inv to get A, then solve A * theta = b using Cholesky */
        /* Actually we already have A_inv, just multiply: theta_hat = A_inv * b */
        for (int i = 0; i < d; i++) {
            theta_hat[arm][i] = 0.0;
            for (int j = 0; j < d; j++) {
                theta_hat[arm][i] += A_inv[arm][i * d + j] * b[arm][j];
            }
        }

        /* Compute x^T * A^{-1} * x (uncertainty) */
        double xAx = 0.0;
        for (int i = 0; i < d; i++) {
            for (int j = 0; j < d; j++) {
                xAx += x[i] * A_inv[arm][i * d + j] * x[j];
            }
        }

        /* Compute predicted reward */
        double predicted = 0.0;
        for (int i = 0; i < d; i++) {
            predicted += theta_hat[arm][i] * x[i];
        }

        /* UCB: predicted + alpha * sqrt(x^T A^{-1} x) */
        double ucb_value = predicted + alpha * sqrt(xAx + 1e-12);

        free(A_copy);

        if (ucb_value > best_ucb) {
            best_ucb = ucb_value;
            best_arm = arm;
        }
    }

    return best_arm;
}

void cbandit_linucb_update(int arm, const double *context, double reward,
                            double **A_inv, double **b,
                            int num_arms, int context_dim) {
    if (arm < 0 || arm >= num_arms) return;

    int d = context_dim;
    double *x = (double*)context;

    /* Update A_inv using Sherman-Morrison:
     * A_t^{-1} = (A_{t-1} + x*x^T)^{-1} = A_{t-1}^{-1} - (A^{-1}x)(x^T A^{-1})/(1+x^T A^{-1}x)
     */
    sherman_morrison_update(A_inv[arm], x, d);

    /* Update b: b_t = b_{t-1} + r * x */
    for (int i = 0; i < d; i++) {
        b[arm][i] += reward * x[i];
    }
}

void cbandit_linucb_init(int num_arms, int context_dim,
                          double **A_inv, double **b) {
    for (int arm = 0; arm < num_arms; arm++) {
        /* Initialize A_inv = I (identity matrix) ¡ª corresponds to ridge penalty lambda=1 */
        A_inv[arm] = (double*)calloc(context_dim * context_dim, sizeof(double));
        for (int i = 0; i < context_dim; i++) {
            A_inv[arm][i * context_dim + i] = 1.0;
        }

        /* Initialize b as zeros */
        b[arm] = (double*)calloc(context_dim, sizeof(double));
    }
}

void cbandit_destroy(ContextualBandit *cb) {
    if (!cb) return;
    for (int i = 0; i < cb->num_arms; i++) {
        free(cb->true_params[i]);
    }
    free(cb->true_params);
    free(cb->current_context);
    free(cb);
}

/* ===================================================================
 * L7: Adversarial Bandit ¡ª EXP3 (Auer et al., 2002)
 *
 * EXP3 = Exponential-weight for Exploration and Exploitation.
 * Works against adversarial (non-stochastic) reward sequences.
 *
 * Maintains weights w_i(t) = exp(eta * sum_{tau=1}^{t} r_hat_i(tau))
 * Selects arm i with probability (1-gamma) * w_i / sum(w_j) + gamma/K.
 *
 * Regret bound: O(sqrt(T * K * log K)) ¡ª sublinear in T.
 * =================================================================== */

EXP3State* exp3_create(int K, double gamma) {
    if (K <= 0) return NULL;

    EXP3State *s = (EXP3State*)calloc(1, sizeof(EXP3State));
    if (!s) return NULL;

    s->num_arms = K;
    s->gamma = gamma;
    /* Optimal learning rate: eta = sqrt(log K / (T * K)),
     * but since T unknown, use gamma/K as heuristic. */
    s->eta = gamma / (double)K;
    s->weights = (double*)calloc(K, sizeof(double));
    s->time_step = 0;

    /* Initialize weights to 1.0 */
    for (int i = 0; i < K; i++) {
        s->weights[i] = 1.0;
    }

    return s;
}

int exp3_select_arm(EXP3State *state) {
    if (!state || state->num_arms <= 0) return -1;
    if (state->num_arms == 1) return 0;

    int K = state->num_arms;
    double gamma = state->gamma;

    /* Compute sum of weights */
    double sum_w = 0.0;
    for (int i = 0; i < K; i++) {
        sum_w += state->weights[i];
    }
    if (sum_w < 1e-12) {
        /* Reset weights to avoid degenerate distribution */
        for (int i = 0; i < K; i++) state->weights[i] = 1.0;
        sum_w = K;
    }

    /* Compute probabilities: p_i = (1-gamma) * w_i/sum_w + gamma/K */
    double prob[128];
    for (int i = 0; i < K && i < 128; i++) {
        prob[i] = (1.0 - gamma) * state->weights[i] / sum_w + gamma / (double)K;
    }

    /* Sample from distribution */
    double r = (double)rand() / RAND_MAX;
    double cum = 0.0;
    for (int i = 0; i < K && i < 128; i++) {
        cum += prob[i];
        if (r <= cum) return i;
    }
    return K - 1;
}

void exp3_update(EXP3State *state, int chosen_arm, double reward) {
    if (!state || chosen_arm < 0 || chosen_arm >= state->num_arms) return;

    int K = state->num_arms;
    double gamma = state->gamma;

    /* Compute probability of choosing this arm */
    double sum_w = 0.0;
    for (int i = 0; i < K; i++) sum_w += state->weights[i];
    if (sum_w < 1e-12) {
        for (int i = 0; i < K; i++) state->weights[i] = 1.0;
        sum_w = K;
    }

    double prob = (1.0 - gamma) * state->weights[chosen_arm] / sum_w
                  + gamma / (double)K;

    /* Importance-weighted estimated reward:
     * r_hat_i = r / p_i if i was chosen, 0 otherwise */
    double estimated_reward = reward / prob;

    /* Update weight: w_i(t+1) = w_i(t) * exp(eta * r_hat_i) */
    state->weights[chosen_arm] *= exp(state->eta * estimated_reward);

    /* Normalize weights to prevent numerical overflow */
    sum_w = 0.0;
    for (int i = 0; i < K; i++) sum_w += state->weights[i];
    if (sum_w > 1e100) {
        double scale = 1.0 / sum_w;
        for (int i = 0; i < K; i++) state->weights[i] *= scale;
    }

    state->time_step++;
}

void exp3_destroy(EXP3State *state) {
    if (!state) return;
    free(state->weights);
    free(state);
}

double exp3_regret_bound(int K, int T) {
    /* EXP3 regret bound: R_T <= 2 * sqrt(e-1) * sqrt(T * K * log K)
     * where e = exp(1), K = number of arms, T = horizon. */
    if (K <= 0 || T <= 0) return 0.0;
    return 2.0 * sqrt(exp(1.0) - 1.0) * sqrt((double)T * (double)K * log((double)K));
}
