#include "atl_lexer.h"

void atl_lexer(Lexer *lexer, Token *token)
{
	ATLSymbol symbol;
	unsigned int index;
	unsigned char c;

#define BETWEEN(V, A, B) (V >= A && V <= B)
#define IS_SPACE(V) (V == ' ' || V == '\t' || V == '\n' || V == '\r' || V =='\f')

next_token:
	index = lexer->input->buffer_index;

	if (input_end(lexer->input, 0)) {
		symbol = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = input_get_current(lexer->input);
	if (IS_SPACE(c)) {
		while(1) {
			input_next(lexer->input);
			if(input_end(lexer->input, 0))
				break;
			c = input_get_current(lexer->input);
			if(!IS_SPACE(c))
				break;
		}
		symbol = ATL_WHITE_SPACE;
	} else if (BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z')) {
		while(1) {
			input_next(lexer->input);
			if(input_end(lexer->input, 0))
				break;
			c = input_get_current(lexer->input);
			if(!BETWEEN(c, 'a', 'z') && !BETWEEN(c, 'A', 'Z') && !BETWEEN(c, '0', '9'))
				break;
		}
		symbol = ATL_IDENTIFIER;
	} else if (BETWEEN(c, '0', '9')) {
		while(1) {
			input_next(lexer->input);
			if(input_end(lexer->input, 0))
				break;
			c = input_get_current(lexer->input);
			if(!BETWEEN(c, '0', '9'))
				break;
		}
		symbol = ATL_INTEGER;
	} else if (c == '"') {
		while(1) {
			unsigned char prev;
			prev = c;
			input_next(lexer->input);
			if(input_end(lexer->input, 0))
				break;
			c = input_get_current(lexer->input);
			if(c == '"' && prev != '\\') {
				input_next(lexer->input);
				break;
			}
		}
		symbol = ATL_STRING;
	} else if (c == '\'') {
		while(1) {
			unsigned char prev;
			prev = c;
			input_next(lexer->input);
			if(input_end(lexer->input, 0))
				break;
			c = input_get_current(lexer->input);
			if(c == '\'' && prev != '\\') {
				input_next(lexer->input);
				break;
			}
		}
		symbol = ATL_STRING;
	} else if (c == '/' && !input_end(lexer->input, 1) && input_lookahead(lexer->input, 1) == '*') {
		while(1) {
			unsigned char prev;
			prev = c;
			input_next(lexer->input);
			if(input_end(lexer->input, 0))
				break;
			c = input_get_current(lexer->input);
			if(c == '/' && prev == '*') {
				input_next(lexer->input);
				break;
			}
		}
		symbol = ATL_COMMENT;
	} else {
		symbol = c;
		input_next(lexer->input);
	}

	if(symbol == ATL_WHITE_SPACE || symbol == ATL_COMMENT)
		goto next_token;

eof:
	token->symbol = symbol;
	token->index = index;
	token->length = lexer->input->buffer_index - index;
}

