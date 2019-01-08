cmake_minimum_required(VERSION 3.9)
cmake_policy(VERSION 3.9)

set (PROJECT openloco)

project(${PROJECT} C CXX)

if (APPLE)
    # Detection of this variable seems to fail with CMake.
    # Since we only support 32-bit builds at the moment, fix it this way.
    # TODO: find out a proper fix for this.
    set(CMAKE_SIZEOF_VOID_P 4)
endif()

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

INCLUDE(FindPkgConfig)

set(USE_BOOST_FS_DEFAULT OFF)
if(APPLE OR MINGW)
	set(USE_BOOST_FS_DEFAULT ON)
endif()

option(USE_BOOST_FILESYSTEM "Use Boost filesystem instead of C++17" ${USE_BOOST_FS_DEFAULT})
option(STRICT "Build with warnings as errors" YES)
option(I386 "Compile for native x86" ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

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
set(COMMON_COMPILE_OPTIONS "${COMMON_COMPILE_OPTIONS} -fstrict-aliasing -Wall -Wextra -Wno-unused-parameter")

if (STRICT)
    set(COMMON_COMPILE_OPTIONS "${COMMON_COMPILE_OPTIONS} -Werror")
endif ()

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

if (I386)
    set(TARGET_M "-m32")
endif ()

# Handle creating the rct2 text and data files on OS X and Linux
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
        set(LOCO_SEGMENT_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -Xlinker -map -Xlinker \"${CMAKE_BINARY_DIR}/openloco.map\"")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -sectcreate loco_text __text \"${OLOCO_TEXT}\"")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -sectcreate loco_data __data \"${OLOCO_DATA}\"")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -segaddr loco_data 0x4d7000")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -segprot loco_data rwx rwx")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -segaddr loco_text 0x401000")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -segprot loco_text rwx rwx")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -segaddr __TEXT 0x2000000")
        set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -read_only_relocs suppress")
        if (NOT I386)
            set(LOCO_SEGMENT_LINKER_FLAGS "${LOCO_SEGMENT_LINKER_FLAGS} -pagezero_size 10000 -image_base 100000000")
        endif()
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
file(GLOB_RECURSE OLOCO_SOURCES "src/openloco/*.cpp")
file(GLOB_RECURSE OLOCO_HEADERS "src/openloco/*.h" "src/openloco/*.hpp")


if (APPLE)
    file(GLOB_RECURSE OLOCO_MM_SOURCES "src/*.mm")
    set_source_files_properties(${OLOCO_MM_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++ -fmodules")
endif ()

set(PIE_FLAG "-fno-pie")

# set necessary flags to compile code as is
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_M} -std=gnu99 ${COMMON_COMPILE_OPTIONS} -Wimplicit")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M} ${COMMON_COMPILE_OPTIONS} -Wnon-virtual-dtor")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M} ${PIE_FLAG}")

file(GLOB_RECURSE LIB_SRCS "src/x86emu/*.c")
add_library(x86emu ${LIB_SRCS})
target_include_directories(x86emu INTERFACE "src/x86emu/include")

find_package(SDL2 REQUIRED)
if (NOT SDL2_LIBRARIES)
	if (TARGET SDL2::SDL2-static)
		set(SDL2_LIBRARIES SDL2::SDL2-static)
	elseif (TARGET SDL2::SDL2)
		set(SDL2_LIBRARIES SDL2::SDL2)
	endif()
endif()

find_package(SDL2_mixer REQUIRED)

# The hint provided here is targetting Arch Linux, a distro of choice for many contributors
find_package(yaml-cpp REQUIRED HINTS /usr/lib32/cmake/yaml-cpp)

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
    target_link_libraries(${PROJECT} x86emu)
    set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS ${LOCO_SEGMENT_LINKER_FLAGS})
endif ()

target_link_libraries(${PROJECT} ${SDL2_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
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
    target_compile_definitions(${PROJECT} PRIVATE _WIN32_WINNT=0x0600 SDL_MAIN_HANDLED=1)
endif()

if (APPLE)
    target_link_libraries(${PROJECT} "-framework Cocoa")

    set_target_properties(${PROJECT} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/distribution/macos/Info.plist.in)

    set(MACOSX_BUNDLE_BUNDLE_NAME "OpenLoco")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${OPENRCT2_COMMIT_SHA1_SHORT}")
    set(MACOSX_BUNDLE_COPYRIGHT "OpenLoco is licensed under the MIT License")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.openloco.OpenLoco")
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon")

    if(${OPENLOCO_BRANCH} EQUAL master)
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG}")
    else()
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG} ${OPENLOCO_BRANCH}")
    endif()

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/distribution/macos/AppIcon.iconset DESTINATION ${CMAKE_BINARY_DIR})
    set(ICON_TARGET "${CMAKE_BINARY_DIR}/AppIcon.iconset")
    set(ICON_OUTPUT "${CMAKE_BINARY_DIR}/AppIcon.icns")
    set(SOURCE_ICON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo")

    add_custom_command(OUTPUT ${ICON_OUTPUT}
        COMMAND cp icon_x16.png   ${ICON_TARGET}/icon_16x16.png
        COMMAND cp icon_x32.png   ${ICON_TARGET}/icon_16x16@2x.png
        COMMAND cp icon_x32.png   ${ICON_TARGET}/icon_32x32.png
        COMMAND cp icon_x64.png   ${ICON_TARGET}/icon_32x32@2x.png
        COMMAND cp icon_x128.png  ${ICON_TARGET}/icon_128x128.png
        COMMAND cp icon_x256.png  ${ICON_TARGET}/icon_128x128@2x.png
        COMMAND cp icon_x256.png  ${ICON_TARGET}/icon_256x256.png
        COMMAND cp icon_x512.png  ${ICON_TARGET}/icon_256x256@2x.png
        COMMAND cp icon_x512.png  ${ICON_TARGET}/icon_512x512.png
        COMMAND cp icon_x1024.png ${ICON_TARGET}/icon_512x512@2x.png
        COMMAND iconutil -c icns ${ICON_TARGET}
        WORKING_DIRECTORY ${SOURCE_ICON_DIR})

    set(BUNDLE_RESOURCES ${ICON_OUTPUT})
    list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md)
    list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CONTRIBUTORS.MD)
    set_target_properties(${PROJECT} PROPERTIES RESOURCE "${BUNDLE_RESOURCES}")
    target_sources(${PROJECT} PUBLIC ${BUNDLE_RESOURCES})

    file(GLOB BUNDLE_LANGUAGES "${CMAKE_CURRENT_SOURCE_DIR}/data/language/*")
    target_sources(${PROJECT} PUBLIC ${BUNDLE_LANGUAGES})
    set_property(
        SOURCE ${BUNDLE_LANGUAGES}
        PROPERTY MACOSX_PACKAGE_LOCATION "Resources/language"
    )

    try_compile(SUPPORT_STD_BYTE ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/std_byte.cpp CXX_STANDARD 17)
    if (NOT SUPPORT_STD_BYTE)
        message(STATUS "std::byte is not supported.")
        target_compile_definitions(${PROJECT} PRIVATE COMPAT_STD_BYTE=1)
    endif ()
endif ()

# Add headers check to verify all headers carry their dependencies.
# Only valid for Clang for now:
# - GCC 8 does not support -Wno-pragma-once-outside-header
# - Other compilers status unknown
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_library(${PROJECT}-headers-check OBJECT ${OLOCO_HEADERS})
    set_target_properties(${PROJECT}-headers-check PROPERTIES LINKER_LANGUAGE CXX)
	set_source_files_properties(${OLOCO_HEADERS} PROPERTIES LANGUAGE CXX)
    add_definitions("-x c++ -Wno-pragma-once-outside-header -Wno-unused-const-variable")
	get_target_property(OPENLOCO_INCLUDE_DIRS ${PROJECT} INCLUDE_DIRECTORIES)
	set_target_properties(${PROJECT}-headers-check PROPERTIES INCLUDE_DIRECTORIES "${OPENLOCO_INCLUDE_DIRS}")
else ()
    # Dummy target to ease invocation
    add_custom_target(${PROJECT}-headers-check)
endif ()
