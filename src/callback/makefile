


all: callback callback2

clean:
	rm -f callback callback2

callback2: Callback2_test.cpp
	g++ -g -std=c++14 -o callback2 Callback2_test.cpp

callback: Callback_test.cpp
	g++ -g -std=c++03 -o callback Callback_test.cpp

