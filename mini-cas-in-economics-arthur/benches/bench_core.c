#include "ace_core.h"
#include "ace_dynamics.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    unsigned int seed = (unsigned int)time(NULL);
    int n_iter = 100000;
    clock_t start, end;
    printf("=== Performance Benchmarks ===

");
    start = clock();
    double sum = 0.0;
    for (int i = 0; i < n_iter; i++) {
        sum += ace_arthur_urn_function(ace_random_uniform(&seed), 2.0);
    }
    end = clock();
    printf("Arthur urn (x%d): %.3f ms
", n_iter,
           1000.0 * (double)(end - start) / CLOCKS_PER_SEC);
    start = clock();
    for (int i = 0; i < n_iter; i++) {
        sum += ace_shannon_entropy(
            (double[]){ace_random_uniform(&seed), ace_random_uniform(&seed)}, 2);
    }
    end = clock();
    printf("Shannon entropy (x%d): %.3f ms
", n_iter,
           1000.0 * (double)(end - start) / CLOCKS_PER_SEC);
    int init[] = {1,1,1};
    ACEPolyaUrn* u = ace_polya_urn_alloc(3, init, 1.0);
    start = clock();
    for (int i = 0; i < 10000; i++) ace_polya_urn_draw(u, &seed);
    end = clock();
    printf("Polya urn x10000 draws: %.3f ms
",
           1000.0 * (double)(end - start) / CLOCKS_PER_SEC);
    ace_polya_urn_free(u);
    printf("
(accumulator=%.6f to prevent optimization)
", sum);
    return 0;
}
