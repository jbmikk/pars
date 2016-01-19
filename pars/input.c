#include "input.h"
#include "cmemory.h"

void input_init_buffer(Input *input, unsigned char *data, unsigned int length)
{
	input->file = NULL;
	input->eof = 0;
	input->buffer_index = 0;
	input->buffer_size = length;
	input->buffer = data;
}

void input_init(Input *input, char *pathname)
{
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
		//TODO:close file
		input_init_buffer(input, buffer, length);
		input->file = file;
		input->is_open = 1;
	} else {
		input_init_buffer(input, NULL, 0);
		input->file = NULL;
		input->is_open = 0;
	}
}

void input_dispose(Input *input)
{
	if(input->file != NULL) {
		c_delete(input->buffer);
		input->buffer = NULL;
		fclose(input->file);
	}
}

unsigned int input_get_index(Input *input)
{
	return input->buffer_index;
}

void input_set_index(Input *input, unsigned int index)
{
	input->buffer_index = index;
}
