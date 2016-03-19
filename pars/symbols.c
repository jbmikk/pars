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

	for (i = size-1; i >= 0; i--) {
		symbol <<= 8;
		symbol = symbol | buffer[i];
	}
	return symbol;
}
