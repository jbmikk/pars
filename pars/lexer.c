#include "lexer.h"
#include "cmemory.h"

LInput* lexer_input_init_buffer(unsigned char *grammar, unsigned int length)
{
    LInput *input;

    input = c_new(LInput,1);
    input->file = NULL;
    input->eof = 0;
    input->buffer_index = 0;
    input->buffer_size = length;
    input->buffer = grammar;
    return input;
}

LInput* lexer_input_init(char *pathname)
{
    LInput *input = NULL;
    FILE *file;
    unsigned char *buffer;
    unsigned int length;

    file = fopen(pathname, "rb");

    if(file)
    {
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer = c_new(unsigned char, length);
        length = fread(buffer, 1, length, file);
        input = lexer_input_init_buffer(buffer, length);
        input->file = file;
    }
    return input;
}

void lexer_input_close(LInput *input)
{
    if(input->file != NULL)
        c_delete(input->buffer);
    c_delete(input);
}

LToken lexer_input_next(LInput *input)
{
    LToken token;
    unsigned char c;

    #define CURRENT (input->buffer[input->buffer_index])
    #define NEXT (input->buffer[++input->buffer_index])
    #define END (input->buffer_index >= input->buffer_size-1)

    #define BETWEEN(V, A, B) (V >= A && V <= B)
    #define IS_SPACE(V) (V == ' ' || V == '\t' || V == '\n')

next_token:

    c = CURRENT;

    if (IS_SPACE(c)) {
        while(!END)
        {
            c = NEXT;
            if(!IS_SPACE(c))
                break;
        }
        token = L_WHITE_SPACE;
    }
    else if (BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z')) {
        while(!END)
        {
            c = NEXT;
            if(!BETWEEN(c, 'a', 'z') && !BETWEEN(c, 'A', 'Z') && !BETWEEN(c, '0', '9'))
                break;
        }
        token = L_IDENTIFIER;
    }
    else if (BETWEEN(c, '0', '9')) {
        while(!END)
        {
            c = NEXT;
            if(!BETWEEN(c, '0', '9'))
                break;
        }
        token = L_INTEGER;
    }
    else if (c == '"') {
        while(!END) {
            unsigned char prev;
            prev = c;
            c = NEXT;
            if(c == '"' && prev != '\\') {
                NEXT;
                break;
            }
        }
        token = L_TERMINAL_STRING;
    }
    else {
        token = c;
        if(!END) {
            NEXT;
        }
    }

    if(token == L_WHITE_SPACE)
        goto next_token;

    return  token;
}

