#ifndef CMEMORY_H
#define CMEMORY_H

#include <stdlib.h>
#include "ctypes.h"

#define c_new(T, N) (T*)c_malloc_n(((csize)sizeof(T))*(N));
#define c_renew(P, T, N) (T*)c_realloc_n(P, ((csize)sizeof(T))*(N));
#define c_delete(P) c_free((cpointer)P)

cpointer c_malloc_n(csize n);
cpointer c_realloc_n(cpointer ptr, csize n);
void c_free(cpointer ptr);

#endif // CMEMORY_H
