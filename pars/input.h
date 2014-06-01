#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

typedef struct _Input {
	FILE *file;
	unsigned char eof;
	unsigned char is_open;
	unsigned char *buffer;
	size_t buffer_size;
	unsigned int buffer_index;
} Input;

Input* input_init_buffer(unsigned char *data, unsigned int length);
Input* input_init(char *pathname);
void input_close(Input *input);
unsigned int input_get_index(Input *input);
void input_set_index(Input *input, unsigned int index);

#endif // INPUT_H
