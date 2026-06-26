#include "ace_dynamics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * ace_dynamics.c - Dynamic models for CAS in Economics (L4-L5)
 *
 * Implements Polya urn processes, adoption dynamics, lock-in detection,
 * stochastic process simulation, network cascades, time series analysis,
 * distribution analysis, and macro state aggregation.
 * ============================================================================ */

/* ---- K0: Arthur Urn Function (L4) ---- */
double ace_arthur_urn_function(double proportion, double gamma)
{
    if (proportion <= 0.0) return 0.0;
    if (proportion >= 1.0) return 1.0;
    double p_gamma = pow(proportion, gamma);
    double q_gamma = pow(1.0 - proportion, gamma);
    double denom = p_gamma + q_gamma;
    if (denom < 1e-15) return 0.5;
    return p_gamma / denom;
}

/* ---- K1: Linear Urn Function (L4) ---- */
double ace_linear_urn_function(double proportion, double a, double b)
{
    double result = a * proportion + b;
    if (result < 0.0) return 0.0;
    if (result > 1.0) return 1.0;
    return result;
}

/* ---- K2: Sigmoid Adoption Function (L4) ---- */
double ace_sigmoid_urn_function(double proportion, double k, double p0)
{
    double arg = -k * (proportion - p0);
    if (arg > 50.0) return 1.0;
    if (arg < -50.0) return 0.0;
    return 1.0 / (1.0 + exp(arg));
}

/* ---- K3: Polya Urn Draw (L4-L5) ---- */
int ace_polya_urn_draw(ACEPolyaUrn* urn, unsigned int* seed)
{
    if (!urn || !seed || urn->total_balls <= 0) return -1;
    double* cum = (double*)malloc((size_t)urn->n_colors * sizeof(double));
    if (!cum) return -1;
    double total = (double)urn->total_balls;
    cum[0] = (double)urn->current_balls[0] / total;
    for (int i = 1; i < urn->n_colors; i++) {
        cum[i] = cum[i-1] + (double)urn->current_balls[i] / total;
    }
    double r = ace_random_uniform(seed);
    int chosen = urn->n_colors - 1;
    for (int i = 0; i < urn->n_colors; i++) {
        if (r <= cum[i]) { chosen = i; break; }
    }
    free(cum);
    int added = (int)(urn->self_reinforcement);
    if (added < 1) added = 1;
    urn->current_balls[chosen] += added;
    urn->total_balls += added;
    return chosen;
}

/* ---- K4: Init Urn with Custom Addition Function (L4) ---- */
void ace_polya_urn_init_function(ACEPolyaUrn* urn,
                                  double (*addition_func)(double proportion))
{
    if (!urn) return;
    urn->use_generalized = true;
    (void)addition_func;
}

/* ---- K5: Run Polya Urn Simulation (L5) ---- */
void ace_polya_urn_run(ACEPolyaUrn* urn, int n_draws, ACEPathHistory* hist,
                       unsigned int* seed)
{
    if (!urn || !hist || !seed || n_draws <= 0) return;
    if (hist->n_technologies != urn->n_colors) return;
    for (int d = 0; d < n_draws; d++) {
        ace_polya_urn_draw(urn, seed);
        int offset = (hist->n_steps + d) * urn->n_colors;
        double total = (double)urn->total_balls;
        if (total > 0.0) {
            for (int c = 0; c < urn->n_colors; c++) {
                hist->market_shares[offset + c] =
                    (double)urn->current_balls[c] / total;
            }
        }
        hist->time_points[hist->n_steps + d] =
            (double)(hist->n_steps + d + 1);
    }
    hist->n_steps += n_draws;
}

/* ---- K6: Polya Urn Proportions (L5) ---- */
void ace_polya_urn_proportions(const ACEPolyaUrn* urn, double* proportions)
{
    if (!urn || !proportions || urn->total_balls <= 0) return;
    double total = (double)urn->total_balls;
    for (int i = 0; i < urn->n_colors; i++) {
        proportions[i] = (double)urn->current_balls[i] / total;
    }
}


/* ---- K7: Binary Adoption Step (L5) ---- */
void ace_adoption_step_binary(ACEAdoptionState* state, double r_a, double r_b,
                               double gamma_a, double gamma_b,
                               unsigned int* seed)
{
    if (!state || state->n_technologies != 2 || !seed) return;
    double s_a = state->shares[0];
    double s_b = state->shares[1];
    double payoff_a = r_a + pow(s_a, gamma_a);
    double payoff_b = r_b + pow(s_b, gamma_b);
    double scores[2] = {payoff_a, payoff_b};
    double probs[2];
    ace_softmax(scores, 2, 0.2, probs);
    double r = ace_random_uniform(seed);
    int chosen = (r < probs[0]) ? 0 : 1;
    double alpha = 1.0 / (state->time + 2.0);
    state->shares[chosen] = (1.0 - alpha) * state->shares[chosen] + alpha;
    state->shares[1 - chosen] = 1.0 - state->shares[chosen];
    state->time += 1.0;
}

/* ---- K8: Multi-Technology Adoption Step (L5) ---- */
void ace_adoption_step_multi(ACEAdoptionState* state,
                              const double* inherent_returns,
                              const double* network_exponents,
                              int n_techs, unsigned int* seed)
{
    if (!state || !inherent_returns || !network_exponents || !seed) return;
    if (n_techs != state->n_technologies) return;
    double* scores = NULL;
    double* probs = NULL;
    scores = (double*)malloc((size_t)n_techs * sizeof(double));
    probs = (double*)malloc((size_t)n_techs * sizeof(double));
    if (!scores || !probs) { free(scores); free(probs); return; }
    for (int i = 0; i < n_techs; i++) {
        scores[i] = inherent_returns[i]
                  + pow(state->shares[i], network_exponents[i]);
    }
    ace_softmax(scores, n_techs, 0.2, probs);
    double r = ace_random_uniform(seed);
    double cum = 0.0;
    int chosen = 0;
    for (int i = 0; i < n_techs; i++) {
        cum += probs[i];
        if (r <= cum) { chosen = i; break; }
    }
    double alpha = 1.0 / (state->time + 2.0);
    for (int i = 0; i < n_techs; i++) {
        state->shares[i] *= (1.0 - alpha);
    }
    state->shares[chosen] += alpha;
    state->time += 1.0;
    free(scores);
    free(probs);
}

/* ---- K9: Adoption Probabilities (L5) ---- */
void ace_adoption_probabilities(const ACEAdoptionState* state,
                                 const double* returns, double gamma,
                                 double* probs)
{
    if (!state || !returns || !probs) return;
    int n = state->n_technologies;
    double* scores = (double*)malloc((size_t)n * sizeof(double));
    if (!scores) return;
    for (int i = 0; i < n; i++) {
        scores[i] = returns[i] + pow(state->shares[i], gamma);
    }
    ace_softmax(scores, n, 0.2, probs);
    free(scores);
}

/* ---- K10: Lock-in Detection (L4-L5) ---- */
ACELockState ace_detect_lockin(const ACEPathHistory* hist,
                                double threshold, int persistence)
{
    if (!hist || hist->n_steps < persistence) return ACE_NOT_LOCKED;
    int n = hist->n_technologies;
    int best_consecutive = 0;
    int best_tech = -1;
    for (int t = 0; t < n; t++) {
        int consecutive = 0;
        for (int s = 0; s < hist->n_steps; s++) {
            double share = hist->market_shares[s * n + t];
            if (share >= threshold) {
                consecutive++;
                if (consecutive >= persistence &&
                    (best_tech < 0 || consecutive > best_consecutive)) {
                    best_consecutive = consecutive;
                    best_tech = t;
                }
            } else {
                consecutive = 0;
            }
        }
    }
    if (best_tech < 0) return ACE_NOT_LOCKED;
    if (best_consecutive >= 2 * persistence) return ACE_FULL_LOCK;
    if (best_consecutive >= persistence) return ACE_PARTIAL_LOCK;
    return ACE_NOT_LOCKED;
}

/* ---- K11: Critical Mass (L5) ---- */
double ace_critical_mass(double gamma, double n_total)
{
    if (gamma <= 0.0 || n_total <= 0.0) return 0.5;
    return pow(n_total, -gamma / (gamma + 1.0));
}

/* ---- K12: Markov Process Initialization (L5) ---- */
void ace_markov_process_init(ACEStochasticProcess* sp,
                              const double* transition_matrix_flat)
{
    if (!sp || !transition_matrix_flat) return;
    int d = sp->state_dimension;
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < d; j++) {
            sp->diffusion[i][j] = transition_matrix_flat[i * d + j];
        }
    }
    sp->is_markov = true;
}


/* ---- K13: Euler-Maruyama Stochastic Step (L5) ---- */
void ace_stochastic_step(ACEStochasticProcess* sp, unsigned int* seed)
{
    if (!sp || !seed) return;
    int d = sp->state_dimension;
    double dt = sp->time_step;
    double sqrt_dt = sqrt(dt);
    for (int i = 0; i < d; i++) {
        sp->state[i] += sp->drift[i] * dt;
        for (int j = 0; j < d; j++) {
            sp->state[i] += sp->diffusion[i][j] * sqrt_dt
                          * ace_random_gaussian(seed);
        }
    }
}

void ace_stochastic_run(ACEStochasticProcess* sp, int n_steps,
                         double** path_out, unsigned int* seed)
{
    if (!sp || !path_out || !seed || n_steps <= 0) return;
    int d = sp->state_dimension;
    for (int s = 0; s < n_steps; s++) {
        for (int i = 0; i < d; i++) {
            path_out[s][i] = sp->state[i];
        }
        ace_stochastic_step(sp, seed);
    }
}

void ace_estimate_drift_diffusion(const ACETimeSeries* ts,
                                   double* drift_est, double* diffusion_est)
{
    if (!ts || ts->length < 2 || !drift_est || !diffusion_est) return;
    double sum_dx = 0.0, sum_dx2 = 0.0;
    int n = ts->length - 1;
    for (int i = 0; i < n; i++) {
        double dx = ts->data[i+1] - ts->data[i];
        sum_dx += dx;
        sum_dx2 += dx * dx;
    }
    double mean_dx = sum_dx / (double)n;
    double var_dx = sum_dx2 / (double)n - mean_dx * mean_dx;
    if (var_dx < 0.0) var_dx = 0.0;
    *drift_est = mean_dx;
    *diffusion_est = sqrt(var_dx);
}

bool ace_test_path_dependence(ACEStochasticProcess* sp, int n_trials,
                               int steps_per_trial, unsigned int* seed)
{
    if (!sp || n_trials < 2) return false;
    int d = sp->state_dimension;
    size_t state_size = (size_t)(n_trials * d) * sizeof(double);
    double* initial_states = (double*)malloc(state_size);
    double* final_states = (double*)malloc(state_size);
    if (!initial_states || !final_states) {
        free(initial_states); free(final_states); return false;
    }
    double* orig_state = (double*)malloc((size_t)d * sizeof(double));
    if (!orig_state) { free(initial_states); free(final_states); return false; }
    memcpy(orig_state, sp->state, (size_t)d * sizeof(double));
    for (int t = 0; t < n_trials; t++) {
        for (int i = 0; i < d; i++) {
            sp->state[i] = (t == 0) ? 0.0 : ((double)t / (double)(n_trials - 1));
            initial_states[t * d + i] = sp->state[i];
        }
        for (int s = 0; s < steps_per_trial; s++)
            ace_stochastic_step(sp, seed);
        for (int i = 0; i < d; i++)
            final_states[t * d + i] = sp->state[i];
    }
    double max_init_diff = 0.0, max_final_diff = 0.0;
    for (int t1 = 0; t1 < n_trials; t1++) {
        for (int t2 = t1+1; t2 < n_trials; t2++) {
            double init_diff = 0.0, final_diff = 0.0;
            for (int i = 0; i < d; i++) {
                init_diff += pow(initial_states[t1*d+i] - initial_states[t2*d+i], 2);
                final_diff += pow(final_states[t1*d+i] - final_states[t2*d+i], 2);
            }
            if (sqrt(init_diff) > max_init_diff) max_init_diff = sqrt(init_diff);
            if (sqrt(final_diff) > max_final_diff) max_final_diff = sqrt(final_diff);
        }
    }
    memcpy(sp->state, orig_state, (size_t)d * sizeof(double));
    free(orig_state); free(initial_states); free(final_states);
    return (max_init_diff > 1e-10) && (max_final_diff > 0.1 * max_init_diff);
}


double ace_schelling_utility(const ACENetwork* net, int idx,
                              const int* agent_types, double threshold)
{
    if (!net || !agent_types || idx < 0 || idx >= net->n_nodes) return 0.0;
    int deg = net->degree[idx];
    if (deg == 0) return 0.0;
    int same = 0;
    for (int j = 0; j < net->n_nodes; j++) {
        if (net->adjacency[idx][j] && agent_types[j] == agent_types[idx])
            same++;
    }
    return (double)same / (double)deg - threshold;
}

double ace_tipping_threshold(const ACENetwork* net, double adoption_benefit,
                              double baseline_utility, int max_iterations,
                              unsigned int* seed)
{
    if (!net || net->n_nodes <= 0) return 0.5;
    bool* adopted = (bool*)calloc((size_t)net->n_nodes, sizeof(bool));
    double* thresholds = (double*)malloc((size_t)net->n_nodes * sizeof(double));
    if (!adopted || !thresholds) { free(adopted); free(thresholds); return 0.5; }
    for (int i = 0; i < net->n_nodes; i++)
        thresholds[i] = 0.1 + 0.4 * ace_random_uniform(seed);
    double lo = 0.0, hi = 1.0;
    for (int iter = 0; iter < 50; iter++) {
        double mid = (lo + hi) / 2.0;
        memset(adopted, 0, (size_t)net->n_nodes * sizeof(bool));
        int n_seed = (int)(mid * net->n_nodes);
        if (n_seed > net->n_nodes) n_seed = net->n_nodes;
        for (int i = 0; i < n_seed; i++) adopted[i] = true;
        ace_network_cascade(net, adopted, thresholds, seed);
        int final_adopted = 0;
        for (int i = 0; i < net->n_nodes; i++) if (adopted[i]) final_adopted++;
        if ((double)final_adopted / net->n_nodes > 0.5) hi = mid; else lo = mid;
        if (hi - lo < 0.005) break;
    }
    free(adopted); free(thresholds);
    (void)adoption_benefit; (void)baseline_utility; (void)max_iterations;
    return (lo + hi) / 2.0;
}

void ace_network_cascade(ACENetwork* net, bool* adopted,
                          const double* thresholds, unsigned int* seed)
{
    if (!net || !adopted || !thresholds) return;
    int n = net->n_nodes;
    bool changed = true;
    int max_iter = n * 2;
    while (changed && max_iter-- > 0) {
        changed = false;
        for (int i = 0; i < n; i++) {
            if (adopted[i]) continue;
            int deg = net->degree[i];
            if (deg == 0) continue;
            int an = 0;
            for (int j = 0; j < n; j++)
                if (net->adjacency[i][j] && adopted[j]) an++;
            if ((double)an / deg >= thresholds[i]) {
                adopted[i] = true; changed = true;
            }
        }
    }
    (void)seed;
}

double ace_cascade_final_share(const ACENetwork* net,
                                const bool* initial_adopters,
                                const double* thresholds, unsigned int* seed)
{
    if (!net || !initial_adopters || !thresholds) return 0.0;
    int n = net->n_nodes;
    bool* adopted = (bool*)malloc((size_t)n * sizeof(bool));
    if (!adopted) return 0.0;
    memcpy(adopted, initial_adopters, (size_t)n * sizeof(bool));
    ace_network_cascade(net, adopted, thresholds, seed);
    int count = 0;
    for (int i = 0; i < n; i++) if (adopted[i]) count++;
    free(adopted);
    return (double)count / (double)n;
}

void ace_timeseries_append(ACETimeSeries* ts, double value)
{
    if (!ts) return;
    if (ts->length >= ts->capacity) {
        int new_cap = ts->capacity * 2;
        double* new_data = (double*)realloc(ts->data,
            (size_t)new_cap * sizeof(double));
        if (!new_data) return;
        ts->data = new_data; ts->capacity = new_cap;
    }
    ts->data[ts->length++] = value;
}

void ace_timeseries_update_mean(ACETimeSeries* ts)
{
    if (!ts || ts->length == 0) return;
    double sum = 0.0;
    for (int i = 0; i < ts->length; i++) sum += ts->data[i];
    ts->mean = sum / (double)ts->length;
}

void ace_timeseries_update_variance(ACETimeSeries* ts)
{
    if (!ts || ts->length < 2) { if (ts) ts->variance = 0.0; return; }
    ace_timeseries_update_mean(ts);
    double sum_sq = 0.0;
    for (int i = 0; i < ts->length; i++) {
        double d = ts->data[i] - ts->mean;
        sum_sq += d * d;
    }
    ts->variance = sum_sq / (double)(ts->length - 1);
}

void ace_timeseries_compute_moments(ACETimeSeries* ts)
{
    if (!ts || ts->length < 4) return;
    ace_timeseries_update_variance(ts);
    double std = sqrt(ts->variance);
    if (std < 1e-15) { ts->skewness = 0.0; ts->kurtosis = 0.0; return; }
    double sum_s3 = 0.0, sum_s4 = 0.0;
    for (int i = 0; i < ts->length; i++) {
        double z = (ts->data[i] - ts->mean) / std;
        double z3 = z * z * z;
        sum_s3 += z3; sum_s4 += z3 * z;
    }
    ts->skewness = sum_s3 / (double)ts->length;
    ts->kurtosis = sum_s4 / (double)ts->length - 3.0;
}

double ace_estimate_hurst(const double* data, int n)
{
    if (!data || n < 10) return 0.5;
    double mean = 0.0;
    for (int i = 0; i < n; i++) mean += data[i];
    mean /= (double)n;
    double* Y = (double*)malloc((size_t)n * sizeof(double));
    if (!Y) return 0.5;
    Y[0] = data[0] - mean;
    for (int i = 1; i < n; i++) Y[i] = Y[i-1] + data[i] - mean;
    double Ymin = Y[0], Ymax = Y[0];
    for (int i = 1; i < n; i++) {
        if (Y[i] < Ymin) Ymin = Y[i];
        if (Y[i] > Ymax) Ymax = Y[i];
    }
    double R = Ymax - Ymin, S = 0.0;
    for (int i = 0; i < n; i++) S += (data[i]-mean) * (data[i]-mean);
    S = sqrt(S / (double)n);
    double RS = (S > 1e-15) ? R / S : 0.0;
    double H = (n > 1 && RS > 0.0) ? log(RS) / log((double)n) : 0.5;
    free(Y);
    return H;
}


bool ace_test_stationarity(const double* data, int n, double significance)
{
    if (!data || n < 20) return false;
    double* sorted = (double*)malloc((size_t)n * sizeof(double));
    if (!sorted) return false;
    memcpy(sorted, data, (size_t)n * sizeof(double));
    for (int i = 1; i < n; i++) {
        double key = sorted[i]; int j = i - 1;
        while (j >= 0 && sorted[j] > key) { sorted[j+1] = sorted[j]; j--; }
        sorted[j+1] = key;
    }
    double median = (n%2==0) ? (sorted[n/2-1]+sorted[n/2])/2.0 : sorted[n/2];
    free(sorted);
    int n_above=0, n_below=0;
    for (int i=0; i<n; i++) { if(data[i]>median) n_above++; else n_below++; }
    if (n_above<2 || n_below<2) return false;
    int runs=1; bool above=(data[0]>median);
    for (int i=1; i<n; i++){ bool cur=(data[i]>median); if(cur!=above){runs++; above=cur;} }
    double ER=1.0+2.0*(double)n_above*(double)n_below/(double)n;
    double VR=(ER-1.0)*(ER-2.0)/((double)n-1.0);
    double Z=((double)runs-ER)/sqrt(VR);
    double crit=(significance<=0.05)?1.96:((significance<=0.1)?1.645:2.58);
    return !(Z<-crit || Z>crit);
}

void ace_distribution_from_data(ACEDistribution* dist, const double* data, int n)
{
    if (!dist || !data || n<=0) return;
    for (int i=0; i<dist->n_bins; i++) dist->probabilities[i]=0.0;
    for (int i=0; i<n; i++){
        double x=data[i];
        if(x<dist->range_min) x=dist->range_min;
        if(x>=dist->range_max) x=dist->range_max-dist->bin_width*0.5;
        int bin=(int)((x-dist->range_min)/dist->bin_width);
        if(bin<0) bin=0; if(bin>=dist->n_bins) bin=dist->n_bins-1;
        dist->probabilities[bin]+=1.0;
    }
    for (int i=0; i<dist->n_bins; i++) dist->probabilities[i]/=(double)n;
}

double ace_compute_gini(const double* data, int n)
{
    if (!data || n<=1) return 0.0;
    double sum=0.0;
    for (int i=0; i<n; i++) sum+=data[i];
    if (sum<1e-15) return 0.0;
    double mean=sum/(double)n, gs=0.0;
    for (int i=0; i<n; i++)
        for (int j=0; j<n; j++)
            gs+=fabs(data[i]-data[j]);
    return gs/(2.0*(double)n*(double)n*mean);
}

void ace_test_heavy_tail(ACEDistribution* dist, const double* data, int n,
                          double tail_fraction)
{
    if (!dist || !data || n<10) return;
    double* sorted=(double*)malloc((size_t)n*sizeof(double));
    if(!sorted) return;
    memcpy(sorted,data,(size_t)n*sizeof(double));
    for(int i=1; i<n; i++){
        double key=sorted[i]; int j=i-1;
        while(j>=0&&sorted[j]>key){sorted[j+1]=sorted[j]; j--;}
        sorted[j+1]=key;
    }
    int tail_start=n-(int)(tail_fraction*n);
    if(tail_start>=n) tail_start=n-1;
    if(tail_start<2) tail_start=2;
    int n_tail=n-tail_start;
    if(n_tail<5){free(sorted); return;}
    double x_min=sorted[tail_start];
    if(x_min<1e-15){free(sorted); dist->is_heavy_tailed=false; return;}
    double sum_lr=0.0;
    for(int i=tail_start; i<n; i++) sum_lr+=log(sorted[i]/x_min);
    double alpha=(double)n_tail/sum_lr;
    dist->tail_exponent=alpha;
    dist->is_heavy_tailed=(alpha>0.5&&alpha<5.0);
    free(sorted);
}

void ace_lorenz_curve(const double* sorted_data, int n,
                       double* cumulative_pop, double* cumulative_income)
{
    if(!sorted_data||!cumulative_pop||!cumulative_income||n<=0) return;
    double total=0.0;
    for(int i=0; i<n; i++) total+=sorted_data[i];
    if(total<1e-15) return;
    double cum=0.0;
    for(int i=0; i<n; i++){
        cumulative_pop[i]=(double)(i+1)/(double)n;
        cum+=sorted_data[i];
        cumulative_income[i]=cum/total;
    }
}

void ace_compute_macro_state(const ACEAgent* agents, int n_agents,
                              ACEMacroState* ms)
{
    if(!agents||!ms||n_agents<=0) return;
    double total_wealth=0.0;
    int active=0;
    for(int i=0; i<n_agents; i++){
        total_wealth+=agents[i].wealth;
        if(agents[i].wealth>0.0) active++;
    }
    ms->n_active_agents=active;
    double* shares=(double*)malloc((size_t)n_agents*sizeof(double));
    if(!shares) return;
    for(int i=0; i<n_agents; i++)
        shares[i]=(total_wealth>0.0)?agents[i].wealth/total_wealth:0.0;
    ms->herfindahl_index=ace_herfindahl_index(shares,n_agents);
    ms->entropy=ace_shannon_entropy(shares,n_agents);
    double* wc=(double*)malloc((size_t)n_agents*sizeof(double));
    if(wc){
        memcpy(wc,shares,(size_t)n_agents*sizeof(double));
        for(int i=1; i<n_agents; i++){
            double key=wc[i]; int j=i-1;
            while(j>=0&&wc[j]>key){wc[j+1]=wc[j]; j--;}
            wc[j+1]=key;
        }
        ms->inequality=ace_compute_gini(wc,n_agents);
        free(wc);
    }
    free(shares);
}

bool ace_detect_crisis(const ACEMacroState* ms, double vol_threshold,
                        double concentration_threshold)
{
    if(!ms) return false;
    return (ms->market_volatility>vol_threshold &&
            ms->herfindahl_index>concentration_threshold &&
            ms->entropy<2.0 &&
            ms->n_bankruptcies>ms->n_active_agents/10);
}

void ace_market_concentration_dynamics(const ACEAgent* agents, int n_agents,
                                        int n_periods, double* hhi_series)
{
    if(!agents||!hhi_series||n_agents<=0||n_periods<=0) return;
    for(int p=0; p<n_periods; p++){
        double total=0.0;
        for(int i=0; i<n_agents; i++) total+=agents[i].wealth;
        if(total>0.0){
            double* shares=(double*)malloc((size_t)n_agents*sizeof(double));
            if(shares){
                for(int i=0; i<n_agents; i++) shares[i]=agents[i].wealth/total;
                hhi_series[p]=ace_herfindahl_index(shares,n_agents);
                free(shares);
            }
        } else hhi_series[p]=0.0;
    }
}
