set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/BinaryStream.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/BitSet.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/EnumFlags.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Exception.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/FileStream.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/FileSystem.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/LocoFixedVector.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/MemoryStream.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Numerics.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Prng.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Span.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Stream.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Timer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Core/Traits.hpp"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BinaryStream.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FileStream.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryStream.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Numerics.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Prng.cpp"
)

set(test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/EnumFlagsTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/FileStreamTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/MemoryStreamTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/NumericsTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/PrngTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/TimerTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/TraitsTest.cpp"
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
        fmt::fmt
)

