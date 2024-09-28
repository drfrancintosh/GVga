#include <malloc.h>

size_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   
   return &__StackLimit  - &__bss_end__;
}

size_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();

   return getTotalHeap() - m.uordblks;
}

void *gmalloc(size_t size) {
    if (getFreeHeap() < size) {
        return NULL;
    }
   return malloc(size);
}

void *gcalloc(unsigned int n, size_t size) {
    if (getFreeHeap() < n * size) {
        return NULL;
    }
   return calloc(n, size);
}

