
INC := -I$(HOME)/0_project/serial_net/external/googletest/googletest/include/
LIB:= -L$(HOME)/0_project/serial_net/out/external/googletest/googletest
all:
	g++ -std=c++14 $(INC) $(LIB) StateChart.cpp fsm_test.cpp -l:libgtest.a -pthread
