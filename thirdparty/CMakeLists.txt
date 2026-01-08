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
        GIT_TAG             2.1.1
        GIT_SHALLOW         ON
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(sfl)
endif()

if (HAS_LIBSTDCPP)
    find_package(TBB REQUIRED)
endif()
