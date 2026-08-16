#include "lib/printf.hpp"
#include "drivers/uart.hpp"
#include "kernel/sem.hpp"

/* INT64 取反会溢出,这里用 u64 直接表示其绝对值 */
static constexpr u64 INT64_MIN_ABS = (u64)1 << 63;

/* 打印互斥锁：内置在 printf 里，调用者无需手动加锁。
 * 用 C 风格聚合初始化（count=1 即互斥锁），避免依赖 C++ 静态构造。
 * 前提：printf 只在任务上下文被调用（异常/中断路径用裸 UART，见 exception_handler.cpp）。 */
static Semaphore print_lock = {1, 0, 0};

static void print_unsigned(u64 num, u32 base, bool upper) {
    char buf[32];  // 用于存储转换后的数字字符
    int i = 0;
    // 使用do-while循环处理数字，包括0的情况
    do {
        u64 digit = num % base;
        if (digit < 10) {
            buf[i++] = '0' + digit;
        } else {
            // 对于16进制等，使用A-F或a-f
            if (upper) {
                buf[i++] = 'A' + digit - 10;
            } else {
                buf[i++] = 'a' + digit - 10;
            }
        }
        num /= base;
    } while (num > 0);

    // 反向输出字符数组
    while (--i >= 0) {
        uart_putc(buf[i]);
    }
}

static void print_signed(s64 num, u32 base) {
    if (num < 0) {
        uart_putc('-');
        // 处理特殊情况：INT64_MIN，避免取反溢出
        if ((u64)num == INT64_MIN_ABS) {
            print_unsigned(INT64_MIN_ABS, base, false);
        } else {
            num = -num;
            print_unsigned((u64)num, base, false);
        }
    } else {
        print_unsigned((u64)num, base, false);
    }
}

void printf(const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);

    sem_wait(&print_lock);   /* 拿锁：保证整段输出不被其他任务打断 */

    while (*fmt) {
        if (*fmt != '%') {
            uart_putc(*fmt++);
            continue;
        }
        /* 跳过 '%' */
        fmt++;
        switch (*fmt) {
            case 'd': case 'i': {
                int val = __builtin_va_arg(ap, int);
                print_signed(val, 10);
                break;
            }
            case 'u': {
                unsigned int val = __builtin_va_arg(ap, unsigned int);
                print_unsigned(val, 10, false);
                break;
            }
            case 'x': {
                unsigned int val = __builtin_va_arg(ap, unsigned int);
                print_unsigned(val, 16, false);
                break;
            }
            case 'X': {
                unsigned int val = __builtin_va_arg(ap, unsigned int);
                print_unsigned(val, 16, true);
                break;
            }
            case 'c': {
                int val = __builtin_va_arg(ap, int);
                uart_putc((char)val);
                break;
            }
            case 's': {
                const char* s = __builtin_va_arg(ap, const char*);
                while (*s) uart_putc(*s++);
                break;
            }
            case 'p': {
                void* p = __builtin_va_arg(ap, void*);
                uart_puts("0x");
                print_unsigned((u64)p, 16, false);
                break;
            }
            case '%': {
                uart_putc('%');
                break;
            }
            default: {
                /* 未知格式,原样输出 % + 字符 */
                uart_putc('%');
                uart_putc(*fmt);
                break;
            }
        }
        fmt++;
    }

    __builtin_va_end(ap);

    sem_post(&print_lock);   /* 放锁 */
}