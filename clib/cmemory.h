#ifndef CMEMORY_H
#define CMEMORY_H

#include <stdlib.h>

#define c_new(T, N) (T*)c_malloc_n(((unsigned int)sizeof(T))*(N));
#define c_renew(P, T, N) (T*)c_realloc_n(P, ((unsigned int)sizeof(T))*(N));
#define c_delete(P) c_free((void *)P)

void *c_malloc_n(unsigned int n);
void *c_realloc_n(void *ptr, unsigned int n);
void c_free(void *ptr);

#endif // CMEMORY_H
