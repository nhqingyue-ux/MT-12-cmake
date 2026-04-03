/* stdlib.h — minimal stub for arm-none-eabi-gcc (no newlib) */
#ifndef _STDLIB_H
#define _STDLIB_H
#include <stddef.h>
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

/* malloc/free stubs — project uses static allocation, not heap */
static inline void *malloc(size_t n)  { (void)n; return (void*)0; }
static inline void  free(void *p)     { (void)p; }

/* abs — guard against redefinition with other headers */
#if !defined(__abs_defined) && !defined(abs)
#define __abs_defined
static inline int abs(int x) { return x < 0 ? -x : x; }
#endif

void _exit(int status);

#endif /* _STDLIB_H */

/* rand/srand — simple LCG, good enough for project use */
extern unsigned int __rand_seed;
static inline void srand(unsigned int seed) { __rand_seed = seed; }
static inline int  rand(void) {
    __rand_seed = __rand_seed * 1103515245u + 12345u;
    return (int)((__rand_seed >> 16) & 0x7FFF);
}
