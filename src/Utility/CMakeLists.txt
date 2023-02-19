set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Utility/Collection.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Utility/Exception.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Utility/Numeric.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Utility/Stream.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Utility/String.hpp")

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Numeric.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/String.cpp")

set(test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/NumericTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/StringTests.cpp")

loco_add_library(Utility STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    TEST_FILES
        ${test_files})

