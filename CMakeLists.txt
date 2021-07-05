cmake_minimum_required(VERSION 3.9)
cmake_policy(VERSION 3.9)

set (PROJECT openloco)

# Note: Searching for CCache must be before project() so project() can use CCache too
# if it is available
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(CCache)

if (CCache_FOUND)
    option(OPENLOCO_USE_CCACHE "Use CCache to improve recompilation speed (optional)" ON)
    if (OPENLOCO_USE_CCACHE)
        # Use e.g. "ccache clang++" instead of "clang++"
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCache_EXECUTABLE}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    "${CCache_EXECUTABLE}")
    endif (OPENLOCO_USE_CCACHE)
endif (CCache_FOUND)

project(${PROJECT} CXX)

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
include(CheckCXXCompilerFlag)

option(STRICT "Build with warnings as errors" YES)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

# Describe current version in terms of closest tag
execute_process(
    COMMAND git describe HEAD --always
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
# only for the one file that uses these definitions: Version.cpp
set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/OpenLoco/Version.cpp
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

# add source files
file(GLOB_RECURSE OLOCO_SOURCES "src/*.cpp")
file(GLOB_RECURSE OLOCO_HEADERS "src/*.h" "src/*.hpp")

if (APPLE)
    file(GLOB_RECURSE OLOCO_MM_SOURCES "src/*.mm")
    set_source_files_properties(${OLOCO_MM_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif ()

set(PIE_FLAG "-fno-pie")
set(TARGET_M "-m32")

# Check if a flag exists and add it to the list of compiler (so, not linker) options
function (ADD_CHECK_CXX_COMPILER_FLAG _CXXFLAGS _CACHE_VAR _FLAG)
    CHECK_CXX_COMPILER_FLAG("${_FLAG}" "${_CACHE_VAR}")
    if (${_CACHE_VAR})
        list(APPEND SUPPORTED_CHECK_CXX_COMPILER_FLAGS "${_FLAG}")
    else ()
        message(STATUS "Unsupported CXXFLAG: ${_FLAG}")
    endif ()
endfunction ()

# Check if a flag exists and add it to the compiler and the linker options
function (ADD_CHECK_CXX_FLAG _CXXFLAGS _CACHE_VAR _FLAG)
    CHECK_CXX_COMPILER_FLAG("${_FLAG}" "${_CACHE_VAR}")
    if (${_CACHE_VAR})
        set(${_CXXFLAGS} "${${_CXXFLAGS}} ${_FLAG}" PARENT_SCOPE)
    else ()
        message(STATUS "Unsupported CXXFLAG: ${_FLAG}")
    endif ()
endfunction ()

ADD_CHECK_CXX_COMPILER_FLAG(CMAKE_CXX_FLAGS CXX_WARN_NON_VIRTUAL_DTOR -Wnon-virtual-dtor)
ADD_CHECK_CXX_FLAG(CMAKE_CXX_FLAGS CXX_WNO_CLOBBERED -Wno-clobbered)

# set necessary flags to compile code as is
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_M} -std=gnu99 ${COMMON_COMPILE_OPTIONS} -Wimplicit")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M} ${COMMON_COMPILE_OPTIONS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M} ${PIE_FLAG}")

find_package(SDL2 REQUIRED)
if (NOT SDL2_LIBRARIES)
	if (TARGET SDL2::SDL2-static)
		set(SDL2_LIBRARIES SDL2::SDL2-static)
	elseif (TARGET SDL2::SDL2)
		set(SDL2_LIBRARIES SDL2::SDL2)
	endif()
endif()

find_package(SDL2_mixer REQUIRED)

find_package(PNG REQUIRED)

find_package(yaml-cpp REQUIRED HINTS /usr/lib32/cmake/yaml-cpp)
include_directories(${YAML_CPP_INCLUDE_DIR})

# Disable optimizations for interop.cpp for all compilers, to allow optimized
# builds without need for -fno-omit-frame-pointer
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/OpenLoco/Interop/Interop.cpp" PROPERTIES COMPILE_FLAGS "-fno-omit-frame-pointer -O0")

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if (MINGW)
    add_library(${PROJECT} ${OLOCO_SOURCES})
    target_link_libraries(${PROJECT} winmm)
else ()
    add_executable(${PROJECT} ${OLOCO_SOURCES} ${OLOCO_MM_SOURCES} ${LOCO_SECTIONS})
    set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS ${LOCO_SEGMENT_LINKER_FLAGS})
endif ()

target_link_libraries(${PROJECT} ${SDL2_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
target_link_libraries(${PROJECT} yaml-cpp ${YAML_CPP_LIBRARIES})
target_link_libraries(${PROJECT} ${PNG_LIBRARIES})


if (NOT MINGW)
    add_dependencies(${PROJECT} segfiles)
endif()

if (NOT APPLE)
    target_link_libraries(${PROJECT} stdc++fs)
endif()

if (MINGW)
    target_compile_definitions(${PROJECT} PRIVATE _WIN32_WINNT=0x0501 SDL_MAIN_HANDLED=1)
    target_link_libraries(${PROJECT} imm32 winmm version)
else ()
    target_compile_definitions(${PROJECT} PRIVATE _NO_LOCO_WIN32_=1)
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

    set(BUNDLE_RESOURCES "")

    find_program(ICONUTIL iconutil)
    if (ICONUTIL)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/distribution/macos/AppIcon.iconset DESTINATION ${CMAKE_BINARY_DIR})
        set(ICON_TARGET "${CMAKE_BINARY_DIR}/AppIcon.iconset")
        set(ICON_OUTPUT "${CMAKE_BINARY_DIR}/AppIcon.icns")
        set(SOURCE_ICON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo")
        add_custom_command(OUTPUT ${ICON_OUTPUT}
                COMMAND cp icon_x16.png ${ICON_TARGET}/icon_16x16.png
                COMMAND cp icon_x32.png ${ICON_TARGET}/icon_16x16@2x.png
                COMMAND cp icon_x32.png ${ICON_TARGET}/icon_32x32.png
                COMMAND cp icon_x64.png ${ICON_TARGET}/icon_32x32@2x.png
                COMMAND cp icon_x128.png ${ICON_TARGET}/icon_128x128.png
                COMMAND cp icon_x256.png ${ICON_TARGET}/icon_128x128@2x.png
                COMMAND cp icon_x256.png ${ICON_TARGET}/icon_256x256.png
                COMMAND cp icon_x512.png ${ICON_TARGET}/icon_256x256@2x.png
                COMMAND cp icon_x512.png ${ICON_TARGET}/icon_512x512.png
                COMMAND cp icon_x1024.png ${ICON_TARGET}/icon_512x512@2x.png
                COMMAND ${ICONUTIL} --convert icns --output ${ICON_OUTPUT} ${ICON_TARGET}
                WORKING_DIRECTORY ${SOURCE_ICON_DIR})
        list(APPEND BUNDLE_RESOURCES ${ICON_OUTPUT})
    endif ()

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
else ()
    # Dummy target to ease invocation
    add_custom_target(${PROJECT}-headers-check)
endif ()

install(FILES "distribution/linux/openloco.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
install(FILES "resources/logo/icon_x16.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps" RENAME "openloco.png")
install(FILES "resources/logo/icon_x32.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps" RENAME "openloco.png")
install(FILES "resources/logo/icon_x64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps" RENAME "openloco.png")
install(FILES "resources/logo/icon_x128.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME "openloco.png")
install(FILES "resources/logo/icon_x256.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME "openloco.png")
install(FILES "resources/logo/icon_x512.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps" RENAME "openloco.png")
install(FILES "resources/logo/icon_steam.svg" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps" RENAME "openloco.svg")
