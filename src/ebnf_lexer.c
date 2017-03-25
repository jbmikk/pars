#include "ebnf_lexer.h"

#define CHARACTER_SET_MODE 1

void ebnf_lexer(Lexer *lexer, Token *t_in, Token *t_out)
{
#define BETWEEN(V, A, B) (V >= A && V <= B)
#define IS_SPACE(V) (V == ' ' || V == '\t' || V == '\n' || V == '\r' || V =='\f')

	Token t1, t2, tn;

next_token:
	input_next_token(lexer->input, t_in, &t1);

	if(t1.symbol == L_EOF) {
		goto eof;
	}

	switch(lexer->mode) {
	case 0:
		if (IS_SPACE(t1.symbol)) {
			tn = t1;
			while(1) {
				input_next_token(lexer->input, &tn, &tn);
				if(!IS_SPACE(tn.symbol)) {
					break;
				}
			}
			token_init(t_out, t1.index, tn.index - t1.index, E_WHITE_SPACE);
		} else if (BETWEEN(t1.symbol, 'a', 'z') || BETWEEN(t1.symbol, 'A', 'Z')) {
			tn = t1;
			while(1) {
				input_next_token(lexer->input, &tn, &tn);
				if(
					!BETWEEN(tn.symbol, 'a', 'z') &&
					!BETWEEN(tn.symbol, 'A', 'Z') &&
					!BETWEEN(tn.symbol, '0', '9')
				) {
					break;
				}
			}
			token_init(t_out, t1.index, tn.index - t1.index, E_META_IDENTIFIER);
		} else if (BETWEEN(t1.symbol, '0', '9')) {
			tn = t1;
			while(1) {
				input_next_token(lexer->input, &tn, &tn);
				if(!BETWEEN(tn.symbol, '0', '9'))
					break;
			}
			token_init(t_out, t1.index, tn.index - t1.index, E_INTEGER);
		} else if (t1.symbol == '"') {
			tn = t1;
			while(1) {
				int prev = tn.symbol;
				input_next_token(lexer->input, &tn, &tn);
				if(tn.symbol == '"' && prev != '\\') {
					break;
				}
			}
			token_init(t_out, t1.index, tn.index - t1.index + 1, E_TERMINAL_STRING);
		} else if (t1.symbol == '\'') {
			tn = t1;
			while(1) {
				int prev = tn.symbol;
				input_next_token(lexer->input, &tn, &tn);
				if(tn.symbol == '\'' && prev != '\\') {
					break;
				}
			}
			token_init(t_out, t1.index, tn.index - t1.index + 1, E_TERMINAL_STRING);
		} else if (t1.symbol == '?') {
			input_next_token(lexer->input, &t1, &t2);
			if(t2.symbol == '[') {
				lexer->mode = CHARACTER_SET_MODE;
				token_init(t_out, t1.index, t2.index - t1.index + 1, E_START_CHARACTER_SET);
			} else {
				tn = t1;
				while(1) {
					int prev = tn.symbol;
					input_next_token(lexer->input, &tn, &tn);
					if(tn.symbol == '?' && prev != '\\') {
						break;
					}
				}
				token_init(t_out, t1.index, tn.index - t1.index + 1, E_SPECIAL_SEQUENCE);
			}
		} else if (t1.symbol == '(') {
			input_next_token(lexer->input, &t1, &t2);
			if(t2.symbol == '*') {
				tn = t2;
				while(1) {
					int prev = tn.symbol;
					input_next_token(lexer->input, &tn, &tn);
					if(tn.symbol == ')' && prev == '*') {
						break;
					}
				}
				token_init(t_out, t1.index, tn.index - t1.index + 1, E_COMMENT);
			} else {
				*t_out = t1;
			}
		} else {
			*t_out = t1;
		}

		if(t_out->symbol == E_WHITE_SPACE || t_out->symbol == E_COMMENT) {
			*t_in = *t_out;
			goto next_token;
		}
		break;
	case CHARACTER_SET_MODE:
		if (t1.symbol == ']') {
			input_next_token(lexer->input, &t1, &t2);
			if (t2.symbol == '?') {
				lexer->mode = 0;
				token_init(t_out, t1.index, t2.index - t1.index + t2.length, E_END_CHARACTER_SET);
			} else {
				*t_out = t1;
			}
		} else {
			*t_out = t1;
		}
		break;
	}

	return;
eof:
	token_init(t_out, t1.index, 0, L_EOF);
}

