#ifndef DINGOO_WRAPPER
#define DINGOO_WRAPPER

/*
 * Define SCREENPRINTF to redirect messages to Dingoo's LCD screen
 * instead of STDOUT
 */
#define SCREENPRINTF

#ifdef SCREENPRINTF
#define printf dingoo_printf
extern void dingoo_printf(char* fmt, ...);
#endif

#define FAST_C_MEM

#ifdef FAST_C_MEM
/* Use faster memcpy/memset routines */
extern void* fast_memcpy(void *OUT, const void *IN, size_t N);
#define memcpy fast_memcpy
extern void* fast_memset(const void *DST, int C, size_t LENGTH);
#define memset fast_memset
#endif

extern void* memcpy32(int *OUT, int *IN, int N);

#ifdef MALLOC_DEBUG
extern void * debug_malloc(unsigned int, const char *);
extern void debug_free(void *, const char *);
#define malloc(x) debug_malloc(x, __FUNCTION__)
#define free(x) debug_free(x, __FUNCTION__)
#endif

#endif
