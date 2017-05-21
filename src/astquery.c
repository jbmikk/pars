#include "astquery.h"

void ast_query_init(AstQuery *query, Ast *ast, int *symbols, unsigned short length)
{
	query->symbols = symbols;
	query->length = length;
	ast_cursor_init(&query->cursor, ast);
}

void ast_query_dispose(AstQuery *query)
{
	query->symbols = NULL;
	query->length = 0;
	ast_cursor_dispose(&query->cursor);
}

AstNode *ast_query_next(AstQuery *query)
{
	AstNode *node;
	int index;
	int symbol;
	int direction;

	if(query->cursor.current) {
		index = query->length - 1;
		direction = -1;
		ast_cursor_pop(&query->cursor);
	} else {
		index = 0;
		direction = 1;
		ast_cursor_push(&query->cursor);
	}

	while (index < query->length && index >= 0) {
		symbol = *(query->symbols + index);

		if(direction > 0) {
			node = ast_cursor_descendant_next_symbol(&query->cursor, symbol);
		} else {
			node = ast_cursor_relative_next_symbol(&query->cursor, symbol);
		}

		if(node) {
			ast_cursor_push(&query->cursor);
			direction = 1;
		} else {
			ast_cursor_pop(&query->cursor);
			direction = -1;
		}
		index += direction;
	}

	return node;
}
