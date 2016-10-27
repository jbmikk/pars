#setup $PWD variable
cd -P .

ROOT=$PWD

run_test()
{
	VALOPTIONS="--suppressions=$ROOT/suppressions.supp --leak-check=full --show-leak-kinds=all --track-origins=yes"

	# test if command matches pattern
	if [ "$2" = "no-pattern" ]; then
		MATCHES=0
	else
		echo "$3"|grep "$2">/dev/null;
		MATCHES=$?
	fi

	# if they match then run command
	if [ $MATCHES -eq 0 ]; then

		if [ "$1" = "leaks" -o "$1" = "leaktrace" ]; then
			echo $3
			valgrind $VALOPTIONS $3
		fi

		if [ "$1" = "run" -o "$1" = "trace" ]; then
			echo $3
			$3
		fi

	fi
}

MODE=${1:-run}
echo "MODE: $MODE"

if [ "$MODE" = "trace" -o "$1" = "leaktrace" ]; then
	TRACE="ON"
else
	TRACE="OFF"
fi

PATTERN=${2:-"no-pattern"}


# Build
mkdir -p build
cd build

cmake .. -DTRACE=$TRACE
make


echo ""
echo "TESTPARS:"
echo "--------"

run_test $MODE $PATTERN ./test_input
run_test $MODE $PATTERN ./test_lexer
run_test $MODE $PATTERN ./test_ebnf_parser
run_test $MODE $PATTERN ./test_fsm
run_test $MODE $PATTERN ./test_ast
run_test $MODE $PATTERN ./test_cli


echo ""
echo "PARSE GRAMMARS:"
echo "--------------"

run_test $MODE $PATTERN "./pars ./tests/grammars/empty_file.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/ab-grammar.txt ./tests/grammars/ab-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/digit-grammar.txt ./tests/grammars/digit-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/integer-grammar.txt ./tests/grammars/integer-source.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/brackets-grammar.txt ./tests/grammars/brackets-source1.txt"
run_test $MODE $PATTERN "./pars ./tests/grammars/brackets-grammar.txt ./tests/grammars/brackets-source2.txt"

cd ..

cd ..

# TODO: short version for leaks
# TODO: add colors for output, specially test results
# TODO: save output?
# >> output.txt 2>&1
