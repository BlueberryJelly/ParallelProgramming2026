cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ParallelProgramming LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/utf-8 /W4)
endif()

add_library(utils INTERFACE)
target_include_directories(utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/utils)

foreach(lab lab_01 lab_02 lab_03 lab_04 lab_05)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/${lab}/CMakeLists.txt)
        add_subdirectory(src/${lab})
    endif()
endforeach()
