

all: build test

#all:
#	g++ -std=c++17 test.cpp 

GTEST:=/usr/src/gtest

INC := -I$(GTEST)/include
LIB:= -L$(GTEST)

build:
#	g++ -std=c++14 $(INC) $(LIB) StateChart.cpp fsm_test.cpp fsm_test2.cpp -l:libgtest.a -pthread
#	g++-8 -std=c++17 $(INC) $(LIB) -o fsm3_test fsm3_test.cpp -l:libgtest.a -pthread
	clang -std=c++17 $(INC) $(LIB) -o fsm3_test fsm3_test.cpp -l:libgtest.a -pthread -lstdc++

test:
	./fsm3_test
