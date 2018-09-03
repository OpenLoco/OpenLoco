cmake_minimum_required(VERSION 3.8)

set (PROJECT openloco)

project(${PROJECT} CXX)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

INCLUDE(FindPkgConfig)

set(USE_BOOST_FS_DEFAULT OFF)
if(APPLE OR MINGW)
	set(USE_BOOST_FS_DEFAULT ON)
endif()

option(USE_BOOST_FILESYSTEM "Use Boost filesystem instead of C++17" ${USE_BOOST_FS_DEFAULT})

if (APPLE)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/opt/openssl/lib/pkgconfig")
endif (APPLE)

# Describe current version in terms of closest tag
execute_process(
    COMMAND git describe HEAD
    COMMAND sed -E "s/-g.+$//"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE OPENLOCO_VERSION_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    # ERROR_QUIET
)

# Define current git branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE OPENLOCO_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Define short commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE OPENLOCO_COMMIT_SHA1_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# To avoid unnecessary rebuilds, set the current branch and short sha1 hash
# only for the one file that uses these definitions: version.cpp
set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/openloco/version.cpp
    PROPERTY COMPILE_DEFINITIONS
    OPENLOCO_VERSION_TAG="${OPENLOCO_VERSION_TAG}"
    OPENLOCO_BRANCH="${OPENLOCO_BRANCH}"
    OPENLOCO_COMMIT_SHA1_SHORT="${OPENLOCO_COMMIT_SHA1_SHORT}")

# Set some compiler features
set(COMMON_COMPILE_OPTIONS "${COMMON_COMPILE_OPTIONS} -fstrict-aliasing -Werror -Wall")

# Poke some holes in -Wall:
set(COMMON_COMPILE_OPTIONS
    "${COMMON_COMPILE_OPTIONS} \
    -Wno-unknown-pragmas \
    -Wno-unused-private-field \
    ")

# Set warnings common to supported compilers
set(COMMON_COMPILE_OPTIONS
    "${COMMON_COMPILE_OPTIONS} \
    -Waddress \
    -Warray-bounds \
    -Wchar-subscripts \
    -Wenum-compare \
    -Wformat \
    -Wignored-qualifiers \
    -Winit-self \
    -Wmissing-declarations \
    -Wnon-virtual-dtor \
    -Wnull-dereference \
    -Wstrict-aliasing \
    -Wstrict-overflow=1 \
    -Wundef \
    -Wunreachable-code \
    ")

set(OBJ_FORMAT "elf32-i386")
set(LINKER_SCRIPT "ld_script_i386.xc")


# Handle creating the rct2 text and data files on OS X and Linux
# See details in src/openrct2.c:openrct2_setup_rct2_segment for how the values
# were derived.
if (UNIX)
    set(OLOCO_EXE ${CMAKE_CURRENT_SOURCE_DIR}/loco.exe)
    set(OLOCO_TEXT ${CMAKE_BINARY_DIR}/openloco_text)
    set(OLOCO_DATA ${CMAKE_BINARY_DIR}/openloco_data)

    add_custom_command(
            OUTPUT ${OLOCO_TEXT}
            COMMAND dd if="${OLOCO_EXE}" of="${OLOCO_TEXT}" bs=4096 skip=1 count=214
            DEPENDS ${OLOCO_EXE}
    )
    add_custom_command(
            OUTPUT ${OLOCO_DATA}
            COMMAND dd if="${OLOCO_EXE}" of="${OLOCO_DATA}" bs=4096 skip=215 count=78
            COMMAND dd if=/dev/zero of="${OLOCO_DATA}" bs=4096 seek=78 count=3133 conv=notrunc
            DEPENDS ${OLOCO_EXE}
    )
    add_custom_target(segfiles DEPENDS ${OLOCO_TEXT} ${OLOCO_DATA})
    if (APPLE)
        set(LOCO_SEGMENT_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sectcreate loco_text __text \"${OLOCO_TEXT}\" -sectcreate loco_data __data ${OLOCO_DATA} -segaddr loco_data 0x4d7000 -segprot loco_data rwx rwx -segaddr loco_text 0x401000 -segprot loco_text rwx rwx -segaddr __TEXT 0x2000000 -read_only_relocs suppress")
    else ()
        # For Linux we have to use objcopy to wrap regular binaries into a linkable
        # format. We use specific section names which are then referenced in a
        # bespoke linker script so they can be placed at predefined VMAs.
        add_custom_command(
            OUTPUT openloco_text_section.o
            COMMAND objcopy --input binary --output ${OBJ_FORMAT} --binary-architecture i386 \"${OLOCO_TEXT}\" openloco_text_section.o --rename-section .data=.loco_text,contents,alloc,load,readonly,code
            DEPENDS segfiles
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_command(
            OUTPUT openloco_data_section.o
            COMMAND objcopy --input binary --output ${OBJ_FORMAT} --binary-architecture i386 \"${OLOCO_DATA}\" openloco_data_section.o --rename-section .data=.loco_data,contents,alloc,load,readonly,data
            DEPENDS segfiles
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_target(linkable_sections DEPENDS openloco_text_section.o openloco_data_section.o)
        set_source_files_properties(
            openloco_text_section.o openloco_data_section.o
            PROPERTIES
            EXTERNAL_OBJECT true
            GENERATED true
        )
        # can't use GLOB here, as the files don't exist yet at cmake-time
        set(LOCO_SECTIONS "${CMAKE_BINARY_DIR}/openloco_data_section.o" "${CMAKE_BINARY_DIR}/openloco_text_section.o")
        set(LOCO_SEGMENT_LINKER_FLAGS "-Wl,-T,\"${CMAKE_CURRENT_SOURCE_DIR}/distribution/linux/${LINKER_SCRIPT}\"")
    endif ()
endif (UNIX)

set(DEBUG_LEVEL 0 CACHE STRING "Select debug level for compilation. Use value in range 0–3.")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG=${DEBUG_LEVEL}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG=${DEBUG_LEVEL}")

# include lib
include_directories("lib/")
# add source files
file(GLOB_RECURSE OLOCO_SOURCES "src/*.cpp" "src/*.h" "src/*.hpp")


if (APPLE)
    file(GLOB_RECURSE OLOCO_MM_SOURCES "src/*.mm")
    set_source_files_properties(${OLOCO_MM_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++ -fmodules")
endif ()

set(PIE_FLAG "-fno-pie")
set(TARGET_M "-m32")

# set necessary flags to compile code as is
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_M} -std=gnu99 ${COMMON_COMPILE_OPTIONS} -Wimplicit")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M} ${COMMON_COMPILE_OPTIONS} -Wnon-virtual-dtor")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M} ${PIE_FLAG}")

PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
SET(SDL2LIBS ${SDL2_LDFLAGS})

find_package(yaml-cpp REQUIRED)

if (USE_BOOST_FILESYSTEM)
    set(Boost_USE_STATIC_LIBS OFF)
    find_package(Boost COMPONENTS filesystem)
    if (NOT Boost_FOUND)
        message("Dynamic Boost not found, retrying with static")
        set(Boost_USE_STATIC_LIBS ON)
        find_package(Boost COMPONENTS filesystem REQUIRED)
    endif ()
endif()

# Disable optimizations for interop.cpp for all compilers, to allow optimized
# builds without need for -fno-omit-frame-pointer
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/openloco/interop/interop.cpp" PROPERTIES COMPILE_FLAGS -fno-omit-frame-pointer)

set(CMAKE_CXX_STANDARD 17)

if (MINGW)
    add_library(${PROJECT} ${OLOCO_SOURCES})
    target_link_libraries(${PROJECT} winmm)
else ()
    add_executable(${PROJECT} ${OLOCO_SOURCES} ${OLOCO_MM_SOURCES} ${LOCO_SECTIONS})
    set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS ${LOCO_SEGMENT_LINKER_FLAGS})
endif ()

target_link_libraries(${PROJECT} SDL2 ${SDL2LIBS})
target_link_libraries(${PROJECT} yaml-cpp ${YAML_CPP_LIBRARIES})

if (NOT MINGW)
    add_dependencies(${PROJECT} segfiles)
endif()

target_compile_definitions(${PROJECT} PRIVATE _NO_LOCO_WIN32_=1)
if (USE_BOOST_FILESYSTEM)
    target_link_libraries(${PROJECT} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
    target_include_directories(${PROJECT} PRIVATE ${Boost_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT} PRIVATE BOOST_SYSTEM_NO_DEPRECATED=1 BOOST_FILESYSTEM_NO_DEPRECATED=1 _OPENLOCO_USE_BOOST_FS_=1)
else()
	target_link_libraries(${PROJECT} stdc++fs)
endif()

if (MINGW)
	target_compile_definitions(${PROJECT} PRIVATE _WIN32_WINNT=0x0600)
endif()

if (APPLE)
    target_link_libraries(${PROJECT} "-framework Cocoa")

    try_compile(SUPPORT_STD_BYTE ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/std_byte.cpp CXX_STANDARD 17)
    if (NOT SUPPORT_STD_BYTE)
        message(STATUS "std::byte is not supported.")
        target_compile_definitions(${PROJECT} PRIVATE COMPAT_STD_BYTE=1)
    endif ()
endif ()
