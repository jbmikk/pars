#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "radixtree.h"

void symbol_to_buffer(unsigned char *buffer, unsigned int *size, int symbol);
int buffer_to_symbol(unsigned char *buffer, unsigned int size);

#endif //SYMBOLS_H
