#!/bin/bash

gawk 'BEGIN { print "namespace test { " }; END { print "}" }; /^#define  RCC_AHB1ENR/ { match($2, "([A-Z]*)_([A-Z0-9]*)_([A-Z0-9]*)", arr); print arr[1] "," arr[2] "," arr[3]}' stm32f4xx.h
# t = match($2, "([A-Z]*)_([A-Z]*)_([A-Z]*)", arr); print arr[1]; }' -- stm32f4xx.h

