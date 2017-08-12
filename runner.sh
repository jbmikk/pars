#!/bin/bash

#setup $PWD variable
cd -P .

ROOT=$PWD

run_test()
{
	VALOPTIONS="--suppressions=$ROOT/suppressions.supp --leak-check=full --show-leak-kinds=all --track-origins=yes"

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
			echo $3
			OUTPUT=$(valgrind $VALOPTIONS $3)
		fi

		if [ "$1" = "calls" ]; then
			echo $3
			OUTPUT=$(valgrind --tool=callgrind $3)
			echo "Usage: callgrind_annotate --auto=yes callgrind.out.pid"
		fi

		if [ "$1" = "cache" ]; then
			echo $3
			OUTPUT=$(valgrind --tool=callgrind --simulate-cache=yes $3)
			echo "Usage: callgrind_annotate --auto=yes callgrind.out.pid"
		fi

		if [ "$1" = "run" -o "$1" = "trace" ]; then
			echo $3
			$3
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
