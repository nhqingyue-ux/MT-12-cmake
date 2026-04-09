#ifndef __DELAY_H__
#define __DELAY_H__

// GCC -O1 removes bare for(w=0;w<N;w++) delay loops; Keil preserves them.
// Keil's for-loop = ~4 cycles/iter (add+cmp+bcc).  Match that here so every
// DELAY_CYCLES(N) in the codebase produces the same wall-time as Keil.
// nop+nop+subs+bne = 4 cycles/iter, ~24ns/iter @168MHz.
#define DELAY_CYCLES(n) do { unsigned _c = (n); \
    __asm volatile("1: nop; nop; subs %0,%0,#1; bne 1b" \
                   : "+r"(_c) : : "cc"); } while(0)

#endif
