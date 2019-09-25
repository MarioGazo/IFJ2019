cmake_minimum_required(VERSION 3.14)
project(VUT_FIT_IFJ2019 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(VUT_FIT_IFJ2019
        ifj2019.c
        dynamic-string.c
        dynamic-string.h)
