.section .text.vectors
.align 11            /* 2048字节对齐,保证 ≥4K (2^11=2K, 配合 linker 的 ALIGN 4K 实际等价) */
.global vector_table
vector_table:
    .rept 16
    b .              /* 每项死循环 */
    .space 124       /* 补满 128 字节: b占4, 面124 */
    .endr