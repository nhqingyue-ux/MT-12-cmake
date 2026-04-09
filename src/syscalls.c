/**
 * syscalls.c — newlib stubs for arm-none-eabi-gcc (brew, no bundled newlib)
 *
 * Provides minimal _sbrk / _write / _read / _close / _fstat / _isatty / _lseek
 * and C runtime init stubs required by the toolchain.
 */

#include <stdint.h>
#include <stddef.h>

/* Minimal stat struct for _fstat stub */
struct stat { int st_mode; };
#define S_IFCHR 0020000

/* Heap end provided by linker script */
extern uint8_t _end;
extern uint8_t _estack;

/* ─── _sbrk ──────────────────────────────────────────────────────────────── */
void *_sbrk(intptr_t incr)
{
    static uint8_t *heap_end = NULL;
    uint8_t *prev_heap_end;

    if (heap_end == NULL)
        heap_end = &_end;

    prev_heap_end = heap_end;

    if ((heap_end + incr) > &_estack)
        return (void *)-1;   /* out of memory */

    heap_end += incr;
    return (void *)prev_heap_end;
}

/* ─── minimal stdio stubs ─────────────────────────────────────────────────── */
/* Forward printf output to UART4 (PA0 TX, 460800 baud) */
#include "stm32f4xx_usart.h"
int _write(int fd, const char *buf, int len) {
    (void)fd;
    for (int i = 0; i < len; i++) {
        while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
        USART_SendData(UART4, (uint8_t)buf[i]);
    }
    return len;
}
int _read(int fd, char *buf, int len)        { (void)fd; (void)buf; (void)len; return -1; }
int _close(int fd)                           { (void)fd; return -1; }
int _fstat(int fd, struct stat *st)          { (void)fd; if(st) st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd)                          { (void)fd; return 1; }
long _lseek(int fd, long off, int whence)    { (void)fd; (void)off; (void)whence; return -1; }
void _exit(int status)                       { (void)status; while(1); }
int _kill(int pid, int sig)                  { (void)pid; (void)sig; return -1; }
int _getpid(void)                            { return 1; }

/* ─── C++ / init_array stubs ─────────────────────────────────────────────── */
void __libc_init_array(void) {}

/* ─── memset / memcpy (safety — in case LTO strips builtins) ─────────────── */
void *memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t *)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s2 = (const uint8_t *)src;
    while (n--) *d++ = *s2++;
    return dst;
}
