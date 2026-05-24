set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Version.hpp"
)

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
