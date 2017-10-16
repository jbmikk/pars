#include "token.h"

void token_init(
	Token *token,
	unsigned int index,
	unsigned int length,
	int symbol,
	unsigned int meta_index
) {
	token->index = index;
	token->length = length;
	token->symbol = symbol;
	token->meta_index = meta_index;
}

