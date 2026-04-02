cmake_minimum_required(VERSION 3.10)
project(PTOGBA_V5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# 源文件
file(GLOB_RECURSE SOUR SOUR in "${CMAKE_SOURCE_DIR}/src/*.cpp")

# 可执行文件
add_executable(ptogba_v5 ${SOURCES})

# 编译选项
target_compile_options(ptogba_v5 PRIVATE -Wall -Wextra)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# 打印信息
message(STATUS "=== PTO-GBA V5 构建配置 ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
