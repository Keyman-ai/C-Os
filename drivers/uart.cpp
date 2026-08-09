#include "drivers/uart.hpp"

/* PL011 MMIO 基地址 */
constexpr u32 UART_BASE = 0x09000000;

/* 寄存器偏移 */
constexpr u32 UARTDR_OFFSET    = 0x00;
constexpr u32 UARTFR_OFFSET    = 0x18;
constexpr u32 UARTLCR_H_OFFSET = 0x2C;
constexpr u32 UARTCR_OFFSET    = 0x30;

/* 用 volatile 指针访问 MMIO */
static inline volatile u32* reg(u32 offset) {
    return reinterpret_cast<volatile u32*>(UART_BASE + offset);
}

void uart_init() {
    /* TODO 1: 禁用 UART:写 UARTCR = 0 */
    *reg(UARTCR_OFFSET) = 0;
    /* TODO 2: 配置 LCR_H:8N1 无 FIFO,值 0x60 */
    *reg(UARTLCR_H_OFFSET) = 0x60;
    /* TODO 3: 使能:UARTCR = 0x301 (UARTEN + TXE + RXE) */
    *reg(UARTCR_OFFSET) = 0x301;
}

void uart_putc(char c) {
    /* '\n' 先补一个 '\r',再发 '\n' */
    if (c == '\n') {
        uart_putc('\r');
    }
    /* 轮询 TXFF,然后写 UARTDR */
    while (*reg(UARTFR_OFFSET) & (1 << 5))
        ;
    *reg(UARTDR_OFFSET) = c;
}

void uart_puts(const char* s) {
    /* TODO 7: 循环调用 uart_putc 直到 '\0' */
    while (*s)
        uart_putc(*s++);
}

char uart_getc() {
    /* 轮询 RXFE (bit 4), 等它为 0 = 有数据 */
    while (*reg(UARTFR_OFFSET) & (1 << 4))
        ;
    return (char)*reg(UARTDR_OFFSET);
}

int uart_getc_nonblock() {
    /* RXFE(bit 4) = 1 表示接收 FIFO 空:没有输入 */
    if (*reg(UARTFR_OFFSET) & (1 << 4)) {
        return -1;
    }
    return *reg(UARTDR_OFFSET) & 0xFF;
}