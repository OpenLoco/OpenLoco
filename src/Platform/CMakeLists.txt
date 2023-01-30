set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Platform/Crash.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Platform/Platform.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Crash.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform.Posix.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform.Windows.cpp"
)

if (APPLE)
    set(private_files 
        "${CMAKE_CURRENT_SOURCE_DIR}src/Platform/Platform.Macos.mm")
    set_source_files_properties(${private_files} PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

loco_add_library(Platform STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)

target_link_libraries(Platform PUBLIC
    Core
    Console
    Utility
)
