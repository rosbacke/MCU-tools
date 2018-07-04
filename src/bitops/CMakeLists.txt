cmake_minimum_required(VERSION 3.0)

project(bitops)

link_directories(
        /usr/src/gtest
)

add_executable(bitops_test bit_ops_test.cpp)
target_compile_options(bitops_test PUBLIC -std=c++14 -pthread)
target_link_libraries(bitops_test gtest pthread)
