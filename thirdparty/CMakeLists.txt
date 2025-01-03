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
        GIT_TAG             1b50109f7bea60bd382d8ea7befce3d2bd67da5f
    )
    FetchContent_MakeAvailable(yaml-cpp)
    loco_thirdparty_target_compile_link_flags(yaml-cpp)

    # Group all the ThirdParty items under a ThirdParty folder in IDEs
    set_target_properties(yaml-cpp PROPERTIES FOLDER "ThirdParty")
endif()

if (${OPENLOCO_BUILD_TESTS})
    find_package(GTest REQUIRED)
endif()

find_package(SDL2 REQUIRED CONFIG)
if (TARGET SDL2::SDL2-static)
    set(SDL2_LIB SDL2::SDL2-static)
elseif (TARGET SDL2::SDL2)
    set(SDL2_LIB SDL2::SDL2)
else ()
    # Even though we asked for SDL2 from a configure mode which would have SDL2 targets
    # it appears we have been given a raw module mode SDL2 so create a wrapper library
    # then we can treat it just like a normal configure mode target
    add_library(SDL2_WRAPPER INTERFACE)
    target_link_libraries(SDL2_WRAPPER INTERFACE ${SDL2_LIBRARIES})
    target_include_directories(SDL2_WRAPPER INTERFACE ${SDL2_INCLUDE_DIRS})
    set(SDL2_LIB SDL2_WRAPPER)
endif()

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

# breakpad is also required
if (MSVC)
    find_package(unofficial-breakpad CONFIG REQUIRED)
    set(BREAKPAD_LIBRARIES unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client)
endif()

find_package(fmt QUIET)
if ( NOT fmt_FOUND )
    ## fmt
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY      https://github.com/fmtlib/fmt
        GIT_TAG             10.1.1
        GIT_SHALLOW         ON
    )
    FetchContent_MakeAvailable(fmt)
    
    loco_thirdparty_target_compile_link_flags(fmt)
    
    # Group all the ThirdParty items under a ThirdParty folder in IDEs
    set_target_properties(fmt PROPERTIES FOLDER "ThirdParty")
endif()

find_package(sfl QUIET)
if ( NOT sfl_FOUND )
    ## sfl
    FetchContent_Declare(
        sfl
        GIT_REPOSITORY      https://github.com/slavenf/sfl-library
        GIT_TAG             1.9.0
        GIT_SHALLOW         ON
    )
    FetchContent_MakeAvailable(sfl)
    
    # Group all the ThirdParty items under a ThirdParty folder in IDEs
    set_target_properties(sfl PROPERTIES FOLDER "ThirdParty")
endif()
