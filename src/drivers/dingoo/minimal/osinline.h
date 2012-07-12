
#ifndef __OSINLINE__
#define __OSINLINE__

/* What goes herein depends heavily on the OS. */

#define DIRTY_H 256
#define DIRTY_V 100

extern char *dirty_new;
#define osd_mark_vector_dirty(x,y) dirty_new[((y)>>4) * DIRTY_H + ((x)>>4)] = 1
#include "minimal.h"
#define osd_cycles dingoo_timer_read

#define clip_short _clip_short
#define _clip_short(x) { int sign = x >> 31; if (sign != (x >> 15)) x = sign ^ ((1 << 15) - 1); }

#define clip_short_ret _clip_short_ret
INLINE int _clip_short_ret(int x) { _clip_short(x); return x; }

#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
    int result;

    __asm__ __volatile__
    (
     "mult %1,%2 \n" 
     "mfhi %0 \n" 
    : "=r"(result) 
    : "r"(x), "r"(y) 
    );

    return result;
}


#if 0
#define vec_div _vec_div
INLINE int _vec_div(int x, int y)
{
    int result=0x00010000;
    if (y>>12)
    {
	    __asm__ __volatile__
	    (
	     "div %1,%2 \n" 
	     "mflo %0 \n"
	    : "=r"(result) 
	    : "r"(x), "r"(y) 
	    );
    }

    return result;
}
#endif

#endif /* __OSINLINE__ */
