#pragma once
#include "uart.hpp"   /* 复用 u32/u64 等类型 */

/* 有符号 64 位,与 u64 对称 */
typedef long long s64;

void printf(const char* fmt, ...);