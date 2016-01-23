sh build.sh OFF

cd build

VALOPTIONS="--suppressions=../../suppressions.supp --leak-check=full --show-leak-kinds=all --track-origins=yes"

echo ""
echo "TESTSTLIB:"
echo "---------"
cd teststlib

valgrind $VALOPTIONS ./test_bsearch
valgrind $VALOPTIONS ./test_radixtree

cd ..

echo ""
echo "TESTPARS:"
echo "--------"
cd testpars

valgrind $VALOPTIONS ./test_lexer
valgrind $VALOPTIONS ./test_ebnf_parser
valgrind $VALOPTIONS ./test_fsm
valgrind $VALOPTIONS ./test_ast
valgrind $VALOPTIONS ./testpars

# TODO: save output?
# >> output.txt 2>&1

cd ..

cd ..

