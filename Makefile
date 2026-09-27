.PHONY: build clean

build:
	cmake -B build -S .
	cmake --build build

build-test:
	cmake --preset test
	cmake --build --preset test

build-trace:
	cmake --preset trace
	cmake --build --preset trace

build-profile:
	cmake --preset profile
	cmake --build --preset profile

test: build-test
	ctest --preset test -L regular $(if $(R),-R "$(R)")

test-trace: build-trace
	ctest --preset trace -L regular $(if $(R),-R "$(R)")

test-profile-run: build-profile
	ctest --preset profile -L regular $(if $(R),-R "$(R)")

test-profile: test-profile-run
	ctest --preset profile -L profile $(if $(R),-R "$(R)")

test-leak: build-test
	ctest --preset test -L valgrind $(if $(R),-R "$(R)")

test-leaktrace: build-trace
	ctest --preset trace -L valgrind $(if $(R),-R "$(R)")

test-calls: build-test
	ctest --preset test -L callgrind_calls $(if $(R),-R "$(R)")

test-cache: build-test
	ctest --preset test -L callgrind_simulate_cache $(if $(R),-R "$(R)")

clean:
	rm -rf build