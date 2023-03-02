set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Console/Console.h"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Console.cpp"
)

loco_add_library(Console STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)

target_link_libraries(Console 
    PUBLIC
        Core
)
