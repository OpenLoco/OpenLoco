set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioChannel.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioEngine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioFormat.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioHandle.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AudioEngine.cpp"
)

set(public_link_libraries
    Core
    Diagnostics
)

loco_add_library(Audio STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    PUBLIC_LINK_LIBRARIES
        ${public_link_libraries}
)
