/* math.h — minimal stub for arm-none-eabi-gcc (no newlib) */
#ifndef _MATH_H
#define _MATH_H
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* GCC builtins — work without newlib */
#define fabsf(x)   __builtin_fabsf(x)
#define fabs(x)    __builtin_fabs(x)
#define sqrtf(x)   __builtin_sqrtf(x)
#define sqrt(x)    __builtin_sqrt(x)
#define floorf(x)  __builtin_floorf(x)
#define ceilf(x)   __builtin_ceilf(x)
#define roundf(x)  __builtin_roundf(x)

#if !defined(__abs_defined) && !defined(abs)
#define __abs_defined
static inline int abs(int x) { return x < 0 ? -x : x; }
static inline long labs(long x) { return x < 0 ? -x : x; }
#endif

#endif /* _MATH_H */
