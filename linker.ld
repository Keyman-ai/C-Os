OUTPUT_FORMAT(elf64-littleaarch64)   /* 输出格式 */
OUTPUT_ARCH(aarch64)
ENTRY(_start)                        /* 入口 */

SECTIONS
{
    . = 0x40080000;                  /* 起始地址 */

    . = ALIGN(16);
    .text : {
        *(.text.boot)                /* boot.S 的代码优先 */
        . = ALIGN(4096);             /* 4K 对齐, VBAR 要求 */
        *(.text.vectors)             /* 向量表 */
        *(.text*)                    /* 其他 .text */
    }

    . = ALIGN(16);
    .rodata : {
        *(.rodata*)
    }

    . = ALIGN(16);
    .data : {
        *(.data*)
    }

    . = ALIGN(16);
    .bss : {
        __bss_start = .;             /* 给 boot.S 用 */
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;               /* 给 boot.S 用 */
    }

    . = ALIGN(16);
    . = . + 0x4000;                  /* 16KB 栈 */
    __stack_top = .;                 /* 给 boot.S 用 */

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.eh_frame_hdr)
    }
}