#include <airi/base.h>
#include <limits.h>
#include <stdarg.h>
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
    return dst;
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

// 一次字符串格式化的输出结果
struct output {
    char *dst;
    size_t cap, count; // 数组总字节数 cap 和已产生字符数量 count
};

// 输出一个 char 到结果，可能被截断，但会被记录
static void emit(struct output *o, char c) {
    if (o->cap && o->count < o->cap - 1) {
        o->dst[o->count] = c;
    }
    ++o->count;
}

// 输出一个数字到格式化结果。接受输出位置，待格式化值，进制，预期获得的字符数量
static void number(struct output *o, uint64_t value, unsigned radix, unsigned width) {
    char digits[64]; // 临时保存数字格式化结果
    unsigned n = 0;  // 已经保存了几个数位
    // 循环转化数字
    do {
        digits[n++] = "0123456789abcdef"[value % radix];
        value /= radix;
    } while (value);
    // 达不到指定宽度则补前导零
    while (width > n) {
        emit(o, '0');
        width--;
    }
    // 从最高位开始填写字符
    while (n) {
        emit(o, digits[--n]);
    }
}

int airi_vsnprintf(char *dst, size_t cap, const char *fmt, va_list args) {
    struct output o = {dst, cap, 0};
    while (*fmt) {
        // 普通字符不变，下一个
        if (*fmt != '%') {
            emit(&o, *fmt++);
            continue;
        }

        // 否则，有%，需要开始格式化解析
        ++fmt;
        // 处理 NUL
        if (!*fmt) {
            emit(&o, '%');
            break;
        }
        // 格式化打印特性：% [填充] [宽度] [参数类型] [输出形式]
        // %08（用零补齐，结果占几个字符）
        unsigned width = 0;
        if (*fmt == '0') {
            ++fmt;
            while (*fmt >= '0' && *fmt <= '9') {
                width = width < 64 ? width * 10 + (unsigned)(*fmt - '0') : 64;
                if (width > 64) {
                    width = 64;
                }
                ++fmt;
            }
        }
        // 格式化打印特性：接下来从格式化列表读取的参数，在数据结构上是多长（如 %08ll）
        // 局部枚举，记录格式化字符串要求将输入参数以解析参数列表
        enum { NORMAL, LONG, LLONG, SIZE } length = NORMAL;
        if (*fmt == 'l') {
            length = LONG;
            ++fmt;
            if (*fmt == 'l') {
                length = LLONG;
                ++fmt;
            }
        } else if (*fmt == 'z') {
            // size_t 类型，对于我们的平台就是 unsigned long
            length = SIZE;
            ++fmt;
        }
        if (!*fmt) {
            emit(&o, '%');
            break;
        }
        // 格式化打印特性：要打印出去的字符串，语义是什么类型
        // 读取格式化参数末尾的字符,也就是传入类型
        char spec = *fmt++;
        if (spec == '%') {
            // 百分号转义
            emit(&o, '%');
        } else if (spec == 's') {
            // 字符串输出
            const char *s = va_arg(args, const char *);
            if (!s) {
                s = "(null)";
            }
            while (*s) {
                emit(&o, *s++);
            }
        } else if (spec == 'c') {
            // 字符输出
            emit(&o, (char)va_arg(args, int));
        } else if (spec == 'p') {
            // 指针输出
            emit(&o, '0');
            emit(&o, 'x');
            number(&o, (uintptr_t)va_arg(args, void *), 16, width);
        } else if (spec == 'd' || spec == 'u' || spec == 'x') {
            uint64_t value;
            if (spec == 'd') { // 有符号十进制数字
                int64_t signed_value = length == LLONG  ? va_arg(args, long long)
                                       : length == LONG ? va_arg(args, long)
                                       : length == SIZE ? va_arg(args, long)
                                                        : va_arg(args, int);
                value = (uint64_t)signed_value;
                if (signed_value < 0) {
                    emit(&o, '-'); // 负号不计入填充
                    value = 0 - value;
                }
            } else { // 无符号十进制数 u 或者十六进制 x
                value = length == LLONG  ? va_arg(args, unsigned long long)
                        : length == LONG ? va_arg(args, unsigned long)
                        : length == SIZE ? va_arg(args, size_t)
                                         : va_arg(args, unsigned);
            }
            //
            number(&o, value, spec == 'x' ? 16 : 10, width);
        } else {
            emit(&o, '%');
            emit(&o, spec);
        }
    }
    if (cap) {
        dst[o.count < cap ? o.count : cap - 1] = '\0';
    }

    return o.count > INT_MAX ? INT_MAX : (int)o.count;
}

int airi_snprintf(char *dst, size_t cap, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = airi_vsnprintf(dst, cap, fmt, args);
    va_end(args);
    return count;
}