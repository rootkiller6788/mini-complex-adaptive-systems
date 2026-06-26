/** test_alife_metrics.c — Tests for ALife metrics */
#include "alife_metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

static void test_shannon_entropy(void)
{
    uint64_t freqs[] = {50, 50};
    double H = alife_shannon_entropy(freqs, 2, 100);
    assert(H > 0.99 && H < 1.01); /* near 1 bit */

    uint64_t freqs2[] = {100, 0};
    H = alife_shannon_entropy(freqs2, 2, 100);
    assert(H < 0.01); /* zero entropy */

    printf("  PASS: shannon entropy\n");
}

static void test_mutual_information(void)
{
    uint64_t joint[] = {25, 25, 25, 25};
    uint64_t marg_x[] = {50, 50};
    uint64_t marg_y[] = {50, 50};
    double mi = alife_mutual_information(joint, marg_x, marg_y, 2, 2, 100);
    assert(mi >= 0.0 && mi <= 1.0);

    printf("  PASS: mutual information\n");
}

static void test_conditional_entropy(void)
{
    uint64_t joint[] = {50, 0, 0, 50}; /* perfect correlation */
    uint64_t marg_x[] = {50, 50};
    double ce = alife_conditional_entropy(joint, marg_x, 2, 2, 100);
    assert(ce < 0.01); /* H(Y|X) = 0 when X determines Y */

    printf("  PASS: conditional entropy\n");
}

static void test_kl_divergence(void)
{
    double p[] = {0.5, 0.5};
    double q[] = {0.5, 0.5};
    double kl = alife_kl_divergence(p, q, 2);
    assert(kl < 0.01); /* D_KL(P||P) = 0 */

    printf("  PASS: KL divergence\n");
}

static void test_js_divergence(void)
{
    double p[] = {0.5, 0.5};
    double q[] = {0.9, 0.1};
    double jsd = alife_js_divergence(p, q, 2);
    assert(jsd >= 0.0 && jsd <= 1.0); /* JSD in [0, 1] */

    printf("  PASS: JS divergence\n");
}

static void test_emergence_measure(void)
{
    double e = alife_emergence_measure(0.3, 0.8);
    assert(e > 0.4 && e < 0.6); /* 0.8 - 0.3 = 0.5 */
    printf("  PASS: emergence measure\n");
}

static void test_lempel_ziv(void)
{
    uint8_t data[100];
    for (int i = 0; i < 100; i++) data[i] = (uint8_t)(i & 1);
    double lz = alife_lempel_ziv_complexity(data, 100);
    assert(lz >= 0.0);
    printf("  PASS: Lempel-Ziv complexity\n");
}

static void test_emergence_analysis(void)
{
    int W = 10, H = 10;
    uint8_t *s0 = (uint8_t *)calloc(W * H, 1);
    uint8_t *s1 = (uint8_t *)calloc(W * H, 1);
    uint8_t *s2 = (uint8_t *)calloc(W * H, 1);

    /* Random initial, ordered final */
    for (int i = 0; i < W * H; i++) {
        s0[i] = (uint8_t)(rand() & 1);
        s1[i] = (uint8_t)((i % 3 == 0) ? 1 : 0);
        s2[i] = (uint8_t)((i % 5 == 0) ? 1 : 0);
    }

    const uint8_t *history[] = {s0, s1, s2};
    alife_metrics_config_t cfg;
    cfg.history_length = 3;
    cfg.pattern_radius = 1;
    cfg.n_bootstrap_samples = 10;
    cfg.seed = 42;
    cfg.significance_level = 0.05;

    alife_emergence_t result = alife_analyze_emergence(history, 3, W, H, &cfg);
    assert(result.emergence_score >= 0.0 && result.emergence_score <= 1.0);
    assert(result.complexity_score >= 0.0 && result.complexity_score <= 1.0);

    free(s0); free(s1); free(s2);
    printf("  PASS: emergence analysis\n");
}

static void test_self_organization(void)
{
    int W = 10, H = 10;
    uint8_t *s0 = (uint8_t *)calloc(W * H, 1);
    uint8_t *s1 = (uint8_t *)calloc(W * H, 1);

    for (int i = 0; i < W * H; i++) {
        s0[i] = (uint8_t)(rand() & 1);
        s1[i] = (uint8_t)(s0[i] ? 1 : 0);
    }

    const uint8_t *history[] = {s0, s1};
    double so = alife_self_organization(history, 2, W, H);
    assert(so >= 0.0 && so <= 1.0);

    free(s0); free(s1);
    printf("  PASS: self-organization\n");
}

static void test_effective_complexity(void)
{
    int W = 5, H = 5;
    uint8_t *s0 = (uint8_t *)calloc(W * H, 1);
    uint8_t *s1 = (uint8_t *)calloc(W * H, 1);

    for (int i = 0; i < W * H; i++) {
        s0[i] = (uint8_t)(i & 1);
        s1[i] = s0[i];
    }

    const uint8_t *history[] = {s0, s1};
    double ec = alife_effective_complexity(history, 2, W, H);
    assert(ec >= 0.0);

    free(s0); free(s1);
    printf("  PASS: effective complexity\n");
}

static void test_comprehensive_test(void)
{
    int W = 8, H = 8;
    uint8_t *s0 = (uint8_t *)calloc(W * H, 1);
    uint8_t *s1 = (uint8_t *)calloc(W * H, 1);
    uint8_t *s2 = (uint8_t *)calloc(W * H, 1);

    for (int i = 0; i < W * H; i++) {
        s0[i] = (uint8_t)(rand() & 1);
        s1[i] = (uint8_t)(s0[i] ^ 1);
        s2[i] = (uint8_t)((s0[i] + s1[i]) & 1);
    }

    const uint8_t *history[] = {s0, s1, s2};
    alife_metrics_config_t cfg;
    cfg.history_length = 3;
    cfg.pattern_radius = 1;
    cfg.n_bootstrap_samples = 5;
    cfg.seed = 42;
    cfg.significance_level = 0.05;

    double scores[5];
    alife_comprehensive_test(history, 3, W, H, &cfg, scores);
    for (int i = 0; i < 5; i++) {
        assert(scores[i] >= 0.0 && scores[i] <= 1.0);
    }

    free(s0); free(s1); free(s2);
    printf("  PASS: comprehensive test\n");
}

int main(void)
{
    printf("Test: ALife Metrics\n");
    test_shannon_entropy();
    test_mutual_information();
    test_conditional_entropy();
    test_kl_divergence();
    test_js_divergence();
    test_emergence_measure();
    test_lempel_ziv();
    test_emergence_analysis();
    test_self_organization();
    test_effective_complexity();
    test_comprehensive_test();
    printf("All ALife Metrics tests passed!\n");
    return 0;
}
