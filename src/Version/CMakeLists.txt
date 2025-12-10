include(FindGit)

set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Version.hpp"
)

find_package(Git)
if (Git_FOUND)
    # Describe current version in terms of closest tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe HEAD --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE OPENLOCO_VERSION_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        # ERROR_QUIET
    )

    if (OPENLOCO_VERSION_TAG)
        # Use a platform agnostic sed equivalent to strip the commit hash from the version tag
        # i.e. "v22.10-9-g8ff1d207" becomes "v22.10-9 "
        string(REGEX REPLACE "-g.+$" " " OPENLOCO_VERSION_TAG ${OPENLOCO_VERSION_TAG})
        # Has to be in two bits due to empty string not being possible to pass to REGEX REPLACE
        string(STRIP ${OPENLOCO_VERSION_TAG} OPENLOCO_VERSION_TAG)
    else()
        # If a low fetch depth is used then nearest tag for git will fail
        # so just use the project version in that case.
        set(OPENLOCO_VERSION_TAG ${PROJECT_VERSION})
    endif()
    
    # Define current git branch
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE OPENLOCO_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Define short commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE OPENLOCO_COMMIT_SHA1_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    message(STATUS "Version: ${OPENLOCO_VERSION_TAG} | Branch: ${OPENLOCO_BRANCH} | Commit: ${OPENLOCO_COMMIT_SHA1_SHORT}")
    
    # Propagate version variables to parent scope for use in other CMakeLists.txt files
    set(OPENLOCO_VERSION_TAG "${OPENLOCO_VERSION_TAG}" PARENT_SCOPE)
    set(OPENLOCO_BRANCH "${OPENLOCO_BRANCH}" PARENT_SCOPE)
    set(OPENLOCO_COMMIT_SHA1_SHORT "${OPENLOCO_COMMIT_SHA1_SHORT}" PARENT_SCOPE)
endif()

loco_add_library(Version INTERFACE
    PUBLIC_FILES
        ${public_files}
)

# Set compile definitions on the interface library
target_compile_definitions(Version
    INTERFACE
        OPENLOCO_VERSION_TAG="${OPENLOCO_VERSION_TAG}"
        OPENLOCO_BRANCH="${OPENLOCO_BRANCH}"
        OPENLOCO_COMMIT_SHA1_SHORT="${OPENLOCO_COMMIT_SHA1_SHORT}"
)
