cd build

echo "BUILD PROJECT"
TRACES=${1:-OFF}

echo TRACES = $TRACES

echo ""
echo "CMAKE:"
echo "------"
cmake .. -DENABLE_FSM_TRACE=$TRACES

echo ""
echo "MAKE:"
echo "-----"
make

cd ..
