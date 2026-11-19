#include <stdio.h>

#include "casiolab_test.h"

unsigned g_checks = 0;
unsigned g_failures = 0;

/* Per-suite counters: without them a FAIL line cannot be attributed to a
 * suite, and the only number printed is the global total. */
static void run_suite(const char* name, void (*fn)(void)) {
    unsigned checks_before = g_checks;
    unsigned failures_before = g_failures;
    fn();
    printf(
        "%-9s %4u checks, %u failures\n",
        name,
        g_checks - checks_before,
        g_failures - failures_before);
}

int main(void) {
    printf("CasioLab native tests\n");
    printf("---------------------\n");

    run_suite("drsp", test_drsp);
    run_suite("decoders", test_decoders);
    run_suite("export", test_export);
    run_suite("live", test_live);
    run_suite("regress", test_regress);

    printf("---------------------\n");
    printf("%u checks, %u failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
