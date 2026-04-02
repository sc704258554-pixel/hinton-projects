cmake_minimum_required(VERSION 3.10)
project(sc-event-scheduler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/usr/include)
link_directories(/usr/lib/aarch64-linux-gnu)

add_executable(event_scheduler top.cpp)
target_link_libraries(event_scheduler systemc pthread)
