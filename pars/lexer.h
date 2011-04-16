#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

typedef struct _LInput {
    FILE *file;
    unsigned char eof;
    unsigned char is_open;
    unsigned char *buffer;
    size_t buffer_size;
    unsigned int buffer_index;
} LInput;

typedef enum {
    L_EOF = 0,
    L_DEFINING_SYMBOL = '=',
    L_CONCATENATE_SYMBOL = ',',
    L_TERMINATOR_SYMBOL = ';',
    L_DEFINITION_SEPARATOR_SYMBOL = '|',
    L_START_GROUP_SYMBOL = '(',
    L_END_GROUP_SYMBOL = ')',
    L_WHITE_SPACE = 256,
    L_INTEGER,
    L_IDENTIFIER,
    L_TERMINAL_STRING
} LToken;

LInput* lexer_input_init_buffer(unsigned char *grammar, unsigned int length);
LInput* lexer_input_init(char *pathname);
void lexer_input_close(LInput *input);
unsigned int lexer_input_get_index(LInput *input);
void lexer_input_set_index(LInput *input, unsigned int index);
LToken lexer_input_next(LInput *input);

#endif //LEXER_H
