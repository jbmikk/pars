#ifndef SYMBOLS_H
#define SYMBOLS_H

void symbol_to_buffer(unsigned char *buffer, unsigned int *size, int symbol);
int buffer_to_symbol(unsigned char *buffer, unsigned int size);

#endif //SYMBOLS_H
