sh build.sh ON

cd build

echo ""
echo "PARSE GRAMMAR:"
echo "--------------"
cd pars
./pars ../testpars/grammars/empty_file.txt
./pars ../testpars/grammars/abtest.txt
cd ..

cd ..
