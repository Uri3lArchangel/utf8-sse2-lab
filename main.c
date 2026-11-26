#include "utf8.h"
#include <stdio.h>
#include <stdlib.h>

/* Whole-file validation: do not apply a non-streaming validator per chunk. */
int main(int argc, char **argv) {
    if (argc != 2) { fputs("usage: utf8check FILE\n", stderr); return 2; }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { perror(argv[1]); return 2; }
    uint8_t chunk[16384], *data = NULL;
    size_t n = 0, cap = 0;
    int code = 2;
    for (;;) {
        size_t got = fread(chunk, 1, sizeof(chunk), fp);
        if (got > SIZE_MAX - n) { fputs("input size overflow\n", stderr); goto done; }
        if (n + got > cap) {
            size_t grown = cap && cap <= SIZE_MAX / 2 ? cap * 2 : n + got;
            if (grown < n + got) grown = n + got;
            void *next = realloc(data, grown);
            if (!next) { perror("realloc"); goto done; }
            data = next; cap = grown;
        }
        for (size_t i = 0; i < got; ++i) data[n + i] = chunk[i];
        n += got;
        if (got < sizeof(chunk)) {
            if (ferror(fp)) { perror("read"); goto done; }
            break;
        }
    }
    const int valid = utf8_sse2(data, n);
    puts(valid ? "valid UTF-8" : "invalid UTF-8");
    code = valid ? 0 : 1;
done:
    fclose(fp);
    free(data);
    return code;
}
