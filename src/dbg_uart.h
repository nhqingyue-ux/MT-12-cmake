#ifndef __DBG_UART_H__
#define __DBG_UART_H__

#include "stm32f4xx_usart.h"

/* Tiny debug helpers — write directly to UART4, no printf/newlib needed */

static inline void dbg_putc(char c) {
    while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
    USART_SendData(UART4, (uint8_t)c);
}

static inline void dbg_puts(const char *s) {
    while (*s) dbg_putc(*s++);
}

static inline void dbg_putu(unsigned int v) {
    char buf[11];
    int i = 0;
    if (v == 0) { dbg_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) dbg_putc(buf[i]);
}

static inline void dbg_puti(int v) {
    if (v < 0) { dbg_putc('-'); v = -v; }
    dbg_putu((unsigned int)v);
}

#endif
