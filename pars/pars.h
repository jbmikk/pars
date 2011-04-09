#ifndef PARS_H
#define PARS_H

#include <stddef.h>
#include <stdio.h>

#define BUFFER_SIZE 4096

typedef struct _PGrammar {
    int placeholder;
} PGrammar;

PGrammar *pars_load_grammar(char *pathname);

#endif //PARS_H
