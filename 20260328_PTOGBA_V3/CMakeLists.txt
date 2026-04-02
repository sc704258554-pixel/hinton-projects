cmake_minimum_required(VERSION 3.10)
project(pto_gba_esl_v3 VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SYSTEMC_SEARCH_PATHS
    /usr
    /usr/local
    /usr/local/systemc-2.3.4
    $ENV{SYSTEMC_HOME}
)

find_path(SYSTEMC_INCLUDE_DIR
    NAMES systemc.h systemc
    PATHS ${SYSTEMC_SEARCH_PATHS}
    PATH_SUFFIXES include
)

find_library(SYSTEMC_LIBRARY
    NAMES systemc
    PATHS ${SYSTEMC_SEARCH_PATHS}
    PATH_SUFFIXES lib lib64 lib-linux64
)

if(NOT SYSTEMC_INCLUDE_DIR)
    message(FATAL_ERROR "SystemC headers not found!")
endif()

message(STATUS "SystemC include: ${SYSTEMC_INCLUDE_DIR}")
if(SYSTEMC_LIBRARY)
    message(STATUS "SystemC library: ${SYSTEMC_LIBRARY}")
endif()

add_compile_options(-Wall -Wextra -O2)
add_definitions(-DSC_INCLUDE_DYNAMIC_PROCESSES)
add_definitions(-DPTO_GBA_VERSION=3)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SYSTEMC_INCLUDE_DIR}
)

set(SOURCES
    src/main.cpp
    src/latency_model.cpp
    src/arbiter.cpp
    src/scenario_gen.cpp
    src/stats.cpp
)

add_executable(pto_gba_esl_v3 ${SOURCES})

if(SYSTEMC_LIBRARY)
    target_link_libraries(pto_gba_esl_v3 ${SYSTEMC_LIBRARY} pthread)
else()
    target_link_libraries(pto_gba_esl_v3 pthread)
endif()

set_target_properties(pto_gba_esl_v3 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

message(STATUS "")
message(STATUS "=== PTO-GBA ESL V3.0 Build Configuration ===")
message(STATUS "Version: V3.0 - Real MoE Stimulus")
message(STATUS "")
