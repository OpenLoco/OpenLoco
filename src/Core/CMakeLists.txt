set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/BitSet.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/FileSystem.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/LocoFixedVector.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Prng.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Span.hpp")

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Prng.cpp")

set(test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/PrngTests.cpp"
)

loco_add_library(Core STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    TEST_FILES
        ${test_files}
)

target_link_libraries(Core
    PUBLIC
        nonstd::span-lite
    PRIVATE
        Utility)

