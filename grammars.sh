#!/bin/bash

MODE=${1:-run}
PATTERN=${2:-"no-pattern"}

source runner.sh

echo ""
echo "PARSE GRAMMARS:"
echo "--------------"

run_test $MODE $PATTERN "./pars ./test/grammars/empty_file.txt" 255
run_test $MODE $PATTERN "./pars ./test/grammars/ab-grammar.txt ./test/grammars/ab-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/except-grammar.txt ./test/grammars/except-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/factor-grammar.txt ./test/grammars/factor-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/digit-grammar.txt ./test/grammars/digit-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/integer-grammar.txt ./test/grammars/integer-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/brackets-grammar.txt ./test/grammars/brackets-target1.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/brackets-grammar.txt ./test/grammars/brackets-target2.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-grammar.txt ./test/grammars/repetition-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/ab2-grammar.txt ./test/grammars/ab2-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-with-continuation-grammar.txt ./test/grammars/repetition-with-continuation-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-inside-group-grammar.txt ./test/grammars/repetition-inside-group-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-inside-group-grammar.txt ./test/grammars/repetition-inside-group-target-2.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/option-grammar.txt ./test/grammars/option-target.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/option-grammar.txt ./test/grammars/option-target-2.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/charset-grammar.txt ./test/grammars/charset-target.txt"


