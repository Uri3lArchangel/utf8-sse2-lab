#include "utf8.h"

int utf8_reference(const uint8_t *p, size_t n)
{
    while (n) {
        uint32_t value = *p++, minimum;
        size_t tail;
        --n;
        if (value < 0x80) continue;
        if ((value & 0xe0) == 0xc0) {
            tail = 1; value &= 0x1f; minimum = 0x80;
        } else if ((value & 0xf0) == 0xe0) {
            tail = 2; value &= 0x0f; minimum = 0x800;
        } else if ((value & 0xf8) == 0xf0) {
            tail = 3; value &= 7; minimum = 0x10000;
        } else {
            return 0;
        }
        if (n < tail) return 0;
        n -= tail;
        while (tail--) {
            const uint32_t b = *p++;
            if ((b & 0xc0) != 0x80) return 0;
            value = (value << 6) | (b & 0x3f);
        }
        if (value < minimum || value > 0x10ffff ||
            (value >= 0xd800 && value <= 0xdfff)) return 0;
    }
    return 1;
}
