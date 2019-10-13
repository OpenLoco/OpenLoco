cmake_minimum_required (VERSION 3.5)
project (memory-allocators LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
        src/Allocator.cpp
        src/FreeListAllocator.cpp
        )

add_library(malloc ${SOURCES})
target_include_directories(malloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)
