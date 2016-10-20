#include "lexer.h"

void lexer_init(Lexer *lexer, Input *input, void (*handler)(Lexer *lexer))
{
	lexer->input = input;
	lexer->index = 0;
	lexer->length = 0;
	lexer->symbol = 0;
	lexer->handler = handler;
}

void lexer_next(Lexer *lexer)
{
	lexer->handler(lexer);
}

void identity_lexer(Lexer *lexer)
{
	LToken token;
	unsigned char c;

#define CURRENT (lexer->input->buffer[lexer->input->buffer_index])
#define NEXT (++lexer->input->buffer_index)
#define END(V) (lexer->input->buffer_index >= lexer->input->buffer_size-V)

next_token:
	lexer->index = lexer->input->buffer_index;

	if (END(0)) {
		token = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = CURRENT;
	token = c;
	NEXT;
eof:
	lexer->symbol = token;
	lexer->length = lexer->input->buffer_index - lexer->index;
}

void ebnf_lexer(Lexer *lexer)
{
	LToken token;
	unsigned char c;

#define CURRENT (lexer->input->buffer[lexer->input->buffer_index])
#define LOOKAHEAD(V) (lexer->input->buffer[lexer->input->buffer_index+(V)])
#define NEXT (++lexer->input->buffer_index)
#define END(V) (lexer->input->buffer_index >= lexer->input->buffer_size-V)

#define BETWEEN(V, A, B) (V >= A && V <= B)
#define IS_SPACE(V) (V == ' ' || V == '\t' || V == '\n')

next_token:
	lexer->index = lexer->input->buffer_index;

	if (END(0)) {
		token = L_EOF;
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
		token = L_WHITE_SPACE;
	} else if (BETWEEN(c, 'a', 'z') || BETWEEN(c, 'A', 'Z')) {
		while(1) {
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(!BETWEEN(c, 'a', 'z') && !BETWEEN(c, 'A', 'Z') && !BETWEEN(c, '0', '9'))
				break;
		}
		token = L_IDENTIFIER;
	} else if (BETWEEN(c, '0', '9')) {
		while(1) {
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(!BETWEEN(c, '0', '9'))
				break;
		}
		token = L_INTEGER;
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
		token = L_TERMINAL_STRING;
	} else if (c == '(' && !END(1) && LOOKAHEAD(1) == '*') {
		while(1) {
			unsigned char prev;
			prev = c;
			NEXT;
			if(END(0))
				break;
			c = CURRENT;
			if(c == ')' && prev == '*') {
				printf("ENDOFCOMMENNT\n");
				NEXT;
				break;
			}
		}
		token = L_COMMENT;
	} else {
		token = c;
		NEXT;
	}

	if(token == L_WHITE_SPACE || token == L_COMMENT)
		goto next_token;

eof:
	lexer->symbol = token;
	lexer->length = lexer->input->buffer_index - lexer->index;
}

