cmake_minimum_required(VERSION 3.10)
project(pto_gba_esl VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 尝试多个 SystemC 路径
set(SYSTEMC_SEARCH_PATHS
    /usr
    /usr/local
    /usr/local/systemc-2.3.4
    $ENV{SYSTEMC_HOME}
)

# 查找 SystemC 头文件
find_path(SYSTEMC_INCLUDE_DIR
    NAMES systemc.h systemc
    PATHS ${SYSTEMC_SEARCH_PATHS}
    PATH_SUFFIXES include
)

# 查找 SystemC 库
find_library(SYSTEMC_LIBRARY
    NAMES systemc
    PATHS ${SYSTEMC_SEARCH_PATHS}
    PATH_SUFFIXES lib lib64 lib-linux64
)

if(NOT SYSTEMC_INCLUDE_DIR)
    message(FATAL_ERROR "SystemC headers not found! Please install SystemC")
endif()

if(NOT SYSTEMC_LIBRARY)
    message(WARNING "SystemC library not found, trying to compile without explicit linking")
endif()

message(STATUS "SystemC include: ${SYSTEMC_INCLUDE_DIR}")
if(SYSTEMC_LIBRARY)
    message(STATUS "SystemC library: ${SYSTEMC_LIBRARY}")
endif()

# 编译选项
add_compile_options(-Wall -Wextra -O2)

# 定义宏
add_definitions(-DSC_INCLUDE_DYNAMIC_PROCESSES)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SYSTEMC_INCLUDE_DIR}
)

# 源文件
set(SOURCES
    src/main.cpp
    src/latency_model.cpp
    src/arbiter.cpp
    src/scenario_gen.cpp
    src/stats.cpp
)

# 可执行文件
add_executable(pto_gba_esl ${SOURCES})

# 链接库
if(SYSTEMC_LIBRARY)
    target_link_libraries(pto_gba_esl ${SYSTEMC_LIBRARY} pthread)
else()
    target_link_libraries(pto_gba_esl pthread)
endif()

# 设置输出目录
set_target_properties(pto_gba_esl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 打印构建信息
message(STATUS "")
message(STATUS "=== PTO-GBA ESL Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SystemC include: ${SYSTEMC_INCLUDE_DIR}")
message(STATUS "")
