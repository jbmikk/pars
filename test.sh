sh build.sh OFF

cd build

echo ""
echo "TESTCLIB:"
echo "---------"
cd testclib
./test_cbsearch
./test_cradixtree
cd ..

echo ""
echo "TESTPARS:"
echo "--------"
cd testpars
./test_lexer
./test_ebnf_parser
./test_fsm
cd ..

cd ..
