cmake_minimum_required(VERSION 3.10)
project(block_allocator C)

set(CMAKE_C_STANDARD 99)

add_library(block_allocator STATIC 
    src/block_allocator.c
)
target_include_directories(block_allocator PUBLIC include)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()