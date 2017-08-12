#!/bin/bash

MODE=${1:-run}
PATTERN=${2:-"no-pattern"}

source runner.sh

echo ""
echo "PARSE GRAMMARS:"
echo "--------------"

run_test $MODE $PATTERN "./pars ./tests/grammars/empty_file.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/ab-grammar.txt ./tests/grammars/ab-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/except-grammar.txt ./tests/grammars/except-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/factor-grammar.txt ./tests/grammars/factor-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/digit-grammar.txt ./tests/grammars/digit-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/integer-grammar.txt ./tests/grammars/integer-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/brackets-grammar.txt ./tests/grammars/brackets-source1.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/brackets-grammar.txt ./tests/grammars/brackets-source2.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/repetition-grammar.txt ./tests/grammars/repetition-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/ab2-grammar.txt ./tests/grammars/ab2-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/repetition-with-continuation-grammar.txt ./tests/grammars/repetition-with-continuation-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/repetition-inside-group-grammar.txt ./tests/grammars/repetition-inside-group-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/repetition-inside-group-grammar.txt ./tests/grammars/repetition-inside-group-source-2.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/option-grammar.txt ./tests/grammars/option-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/option-grammar.txt ./tests/grammars/option-source-2.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/charset-grammar.txt ./tests/grammars/charset-source.txt"


