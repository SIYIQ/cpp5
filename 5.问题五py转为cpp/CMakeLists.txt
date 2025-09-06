# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(Problem5_Solver CXX)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 依赖库查找 ---
# 1. 查找 Eigen 库
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# 2. 查找 Boost 库 (只查找头文件，不查找具体组件)
find_package(Boost REQUIRED)

# 3. 查找 NLopt 库
find_package(NLopt REQUIRED)

# --- 源码文件 ---
set(SOURCES
    config.cpp
    core_objects.cpp
    geometry.cpp
    optimizer.cpp
    utils.cpp
    solve_problem_5.cpp
)

# --- 可执行文件定义 ---
add_executable(${PROJECT_NAME} ${SOURCES})

# --- 链接依赖 ---
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${EIGEN3_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${NLOPT_INCLUDE_DIRS}
)
# 移除了 Boost::system，因为 odeint 是纯头文件库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${NLOPT_LIBRARIES}
)


# 打印状态信息
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Eigen3 include directory: ${EIGEN3_INCLUDE_DIR}")
message(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "NLopt include directories: ${NLOPT_INCLUDE_DIRS}")
message(STATUS "NLopt libraries: ${NLOPT_LIBRARIES}")