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

		if [ "$1" = "leaks" ]; then
			valgrind $VALOPTIONS $3
		fi

		if [ "$1" = "run" -o "$1" = "trace" ]; then
			$3
		fi

	fi
}

MODE=${1:-run}
echo "MODE: $MODE"

if [ "$MODE" = "trace" ]; then
	TRACE="ON"
else
	TRACE="OFF"
fi

PATTERN=${2:-"no-pattern"}

sh build.sh $TRACE

cd build

echo ""
echo "TESTSTLIB:"
echo "---------"
cd teststlib

run_test $MODE $PATTERN ./test_bsearch
run_test $MODE $PATTERN ./test_radixtree

cd ..

echo ""
echo "TESTPARS:"
echo "--------"
cd testpars

run_test $MODE $PATTERN ./test_lexer
run_test $MODE $PATTERN ./test_ebnf_parser
run_test $MODE $PATTERN ./test_fsm
run_test $MODE $PATTERN ./test_ast
run_test $MODE $PATTERN ./testpars

cd ..

echo ""
echo "PARSE GRAMMARS:"
echo "--------------"
cd pars
run_test $MODE $PATTERN "./pars ../testpars/grammars/empty_file.txt"
run_test $MODE $PATTERN "./pars ../testpars/grammars/abtest.txt ../testpars/grammars/ab-source.txt"
cd ..

cd ..

cd ..

# TODO: short version for leaks
# TODO: add colors for output, specially test results
# TODO: save output?
# >> output.txt 2>&1
