#include "cmemory.h"

cpointer c_malloc_n(csize n)
{
    cpointer ptr;
    if(n > 0)
    {
        ptr = (cpointer) malloc(n);
        if (ptr == NULL)
            exit(EXIT_FAILURE);
    }
    else ptr = NULL;
    return ptr;
}

cpointer c_realloc_n(cpointer ptr, csize n)
{
    if(n > 0)
    {
        ptr = (cpointer) realloc(ptr, n);
        if (ptr == NULL)
            exit(EXIT_FAILURE);
    }
    else ptr = NULL;
    return ptr;
}

void c_free(cpointer ptr)
{
    free(ptr);
}

