set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Platform/Crash.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Platform/Debug.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Platform/Platform.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Crash.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform.Macos.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform.Posix.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform.Windows.cpp"
)

if (APPLE)
    set_source_files_properties(${private_files} PROPERTIES COMPILE_FLAGS "-x objective-c++")
else()
    # Just want to keep the file in generated project but not compile it.
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform.Macos.mm"
        PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

set(public_link_libraries
    Core
    Utility
)

if (MSVC)
    list(APPEND public_link_libraries ${BREAKPAD_LIBRARIES})
    list(APPEND public_link_libraries "Winmm.lib")
endif()

loco_add_library(Platform STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    PUBLIC_LINK_LIBRARIES
        ${public_link_libraries}
)

if (MSVC)
    target_compile_definitions(Platform
        PRIVATE
            USE_BREAKPAD)
endif()
