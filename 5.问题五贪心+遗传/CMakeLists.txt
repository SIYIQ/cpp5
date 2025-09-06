cmake_minimum_required(VERSION 3.12)
project(SmokeObscurationOptimizer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 优化选项
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# 默认为Release模式
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 查找Eigen3
find_package(Eigen3 REQUIRED)

# 查找OpenMP (用于并行化)
find_package(OpenMP REQUIRED)

# 添加源文件
set(SOURCES
    config.cpp
    geometry.cpp
    core_objects.cpp
    boundary_calculator.cpp
    task_allocator.cpp
    threat_assessor.cpp
    strategy_calculator.cpp
    optimizer.cpp
)

# 创建库
add_library(smoke_optimizer_lib ${SOURCES})

# 链接依赖
target_link_libraries(smoke_optimizer_lib 
    PUBLIC Eigen3::Eigen
    PUBLIC OpenMP::OpenMP_CXX
)

# 设置包含目录
target_include_directories(smoke_optimizer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 编译选项
target_compile_options(smoke_optimizer_lib PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-ffast-math>
)

# 主执行文件
add_executable(solve_problem_5 solve_problem_5.cpp)
target_link_libraries(solve_problem_5 smoke_optimizer_lib)

# 新的全局优化版本
add_executable(solve_problem_5_new solve_problem_5_new.cpp)
target_link_libraries(solve_problem_5_new smoke_optimizer_lib)

# 测试可执行文件 (可选)
# add_executable(test_geometry test_geometry.cpp)
# target_link_libraries(test_geometry smoke_optimizer_lib)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CXX flags (Debug): ${CMAKE_CXX_FLAGS_DEBUG}")
