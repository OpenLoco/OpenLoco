set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Interop/Interop.hpp"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Interop.cpp"
)

loco_add_library(Interop STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)

target_link_libraries(Interop 
    PUBLIC
        Diagnostics
)
