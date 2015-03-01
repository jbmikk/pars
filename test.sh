sh build.sh OFF

cd build

echo ""
echo "TESTCLIB:"
echo "---------"
cd testclib
./test_bsearch
./test_cradixtree
cd ..

echo ""
echo "TESTPARS:"
echo "--------"
cd testpars
./test_lexer
./test_ebnf_parser
./test_fsm
./test_ast
cd ..

cd ..

# test with valgrind 
# valgrind -v --suppressions=suppressions.supp --leak-check=full build/testclib/test_bsearch
