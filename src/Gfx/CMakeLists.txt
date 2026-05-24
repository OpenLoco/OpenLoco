set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Gfx/PngImage.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PngImage.cpp"
)

set(public_link_libraries
    Core
    Diagnostics
    PNG::PNG
)

loco_add_library(Gfx STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
    PUBLIC_LINK_LIBRARIES
        ${public_link_libraries}
)
