set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Math/Bound.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Math/Trigonometry.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Math/Vector.hpp"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Trigonometry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vector.cpp"
)

loco_add_library(Math STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)
