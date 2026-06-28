.section .text.boot
.global _start

_start:
    /* TODO 1: 清零 .bss */
    /*   加载 __bss_start 到 x0 */
    ldr x0, =__bss_start      /* x0 = bss 起始地址 */
    /*   加载 __bss_ned   到 x1 */
    ldr x1, =__bss_end        /* x1 = bss 结束地址 */
    /*   循环: cmp x0,x1; b.ge 2f; str xzr,[x0],#8; b 1b; 2: */
1:  cmp x0, x1                /* 比较 x0 和 x1 */
    b.ge 2f                   /* x0 >= x1 就跳出循环,去 2: */
    str xzr, [x0], #8         /* 把 0 写到 x0 指向的地址,x0 再 += 8 */
    b 1b                      /* 回到 1: 继续循环 */
    /* TODO 2: 设置 sp */
    /*   ldr x0, =__stack_top; mov sp, x0 */
2:  ldr x0, =__stack_top      /* 加载栈顶地址到 x0 */
    mov sp, x0                /* sp = x0 */
    ldr x0, =vector_table
    msr VBAR_EL1, x0          /* 设置中断向量表，msr 是 "Move to System Register" 的缩写，用于将数据写入系统寄存器 */
    isb                        /* 指令同步屏障,确保后续指令看到新 VBAR */
    /* TODO 3: 跳到 kmain */
    /*   bl kmain */
    bl kmain                  /* 跳转到 kmain,返回地址存 x30 */
    /* TODO 4: 兜底 */
    /*   3: wfi; b 3b */
3:  wfi                       /* 等中断(实际上当 halt 用) */
    b 3b                      /* 回到 3: 死循环 */