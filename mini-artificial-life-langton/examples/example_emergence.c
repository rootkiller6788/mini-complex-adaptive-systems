/**
 * example_emergence.c — Emergence and complexity measurement
 *
 * Demonstrates:
 *   - Creating state evolution history from CA simulation
 *   - Computing emergence metrics
 *   - Open-endedness detection
 *   - Comprehensive ALife test
 *
 * Uses Game of Life patterns to illustrate self-organization
 * and emergent complexity.
 */
#include "cellular_automaton.h"
#include "alife_metrics.h"
#include "langton_lambda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(void)
{
    printf("=== Emergence and Complexity in CA ===\n\n");

    int W = 40, H = 40;
    int n_generations = 30;

    /* Set up Game of Life */
    ca_config_t cfg = ca_config_default(CA_DIM_2D, W, H);
    cfg.boundary = CA_BOUNDARY_FIXED;
    ca_rule_totalistic_t gol_rule = ca_rule_totalistic_create(2, "3", "23");
    ca_config_set_totalistic(&cfg, &gol_rule);

    ca_state_t *state = ca_state_create(&cfg);
    ca_state_randomize(state, 12345);

    /* Allocate state history (pointers to grid snapshots) */
    int max_history = n_generations + 1;
    uint8_t **history = (uint8_t **)malloc((size_t)max_history * sizeof(uint8_t *));
    for (int t = 0; t < max_history; t++) {
        history[t] = (uint8_t *)calloc((size_t)(W * H), sizeof(uint8_t));
    }

    /* Record initial state */
    memcpy(history[0], state->cells, (size_t)(W * H) * sizeof(uint8_t));

    /* Evolve and record */
    printf("Evolving Game of Life for %d generations...\n", n_generations);
    for (int t = 1; t <= n_generations; t++) {
        ca_evolve_step(state);
        memcpy(history[t], state->cells, (size_t)(W * H) * sizeof(uint8_t));
        if (t % 10 == 0) {
            printf("  Generation %d complete\n", t);
        }
    }

    /* Compute metrics */
    alife_metrics_config_t metrics_cfg;
    metrics_cfg.history_length = n_generations;
    metrics_cfg.pattern_radius = 1;
    metrics_cfg.n_bootstrap_samples = 20;
    metrics_cfg.seed = 42;
    metrics_cfg.significance_level = 0.05;

    printf("\n--- ALife Metrics ---\n");

    alife_emergence_t emergence = alife_analyze_emergence(
        (const uint8_t **)history, (size_t)max_history, W, H, &metrics_cfg);

    printf("Emergence score:         %.4f\n", emergence.emergence_score);
    printf("Self-organization:       %.4f\n", emergence.self_org_score);
    printf("Complexity score:        %.4f\n", emergence.complexity_score);
    printf("Novelty rate:            %.4f\n", emergence.novelty_rate);
    printf("Predictability gain:     %.4f\n", emergence.predictability_gain);

    /* Self-organization */
    double so = alife_self_organization((const uint8_t **)history,
                                         (size_t)max_history, W, H);
    printf("\nSpatial self-organization: %.4f\n", so);

    /* Lempel-Ziv complexity */
    double lz = alife_lempel_ziv_complexity(state->cells, (size_t)(W * H));
    printf("Lempel-Ziv complexity:     %.4f\n", lz);

    /* Effective complexity */
    double ec = alife_effective_complexity((const uint8_t **)history,
                                            (size_t)max_history, W, H);
    printf("Effective complexity:      %.4f\n", ec);

    /* Thermodynamic depth */
    double td = alife_thermodynamic_depth((const uint8_t **)history,
                                           (size_t)max_history, W, H);
    printf("Thermodynamic depth:       %.4f\n", td);

    /* Multi-scale complexity */
    double msc = alife_multiscale_complexity(state->cells, W, H, 4);
    printf("Multi-scale complexity:    %.4f\n", msc);

    /* Phi (integrated information) */
    double phi = alife_phi_integrated_info(state->cells, W, H, 2);
    printf("Integrated info (Phi):     %.4f\n", phi);

    /* Statistical complexity */
    alife_stat_complexity_t sc = alife_statistical_complexity_calc(
        (const uint8_t **)history, (size_t)max_history,
        W, H, 2, &metrics_cfg);
    printf("\nStatistical complexity:\n");
    printf("  C_mu:                   %.4f\n", sc.statistical_complexity);
    printf("  Entropy rate (h_mu):    %.4f\n", sc.entropy_rate_estimate);
    printf("  Causal states:          %llu\n", (unsigned long long)sc.n_causal_states);
    printf("  Predictive info:        %.4f\n", sc.predictive_information);

    /* Open-endedness detection */
    double novelty_trend;
    bool open_ended = alife_detect_open_ended((const uint8_t **)history,
                                                (size_t)max_history,
                                                W, H, &novelty_trend);
    printf("\nOpen-ended evolution:     %s\n", open_ended ? "DETECTED" : "not detected");
    printf("Novelty trend:            %.6f\n", novelty_trend);

    /* Comprehensive test */
    double scores[5];
    alife_comprehensive_test((const uint8_t **)history, (size_t)max_history,
                              W, H, &metrics_cfg, scores);
    printf("\n=== Comprehensive ALife Test ===\n");
    printf("(1) Emergence:           %.4f\n", scores[0]);
    printf("(2) Self-organization:   %.4f\n", scores[1]);
    printf("(3) Adaptation:          %.4f\n", scores[2]);
    printf("(4) Open-endedness:      %.4f\n", scores[3]);
    printf("(5) Life-likeness:       %.4f\n", scores[4]);

    /* Cleanup */
    for (int t = 0; t < max_history; t++) {
        free(history[t]);
    }
    free(history);
    ca_state_destroy(state);
    free(gol_rule.birth);
    free(gol_rule.survival);
    ca_config_free(&cfg);

    printf("\n=== Emergence Analysis Complete ===\n");
    return 0;
}
