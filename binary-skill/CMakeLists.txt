cmake_minimum_required(VERSION 3.20)
project(binary_skill CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(binary_skill_cli main.cpp binary_skill.hpp)
target_include_directories(binary_skill_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(benchmark_runner benchmark.cpp binary_skill.hpp)
target_include_directories(benchmark_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
