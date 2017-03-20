#include "token.h"

void token_init(Token *token)
{
	token->symbol = 0;
	token->index = 0;
	token->length = 0;
}

