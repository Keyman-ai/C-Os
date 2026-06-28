#include "uart.hpp"
#include "printf.hpp"

/* TODO 1: 声明 kmain 为 extern "C" */
/* extern "C" void kmain(void) { ... } */

extern "C" void kmain(void) {
    /* TODO 2: 初始化 UART */
    uart_init();

    /* TODO 3: 打印 Hello */
    uart_puts("Hello, aarch64 OS from C++!\n");
    printf("int=%d hex=%x str=%s char=%c\n", -42, 0xCAFEBABE, "hi", 'Q');

    /* TODO 4: PSCI SYSTEM_OFF 关机 */
    __asm__ __volatile__(
        "movz w0, #0x0008\n"        /* w0 = 0x00000008 */
        "movk w0, #0x8400, lsl #16\n" /* w0 = 0x84000008 */
        "hvc #0"
        ::: "memory"
    );

    /* TODO 5: 兜底,万一 HVC 没关机 */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}