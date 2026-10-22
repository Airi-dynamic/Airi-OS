#ifndef AIRI_BASE_H
#define AIRI_BASE_H
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// 内存字节操作函数
void *airi_memcpy(void *dst, const void *src, size_t n);
void *airi_memmove(void *dst, const void *src, size_t n);
void *airi_memset(void *dst, int value, size_t n);
int airi_memcmp(const void *a, const void *b, size_t n);
size_t airi_strlen(const char *s);

// 字符串格式化函数
int airi_vsnprintf(char *dst, size_t cap, const char *fmt, va_list args);
int airi_snprintf(char *dst, size_t cap, const char *fmt, ...);
#endif
