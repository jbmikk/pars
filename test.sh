#!/bin/bash

MODE=${1:-run}
PATTERN=${2:-"no-pattern"}

source runner.sh

echo ""
echo "TESTPARS:"
echo "--------"

run_test $MODE $PATTERN ./test_input
run_test $MODE $PATTERN ./test_lexer
run_test $MODE $PATTERN ./test_ebnf_parser
run_test $MODE $PATTERN ./test_fsm
run_test $MODE $PATTERN ./test_parser
run_test $MODE $PATTERN ./test_ast
run_test $MODE $PATTERN ./test_cli
run_test $MODE $PATTERN ./test_astquery


