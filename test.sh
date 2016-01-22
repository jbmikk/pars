sh build.sh OFF

cd build

echo ""
echo "TESTSTLIB:"
echo "---------"
cd teststlib
./test_bsearch
./test_radixtree
cd ..

echo ""
echo "TESTPARS:"
echo "--------"
cd testpars
./test_lexer
./test_ebnf_parser
./test_fsm
./test_ast
./testpars
cd ..

cd ..

# test with valgrind 
# valgrind -v --suppressions=suppressions.supp --leak-check=full --show-leak-kinds=all build/teststlib/test_bsearch >> output.txt 2>&1
# valgrind -v --suppressions=suppressions.supp --leak-check=full --show-leak-kinds=all build/teststlib/test_radixtree >> output.txt 2>&1
