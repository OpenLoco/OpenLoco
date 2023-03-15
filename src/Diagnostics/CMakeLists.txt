set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Diagnostics/Logging.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp"
)

loco_add_library(Diagnostics STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)

target_link_libraries(Diagnostics 
    PUBLIC
        Core
)
