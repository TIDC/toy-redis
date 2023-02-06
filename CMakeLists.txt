cmake_minimum_required(VERSION 3.15.0)
project(toy_redis VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS "-fcoroutines")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug 模式")
    add_definitions(
        -g
        -ggdb
        -O0
        -DDEBUG	# 定义宏 DEBUG
    )
endif()

enable_testing()
include(cmake/create_test.cmake)
add_subdirectory(tests)

add_executable(
    ${PROJECT_NAME}
    main.cpp)

target_include_directories(
    ${PROJECT_NAME} PRIVATE 
    .
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE 
    pthread
)