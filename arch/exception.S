.section .text.vectors
.align 11            /* 2048字节对齐,保证 ≥4K (2^11=2K, 配合 linker 的 ALIGN 4K 实际等价) */
.global vector_table
vector_table:
    /* 0x000: Sync, current EL with SP0 */
      b exception_common
      .space 124
      /* 0x080: IRQ, current EL with SP0 */
      b exception_common
      .space 124
      /* 0x100: FIQ, current EL with SP0 */
      b exception_common
      .space 124
      /* 0x180: SError, current EL with SP0 */
      b exception_common
      .space 124
      /* 0x200: Sync, current EL with SPx */
      b exception_common
      .space 124
      /* 0x280: IRQ, current EL with SPx  ← 定时器走这里 */
      b irq_common
      .space 124
      /* 0x300: FIQ, current EL with SPx */
      b exception_common
      .space 124
      /* 0x380: SError, current EL with SPx */
      b exception_common
      .space 124
      /* 0x400-0x780: lower EL entries */
      .rept 8
      b exception_common
      .space 124
      .endr

.section .text
exception_common:
    sub sp, sp, #272
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]

    mrs x1, ELR_EL1
    mrs x0, ESR_EL1
    mrs x2, SPSR_EL1
    stp x1, x2, [sp, #240]    /* ELR + SPSR */
    str x0,      [sp, #256]    /* ESR */
    bl exception_handler
    /* handler 里死循环,不会返回 */

irq_common:
    /* 保存通用寄存器 x0-x29 */
    sub sp, sp, #272
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]

    /* 保存 ELR + SPSR + ESR */
    mrs x21, ESR_EL1
    mrs x22, ELR_EL1
    mrs x23, SPSR_EL1
    stp x22, x23, [sp, #240]
    str x21,      [sp, #256]
    str x30,      [sp, #264]    /* 保存 LR */

    /* 调用 C irq_handler */
    bl irq_handler

    /* 恢复 ELR 和 SPSR（eret 需要） */
    ldp x22, x23, [sp, #240]
    msr ELR_EL1,  x22
    msr SPSR_EL1, x23

    /* 恢复通用寄存器 x0-x29 */
    ldp x0,  x1,  [sp, #0]
    ldp x2,  x3,  [sp, #16]
    ldp x4,  x5,  [sp, #32]
    ldp x6,  x7,  [sp, #48]
    ldp x8,  x9,  [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]

    ldr x30,      [sp, #264]    /* 恢复 LR */
    add sp, sp, #272
    eret
