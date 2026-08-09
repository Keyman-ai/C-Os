#include "lib/string.hpp"

int strlen(const char* s)
{
    int n = 0;
    while (*s++) n++;
    return n;
}

int strcmp(const char* a, const char* b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, int n)
{
    while (n-- > 0 && *a && *a == *b) {
        a++;
        b++;
    }
    if (n < 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

void* memset(void* dst, int c, int n)
{
    unsigned char* p = (unsigned char*)dst;
    while (n-- > 0) *p++ = (unsigned char)c;
    return dst;
}

void* memcpy(void* dst, const void* src, int n)
{
    unsigned char*       d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (n-- > 0) *d++ = *s++;
    return dst;
}
