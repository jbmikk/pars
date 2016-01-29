#include "symbols.h"

void symbol_to_buffer(unsigned char *buffer, unsigned int *size, int symbol)
{
    int remainder = symbol;
    int i;

    for (i = 0; i < sizeof(int); i++) {
        buffer[i] = remainder & 0xFF;
        remainder >>= 8;
        if(remainder == 0)
        {
            i++;
            break;
        }
    }
    *size = i;
}

int buffer_to_symbol(unsigned char *buffer, unsigned int size)
{
	int symbol = 0;
	int i;

	for (i = 0; i < size; i++) {
		symbol <<= 8;
		symbol = symbol | buffer[i];
	}
	return symbol;
}

void *radix_tree_get_int(Node *tree, int number)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, number);
	return radix_tree_get(tree, buffer, size);
}

void radix_tree_set_int(Node *tree, int number, void *data)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, number);
	radix_tree_set(tree, buffer, size, data);
}

void *radix_tree_get_next_int(Node *tree, int number)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, number);
	return radix_tree_get_next(tree, buffer, size);
}


