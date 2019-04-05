#!/bin/bash

MODE=${1:-run}
PATTERN=${2:-"no-pattern"}

source runner.sh

echo ""
echo "PARSE GRAMMARS:"
echo "--------------"

run_test $MODE $PATTERN "./pars ./test/grammars/empty_file.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/ab-grammar.txt ./test/grammars/ab-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/except-grammar.txt ./test/grammars/except-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/factor-grammar.txt ./test/grammars/factor-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/digit-grammar.txt ./test/grammars/digit-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/integer-grammar.txt ./test/grammars/integer-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/brackets-grammar.txt ./test/grammars/brackets-source1.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/brackets-grammar.txt ./test/grammars/brackets-source2.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-grammar.txt ./test/grammars/repetition-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/ab2-grammar.txt ./test/grammars/ab2-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-with-continuation-grammar.txt ./test/grammars/repetition-with-continuation-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-inside-group-grammar.txt ./test/grammars/repetition-inside-group-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/repetition-inside-group-grammar.txt ./test/grammars/repetition-inside-group-source-2.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/option-grammar.txt ./test/grammars/option-source.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/option-grammar.txt ./test/grammars/option-source-2.txt"
run_test $MODE $PATTERN "./pars ./test/grammars/charset-grammar.txt ./test/grammars/charset-source.txt"


