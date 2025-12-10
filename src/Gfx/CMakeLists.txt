set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Gfx/PngImage.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PngImage.cpp"
)

loco_add_library(Gfx STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)

target_link_libraries(Gfx
    PUBLIC
        Core
        Diagnostics
        PNG::PNG
)
