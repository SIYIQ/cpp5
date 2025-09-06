cmake_minimum_required(VERSION 3.16)
project(problem_3_4_solver VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(MSVC)
    add_compile_options(/W4)
    add_compile_options(/utf-8)  # 支持UTF-8编码
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# 设置构建类型为Release以获得更好的性能
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 查找依赖
find_package(Eigen3 REQUIRED)

# OpenMP查找（可选）
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found")
else()
    message(WARNING "OpenMP not found - parallel execution will be disabled")
endif()

# 定义源文件
set(CORE_SOURCES
    config.cpp
    geometry.cpp
    core_objects.cpp
    optimizer.cpp
    boundary_calculator.cpp
)

# 创建核心库
add_library(problem_34_core STATIC ${CORE_SOURCES})

# 设置核心库的包含目录和链接库
target_include_directories(problem_34_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(problem_34_core PUBLIC Eigen3::Eigen)

# 如果找到OpenMP，链接它
if(OpenMP_CXX_FOUND)
    target_link_libraries(problem_34_core PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(problem_34_core PUBLIC OPENMP_ENABLED)
endif()

# 问题3可执行文件
add_executable(solve_problem_3 solve_problem_3.cpp)
target_link_libraries(solve_problem_3 PRIVATE problem_34_core)

# 问题4可执行文件
add_executable(solve_problem_4 solve_problem_4.cpp)
target_link_libraries(solve_problem_4 PRIVATE problem_34_core)

# 边界计算器独立工具（如果需要）
add_executable(boundary_calculator_tool boundary_calculator.cpp)
target_link_libraries(boundary_calculator_tool PRIVATE problem_34_core)
target_compile_definitions(boundary_calculator_tool PRIVATE STANDALONE_TOOL)

# 安装配置
install(TARGETS solve_problem_3 solve_problem_4 boundary_calculator_tool
    RUNTIME DESTINATION bin
)

# 如果是Windows，复制必要的DLL
if(WIN32)
    # 这里可以添加复制DLL的逻辑，如果需要的话
endif()

# 显示配置信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Eigen3 version: ${Eigen3_VERSION}")
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP version: ${OpenMP_CXX_VERSION}")
endif()
