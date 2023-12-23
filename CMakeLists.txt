cmake_minimum_required(VERSION 3.27)
project(shad_fix_set)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_ASAN "-g -fsanitize=address,undefined -fno-sanitize-recover=all"
        CACHE STRING "Compiler flags in asan build"
        FORCE)

set(CMAKE_CXX_FLAGS_UBSAN "-g -fsanitize=address,undefined"
        CACHE STRING "Compiler flags in ubsan build"
        FORCE)


add_executable(shad_fix_set run_fixed_set.cpp)
