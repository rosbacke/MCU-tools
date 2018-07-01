


all: bitops test

.PHONY: test
test : bitops
	./bitops

clean:
	rm -f bitops

bitops: bit_ops_test.cpp bitops.h
	g++ -g -pthread -std=c++14 -I. -o bitops bit_ops_test.cpp -L/usr/src/gtest -lgtest

