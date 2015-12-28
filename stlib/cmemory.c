#include "cmemory.h"

void *c_malloc_n(unsigned int n)
{
	void *ptr;
	if(n > 0) {
		ptr = (void *) malloc(n);
		if (ptr == NULL)
			exit(EXIT_FAILURE);
	} else {
		ptr = NULL;
	}
	return ptr;
}

void *c_realloc_n(void *ptr, unsigned int n)
{
	ptr = (void *) realloc(ptr, n);
	if(n > 0 && !ptr) {
		exit(EXIT_FAILURE);
	}
	return ptr;
}

void c_free(void *ptr)
{
	free(ptr);
}

