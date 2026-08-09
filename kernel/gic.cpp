#include "types.hpp"
#include "arch/sysreg.hpp"
#include "kernel/gic.hpp"

/* GICv3 MMIO 基地址 (QEMU virt machine) */
constexpr u64 GICD_BASE = 0x08000000;  /* Distributor */
constexpr u64 GICR_BASE = 0x080A0000;  /* Redistributor (CPU0) */
constexpr u64 GICR_SGI_BASE = 0x080B0000;

static inline void mmio_write32(u64 addr, u32 val) {
    *reinterpret_cast<volatile u32*>(addr) = val;
}
static inline u32 mmio_read32(u64 addr) {
    return *reinterpret_cast<volatile u32*>(addr);
}

void gic_init()
{
    u32 val;
    /* 1. 开启 ARE_NS */
    val = mmio_read32(GICD_BASE + 0x0000);
    val |= (1 << 4);
    mmio_write32(GICD_BASE + 0x0000, val);
    while (mmio_read32(GICD_BASE + 0x0000) & (1 << 31))
        ;

    /* 2. 使能 Group 0 + Group 1 */
    val = mmio_read32(GICD_BASE + 0x0000);
    val |= (1 << 3);
    val |= (1 << 2);
    val |= (1 << 1);
    mmio_write32(GICD_BASE + 0x0000, val);
    while (mmio_read32(GICD_BASE + 0x0000) & (1 << 31))
        ;

    /* 3. 唤醒 CPU0 的 redistributor */
    val = mmio_read32(GICR_BASE + 0x0014);
    val &= ~(1 << 1);
    mmio_write32(GICR_BASE + 0x0014, val);
    while (mmio_read32(GICR_BASE + 0x0014) & (1 << 2))
        ;

    /* 4. PPI 30 设为 Group 1 Non-secure */
    mmio_write32(GICR_SGI_BASE + 0x0080, 1 << 30);

    /* 4.5. 使能 PPI 30 */
    mmio_write32(GICR_SGI_BASE + 0x0100, 1 << 30);

    /* 5. CPU Interface —— 启用系统寄存器接口 */
    val = READ_SYSREG(ICC_SRE_EL1);
    val |= 1;
    WRITE_SYSREG(ICC_SRE_EL1, val);
    __asm__ __volatile__("isb");

    /* 6. 设优先级掩码: 0xFF = 最低, 允许所有优先级 */
    WRITE_SYSREG(ICC_PMR_EL1, 0xFF);

    /* 7. 使能 Group 1 中断 */
    WRITE_SYSREG(ICC_IGRPEN1_EL1, 1);
    __asm__ __volatile__("isb");
}
