set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioChannel.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioEngine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioFormat.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Audio/AudioHandle.h"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(private_files
        "${CMAKE_CURRENT_SOURCE_DIR}/src/AudioEngine.WebAudio.cpp"
    )
else()
    set(private_files
        "${CMAKE_CURRENT_SOURCE_DIR}/src/AudioEngine.cpp"
    )
endif()

set(public_link_libraries
    Core
    Diagnostics
)

set(private_link_libraries)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    list(APPEND private_link_libraries ${OPENAL_LIBRARIES})
endif()

loco_add_library(Audio STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    PUBLIC_LINK_LIBRARIES
        ${public_link_libraries}
    PRIVATE_LINK_LIBRARIES
        ${private_link_libraries}
)
