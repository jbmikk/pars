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
    unsigned int current;
} LInput;

typedef enum {
    L_WHITE_SPACE = 256,
    L_INTEGER,
    L_IDENTIFIER,
    L_TERMINAL_STRING,
    L_DEFINING_SYMBOL,
    L_CONCATENATE_SYMBOL,
    L_TERMINATOR_SYMBOL,
    L_DEFINITION_SEPARATOR_SYMBOL
} LToken;

LInput* lexer_input_init(char *pathname);
LToken lexer_input_next(LInput *input);

#endif //LEXER_H
