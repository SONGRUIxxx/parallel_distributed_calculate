cmake_minimum_required(VERSION 3.20)
project(ParDist VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
endif()

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 构建 ParDist (交互式选择 server/client 模式，整合所有功能)
add_executable(pardist 
    src/UDP.cpp
    src/main.cpp
    src/common.cpp
    src/basic.cpp
    src/speed_up.cpp
)

# 添加编译选项以启用 SSE/AVX 指令集
target_compile_options(pardist PRIVATE -msse -msse2 -msse3 -msse4.1 -mavx)

# 链接 pthread 和 OpenMP 库
target_link_libraries(pardist pthread OpenMP::OpenMP_CXX)

# 打印构建信息
message(STATUS "ParDist - UDP Communication Tool")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "SSE/AVX optimizations enabled")
message(STATUS "OpenMP parallel support enabled")
