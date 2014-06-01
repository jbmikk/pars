#include "input.h"
#include "cmemory.h"

Input* input_init_buffer(unsigned char *data, unsigned int length)
{
	Input *input;

	input = c_new(Input,1);
	input->file = NULL;
	input->eof = 0;
	input->buffer_index = 0;
	input->buffer_size = length;
	input->buffer = data;
	return input;
}

Input* input_init(char *pathname)
{
	Input *input = NULL;
	FILE *file;
	unsigned char *buffer;
	unsigned int length;

	file = fopen(pathname, "rb");

	if(file)
	{
		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);
		if(length > 0) {
			buffer = c_new(unsigned char, length);
			length = fread(buffer, 1, length, file);
		} else {
			buffer = NULL;
		}
		input = input_init_buffer(buffer, length);
		input->file = file;
		input->is_open = 1;
	}
	return input;
}

void input_close(Input *input)
{
	if(input->file != NULL)
		c_delete(input->buffer);
	c_delete(input);
}

unsigned int input_get_index(Input *input)
{
	return input->buffer_index;
}

void input_set_index(Input *input, unsigned int index)
{
	input->buffer_index = index;
}
