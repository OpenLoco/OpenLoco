cmake_minimum_required(VERSION 2.6)

set (PROJECT openloco)

project(${PROJECT})

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

INCLUDE(FindPkgConfig)

if (APPLE)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/opt/openssl/lib/pkgconfig")
endif (APPLE)

set(COMMON_COMPILE_OPTIONS "${COMMON_COMPILE_OPTIONS} -fstrict-aliasing -Werror -Wundef -Wmissing-declarations -Winit-self -Warray-bounds -Waddress -Wchar-subscripts -Wenum-compare")

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
            OUTPUT openloco_text
            COMMAND dd if="${OLOCO_EXE}" of="${OLOCO_TEXT}" bs=4096 skip=1 count=214
            DEPENDS ${OLOCO_EXE}
    )
    add_custom_command(
            OUTPUT openloco_data
            COMMAND dd if="${OLOCO_EXE}" of="${OLOCO_DATA}" bs=4096 skip=215 count=78
            COMMAND dd if=/dev/zero of="${OLOCO_DATA}" bs=4096 seek=78 count=3133 conv=notrunc
            DEPENDS ${OLOCO_EXE}
    )
    add_custom_target(segfiles DEPENDS openloco_text openloco_data)
    if (APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sectcreate loco_text __text ${OLOCO_TEXT} -sectcreate loco_data __data ${OLOCO_DATA} -segaddr loco_data 0x4d7000 -segprot loco_data rwx rwx -segaddr loco_text 0x401000 -segprot loco_text rwx rwx -segaddr __TEXT 0x2000000 -read_only_relocs suppress")
    else ()
        # For Linux we have to use objcopy to wrap regular binaries into a linkable
        # format. We use specific section names which are then referenced in a
        # bespoke linker script so they can be placed at predefined VMAs.
        add_custom_command(
            OUTPUT openloco_text_section.o
            COMMAND objcopy --input binary --output ${OBJ_FORMAT} --binary-architecture i386 openloco_text openloco_text_section.o --rename-section .data=.loco_text,contents,alloc,load,readonly,code
            DEPENDS segfiles
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_command(
            OUTPUT openloco_data_section.o
            COMMAND objcopy --input binary --output ${OBJ_FORMAT} --binary-architecture i386 openloco_data openloco_data_section.o --rename-section .data=.loco_data,contents,alloc,load,readonly,data
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
		set(LOCO_SEGMENT_LINKER_FLAGS "-Wl,-T,\"${ROOT_DIR}/distribution/linux/${LINKER_SCRIPT}\"")
    endif ()
endif (UNIX)

set(DEBUG_LEVEL 0 CACHE STRING "Select debug level for compilation. Use value in range 0–3.")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG=${DEBUG_LEVEL}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG=${DEBUG_LEVEL}")

# include lib
include_directories("lib/")
# add source files
file(GLOB_RECURSE ORCT2_SOURCES "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")

set(PIE_FLAG "-fno-pie")

set(TARGET_M "-m32")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_M}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")

# set necessary flags to compile code as is
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_M} -std=gnu99 ${COMMON_COMPILE_OPTIONS} -Wimplicit")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_M} -std=gnu++11 ${COMMON_COMPILE_OPTIONS} -Wnon-virtual-dtor -std=c++1z")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TARGET_M}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS} ${PIE_FLAG}")

PKG_CHECK_MODULES(SDL2 REQUIRED sdl2)
SET(SDL2LIBS ${SDL2_DEFINITIONS})

set(Boost_USE_STATIC_LIBS ON)
set(BOOST_INCLUDEDIR "/Users/Marijn/Downloads/boost_1_66_0")
set(BOOST_LIBRARYDIR "/Users/Marijn/Downloads/boost_1_66_0/stage/lib")
find_package(Boost COMPONENTS filesystem REQUIRED)

add_executable(${PROJECT} ${ORCT2_SOURCES} ${ORCT2_MM_SOURCES} ${LOCO_SECTIONS})
add_dependencies(${PROJECT} segfiles)
target_link_libraries(${PROJECT} SDL2 ${SDL2LIBS} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
if(EXISTS /Users/Marijn/Downloads/SDL2-2.0.7/build/libSDL2.dylib)
	target_link_libraries(${PROJECT} /Users/Marijn/Downloads/SDL2-2.0.7/build/libSDL2.dylib "/Users/Marijn/Downloads/boost_1_66_0/stage/lib/libboost_filesystem.a")
endif()
