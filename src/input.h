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
char input_get_current(Input *input);
char input_lookahead(Input *input, int i);
void input_next(Input *input);
int input_end(Input *input, int i);

#endif // INPUT_H
