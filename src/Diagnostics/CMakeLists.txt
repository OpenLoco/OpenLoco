set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/Assertion.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/LogFile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/LogLevel.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/LogSink.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/LogTerminal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/Logging.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LogFile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LogSink.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LogTerminal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp"
)

# Emscripten-specific source files (override native implementations)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # Remove native implementations
    list(REMOVE_ITEM private_files 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LogTerminal.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp")
    # Add Emscripten-specific implementations
    list(APPEND private_files 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/LogTerminal.Emscripten.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.Emscripten.cpp")
endif()

set(test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/AssertionTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/LoggingTests.cpp"
)

set(public_link_libraries
    Core
    Platform
)

loco_add_library(Diagnostics STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    TEST_FILES
        ${test_files}
    PUBLIC_LINK_LIBRARIES
        ${public_link_libraries}
)
