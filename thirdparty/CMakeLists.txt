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
        GIT_TAG             c2680200486572baf8221ba052ef50b58ecd816e # 0.8.0 + cmake fix patch 
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

# Emscripten-specific dependency handling
# Use CMAKE_SYSTEM_NAME to detect Emscripten as it's set in the toolchain file
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # For Emscripten builds, use Emscripten ports for SDL3 and libraries
    # The actual linking is done via -sUSE_SDL=3, etc. in the toolchain
    # We create imported targets with the proper link options
    
    # SDL3 - Emscripten port provides SDL3
    if(NOT TARGET SDL3::SDL3)
        add_library(SDL3::SDL3 INTERFACE IMPORTED)
        # The -sUSE_SDL=3 flag is in the toolchain linker flags
        
        # Add SDL3 include path - try multiple sources
        if(EMSCRIPTEN_SDL3_INCLUDE_DIR AND EXISTS "${EMSCRIPTEN_SDL3_INCLUDE_DIR}/SDL3/SDL.h")
            target_include_directories(SDL3::SDL3 INTERFACE "${EMSCRIPTEN_SDL3_INCLUDE_DIR}")
            message(STATUS "Using SDL3 include dir from toolchain: ${EMSCRIPTEN_SDL3_INCLUDE_DIR}")
        elseif(DEFINED ENV{EMSDK})
            # Fallback: try to find SDL3 headers directly from Emscripten SDK
            set(EMSDK_ROOT "$ENV{EMSDK}/upstream/emscripten")
            set(SDL3_SEARCH_PATHS
                "${EMSDK_ROOT}/cache/sysroot/include"
                "${EMSDK_ROOT}/system/include"
                "${EMSDK_ROOT}/include"
            )
            set(SDL3_FOUND FALSE)
            foreach(SDL3_PATH ${SDL3_SEARCH_PATHS})
                if(EXISTS "${SDL3_PATH}/SDL3/SDL.h")
                    target_include_directories(SDL3::SDL3 INTERFACE "${SDL3_PATH}")
                    message(STATUS "Using SDL3 include dir (fallback): ${SDL3_PATH}")
                    set(SDL3_FOUND TRUE)
                    break()
                endif()
            endforeach()
            if(NOT SDL3_FOUND)
                message(WARNING "SDL3 headers not found! Build may fail. Ensure SDL3 port is installed.")
            endif()
        endif()
    endif()
    
    # fmt - For Emscripten, use header-only mode
    # Fetch fmt headers but don't build the library, use FMT_HEADER_ONLY
    if(NOT TARGET fmt::fmt)
        FetchContent_Declare(	
            fmt	
            GIT_REPOSITORY      https://github.com/fmtlib/fmt	
            GIT_TAG             11.1.4	
            GIT_SHALLOW         ON	
        )	
        FetchContent_Populate(fmt)
        
        add_library(fmt::fmt INTERFACE IMPORTED)
        target_compile_definitions(fmt::fmt INTERFACE FMT_HEADER_ONLY)
        target_include_directories(fmt::fmt INTERFACE ${fmt_SOURCE_DIR}/include)
    endif()
    
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB INTERFACE IMPORTED)
    endif()
    if(NOT TARGET PNG::PNG)
        add_library(PNG::PNG INTERFACE IMPORTED)
    endif()

    set(PNG_LIBRARIES PNG::PNG)

else()
    # Native platform dependency handling
    find_package(SDL3 REQUIRED CONFIG)

    find_package(PNG REQUIRED)
    set(PNG_LIBRARIES PNG::PNG)
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
endif()

# breakpad is also required
if (MSVC)
    find_package(unofficial-breakpad CONFIG REQUIRED)
    set(BREAKPAD_LIBRARIES unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client)
endif()

# fmt library is required for formatting operations
# For Emscripten builds, we use the interface target created above
# For native builds, we find or fetch fmt
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
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
