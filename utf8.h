#ifndef UTF8_H
#define UTF8_H
#include <stddef.h>
#include <stdint.h>

/* 1 if p[0..n) is valid UTF-8, otherwise 0. NULL is allowed only if n==0. */
int utf8_sse2(const uint8_t *p, size_t n);
int utf8_reference(const uint8_t *p, size_t n);
#endif
