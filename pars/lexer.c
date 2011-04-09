#include "lexer.h"
#include "cmemory.h"

void lexer_input_fill(LInput *input, unsigned int size)
{
    input->buffer_size = fread(input->buffer, 1, size, input->file);
    input->buffer_index = 0;
}

LInput* lexer_input_init(char *pathname)
{
    LInput *input;
    FILE *file;
    unsigned int file_size;

    input = c_new(LInput,1);

    file = fopen(pathname, "rb");
    input->file = file;
    input->eof = 0;

    if(file)
    {
        input->is_open = 1;
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        input->buffer = c_new(unsigned char, file_size);
        lexer_input_fill(input, file_size);
    }
    return input;
}

LToken lexer_input_next(LInput *input)
{
    LToken token;
    unsigned char c;

    #define NEXT (input->buffer[input->buffer_index++])
    #define END (input->buffer_index >= input->buffer_size)

    #define BETWEEN(V, A, B) (V >= A && V <= B)
    #define IS_SPACE(V) (V == ' ' || V == '\t' || V == '\n')

    c = input->current;

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
    else if (BETWEEN(c, '0', '9'))
    {
        while(!END)
        {
            c = NEXT;
            if(!BETWEEN(c, '0', '9'))
                break;
        }
        token = L_INTEGER;
    }
    else if (c == '"')
    {
        while(!END)
        {
            unsigned char prev;
            prev = c;
            c = NEXT;
            if(c == '"' && prev != '\\')
                break;
        }
        token = L_TERMINAL_STRING;
    }
    input->current = c;
    return  token;
}

