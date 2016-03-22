#include "arrays.h"

void int_to_array(unsigned char *array, unsigned int *size, int integer)
{
    int remainder = integer;
    int i;

    for (i = 0; i < sizeof(int); i++) {
        array[i] = remainder & 0xFF;
        remainder >>= 8;
        if(remainder == 0)
        {
            i++;
            break;
        }
    }
    *size = i;
}

int array_to_int(unsigned char *array, unsigned int size)
{
	int symbol = 0;
	int i;

	for (i = size-1; i >= 0; i--) {
		symbol <<= 8;
		symbol = symbol | array[i];
	}
	return symbol;
}
