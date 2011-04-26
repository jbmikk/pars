#ifndef CMEMORY_H
#define CMEMORY_H

#include <stdlib.h>
#include "ctypes.h"

#define c_new(T, N) (T*)c_malloc_n(((csize)sizeof(T))*(N));
#define c_delete(P) c_free((cpointer)P)

cpointer c_malloc_n(csize n);
void c_free(cpointer ptr);

#endif // CMEMORY_H
