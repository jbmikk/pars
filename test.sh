#!/bin/bash

MODE=${1:-run}
PATTERN=${2:-"no-pattern"}

source runner.sh

echo ""
echo "TESTPARS:"
echo "--------"

run_test $MODE $PATTERN ./input_test
run_test $MODE $PATTERN ./lexer_test
run_test $MODE $PATTERN ./ebnf_parser_test
run_test $MODE $PATTERN ./fsm_test
run_test $MODE $PATTERN ./parser_test
run_test $MODE $PATTERN ./ast_test
run_test $MODE $PATTERN ./cli_test
run_test $MODE $PATTERN ./astquery_test


