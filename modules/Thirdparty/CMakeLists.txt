set(public_files 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Thirdparty/filesystem.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Thirdparty/span.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Thirdparty/variant.hpp"
)

loco_add_library(Thirdparty INTERFACE 
    PUBLIC_FILES ${public_files}
)
