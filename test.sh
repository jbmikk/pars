#!/bin/bash

MODE=${1:-run}
PATTERN=${2:-"no-pattern"}

source runner.sh

echo ""
echo "TESTPARS:"
echo "--------"

run_test $MODE $PATTERN ./fsm_test


