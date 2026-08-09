#include "drivers/shell.hpp"
#include "drivers/uart.hpp"
#include "lib/printf.hpp"
#include "kernel/page_alloc.hpp"
#include "lib/string.hpp"
#include "kernel/sched.hpp"      /* schedule()：没输入时交权 */

static char buf[128];
static int  pos = 0;

static void prompt() {
    uart_puts("> ");
}

static void exec_cmd() {
    buf[pos] = '\0';

    if (buf[0] == '\0') {
        /* 空行 */
        return;
    }

    if (strcmp(buf, "help") == 0) {
        uart_puts("help alloc echo clear\r\n");
        return;
    }

    if (strcmp(buf, "alloc") == 0) {
        void* p = alloc_page();
        if (p) {
            printf("alloc: %p\n", p);
        } else {
            uart_puts("out of memory\r\n");
        }
        return;
    }

    if (strncmp(buf, "echo ", 5) == 0) {
        uart_puts(buf + 5);
        uart_puts("\r\n");
        return;
    }

    if (strcmp(buf, "clear") == 0) {
        uart_puts("\r\n\r\n\r\n");
        return;
    }

    printf("unknown: %s\n", buf);
}

void shell_run() {
    uart_puts("shell ready\r\n");
    prompt();

    while (1) {
        int c = uart_getc_nonblock();
        if (c < 0) {
            schedule();          /* 没输入：交权，让 A/B 跑 */
            continue;
        }
        if (c == 0x03) power_off();   /* Ctrl+C：关机 */

        if (c == '\r') {
            uart_puts("\r\n");
            exec_cmd();
            pos = 0;
            prompt();
        } else if (c == 0x7F || c == '\b') {
            if (pos > 0) {
                uart_puts("\b \b");
                pos--;
            }
        } else if (c >= ' ' && pos < 127) {
            uart_putc(c);
            buf[pos++] = c;
        }
    }
}
