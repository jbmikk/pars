#include "ebnf_parser.h"
#include "parsercontext.h"
#include "symbols.h"
#include "astlistener.h"
#include "controlloop.h"
#include "bcode.h"

#include "cmemory.h"
#include "dbg.h"

#include <setjmp.h>

#define nzs(S) (S), (strlen(S))

jmp_buf on_error;

void parse_error(Source *source, unsigned int index)
{
	longjmp(on_error, 1);
}

void ebnf_build_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	int META_IDENTIFIER = fsm_get_symbol_id(fsm, nzs("meta_identifier"));
	int INTEGER = fsm_get_symbol_id(fsm, nzs("integer"));
	int EXCEPT_SYMBOL = fsm_get_symbol_id(fsm, nzs("except_symbol"));
	int CONCATENATE_SYMBOL = fsm_get_symbol_id(fsm, nzs("concatenate_symbol"));
	int DEFINITION_SEPARATOR_SYMBOL = fsm_get_symbol_id(fsm, nzs("definition_separator_symbol"));
	int REPETITION_SYMBOL = fsm_get_symbol_id(fsm, nzs("repetition_symbol"));
	int DEFINING_SYMBOL = fsm_get_symbol_id(fsm, nzs("defining_symbol"));
	int TERMINATOR_SYMBOL = fsm_get_symbol_id(fsm, nzs("terminator_symbol"));
	int TERMINAL_STRING = fsm_get_symbol_id(fsm, nzs("terminal_string"));
	int SPECIAL_SEQUENCE = fsm_get_symbol_id(fsm, nzs("special_sequence"));
	int START_GROUP_SYMBOL = fsm_get_symbol_id(fsm, nzs("start_group_symbol"));
	int END_GROUP_SYMBOL = fsm_get_symbol_id(fsm, nzs("end_group_symbol"));
	int START_OPTION_SYMBOL = fsm_get_symbol_id(fsm, nzs("start_option_symbol"));
	int END_OPTION_SYMBOL = fsm_get_symbol_id(fsm, nzs("end_option_symbol"));
	int START_REPETITION_SYMBOL = fsm_get_symbol_id(fsm, nzs("start_repetition_symbol"));
	int END_REPETITION_SYMBOL = fsm_get_symbol_id(fsm, nzs("end_repetition_symbol"));
	int START_CHARACTER_SET= fsm_get_symbol_id(fsm, nzs("start_character_set"));
	int CHARACTER_SET_ITEM = fsm_get_symbol_id(fsm, nzs("character_set_item"));
	int CHARACTER_RANGE_SYMBOL = fsm_get_symbol_id(fsm, nzs("character_range_symbol"));
	int END_CHARACTER_SET = fsm_get_symbol_id(fsm, nzs("end_character_set"));

	//Syntactic Primary
	fsm_builder_define(&builder, nzs("syntactic_primary"));
	fsm_builder_group_start(&builder);

	fsm_builder_terminal(&builder, META_IDENTIFIER);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, TERMINAL_STRING);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, SPECIAL_SEQUENCE);
	fsm_builder_or(&builder);

	fsm_builder_nonterminal(&builder,  nzs("character_set"));
	fsm_builder_or(&builder);


	fsm_builder_terminal(&builder, START_GROUP_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, END_GROUP_SYMBOL);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, START_OPTION_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, END_OPTION_SYMBOL);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, START_REPETITION_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, END_REPETITION_SYMBOL);
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	//Character
	fsm_builder_define(&builder, nzs("character"));
	fsm_builder_terminal(&builder, CHARACTER_SET_ITEM);
	fsm_builder_end(&builder);

	//Character primary
	fsm_builder_define(&builder, nzs("character_primary"));
	fsm_builder_nonterminal(&builder,  nzs("character"));
	fsm_builder_option_group_start(&builder);
	fsm_builder_terminal(&builder, CHARACTER_RANGE_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("character"));
	fsm_builder_option_group_end(&builder);
	fsm_builder_end(&builder);

	//Character set
	fsm_builder_define(&builder, nzs("character_set"));
	fsm_builder_terminal(&builder, START_CHARACTER_SET);
	fsm_builder_nonterminal(&builder,  nzs("character_primary"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("character_primary"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, END_CHARACTER_SET);
	fsm_builder_end(&builder);

	//Syntactic Factor
	fsm_builder_define(&builder, nzs("syntactic_factor"));
	fsm_builder_option_group_start(&builder);
	fsm_builder_terminal(&builder, INTEGER);
	fsm_builder_terminal(&builder, REPETITION_SYMBOL);
	fsm_builder_option_group_end(&builder);
	fsm_builder_nonterminal(&builder,  nzs("syntactic_primary"));
	fsm_builder_end(&builder);

	//Syntactic Exception
	fsm_builder_define(&builder, nzs("syntactic_exception"));
	fsm_builder_nonterminal(&builder,  nzs("syntactic_factor"));
	fsm_builder_end(&builder);

	//Syntactic Term
	fsm_builder_define(&builder, nzs("syntactic_term"));
	fsm_builder_nonterminal(&builder,  nzs("syntactic_factor"));
	fsm_builder_option_group_start(&builder);
	fsm_builder_terminal(&builder, EXCEPT_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("syntactic_exception"));
	fsm_builder_option_group_end(&builder);
	fsm_builder_end(&builder);

	//Single Definition
	fsm_builder_define(&builder, nzs("single_definition"));
	fsm_builder_nonterminal(&builder,  nzs("syntactic_term"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal(&builder, CONCATENATE_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("syntactic_term"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Definitions List
	fsm_builder_define(&builder, nzs("definitions_list"));
	fsm_builder_nonterminal(&builder,  nzs("single_definition"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal(&builder, DEFINITION_SEPARATOR_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("single_definition"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Syntax Rule
	fsm_builder_define(&builder, nzs("syntax_rule"));
	fsm_builder_terminal(&builder, META_IDENTIFIER);
	fsm_builder_terminal(&builder, DEFINING_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, TERMINATOR_SYMBOL);
	fsm_builder_end(&builder);

	//Syntax
	fsm_builder_define(&builder, nzs("syntax"));
	fsm_builder_nonterminal(&builder,  nzs("syntax_rule"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("syntax_rule"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

void ebnf_build_lexer_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	fsm_builder_set_mode(&builder, nzs(".default"));

	//White space
	//TODO: Add other white space characters
	fsm_builder_define(&builder, nzs("white_space"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal(&builder, ' ');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\t');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\n');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\r');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\f');
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal(&builder, ' ');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\t');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\n');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\r');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\f');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//TODO: should be able to filter certain nonterminals from the root.
	/*fsm_builder_define(&builder, nzs("punctuation_symbols"));
	fsm_builder_terminal(&builder, ',');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '.');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ':');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ';');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '`');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '|');
	fsm_builder_end(&builder);*/

	//Meta identifiers
	//TODO: should add whitespace to comply with EBNF specs
	fsm_builder_define(&builder, nzs("meta_identifier"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Integer
	fsm_builder_define(&builder, nzs("integer"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Terminal string
	//TODO: add all utf8 ranges
	fsm_builder_define(&builder, nzs("terminal_string"));
	fsm_builder_terminal(&builder, '"');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_copy(&builder,  nzs("white_space"));
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ',');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '.');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ':');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ';');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '`');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '|');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\\');
	fsm_builder_terminal(&builder, '"');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, '"');
	fsm_builder_end(&builder);

	//Terminal string
	//TODO: add all utf8 ranges
	fsm_builder_define(&builder, nzs("terminal_string"));
	fsm_builder_terminal(&builder, '\'');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_copy(&builder,  nzs("white_space"));
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ',');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '.');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ':');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ';');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '`');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '|');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\\');
	fsm_builder_terminal(&builder, '\'');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, '\'');
	fsm_builder_end(&builder);

	//Special sequence
	//TODO: add white space
	fsm_builder_define(&builder, nzs("special_sequence"));
	fsm_builder_terminal(&builder, '?');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_copy(&builder,  nzs("white_space"));
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ',');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '.');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ':');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ';');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '`');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '|');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\\');
	fsm_builder_terminal(&builder, '?');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, '?');
	fsm_builder_end(&builder);

	//Character range start
	fsm_builder_define(&builder, nzs("start_character_set"));
	fsm_builder_terminal(&builder, '?');
	fsm_builder_terminal(&builder, '[');
	fsm_builder_end(&builder);
	fsm_builder_mode_push(&builder, nzs("character_set_mode"));

	//Character set mode
	fsm_builder_set_mode(&builder, nzs("character_set_mode"));

	//Character set item
	fsm_builder_define(&builder, nzs("character_set_item"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	//Character range symbol
	fsm_builder_define(&builder, nzs("character_range_symbol"));
	fsm_builder_terminal(&builder, '-');
	fsm_builder_end(&builder);

	//Character range end
	fsm_builder_define(&builder, nzs("end_character_set"));
	fsm_builder_terminal(&builder, ']');
	fsm_builder_terminal(&builder, '?');
	fsm_builder_end(&builder);
	fsm_builder_mode_pop(&builder);

	fsm_builder_set_mode(&builder, nzs(".default"));

	//Comment
	//TODO: add white space
	fsm_builder_define(&builder, nzs("comment"));
	fsm_builder_terminal(&builder, '(');
	fsm_builder_terminal(&builder, '*');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_copy(&builder,  nzs("white_space"));
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ',');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '.');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ':');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, ';');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '`');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\'');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '"');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '-');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '|');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, '*');
	//TODO: if asterisk followed by a non-asterisk it should be accepted
	fsm_builder_terminal(&builder, ')');
	fsm_builder_end(&builder);

	//TODO: support for self identifying symbols in fsm
	//Start group symbol
	fsm_builder_define(&builder, nzs("start_group_symbol"));
	fsm_builder_terminal(&builder, '(');
	fsm_builder_end(&builder);

	//End group symbol
	fsm_builder_define(&builder, nzs("end_group_symbol"));
	fsm_builder_terminal(&builder, ')');
	fsm_builder_end(&builder);

	//Start repeat symbol
	fsm_builder_define(&builder, nzs("start_repetition_symbol"));
	fsm_builder_terminal(&builder, '{');
	fsm_builder_end(&builder);

	//End repeat symbol
	fsm_builder_define(&builder, nzs("end_repetition_symbol"));
	fsm_builder_terminal(&builder, '}');
	fsm_builder_end(&builder);

	//Start option symbol
	fsm_builder_define(&builder, nzs("start_option_symbol"));
	fsm_builder_terminal(&builder, '[');
	fsm_builder_end(&builder);

	//End option symbol
	fsm_builder_define(&builder, nzs("end_option_symbol"));
	fsm_builder_terminal(&builder, ']');
	fsm_builder_end(&builder);

	//Defining symbol
	fsm_builder_define(&builder, nzs("defining_symbol"));
	fsm_builder_terminal(&builder, '=');
	fsm_builder_end(&builder);

	//Concatenate symbol
	fsm_builder_define(&builder, nzs("concatenate_symbol"));
	fsm_builder_terminal(&builder, ',');
	fsm_builder_end(&builder);

	//Except symbol
	fsm_builder_define(&builder, nzs("except_symbol"));
	fsm_builder_terminal(&builder, '-');
	fsm_builder_end(&builder);

	//Terminator symbol
	fsm_builder_define(&builder, nzs("terminator_symbol"));
	fsm_builder_terminal(&builder, ';');
	fsm_builder_end(&builder);

	//Definition separator symbol
	fsm_builder_define(&builder, nzs("definition_separator_symbol"));
	fsm_builder_terminal(&builder, '|');
	fsm_builder_end(&builder);

	//Repetition symbol
	fsm_builder_define(&builder, nzs("repetition_symbol"));
	fsm_builder_terminal(&builder, '*');
	fsm_builder_end(&builder);

	fsm_builder_lexer_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

int ebnf_lexer_pipe(void *_context, void *_tran)
{
	ParserContext *context = (ParserContext *)_context;
	Transition *tran = (Transition *)_tran;

	if(tran->action->type != ACTION_ACCEPT) {
		return 0;
	}
	Token token = tran->reduction;

	Symbol *comment = symbol_table_get(&context->parser->table, "comment", 7);
	Symbol *white_space = symbol_table_get(&context->parser->table, "white_space", 11);

	Continuation cont;
	cont.type = CONTINUATION_NEXT;
	//Filter white space and tokens
	if(token.symbol != comment->id && token.symbol != white_space->id) {
		cont = input_loop(&context->proxy_input, &context->thread, token);
	}
	return cont.type == CONTINUATION_ERROR;
}

void ebnf_build_parser_context(ParserContext *context)
{
	listener_init(&context->parse_setup_lexer, NULL, context);
	listener_init(&context->parse_setup_fsm, NULL, context);
	listener_init(&context->parse_start, ast_parse_start, context);
	listener_init(&context->parse_loop, control_loop_linear, context);
	listener_init(&context->parse_end, ast_parse_end, context);
	listener_init(&context->parse_error, ast_parse_error, context);

	listener_init(&context->lexer_pipe, ebnf_lexer_pipe, context);
	listener_init(&context->parser_pipe, ast_parser_pipe, context);
}

int ebnf_build_parser(Parser *parser)
{
	ebnf_build_lexer_fsm(&parser->lexer_fsm);
	ebnf_build_fsm(&parser->fsm);

	parser->build_context = ebnf_build_parser_context;

	return 0;
//error:
	//TODO: free

	//return -1;
}

static int parse_utf8(char *array, unsigned int size)
{
	//TODO properly parse utf8 point codes, for now only ascii.
	int code = array[0];
	return code;
}

void ebnf_build_character_set(FsmBuilder *builder, AstCursor *cur)
{
	char *string;
	int length;
	int first = 1;

	int E_CHARACTER_PRIMARY = ast_get_symbol(cur, nzs("character_primary"));
	int E_CHARACTER = ast_get_symbol(cur, nzs("character"));

	ast_cursor_depth_next_symbol(cur, E_CHARACTER_PRIMARY);
	do {
		ast_cursor_push(cur);

		if(first) {
			first = 0;
		} else {
			fsm_builder_or(builder);
		}
		ast_cursor_depth_next_symbol(cur, E_CHARACTER);

		ast_cursor_get_string(cur, &string, &length);
		int start = parse_utf8(string, length);

		if(ast_cursor_next_sibling_symbol(cur, E_CHARACTER)) {
			ast_cursor_get_string(cur, &string, &length);
			int end = parse_utf8(string, length);
			//TODO: Verify start < end
			fsm_builder_terminal_range(builder, (Range){start, end});
		} else {
			fsm_builder_terminal(builder, start);
		}
		ast_cursor_pop(cur);

	} while(ast_cursor_next_sibling_symbol(cur, E_CHARACTER_PRIMARY));
}

void ebnf_build_definitions_list(FsmBuilder *builder, AstCursor *cur);

void ebnf_build_syntactic_primary(FsmBuilder *builder, AstCursor *cur)
{
	AstNode *node = ast_cursor_depth_next(cur);
	char *string;
	int length, i;
	
	int CHARACTER_SET = ast_get_symbol(cur, nzs("character_set"));
	int META_IDENTIFIER = ast_get_symbol(cur, nzs("meta_identifier"));
	int DEFINITIONS_LIST = ast_get_symbol(cur, nzs("definitions_list"));
	int TERMINAL_STRING = ast_get_symbol(cur, nzs("terminal_string"));
	int SPECIAL_SEQUENCE = ast_get_symbol(cur, nzs("special_sequence"));
	int START_GROUP_SYMBOL = ast_get_symbol(cur, nzs("start_group_symbol"));
	int START_REPETITION_SYMBOL = ast_get_symbol(cur, nzs("start_repetition_symbol"));
	int START_OPTION_SYMBOL = ast_get_symbol(cur, nzs("start_option_symbol"));

	if(node->token.symbol == META_IDENTIFIER) {
		ast_cursor_get_string(cur, &string, &length);
		fsm_builder_nonterminal(builder, string, length);
	} else if (node->token.symbol == TERMINAL_STRING) {
		ast_cursor_get_string(cur, &string, &length);
		for(i = 1; i < length-1; i++) {
			//TODO: literal strings should be tokenized into simbols (utf8)
			fsm_builder_terminal(builder, string[i]);
		}
	} else if (node->token.symbol == SPECIAL_SEQUENCE) {
		//TODO: define special sequences behaviour
		log_warn("Special sequence is not defined");
	} else if (node->token.symbol == START_GROUP_SYMBOL) {
		ast_cursor_depth_next_symbol(cur, DEFINITIONS_LIST);
		fsm_builder_group_start(builder);
		ebnf_build_definitions_list(builder, cur);
		fsm_builder_group_end(builder);
	} else if (node->token.symbol == START_REPETITION_SYMBOL) {
		ast_cursor_depth_next_symbol(cur, DEFINITIONS_LIST);
		fsm_builder_loop_group_start(builder);
		ebnf_build_definitions_list(builder, cur);
		fsm_builder_loop_group_end(builder);
	} else if (node->token.symbol == START_OPTION_SYMBOL) {
		ast_cursor_depth_next_symbol(cur, DEFINITIONS_LIST);
		fsm_builder_option_group_start(builder);
		ebnf_build_definitions_list(builder, cur);
		fsm_builder_option_group_end(builder);
	} else if (node->token.symbol == CHARACTER_SET) {
		fsm_builder_group_start(builder);
		ebnf_build_character_set(builder, cur);
		fsm_builder_group_end(builder);
	} else {
		//TODO:sentinel??
	}
}

void ebnf_build_syntactic_factor(FsmBuilder *builder, AstCursor *cur)
{
	int E_SYNTACTIC_PRIMARY = ast_get_symbol(cur, nzs("syntactic_primary"));

	/*
	 * TODO: Can't use depth_next methods to probe optional children.
	 * It may jump to its grandchildren.
	if(ast_cursor_depth_next_symbol(cur, INTEGER)) {
		log_warn("Syntactic factors are not supported");
	}
	*/

	ast_cursor_depth_next_symbol(cur, E_SYNTACTIC_PRIMARY);
	ebnf_build_syntactic_primary(builder, cur);
}

void ebnf_build_syntactic_term(FsmBuilder *builder, AstCursor *cur)
{
	int E_SYNTACTIC_FACTOR = ast_get_symbol(cur, nzs("syntactic_factor"));
	int E_SYNTACTIC_EXCEPTION = ast_get_symbol(cur, nzs("syntactic_exception"));

	ast_cursor_depth_next_symbol(cur, E_SYNTACTIC_FACTOR);
	ast_cursor_push(cur);
	ebnf_build_syntactic_factor(builder, cur);
	ast_cursor_pop(cur);
	if(ast_cursor_next_sibling_symbol(cur, E_SYNTACTIC_EXCEPTION)) {
		log_warn("Syntactic exceptions are not supported");
	}
}

void ebnf_build_single_definition(FsmBuilder *builder, AstCursor *cur)
{
	int E_SYNTACTIC_TERM = ast_get_symbol(cur, nzs("syntactic_term"));
	ast_cursor_depth_next_symbol(cur, E_SYNTACTIC_TERM);
	do {
		ast_cursor_push(cur);
		ebnf_build_syntactic_term(builder, cur);
		ast_cursor_pop(cur);
	} while(ast_cursor_next_sibling_symbol(cur, E_SYNTACTIC_TERM));
}

void ebnf_build_definitions_list(FsmBuilder *builder, AstCursor *cur)
{
	int first = 1;
	int E_SINGLE_DEFINITION = ast_get_symbol(cur, nzs("single_definition"));
	ast_cursor_depth_next_symbol(cur, E_SINGLE_DEFINITION);
	do {
		ast_cursor_push(cur);
		if(first) {
			first = 0;
		} else {
			fsm_builder_or(builder);
		}
		ebnf_build_single_definition(builder, cur);
		ast_cursor_pop(cur);
	} while(ast_cursor_next_sibling_symbol(cur, E_SINGLE_DEFINITION));
}

void ebnf_build_syntax_rule(FsmBuilder *builder, AstCursor *cur)
{
	char *string;
	int length;

	//TODO: maybe we should use more precise cursor functions.
	// If the ast is badly formed we could end up reading a different node
	// than expected. This situations should be handled gracefully
	// Maybe an error could be thrown or a sentinel could be placed.
	int META_IDENTIFIER = ast_get_symbol(cur, nzs("meta_identifier"));
	ast_cursor_depth_next_symbol(cur, META_IDENTIFIER);
	ast_cursor_get_string(cur, &string, &length);
	fsm_builder_define(builder, string, length);

	int DEFINITIONS_LIST = ast_get_symbol(cur, nzs("definitions_list"));
	ast_cursor_next_sibling_symbol(cur, DEFINITIONS_LIST);
	fsm_builder_group_start(builder);
	ebnf_build_definitions_list(builder, cur);
	fsm_builder_group_end(builder);
	fsm_builder_end(builder);
}	

void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast)
{
	AstCursor cur;
	FsmBuilder builder;
	Bcode bcode;

	ast_cursor_init(&cur, ast);
	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	// push symbol selector syntax_rule

	int E_SYNTAX_RULE = ast_get_symbol(&cur, nzs("syntax_rule"));

	bcode_add_instruction( &bcode, &(Instruction){ OPCODE_SELECT, 1 });
	bcode_add_int(&bcode, E_SYNTAX_RULE);

	bcode_add_instruction( &bcode, &(Instruction){ OPCODE_EACH, 0 });

	bcode_add_instruction( &bcode, &(Instruction){ OPCODE_CALL, 0 });
	bcode_add_int(&bcode, BUILD_SYNTAX_RULE);

	bcode_add_instruction( &bcode, &(Instruction){ OPCODE_END, 0 });

	// lookup selector
	while(ast_cursor_depth_next_symbol(&cur, E_SYNTAX_RULE)) {
		ast_cursor_push(&cur);
		//call ebnf_build_syntax_rule
		ebnf_build_syntax_rule(&builder, &cur);
		ast_cursor_pop(&cur);
	}
	// pop symbol selector

	fsm_builder_done(&builder, L_EOF);
	//ast_done?

	fsm_builder_dispose(&builder);
	ast_cursor_dispose(&cur);
	
}
