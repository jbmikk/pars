#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

typedef struct _Input {
	FILE *file;
	unsigned char eof;
	unsigned char is_open;
	char *buffer;
	size_t buffer_size;
	unsigned int buffer_index;
} Input;

void input_init_buffer(Input *input, char *data, unsigned int length);
void input_init(Input *input, char *pathname);
void input_dispose(Input *input);
unsigned int input_get_index(Input *input);
void input_set_index(Input *input, unsigned int index);

#endif // INPUT_H
