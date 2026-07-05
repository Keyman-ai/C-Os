.section .text.vectors
.align 11            /* 2048字节对齐,保证 ≥4K (2^11=2K, 配合 linker 的 ALIGN 4K 实际等价) */
.global vector_table
vector_table:
    .rept 16
    b exception_common
    .space 124       /* 补满 128 字节: b占4, 面124 */
    .endr

.section .text
exception_common:
    sub sp, sp, #272  /* 将栈指针（sp）减去 272，实际上是在栈上分配 272 字节的空间 */
    stp x0,  x1,  [sp, #0]  /* 将寄存器 x0 和 x1 的值存储到由栈指针 sp 指向的内存位置。具体来说，它将 x0 存储到 sp+0 的位置，将 x1 存储到 sp+8 的位置（因为每个寄存器值占 8 字节） */
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

    /* 先取 ELR 给 x1,再取 ESR 给 x0,顺序不能反 */
    mrs x1, ELR_EL1
    mrs x0, ESR_EL1
    mrs x2, SPSR_EL1
    stp x1, x2, [sp, #240]    /* ELR + SPSR */
    str x0,      [sp, #256]    /* ESR */
    bl exception_handler
    /* handler 里死循环,不会返回 */