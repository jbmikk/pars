#ifndef ATL_LEXER_H
#define ATL_LEXER_H

#include "lexer.h"

typedef enum {
	ATL_START_BLOCK_SYMBOL = '{',
	ATL_END_BLOCK_SYMBOL = '}',
	ATL_WHITE_SPACE = 256,
	ATL_COMMENT,
	ATL_INTEGER,
	ATL_IDENTIFIER,
	ATL_STRING
} ATLSymbol;

void atl_lexer(Lexer *lexer, Token *t_in, Token *t_out);

#endif //ATL_LEXER_H
