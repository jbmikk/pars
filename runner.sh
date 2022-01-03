#!/bin/bash

#setup $PWD variable
cd -P .

ROOT=$PWD

run_test()
{
	VALOPTIONS="--suppressions=$ROOT/suppressions.supp --leak-check=full --show-leak-kinds=all --track-origins=yes"
	EXPECTED=${4:-0} 

	# test if command matches pattern
	if [ "$2" = "no-pattern" ]; then
		MATCHES=0
	else
		echo "$3"|grep "$2">/dev/null;
		MATCHES=$?
	fi

	# if they match then run command
	if [ $MATCHES -eq 0 ]; then

		if [ "$1" = "leaks" -o "$1" = "leaktrace" ]; then
			OUTPUT=$(valgrind $VALOPTIONS $3 2>&1)
			STATUS=$?
		fi

		if [ "$1" = "calls" ]; then
			OUTPUT=$(valgrind --tool=callgrind $3 2>&1)
			STATUS=$?
			echo "Usage: callgrind_annotate --auto=yes callgrind.out.pid"
		fi

		if [ "$1" = "cache" ]; then
			OUTPUT=$(valgrind --tool=callgrind --simulate-cache=yes $3 2>&1)
			STATUS=$?
			echo "Usage: callgrind_annotate --auto=yes callgrind.out.pid"
		fi

		if [ "$1" = "run" -o "$1" = "trace" ]; then
			OUTPUT=$($3 2>&1)
			STATUS=$?
		fi

		if [ $STATUS -eq $EXPECTED ]; then
			echo -e "$3 \e[32mOK\e[0m"
		else
			echo "$OUTPUT"
			echo "EXIT STATUS=$STATUS"
			echo -e "$3 \e[31mERROR\e[0m"
			exit 1
		fi
	fi
}

echo "MODE: $MODE"

if [ "$MODE" = "trace" -o "$1" = "leaktrace" ]; then
	TRACE="ON"
else
	TRACE="OFF"
fi

if [ "$MODE" = "prof" ]; then
	PROFILE="ON"
else
	PROFILE="OFF"
fi


# Build
mkdir -p build
cd build

cmake .. -DTRACE=$TRACE -DPROFILE=$PROFILE
make

if [ "$MODE" = "prof" ]; then
	echo "Usage gprof executable gmon.out > output.txt"
fi

# TODO: short version for leaks
# TODO: add colors for output, specially test results
# TODO: save output?
# >> output.txt 2>&1
