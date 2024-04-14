cmake_minimum_required(VERSION 3.16)
project(untitled1)

set(CMAKE_CXX_STANDARD 11)

add_executable( common.h page_cache.h page_cache.cpp central_cache.h central_cache.cpp thread_cache.h thread_cache.cpp alloc.h main.cpp)