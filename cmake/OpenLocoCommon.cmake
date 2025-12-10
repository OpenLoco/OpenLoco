# Create INTERFACE library for common OpenLoco targets
add_library(OpenLocoCommonInterface INTERFACE)

# MSVC
set(COMMON_COMPILE_OPTIONS_MSVC
    /MP                      # Multithreaded compilation
    $<$<CONFIG:Debug>:/ZI>   # Debug Edit and Continue (Hot reload)
    $<$<CONFIG:Debug>:/MTd>  # Statically link MSVC runtime (Debug)
    $<$<CONFIG:Release>:/Zi> # Debug information in release
    $<$<CONFIG:Release>:/MT> # Statically link MSVC runtime (Release)
    $<$<CONFIG:Release>:/Oi> # Intrinsics
    $<$<CONFIG:RelWithDebInfo>:/MT> # Statically link MSVC runtime (RelWithDebInfo)
    $<$<CONFIG:RelWithDebInfo>:/Oi> # Intrinsics

    $<$<BOOL:${STRICT}>:/WX> # Warnings are errors (STRICT ONLY)
    /W4                      # Warning level 4
                             # Poke holes in W4 due to our interop code
    /wd4068                  #   4068: unknown pragma
    /wd4200                  #   4200: nonstandard extension used : zero-sized array in struct/union
    /wd4201                  #   4201: nonstandard extension used : nameless struct/union
    /wd4244                  #   4244: 'argument' : conversion from 'type1' to 'type2', possible loss of data
    /Zc:char8_t-             # Enable char8_t<->char conversion :(
    /Zc:__cplusplus          # Enable correct reporting for __cplusplus
    /utf-8
)

# GNU/CLANG
set(COMMON_COMPILE_OPTIONS_GNU
    -fstrict-aliasing
    -Wall
    -Wextra
    -Wtype-limits
    $<$<BOOL:${STRICT}>:-Werror>         # Warnings are errors (STRICT ONLY)

    # Poke some holes in -Wall:
    -Wno-unknown-pragmas
    -Wno-unused-private-field
    -Waddress
    # -Warray-bounds
    # compilers often get confused about our memory access patterns, disable some of the warnings
    -Wno-array-bounds
    $<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-overflow> # clang does not understand following options and errors with -Wunknown-warning-option
    $<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-overread>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-truncation>
    -Wchar-subscripts
    -Wenum-compare
    -Wformat
    -Wignored-qualifiers
    -Winit-self
    -Wmissing-declarations
    -Wnon-virtual-dtor
    -Wnull-dereference
    -Wstrict-aliasing
    -Wstrict-overflow=1
    -Wundef
    -Wunreachable-code
    -fno-char8_t             # Enable char8_t<->char conversion :(
    -Wno-deprecated-declarations
)

target_compile_options(OpenLocoCommonInterface INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:${COMMON_COMPILE_OPTIONS_MSVC}>
    $<$<CXX_COMPILER_ID:GNU>:${COMMON_COMPILE_OPTIONS_GNU}>
    $<$<CXX_COMPILER_ID:Clang>:${COMMON_COMPILE_OPTIONS_GNU}>
    $<$<CXX_COMPILER_ID:AppleClang>:${COMMON_COMPILE_OPTIONS_GNU}>
)

# MSVC
set(COMMON_LINK_OPTIONS_MSVC
    $<$<CONFIG:Release>:/DEBUG>             # Generate debug symbols even in release
    $<$<CONFIG:Debug>:/INCREMENTAL>         # Incremental linking required for hot reload
    /SAFESEH:NO                             # No safeseh linking required for hot reload and also crashes loading when enabled
    $<$<CONFIG:Release>:/OPT:ICF>           # COMDAT folding
    $<$<CONFIG:Release>:/OPT:REF>           # Eliminate unreferenced code/data
    $<$<CONFIG:RelWithDebInfo>:/OPT:ICF>    # COMDAT folding
    $<$<CONFIG:RelWithDebInfo>:/OPT:REF>    # Eliminate unreferenced code/data
)

target_link_options(OpenLocoCommonInterface INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:${COMMON_LINK_OPTIONS_MSVC}>
)

target_compile_features(OpenLocoCommonInterface INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
target_compile_definitions(OpenLocoCommonInterface INTERFACE DEBUG=${DEBUG_LEVEL})
