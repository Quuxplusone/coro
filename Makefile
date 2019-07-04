all: test

test: examples/*.cpp
	test/compile-on-godbolt.py --run $^
