#pragma once

/* 字符串工具库 —— 不依赖 libc 的手写实现 */

int           strlen(const char* s);
int           strcmp(const char* a, const char* b);
int           strncmp(const char* a, const char* b, int n);
void*         memset(void* dst, int c, int n);
void*         memcpy(void* dst, const void* src, int n);
