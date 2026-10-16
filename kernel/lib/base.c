#include <airi/base.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

void *airi_memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

void *airi_memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    // 内存地址可能重叠，需要确定方向
    if ((uintptr_t)d <= (uintptr_t)s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i != 0; --i) {
            d[i - 1] = s[i - 1];
        }
    }
}

void *airi_memset(void *dst, int value, size_t n) {
    unsigned char *d = dst;
    for (size_t i = 0; i < n; ++i) {
        d[i] = (unsigned char)value;
    }
    return dst;
}

int airi_memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    for (size_t i = 0; i < n; i++) {
        if (x[i] != y[i]) {
            return (int)x[i] - (int)y[i];
        }
    }
    return 0;
}

size_t airi_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}