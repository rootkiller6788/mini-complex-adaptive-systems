/**
 * @file    test_sfi_emergence.c
 * @brief   Tests for emergence detection algorithms
 */

#include "../include/sfi_emergence.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

#define TEST_PASS() printf("  PASS: %s\n", __func__)

void test_mutual_information_zero(void) {
    double micro[4] = {1.0, 2.0, 3.0, 4.0};
    double macro[4] = {5.0, 6.0, 7.0, 8.0};
    double mi = sfi_mutual_information_micro_macro(micro, 1, 4, macro, 1);
    assert(mi >= 0.0);
    TEST_PASS();
}

void test_mutual_information_null(void) {
    assert(sfi_mutual_information_micro_macro(NULL, 1, 4, (double[]){0}, 1) == 0.0);
    double micro[4] = {0};
    assert(sfi_mutual_information_micro_macro(micro, 0, 4, micro, 1) == 0.0);
    assert(sfi_mutual_information_micro_macro(micro, 1, 1, micro, 1) == 0.0);
    TEST_PASS();
}

void test_transfer_entropy(void) {
    /* Two coupled signals: dst = src delayed by 1 */
    double src[100], dst[100];
    for (int i = 0; i < 100; i++) {
        src[i] = sin((double)i * 0.1);
        dst[i] = (i > 0) ? src[i - 1] : 0.0;
    }

    double te = sfi_transfer_entropy(src, dst, 100, 2, 1);
    assert(te >= 0.0);
    assert(sfi_transfer_entropy(NULL, dst, 100, 2, 1) == 0.0);
    assert(sfi_transfer_entropy(src, dst, 5, 2, 1) == 0.0);

    TEST_PASS();
}

void test_fit_power_law(void) {
    /* Generate approximate power-law data */
    double data[100];
    for (int i = 0; i < 100; i++) {
        data[i] = 1.0 / ((double)i * 0.01 + 0.1) + 1.0;
    }

    double alpha, xmin, ks;
    int rc = sfi_fit_power_law(data, 100, &alpha, &xmin, &ks);
    assert(rc == 0);
    assert(alpha > 1.0);
    assert(xmin > 0.0);
    assert(ks >= 0.0 && ks <= 1.0);

    assert(sfi_fit_power_law(NULL, 5, &alpha, &xmin, &ks) == -1);
    assert(sfi_fit_power_law(data, 5, &alpha, &xmin, &ks) == -1);

    TEST_PASS();
}

void test_hurst_exponent(void) {
    /* Random walk ? H ? 0.5 */
    double rw[256];
    rw[0] = 0.0;
    for (int i = 1; i < 256; i++) {
        rw[i] = rw[i - 1] + ((double)(i * 1234567 % 1000) / 1000.0 - 0.5);
    }

    double H = sfi_hurst_exponent_rs(rw, 256);
    assert(H >= 0.0 && H <= 1.0);

    assert(sfi_hurst_exponent_rs(NULL, 10) == 0.5);
    assert(sfi_hurst_exponent_rs(rw, 4) == 0.5);

    TEST_PASS();
}

void test_gini_coefficient(void) {
    double equal[5] = {1.0, 1.0, 1.0, 1.0, 1.0};
    double g = sfi_gini_coefficient(equal, 5);
    assert(fabs(g) < 0.001);

    double unequal[5] = {0.0, 0.0, 0.0, 0.0, 10.0};
    g = sfi_gini_coefficient(unequal, 5);
    assert(g > 0.5);

    assert(sfi_gini_coefficient(NULL, 1) == 0.0);

    TEST_PASS();
}

void test_multiscale_entropy(void) {
    double series[200];
    for (int i = 0; i < 200; i++)
        series[i] = sin((double)i * 0.05) + 0.5 * cos((double)i * 0.13);

    double mse = sfi_multiscale_entropy(series, 200, 5, 0.2);
    assert(mse >= 0.0);

    assert(sfi_multiscale_entropy(NULL, 10, 5, 0.2) == 0.0);
    assert(sfi_multiscale_entropy(series, 10, 5, 0.2) == 0.0);

    TEST_PASS();
}

void test_detect_emergence(void) {
    double micro[50], macro[50];
    /* Micro determines macro with noise */
    for (int i = 0; i < 50; i++) {
        micro[i] = sin((double)i * 0.2);
        macro[i] = micro[i] * 2.0 + ((double)(i % 7) / 10.0 - 0.35);
    }

    sfi_emergence_detector_t result;
    sfi_detect_emergence(micro, 1, macro, 1, 50, &result);
    assert(result.emergence_strength >= 0.0 && result.emergence_strength <= 1.0);
    /* Mutual information should be positive (micro determines macro) */
    assert(result.mutual_info_micro_macro >= 0.0);

    sfi_detect_emergence(NULL, 1, macro, 1, 50, &result);
    assert(result.detection == SFI_EMERGE_NOMINAL);

    TEST_PASS();
}

int main(void) {
    printf("=== SFI Emergence Tests ===\n");
    test_mutual_information_zero();
    test_mutual_information_null();
    test_transfer_entropy();
    test_fit_power_law();
    test_hurst_exponent();
    test_gini_coefficient();
    test_multiscale_entropy();
    test_detect_emergence();
    printf("\nAll emergence tests passed!\n");
    return 0;
}
