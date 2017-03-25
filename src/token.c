#include "token.h"

void token_init(Token *token, unsigned int index, unsigned int length, int symbol)
{
	token->index = index;
	token->length = length;
	token->symbol = symbol;
}

