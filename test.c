#define _DEFAULT_SOURCE
#include "utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static size_t checks;
static void verify(const uint8_t *p, size_t n, int expected) {
    int ref = utf8_reference(p, n), got = utf8_sse2(p, n);
    ++checks;
    if (ref == got && (expected < 0 || got == expected)) return;
    fprintf(stderr, "check %zu: len=%zu ref=%d asm=%d expected=%d:",
            checks, n, ref, got, expected);
    for (size_t i = 0; i < n && i < 32; ++i) fprintf(stderr, " %02x", (unsigned)p[i]);
    fputc('\n', stderr);
    exit(1);
}

static size_t encode(uint8_t *p, uint32_t v) {
    if (v < 0x80) { p[0] = (uint8_t)v; return 1; }
    const size_t n = v < 0x800 ? 2 : v < 0x10000 ? 3 : 4;
    for (size_t i = n - 1; i; --i, v >>= 6) p[i] = (uint8_t)(0x80 | (v & 0x3f));
    p[0] = (uint8_t)((n == 2 ? 0xc0 : n == 3 ? 0xe0 : 0xf0) | v);
    return n;
}

static uint32_t prng = 0x83bd91f5;
static uint32_t next(void) {
    prng ^= prng << 13; prng ^= prng >> 17; prng ^= prng << 5;
    return prng;
}

static void boundaries(void) {
    const long page_raw = sysconf(_SC_PAGESIZE);
    if (page_raw < 128) { fputs("unsupported page size\n", stderr); exit(1); }
    const size_t page = (size_t)page_raw;
    uint8_t *mem = mmap(NULL, page * 3, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED || mprotect(mem + page, page, PROT_READ | PROT_WRITE)) {
        perror("guard pages"); exit(1);
    }
    uint8_t *start = mem + page, *end = start + page;
    for (size_t n = 0; n <= 80; ++n) {
        uint8_t *p = end - n;
        memset(p, 'x', n); verify(p, n, 1);
        memset(start, 'b', n); verify(start, n, 1);
        if (n) { p[n - 1] = 0xf0; verify(p, n, 0); }
        if (n >= 2) { p[n - 2] = 0xe2; p[n - 1] = 0x82; verify(p, n, 0); }
        if (n >= 3) { p[n - 3] = 0xf0; p[n - 2] = 0x90; p[n - 1] = 0x80; verify(p, n, 0); }
        if (n >= 4) { encode(p + n - 4, 0x10ffff); verify(p, n, 1); }
    }
    if (munmap(mem, page * 3)) { perror("munmap"); exit(1); }
}

int main(void) {
    uint8_t buf[512];
    verify(NULL, 0, 1);
    for (unsigned a = 0; a <= 255; ++a) {
        buf[0] = (uint8_t)a;
        verify(buf, 1, a < 0x80);
        for (unsigned b = 0; b <= 255; ++b) {
            buf[1] = (uint8_t)b;
            verify(buf, 2, (a < 0x80 && b < 0x80) ||
                   (a >= 0xc2 && a <= 0xdf && b >= 0x80 && b <= 0xbf));
        }
    }
    for (uint32_t u = 0; u <= 0x10ffff; ++u) {
        size_t off = u % 32;
        memset(buf, 's', sizeof(buf));
        size_t n = encode(buf + off, u);
        verify(buf, off + n + 19, !(u >= 0xd800 && u <= 0xdfff));
        for (size_t cut = 1; cut < n; ++cut) verify(buf, off + cut, 0);
    }
    for (size_t i = 0; i < 20000; ++i) {
        size_t n = next() % sizeof(buf);
        for (size_t j = 0; j < n; ++j) buf[j] = (uint8_t)next();
        verify(buf, n, -1);
        n = 0;
        for (size_t j = 0; j < 120; ++j) {
            uint32_t u = next() % 0x110000;
            if (u >= 0xd800 && u <= 0xdfff) u = ' ';
            n += encode(buf + n, u);
        }
        verify(buf, n, 1);
        buf[next() % n] = (uint8_t)next();
        verify(buf, n, -1);
    }
    static const uint8_t invalid[][4] = {
        {0xe0,0x9f,0xbf,'a'}, {0xed,0xa0,0x80,'a'},
        {0xf0,0x8f,0xbf,0xbf}, {0xf4,0x90,0x80,0x80}, {0xf5,0x80,0x80,0x80},
        {0xe2,0xa0,'x','a'}, {0xf0,0x90,0x80,'x'}
    };
    for (size_t off = 0; off < 32; ++off)
        for (size_t j = 0; j < sizeof(invalid) / sizeof(invalid[0]); ++j) {
            memset(buf, 'a', sizeof(buf));
            memcpy(buf + off, invalid[j], 4); verify(buf, off + 24, 0);
        }
    boundaries();
    printf("%zu independent expected/differential checks passed (including guard pages)\n", checks);
    return 0;
}
