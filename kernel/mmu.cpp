#include "kernel/mmu.hpp"
#include "arch/sysreg.hpp"

// Level 1 页表：512 项，每项 8 字节 = 4KB，必须 4KB 对齐
static u64 __attribute__((aligned(4096))) l1_table[512];
// Level 2 页表：512 项，每项 8 字节 = 4KB
static u64 __attribute__((aligned(4096))) l2_table[512];

void mmu_init()
{
    // MAIR_EL1: Attr0=0xFF(Normal), Attr1=0x04(Device)
    u64 mair = (0xFFUL << 0) | (0x04UL << 8);
    WRITE_SYSREG(MAIR_EL1, mair);

    // L2: 0x00000000 → Normal (Attr0)
    l2_table[0] = (0ULL << 21)
                | (1 << 10)     // AF
                | (2 << 8)      // inner shareable
                | (1 << 5)      // NS
                | (0 << 2)      // Attr0 = Normal
                | (1 << 0);     // valid block
    // L2: 0x08000000 (GIC) → Device (Attr1)
    l2_table[64] = (0x08000000ULL << 0)
                 | (1 << 10)
                 | (2 << 8)
                 | (1 << 5)
                 | (1 << 2)      // Attr1 = Device
                 | (1 << 0);
    // L2: 0x09000000 (UART) → Device (Attr1)
    l2_table[72] = (0x09000000ULL << 0)
                 | (1 << 10)
                 | (2 << 8)
                 | (1 << 5)
                 | (1 << 2)
                 | (1 << 0);
    // L1 Entry 0 → points to l2_table
    l1_table[0] = (u64)&l2_table | 3;  // bit[1:0] = 0b11 (table + valid)
    // L1 Entry 1 → 1GB block at 0x40000000 (kernel), Normal
    l1_table[1] = (1ULL << 30)       // PA[47:30] = 1 → 0x40000000
                | (1 << 10)           // AF
                | (2 << 8)            // inner shareable
                | (1 << 5)            // NS
                | (0 << 2)            // Attr0 = Normal
                | (1 << 0);           // valid block
    // 设置页表基地址
    WRITE_SYSREG(TTBR0_EL1, (u64)&l1_table);
    __asm__ __volatile__("isb");
    // TCR_EL1: 4KB granule, 39-bit VA (T0SZ=25)
    u64 tcr = (25 << 0)      // T0SZ = 25
            | (0 << 14)      // TG0 = 4KB
            | (2 << 12)      // inner shareable
            | (1 << 10)      // outer cacheable
            | (1 << 8);      // inner cacheable
    WRITE_SYSREG(TCR_EL1, tcr);
    __asm__ __volatile__("isb");
    // 启用 MMU: 读、设 M bit、回写
    u64 sctlr = READ_SYSREG(SCTLR_EL1);
    sctlr |= 1;                  // bit0 = M, 打开 MMU
    WRITE_SYSREG(SCTLR_EL1, sctlr);
    __asm__ __volatile__("isb");
}