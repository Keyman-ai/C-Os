#include "drivers/uart.hpp"
#include "lib/printf.hpp"
#include "kernel/gic.hpp"
#include "kernel/timer.hpp"
#include "kernel/mmu.hpp"
#include "kernel/page_alloc.hpp"
#include "drivers/shell.hpp"
#include "kernel/sched.hpp"

/* PSCI SYSTEM_OFF:让 QEMU 关机退出 */
void power_off()
{
    __asm__ __volatile__(
        "movz w0, #0x0008\n"        /* w0 = 0x00000008（写低 16 位，其余清零） */
        "movk w0, #0x8400, lsl #16\n" /* w0 = 0x84000008（写高 16 位，低 16 位保留） */
        "hvc #0\n"                /* 陷入固件执行 */
    );
    for (;;) __asm__ __volatile__("wfi");   /* 兜底:关机失败就停在这,绝不掉进垃圾地址 */
}

/* 线程 A：打印 + 忙等模拟干活 */
void thread_a()
{
    for (;;) {
        printf("A: 线程A在跑\n");
        for (volatile int i = 0; i < 1000000; i++);
    }
}

/* 线程 B：打印 + 忙等模拟干活 */
void thread_b()
{
    for (;;) {
        printf("B: 线程B在跑\n");
        for (volatile int i = 0; i < 1000000; i++);
    }
}

extern "C" void kmain(void) {
    uart_init();
    uart_puts("Hello, aarch64 OS from C++!\n");
    printf("int=%d hex=%x str=%s char=%c\n", -42, 0xCAFEBABE, "hi", 'Q');

    mmu_init();
    page_alloc_init();
    gic_init();
    timer_init();

    sched_init();            /* 清空任务表 */
    task_create(thread_a);   /* 创建线程 A */
    task_create(thread_b);   /* 创建线程 B */
    task_create(shell_run);      /* shell 作为线程参与调度 */
    for (;;) schedule();     /* kmain 交权：第一次调度把自己存进 boot，永不返回 */
}
