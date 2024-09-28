#include <malloc.h>

extern size_t getTotalHeap(void);
extern size_t getFreeHeap(void);
extern void *gmalloc(size_t size);
extern void *gcalloc(unsigned int n, size_t size);