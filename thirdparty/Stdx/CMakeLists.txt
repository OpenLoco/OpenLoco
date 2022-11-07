set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ThirdParty/Stdx/span.hpp"
)

loco_add_library(ThirdPartyStdx INTERFACE 
    PUBLIC_FILES ${public_files}
)
