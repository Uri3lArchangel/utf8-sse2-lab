#define _POSIX_C_SOURCE 200809L
#include "utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { RUNS = 7, REPEAT = 48, LEN = 1 << 20 };
typedef int (*validate)(const uint8_t *, size_t);
static volatile unsigned sink;
static double now(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) { perror("clock_gettime"); exit(1); }
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}
static double sample(validate f, const uint8_t *p) {
    double start = now();
    unsigned accepted = 0;
    for (unsigned i = 0; i < REPEAT; ++i) accepted += (unsigned)f(p, LEN);
    double elapsed = now() - start;
    sink = accepted;
    if (accepted != REPEAT || elapsed <= 0) exit(1);
    return (double)REPEAT * LEN / (elapsed * 1073741824.0);
}
static int order(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static void report(const char *name, const uint8_t *p) {
    double ref[RUNS], simd[RUNS];
    if (!utf8_reference(p, LEN) || !utf8_sse2(p, LEN)) exit(1);
    for (unsigned i = 0; i < RUNS; ++i) {
        if (i % 2) { ref[i] = sample(utf8_reference,p); simd[i] = sample(utf8_sse2,p); }
        else { simd[i] = sample(utf8_sse2,p); ref[i] = sample(utf8_reference,p); }
    }
    qsort(ref, RUNS, sizeof(*ref), order); qsort(simd, RUNS, sizeof(*simd), order);
    printf("%-22s %9.2f %9.2f %8.2fx\n", name, ref[RUNS/2], simd[RUNS/2], simd[RUNS/2]/ref[RUNS/2]);
}
int main(void) {
    uint8_t *p = malloc(LEN);
    if (!p) return 1;
    printf("Median of %d x %d calls on a reused %d MiB buffer (GiB/s)\n", RUNS, REPEAT, LEN >> 20);
    printf("%-22s %9s %9s %9s\n", "workload", "C scalar", "SSE2", "ratio");
    memset(p, 'A', LEN); report("ASCII", p);
    for (size_t i = 0; i < LEN; i += 64) memcpy(p+i+60, "\xf0\x9f\x8c\x8d", 4);
    report("mixed (1 scalar/64B)", p);
    for (size_t i = 0; i < LEN; i += 4) memcpy(p+i, "\xf0\x9f\x8c\x8d", 4);
    report("all 4-byte", p);
    free(p);
    return 0;
}
