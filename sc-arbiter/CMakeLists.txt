cmake_minimum_required(VERSION 3.10)
project(sc-arbiter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/usr/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
link_directories(/usr/lib/aarch64-linux-gnu)

add_executable(arbiter_tb tb.cpp)
target_link_libraries(arbiter_tb systemc pthread)
