/* stdio.h — minimal stub for arm-none-eabi-gcc (no newlib) */
#ifndef _STDIO_H
#define _STDIO_H
#include <stddef.h>
#include <stdarg.h>

/* printf / sprintf stub — project uses UART directly; these are no-ops */
static inline int printf(const char *fmt, ...)  { (void)fmt; return 0; }
static inline int sprintf(char *s, const char *fmt, ...) { (void)s; (void)fmt; return 0; }
static inline int snprintf(char *s, size_t n, const char *fmt, ...) { (void)s; (void)n; (void)fmt; return 0; }

typedef void FILE;
#define stdout ((FILE*)0)
#define stderr ((FILE*)0)
static inline int fprintf(FILE *f, const char *fmt, ...) { (void)f; (void)fmt; return 0; }
static inline int fputc(int c, FILE *f) { (void)f; return c; }
static inline int fputs(const char *s, FILE *f) { (void)s; (void)f; return 0; }

#endif /* _STDIO_H */
