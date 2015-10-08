cmake_minimum_required(VERSION 3.3)
project(native)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    frame.c callframes.h)
include_directories(/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers/)

add_executable(native ${SOURCE_FILES})