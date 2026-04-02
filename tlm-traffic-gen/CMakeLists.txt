cmake_minimum_required(VERSION 3.10)
project(tlm-traffic-gen CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SystemC from apt (standard paths)
include_directories(/usr/include)
link_directories(/usr/lib/aarch64-linux-gnu)

# Executable
add_executable(traffic_gen
    top.cpp
)

target_link_libraries(traffic_gen
    systemc
    pthread
)
