
#include "bcode.h"
#include "cmemory.h"

#include <string.h>

// Rule declaration functions

#define CHUNK_SIZE 16;

void bcode_init(Bcode *bcode)
{
	bcode->frame = NULL;
	bcode->size = 0;
	bcode->assigned = 0;
}

void bcode_dispose(Bcode *bcode)
{
	c_delete(bcode->frame);
}

void _add_data(Bcode *bcode, void *data, unsigned int size)
{
	int available = bcode->size - bcode->assigned;
	if(available < size) {
		bcode->size += CHUNK_SIZE;
		bcode->frame = c_realloc_n(bcode->frame, bcode->size);
	}
	memcpy(bcode->frame + bcode->assigned, data, size);
	bcode->assigned += size;
}

void bcode_add_instruction(Bcode *bcode, Instruction *instruction)
{
	_add_data(bcode, &instruction, sizeof(Instruction));
}

void bcode_add_int(Bcode *bcode, int integer)
{
	_add_data(bcode, &integer, sizeof(int));
}

Instruction *bcode_get_instruction(Bcode *bcode, unsigned int index)
{
	return (Instruction*)(bcode->frame + index);
}
