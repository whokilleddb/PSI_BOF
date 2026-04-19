#include "psi.h"

// clang's loop-idiom pass folds byte loops into calls to strlen / memcpy /
// memset, which Beacon's loader cannot resolve. Provide local implementations
// and tag them optnone so the compiler doesn't recursively rewrite their
// bodies into calls to themselves.
extern "C" {
    __attribute__((optnone))
    size_t strlen(const char *s) {
        const char *p = s;
        while (*p) ++p;
        return (size_t)(p - s);
    }

    __attribute__((optnone))
    void *memcpy(void *dst, const void *src, size_t n) {
        unsigned char       *d = (unsigned char *)dst;
        const unsigned char *s = (const unsigned char *)src;
        while (n--) *d++ = *s++;
        return dst;
    }

    __attribute__((optnone))
    void *memset(void *dst, int v, size_t n) {
        unsigned char *d = (unsigned char *)dst;
        while (n--) *d++ = (unsigned char)v;
        return dst;
    }
}
