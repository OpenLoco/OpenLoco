set(public_files 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/filesystem.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/span.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/variant.hpp"
)

loco_add_library(Thirdparty INTERFACE 
    PUBLIC_FILES ${public_files}
)
