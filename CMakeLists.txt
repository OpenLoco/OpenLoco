cmake_minimum_required(VERSION 3.22)
cmake_policy(VERSION 3.22)

# IPO warning.
cmake_policy(SET CMP0069 NEW) 
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

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

if (WIN32)
    set(OPENLOCO_USE_VCPKG_DEFAULT YES)
else()
    set(OPENLOCO_USE_VCPKG_DEFAULT NO)
endif()
option(OPENLOCO_USE_VCPKG "Use vcpkg for dependencies and toolchain" ${OPENLOCO_USE_VCPKG_DEFAULT})


# On windows VCPKG is used to get all dependencies other platforms can use it as well by setting OPENLOCO_USE_VCPKG
if (OPENLOCO_USE_VCPKG)
    if (EXISTS ${VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    else()
        # Provide a message for first time invocations
        if (NOT CMAKE_TOOLCHAIN_FILE)
            message("VCPKG not installed, fetching and building VCPKG. Warning may take a while on slow connections.")
        endif ()
        FetchContent_Declare(vcpkg
            GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
            GIT_TAG 89dc8be6dbcf18482a5a1bf86a2f4615c939b0fb)
        FetchContent_MakeAvailable(vcpkg)

        set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "CMake toolchain file")
    endif()
endif ()

project(${PROJECT} LANGUAGES CXX VERSION 25.11)

# Enable IPO for all Release targets by default
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(ipo_supported)
    # Enable IPO globally for all Release builds
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    message(STATUS "IPO/LTO enabled for all Release targets.")
else()
    message(WARNING "IPO/LTO not supported: ${ipo_error}")
endif()

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

include(FindPkgConfig)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)
include(OpenLocoCommon)
include(OpenLocoUtility)

option(STRICT "Build with warnings as errors" YES)
option(OPENLOCO_BUILD_TESTS "Build tests" YES)
option(OPENLOCO_HEADER_CHECK "Verify all public interfaces are standalone" NO)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

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

FetchContent_Declare(openloco_objects
            URL      https://github.com/OpenLoco/OpenGraphics/releases/download/v0.1.5/objects.zip
            URL_HASH SHA256=fe8943fabad8eb07cebab5354589abd7e798a705f7993bb4d9dab2122b4fe96e)
FetchContent_MakeAvailable(openloco_objects)

add_subdirectory(src)

if (NOT APPLE)
    if (${CMAKE_VERSION} VERSION_GREATER 3.26)
        # Uses the copy_directory_if_different to avoid copying objects/language every time (3.26 feature)
        add_custom_target(OpenLocoAssets ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:OpenLoco>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${openloco_objects_SOURCE_DIR}" "$<TARGET_FILE_DIR:OpenLoco>/data/objects"
            COMMENT "Copying over data and objects folder"
            DEPENDS OpenLoco
        )
    else()
        add_custom_target(OpenLocoAssets ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:OpenLoco>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${openloco_objects_SOURCE_DIR}" "$<TARGET_FILE_DIR:OpenLoco>/data/objects"
            COMMENT "Copying over data and objects folder"
            DEPENDS OpenLoco
        )
    endif()
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OpenLoco)

install(TARGETS OpenLoco
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION "${CMAKE_INSTALL_BINDIR}")

if (UNIX AND NOT APPLE)
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
endif()

if (NOT APPLE)
    install(DIRECTORY "data/" DESTINATION "${CMAKE_INSTALL_BINDIR}/data")
    install(DIRECTORY "${openloco_objects_SOURCE_DIR}" DESTINATION "${CMAKE_INSTALL_BINDIR}/data/objects")
endif()
