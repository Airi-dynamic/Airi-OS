#include <airi/base.h>

// 暴露系统调用
void *memcpy(void *d, const void *s, size_t n) { return airi_memcpy(d, s, n); }
void *memmove(void *d, const void *s, size_t n) { return airi_memmove(d, s, n); }
void *memset(void *d, int c, size_t n) { return airi_memset(d, c, n); }
int memcmp(const void *a, const void *b, size_t n) { return airi_memcmp(a, b, n); }