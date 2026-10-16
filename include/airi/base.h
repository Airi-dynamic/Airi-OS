#ifndef AIRI_BASE_H
#define AIRI_BASE_H
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

void *airi_memcpy(void *dst, const void *src, size_t n);
void *airi_memmove(void *dst, const void *src, size_t n);
void *airi_memset(void *dst, int value, size_t n);
int airi_memcmp(const void *a, const void *b, size_t n);
size_t airi_strlen(const char *s);
#endif
