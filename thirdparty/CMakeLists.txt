## yaml-cpp
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

## ghc-filesystem
FetchContent_Declare(
    ghc_filesystem
    GIT_REPOSITORY      https://github.com/gulrak/filesystem
    GIT_TAG             v1.5.12
    GIT_SHALLOW         ON
)

FetchContent_MakeAvailable(ghc_filesystem)

## span-lite
FetchContent_Declare(
    span-lite
    GIT_REPOSITORY      https://github.com/martinmoene/span-lite
    GIT_TAG             v0.10.3
    GIT_SHALLOW         ON
)

FetchContent_MakeAvailable(span-lite)
