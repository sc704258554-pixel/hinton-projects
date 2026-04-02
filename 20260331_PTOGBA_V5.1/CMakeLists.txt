cmake_minimum_required(VERSION 3.10)
project(PTOGBA_V51)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SystemC 路径
set(SYSTEMC_PREFIX /usr/local/systemc-2.3.3)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SYSTEMC_PREFIX}/include
)

# 库目录
link_directories(
    ${SYSTEMC_PREFIX}/lib-linux64
)

# 源文件
set(SOURCES
    src/top.cpp
)

# 可执行文件
add_executable(ptogba_v51 ${SOURCES})

# 链接库
target_link_libraries(ptogba_v51 systemc)

# 编译选项
target_compile_options(ptogba_v51 PRIVATE -Wall -Wextra)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# 打印信息
message(STATUS "=== PTO-GBA V5.1 构建配置 ===")
message(STATUS "SystemC: ${SYSTEMC_PREFIX}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
