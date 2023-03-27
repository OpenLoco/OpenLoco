set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Interop/Interop.hpp"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Hook.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Interop.cpp"
)

loco_add_library(Interop STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)

# Disable optimizations for interop.cpp for all compilers, to allow optimized
# builds without need for -fno-omit-frame-pointer
if (MSVC)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/Interop/Interop.cpp" PROPERTIES COMPILE_FLAGS "/Oy- /Od")
else ()
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/Interop/Interop.cpp" PROPERTIES COMPILE_FLAGS "-fno-omit-frame-pointer -O0")
endif ()

target_link_libraries(Interop 
    PUBLIC
        Diagnostics
)
