#include "atl_lexer.h"

void atl_lexer(Lexer *lexer, Token *token)
{
	ATLSymbol symbol;
	unsigned int index;
	unsigned char c;

#define CURRENT (lexer->input->buffer[lexer->input->buffer_index])
#define LOOKAHEAD(V) (lexer->input->buffer[lexer->input->buffer_index+(V)])
#define NEXT (++lexer->input->buffer_index)
#define END(V) (lexer->input->buffer_index >= lexer->input->buffer_size-V)

#define BETWEEN(V, A, B) (V >= A && V <= B)
#define IS_SPACE(V) (V == ' ' || V == '\t' || V == '\n' || V == '\r' || V =='\f')

next_token:
	index = lexer->input->buffer_index;

	if (END(0)) {
		symbol = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = CURRENT;
	if (IS_SPACE(c)) {
		while(1) {
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(!IS_SPACE(c))
				break;
		}
		symbol = ATL_WHITE_SPACE;
	} else if (BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z')) {
		while(1) {
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(!BETWEEN(c, 'a', 'z') && !BETWEEN(c, 'A', 'Z') && !BETWEEN(c, '0', '9'))
				break;
		}
		symbol = ATL_IDENTIFIER;
	} else if (BETWEEN(c, '0', '9')) {
		while(1) {
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(!BETWEEN(c, '0', '9'))
				break;
		}
		symbol = ATL_INTEGER;
	} else if (c == '"') {
		while(1) {
			unsigned char prev;
			prev = c;
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(c == '"' && prev != '\\') {
				NEXT;
				break;
			}
		}
		symbol = ATL_STRING;
	} else if (c == '\'') {
		while(1) {
			unsigned char prev;
			prev = c;
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(c == '\'' && prev != '\\') {
				NEXT;
				break;
			}
		}
		symbol = ATL_STRING;
	} else if (c == '/' && !END(1) && LOOKAHEAD(1) == '*') {
		while(1) {
			unsigned char prev;
			prev = c;
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(c == '/' && prev == '*') {
				NEXT;
				break;
			}
		}
		symbol = ATL_COMMENT;
	} else {
		symbol = c;
		NEXT;
	}

	if(symbol == ATL_WHITE_SPACE || symbol == ATL_COMMENT)
		goto next_token;

eof:
	token->symbol = symbol;
	token->index = index;
	token->length = lexer->input->buffer_index - index;
}

