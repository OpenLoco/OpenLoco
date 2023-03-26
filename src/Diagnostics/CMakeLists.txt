set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/LogFile.h"
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

set(test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/LoggingTests.cpp"
)

loco_add_library(Diagnostics STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    TEST_FILES
        ${test_files}
)

target_link_libraries(Diagnostics 
    PUBLIC
        Core
)
