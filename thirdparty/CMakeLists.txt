# TODO: look into using FetchContent's FIND_PACKAGE_ARGS when next increasing CMake version
find_package(yaml-cpp)
find_package(span-lite)

if ( NOT yaml-cpp_FOUND AND NOT span-lite_FOUND ) 
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

    ## span-lite
    FetchContent_Declare(
        span-lite
        GIT_REPOSITORY      https://github.com/martinmoene/span-lite
        GIT_TAG             v0.10.3
        GIT_SHALLOW         ON
    )
    FetchContent_MakeAvailable(span-lite)

    # Group all the ThirdParty items under a ThirdParty folder in IDEs
    set_target_properties(yaml-cpp span-lite PROPERTIES FOLDER "ThirdParty")
endif()
