/**
 * example_lambda_analysis.c — Langton's Lambda parameter analysis
 *
 * Demonstrates:
 *   - Rule generation at fixed lambda
 *   - Phase transition sampling
 *   - Edge of chaos detection
 *   - Lambda-entropy phase diagram
 */
#include "langton_lambda.h"
#include "cellular_automaton.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void)
{
    printf("=== Langton's Lambda Parameter Analysis ===\n\n");

    lambda_config_t base_cfg;
    base_cfg.num_states = 2;
    base_cfg.num_inputs = 5;
    base_cfg.quiescent_state = 0;
    base_cfg.table_size = 32; /* 2^5 */
    base_cfg.lambda = 0.0;

    printf("Rule space: k=%d, N=%d => |R| = %llu rules\n",
           base_cfg.num_states,
           base_cfg.num_inputs,
           (unsigned long long)(1ULL << base_cfg.table_size));

    /* Generate rules at a single lambda value */
    printf("\n--- Single Lambda Sample (lambda = 0.27) ---\n");
    base_cfg.lambda = 0.27;
    base_cfg.non_quiescent_n = 9;

    lambda_rule_t *rule = lambda_rule_generate(&base_cfg, 42);
    if (rule) {
        printf("Generated rule:\n");
        printf("  lambda = %.4f (target: %.4f)\n", rule->actual_lambda, base_cfg.lambda);
        printf("  entropy = %.4f\n", rule->entropy);
        printf("  Z parameter = %.4f\n", rule->z_parameter);

        /* Show first 8 rule table entries */
        printf("  Rule table (first 8): ");
        for (int i = 0; i < 8 && i < (int)rule->config.table_size; i++) {
            printf("%d ", rule->rule_table[i]);
        }
        printf("\n");

        lambda_rule_destroy(rule);
    }

    /* Phase transition sweep */
    printf("\n--- Lambda Sweep (0.0 - 1.0) ---\n");
    int n_points = 11;
    lambda_phase_point_t *sweep = lambda_sweep_phase(&base_cfg, n_points, 20, 42);

    if (sweep) {
        printf("%8s %12s %12s %15s\n", "lambda", "avg_entropy", "transient", "dominant_class");
        printf("%8s %12s %12s %15s\n", "------", "-----------", "---------", "--------------");

        for (int i = 0; i < n_points; i++) {
            const char *class_name;
            switch (sweep[i].dominant_class) {
                case CA_CLASS_I:  class_name = "I (frozen)"; break;
                case CA_CLASS_II: class_name = "II (periodic)"; break;
                case CA_CLASS_III: class_name = "III (chaotic)"; break;
                case CA_CLASS_IV: class_name = "IV (complex)"; break;
                default: class_name = "unknown"; break;
            }

            printf("%8.3f %12.6f %12.1f %15s\n",
                   sweep[i].lambda,
                   sweep[i].avg_entropy,
                   sweep[i].transient_length,
                   class_name);
        }

        /* Detect edge of chaos */
        double lambda_c = lambda_detect_edge_of_chaos(sweep, n_points);
        printf("\nEstimated edge of chaos: lambda_c = %.4f\n", lambda_c);
        printf("(Langton's original estimate: lambda_c ~ 0.273)\n");

        /* Test edge hypothesis */
        double peak_lambda, peak_prob;
        bool supported = lambda_test_edge_hypothesis(sweep, n_points,
                                                      &peak_lambda, &peak_prob);
        printf("Edge hypothesis peak: lambda = %.4f, P(Complex) = %.4f\n",
               peak_lambda, peak_prob);
        printf("Hypothesis supported: %s\n", supported ? "YES" : "NO");

        /* Critical exponent */
        double beta = lambda_critical_exponent(sweep, n_points, lambda_c);
        printf("Critical exponent beta: %.4f\n", beta);

        free(sweep);
    }

    printf("\n=== Lambda Analysis Complete ===\n");
    return 0;
}
