#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "radixtree.h"

void symbol_to_buffer(unsigned char *buffer, unsigned int *size, int symbol);
int buffer_to_symbol(unsigned char *buffer, unsigned int size);

//Radix tree extensions
//TODO: Maybe remove this extensios and create buffer structs and macros
// This way we don't need this extensions and can pass a struct as a key
// for radix tree.
void *radix_tree_get_int(Node *tree, int number);
void *radix_tree_get_next_int(Node *tree, int number);
void radix_tree_set_int(Node *tree, int number, void *data);

#endif //SYMBOLS_H
