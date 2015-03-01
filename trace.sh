sh build.sh ON

cd build

echo ""
echo "TESTCLIB:"
echo "---------"
cd testclib
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
cd ..

cd ..
