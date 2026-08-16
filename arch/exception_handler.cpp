#include "drivers/uart.hpp"

/* 异常路径不碰 printf：printf 内部持有互斥锁（见 lib/printf.cpp），
 * 若异常恰好发生在另一个任务的 printf 临界区内，再调 printf 会拿锁死锁。
 * 这里用裸 UART 输出，任何上下文都安全。 */

/* 输出十六进制数（省略前导零，0 时输出一个 0） */
static void uart_hex(u64 v) {
    uart_puts("0x");
    bool started = false;
    for (int i = 15; i >= 0; i--) {
        int d = (v >> (i * 4)) & 0xF;
        if (d || started || i == 0) {
            started = true;
            uart_putc(d < 10 ? '0' + d : 'a' + d - 10);
        }
    }
}

extern "C" void exception_handler(u64 esr, u64 elr) {
    unsigned int ec = (esr >> 26) & 0x3F;
    unsigned int iss = esr & 0x1FFFFFF;
    uart_puts("EXCEPTION: EC=");
    uart_hex(ec);
    uart_puts(" ISS=");
    uart_hex(iss);
    uart_puts(" ELR=");
    uart_hex(elr);
    uart_puts("\n");
    while (1) { __asm__ volatile("wfi"); }
}
