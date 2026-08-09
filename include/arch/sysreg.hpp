#pragma once
#include "types.hpp"

/* 读 aarch64 系统寄存器 */
#define READ_SYSREG(reg) ({                        \
    u64 _val;                                       \
    __asm__ __volatile__("mrs %0, " #reg : "=r"(_val)); \
    _val;                                           \
})

/* 写 aarch64 系统寄存器 */
#define WRITE_SYSREG(reg, val)                      \
    __asm__ __volatile__("msr " #reg ", %0" :: "r"((u64)(val)))
