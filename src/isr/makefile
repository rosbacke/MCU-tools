
.PHONY: all
all: test

.PHONY: test
test: isr_test isr.h isr_test.cpp
	./isr_test
	
isr_test: isr.h isr_test.cpp
	g++ -g -std=c++14 -o isr_test isr_test.cpp
	

.PHONY: clean
clean:
	rm isr_test