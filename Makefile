.PHONY: build clean

build:
	cmake -B build -S .
	cmake --build build

clean:
	rm -rf build