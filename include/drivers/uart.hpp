#pragma once
#include "types.hpp"

/* 初始化 PL011 */
void uart_init();

/* 发送一个字节,轮询 TXFF */
void uart_putc(char c);

/* 发送字符串 */
void uart_puts(const char* s);

char uart_getc();

/* 非阻塞读:有字符返回字符,没字符返回 -1 */
int uart_getc_nonblock();