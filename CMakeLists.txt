cmake_minimum_required(VERSION 3.22)
cmake_policy(VERSION 3.22)

set (PROJECT openloco)

# Store the top level source directory in a variable to be passed down to targets.
set(OPENLOCO_PROJECT_PATH ${CMAKE_SOURCE_DIR})

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

include(FetchContent)

# On windows VCPKG is used to get all dependencies other platforms can use it as well by specifying it
# in the CMAKE_TOOLCHAIN_FILE
if (WIN32)
    if (EXISTS ${VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    else()
        # Provide a message for first time invocations
        if (NOT CMAKE_TOOLCHAIN_FILE)
            message("VCPKG not installed on windows fetching and building VCPKG. Warning may take a while on slow connections.")
        endif ()
        FetchContent_Declare(vcpkg
            GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
            GIT_TAG 1de2026f28ead93ff1773e6e680387643e914ea1)
        FetchContent_MakeAvailable(vcpkg)

        set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "CMake toolchain file")
    endif()
endif ()

project(${PROJECT} LANGUAGES CXX VERSION 24.12)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if (APPLE)
    # Detection of this variable seems to fail with CMake.
    # Since we only support 32-bit builds at the moment, fix it this way.
    # TODO: find out a proper fix for this.
    set(CMAKE_SIZEOF_VOID_P 4)
endif()

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

include(FindPkgConfig)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)
include(OpenLocoUtility)

option(STRICT "Build with warnings as errors" YES)
option(OPENLOCO_BUILD_TESTS "Build tests" YES)
option(OPENLOCO_HEADER_CHECK "Verify all public interfaces are standalone" NO)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set(DEBUG_LEVEL 0 CACHE STRING "Select debug level for compilation. Use value in range 0–3.")
list(APPEND COMMON_COMPILE_OPTIONS -DDEBUG=${DEBUG_LEVEL})

if (MINGW)
    # MingW32 does not at present compile OpenLoco.exe
    # so we must ensure the .dll file has the same
    # prefix on Windows and MingW32 i.e. none
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Display source files nicely in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# System dependencies
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Thirdparty dependencies
include(thirdparty/CMakeLists.txt)

if (${OPENLOCO_BUILD_TESTS})
    enable_testing()
endif()

# Handle creating the rct2 text and data files on OS X and Linux
if (UNIX)
    set(OBJ_FORMAT "elf32-i386")
    set(LINKER_SCRIPT "ld_script_i386.xc")

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
endif()

FetchContent_Declare(openloco_objects
            URL      https://github.com/OpenLoco/OpenGraphics/releases/download/v0.1.0/objects.zip
            URL_HASH SHA256=3fd5b10681d8f2f9842213d8fca98222485f509193894af15c290abd29a28126)
FetchContent_MakeAvailable(openloco_objects)

add_subdirectory(src)

if (MSVC)
    if (${CMAKE_VERSION} VERSION_GREATER 3.26)
        # Uses the copy_directory_if_different to avoid copying objects/language every time (3.26 feature)
        add_custom_target(OpenLocoExe ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/loco.exe" "$<TARGET_FILE_DIR:OpenLoco>/OpenLoco.exe"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:OpenLoco>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${openloco_objects_SOURCE_DIR}" "$<TARGET_FILE_DIR:OpenLoco>/data/objects"
            COMMENT "Copying over data folder and executable"
            DEPENDS OpenLoco
        )
    else()
        add_custom_target(OpenLocoExe ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/loco.exe" "$<TARGET_FILE_DIR:OpenLoco>/OpenLoco.exe"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:OpenLoco>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${openloco_objects_SOURCE_DIR}" "$<TARGET_FILE_DIR:OpenLoco>/data/objects"
            COMMENT "Copying over data folder and executable"
            DEPENDS OpenLoco
        )
    endif()
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OpenLocoExe)
    set_property(TARGET OpenLocoExe PROPERTY VS_DEBUGGER_COMMAND $<TARGET_FILE_DIR:OpenLoco>/OpenLoco.exe)
    set_property(TARGET OpenLocoExe PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:OpenLoco>)
endif()

install(TARGETS OpenLoco
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "CHANGELOG.md" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(FILES "CONTRIBUTORS.md" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(FILES "LICENSE" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(FILES "README.md" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(FILES "distribution/linux/openloco.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
install(FILES "src/Resources/src/logo/icon_x16.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps" RENAME "openloco.png")
install(FILES "src/Resources/src/logo/icon_x32.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps" RENAME "openloco.png")
install(FILES "src/Resources/src/logo/icon_x64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps" RENAME "openloco.png")
install(FILES "src/Resources/src/logo/icon_x128.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps" RENAME "openloco.png")
install(FILES "src/Resources/src/logo/icon_x256.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME "openloco.png")
install(FILES "src/Resources/src/logo/icon_x512.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps" RENAME "openloco.png")
install(FILES "src/Resources/src/logo/icon_steam.svg" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps" RENAME "openloco.svg")
install(DIRECTORY "data/" DESTINATION "${CMAKE_INSTALL_BINDIR}/data")
install(DIRECTORY "${openloco_objects_SOURCE_DIR}" DESTINATION "${CMAKE_INSTALL_BINDIR}/data/objects")
