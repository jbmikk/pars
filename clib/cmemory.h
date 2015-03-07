#ifndef CMEMORY_H
#define CMEMORY_H

#include <stdlib.h>

#define csize unsigned int

#define c_new(T, N) (T*)c_malloc_n(((csize)sizeof(T))*(N));
#define c_renew(P, T, N) (T*)c_realloc_n(P, ((csize)sizeof(T))*(N));
#define c_delete(P) c_free((void *)P)

void *c_malloc_n(csize n);
void *c_realloc_n(void *ptr, csize n);
void c_free(void *ptr);

#endif // CMEMORY_H
