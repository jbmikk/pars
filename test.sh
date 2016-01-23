#setup $PWD variable
cd -P .

ROOT=$PWD

run_test()
{
	VALOPTIONS="--suppressions=$ROOT/suppressions.supp --leak-check=full --show-leak-kinds=all --track-origins=yes"

	if [ "$1" = "leaks" ]; then
		valgrind $VALOPTIONS $2
	fi

	if [ "$1" = "run" -o "$1" = "trace" ]; then
		$2
	fi
}

MODE=${1:-run}
echo "MODE: $MODE"

if [ "$MODE" = "trace" ]; then
	TRACE="ON"
else
	TRACE="OFF"
fi

sh build.sh $TRACE

cd build

echo ""
echo "TESTSTLIB:"
echo "---------"
cd teststlib

run_test $MODE ./test_bsearch
run_test $MODE ./test_radixtree

cd ..

echo ""
echo "TESTPARS:"
echo "--------"
cd testpars

run_test $MODE ./test_lexer
run_test $MODE ./test_ebnf_parser
run_test $MODE ./test_fsm
run_test $MODE ./test_ast
run_test $MODE ./testpars

cd ..

cd ..

# TODO: short version for leaks
# TODO: add colors for output, specially test results
# TODO: save output?
# >> output.txt 2>&1
