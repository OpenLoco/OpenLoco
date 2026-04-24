set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Utility/LookupTable.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Utility/String.hpp"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/String.cpp"
)

set(test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/LookupTableTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/StringTests.cpp"
)

set(public_link_libraries
)

loco_add_library(Utility STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    TEST_FILES
        ${test_files}
    PUBLIC_LINK_LIBRARIES
        ${public_link_libraries}
)
