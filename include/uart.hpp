#pragma once

/* 自己定义类型,不用 <cstdint> */
typedef unsigned char  u8;
typedef unsigned int   u32;
typedef unsigned long  u64;

/* 初始化 PL011 */
void uart_init();

/* 发送一个字节,轮询 TXFF */
void uart_putc(char c);

/* 发送字符串 */
void uart_puts(const char* s);