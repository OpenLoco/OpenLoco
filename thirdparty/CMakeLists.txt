# TODO: look into using FetchContent's FIND_PACKAGE_ARGS when next increasing CMake version
find_package(yaml-cpp QUIET)
if ( NOT yaml-cpp_FOUND )
    ## yaml-cpp
    # Get rid of yaml uninstall target
    add_library(uninstall INTERFACE)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "")
    set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "")
    set(YAML_CPP_INSTALL OFF CACHE BOOL "")
    set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "")

    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY      https://github.com/jbeder/yaml-cpp
        GIT_TAG             yaml-cpp-0.9.0
    )
    FetchContent_MakeAvailable(yaml-cpp)

    # Group all the ThirdParty items under a ThirdParty folder in IDEs
    set_target_properties(yaml-cpp PROPERTIES FOLDER "ThirdParty")
else()
    if (NOT TARGET yaml-cpp::yaml-cpp)
        # Some system versions of yaml-cpp don't have the proper target
        # so we are defining it so we can follow yaml-cpp's conventions
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
    endif()
endif()

if (${OPENLOCO_BUILD_TESTS})
    find_package(GTest REQUIRED)
endif()

find_package(SDL3 REQUIRED CONFIG)

find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED) # Dependency of PNG not really required but vcpkg not mapping static png dependencies
if (NOT APPLE AND NOT MSVC)
    find_package(OpenAL CONFIG)
    if (OpenAL_FOUND)
        set(OPENAL_LIBRARIES OpenAL::OpenAL)
    else()
        pkg_check_modules(OPENAL REQUIRED openal)
    endif()
else()
    find_package(OpenAL CONFIG REQUIRED)
    set(OPENAL_LIBRARIES OpenAL::OpenAL)
endif()

# breakpad needs ATL (not in all VS Build Tools installs)
if (MSVC)
    find_package(unofficial-breakpad CONFIG)
    if (unofficial-breakpad_FOUND)
        set(BREAKPAD_LIBRARIES unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client CACHE INTERNAL "Breakpad libs")
    else()
        set(BREAKPAD_LIBRARIES "" CACHE INTERNAL "Breakpad libs")
        message(STATUS "Breakpad not found; crash dumps disabled")
    endif()
endif()

find_package(fmt QUIET)
if ( NOT fmt_FOUND )	
    ## fmt	
    FetchContent_Declare(	
        fmt	
        GIT_REPOSITORY      https://github.com/fmtlib/fmt	
        GIT_TAG             11.1.4	
        GIT_SHALLOW         ON	
    )	
    FetchContent_MakeAvailable(fmt)	

    # Group all the ThirdParty items under a ThirdParty folder in IDEs	
    set_target_properties(fmt PROPERTIES FOLDER "ThirdParty")	
endif()

find_package(sfl QUIET)
if ( NOT sfl_FOUND )
    ## sfl
    FetchContent_Declare(
        sfl
        GIT_REPOSITORY      https://github.com/slavenf/sfl-library
        GIT_TAG             2.2.0
        GIT_SHALLOW         ON
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(sfl)
endif()

if (HAS_LIBSTDCPP)
    find_package(TBB)
    if (NOT TBB_FOUND )
        message(WARNING "TBB could not be found but using libstdc++ multithreding may be broken")
    endif()
endif()

# stb_truetype is a single public-domain header; download it instead of vendoring ~5k lines.
set(STB_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/stb")
set(_stb_truetype_h "${STB_INCLUDE_DIR}/stb_truetype.h")
if (NOT EXISTS "${_stb_truetype_h}")
    file(MAKE_DIRECTORY "${STB_INCLUDE_DIR}")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/nothings/stb/f1c79c02822848a9bed4315b12c8c8f3761e1296/stb_truetype.h"
        "${_stb_truetype_h}"
        EXPECTED_HASH SHA256=ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab
        TLS_VERIFY ON
        STATUS _stb_status
    )
    list(GET _stb_status 0 _stb_code)
    if (NOT _stb_code EQUAL 0)
        message(FATAL_ERROR "Failed to download stb_truetype.h: ${_stb_status}")
    endif()
endif()
