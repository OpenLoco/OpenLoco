cmake_minimum_required(VERSION 3.8)

project(openloco)

# Sources
file(GLOB_RECURSE OPENLOCO_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)
PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)

add_executable(openloco ${OPENLOCO_SOURCES})

if (MINGW)
    target_compile_definitions(openloco PRIVATE _WIN32_WINNT=0x0600)
endif()

target_link_libraries(openloco sdl2)
