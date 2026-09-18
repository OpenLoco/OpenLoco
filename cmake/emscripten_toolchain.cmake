# Emscripten Toolchain for OpenLoco
# This toolchain configures CMake to build OpenLoco for WebAssembly using Emscripten

# Check for Emscripten environment
if(DEFINED ENV{EMSDK})
    message(STATUS "Emscripten SDK found at: $ENV{EMSDK}")
else()
    message(WARNING "Emscripten SDK (EMSDK) environment variable is not set. Please run 'emsdk_env.bat' (Windows) or 'source emsdk_env.sh' (Linux/Mac) before building.")
endif()

# Set the system name to Emscripten
set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_VERSION 1)

# Set EMSDK_PATH for later use
if(DEFINED ENV{EMSDK})
    set(EMSDK_PATH "$ENV{EMSDK}")
else()
    set(EMSDK_PATH "")
endif()

# Find Emscripten compilers - try multiple approaches
# 1. First try to find them in PATH (if emsdk_env was run)
# On Windows, Emscripten uses .bat files, so we need to search for those explicitly
if(WIN32)
    find_program(CMAKE_C_COMPILER emcc.bat)
    find_program(CMAKE_CXX_COMPILER em++.bat)
    find_program(CMAKE_RANLIB emranlib.bat)
    find_program(CMAKE_AR emar.bat)
    # Also try without .bat in case they're wrapper scripts
    if(NOT CMAKE_C_COMPILER)
        find_program(CMAKE_C_COMPILER emcc)
    endif()
    if(NOT CMAKE_CXX_COMPILER)
        find_program(CMAKE_CXX_COMPILER em++)
    endif()
else()
    find_program(CMAKE_C_COMPILER emcc)
    find_program(CMAKE_CXX_COMPILER em++)
    find_program(CMAKE_RANLIB emranlib)
    find_program(CMAKE_AR emar)
endif()

# 2. If not in PATH, try to find them in EMSDK installation
if(NOT CMAKE_C_COMPILER AND DEFINED ENV{EMSDK})
    set(EMSDK_EMSCRIPTEN_PATH "$ENV{EMSDK}/upstream/emscripten")
    if(WIN32)
        find_program(CMAKE_C_COMPILER emcc.bat PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        find_program(CMAKE_CXX_COMPILER em++.bat PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        find_program(CMAKE_RANLIB emranlib.bat PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        find_program(CMAKE_AR emar.bat PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        if(NOT CMAKE_C_COMPILER)
            find_program(CMAKE_C_COMPILER emcc PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        endif()
        if(NOT CMAKE_CXX_COMPILER)
            find_program(CMAKE_CXX_COMPILER em++ PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        endif()
    else()
        find_program(CMAKE_C_COMPILER emcc PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        find_program(CMAKE_CXX_COMPILER em++ PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        find_program(CMAKE_RANLIB emranlib PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
        find_program(CMAKE_AR emar PATHS "${EMSDK_EMSCRIPTEN_PATH}" NO_DEFAULT_PATH)
    endif()
endif()

# Verify compilers were found
if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR
        "Emscripten compilers not found in PATH!\n"
        "Please either:\n"
        "Run 'emsdk_env.bat\\sh' in this terminal before building\n"
        "Emscripten SDK location: $ENV{EMSDK}")
endif()

message(STATUS "Found Emscripten C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Found Emscripten C++ compiler: ${CMAKE_CXX_COMPILER}")

# Set the find root path mode
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Add the Emscripten SDL3 port include path
if(DEFINED ENV{EMSDK})
    set(EMSCRIPTEN_ROOT "$ENV{EMSDK}/upstream/emscripten")
    # Try multiple possible sysroot locations for SDL3 headers
    set(POSSIBLE_SYSROOTS
        "${EMSCRIPTEN_ROOT}/cache/sysroot"
        "${EMSCRIPTEN_ROOT}/system"
        "${EMSCRIPTEN_ROOT}"
    )

    # Find SDL3 headers - they should be in include/SDL3/SDL.h
    set(EMSCRIPTEN_SDL3_INCLUDE_DIR "")
    foreach(SYSROOT ${POSSIBLE_SYSROOTS})
        if(EXISTS "${SYSROOT}/include/SDL3/SDL.h")
            set(EMSCRIPTEN_SDL3_INCLUDE_DIR "${SYSROOT}/include")
            message(STATUS "Found SDL3 headers at: ${EMSCRIPTEN_SDL3_INCLUDE_DIR}")
            break()
        endif()
    endforeach()

    if(NOT EMSCRIPTEN_SDL3_INCLUDE_DIR)
        message(WARNING "SDL3 headers not found in Emscripten SDK! Please ensure SDL3 port is installed: embuilder build sdl3")
    else()
        # Set as both cache and regular variable to ensure it's available
        set(EMSCRIPTEN_SDL3_INCLUDE_DIR "${EMSCRIPTEN_SDL3_INCLUDE_DIR}" CACHE PATH "SDL3 include directory for Emscripten" FORCE)
    endif()


endif()

# Emscripten-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")
set(CMAKE_CROSSCOMPILING TRUE)

# Enable position independent code (required for WASM)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable IPO/LTO for Emscripten initially (can be enabled later if needed)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emscripten-specific compile flags
set(EMSCRIPTEN_COMPILE_FLAGS
    "-fstrict-aliasing"
    "-fexceptions"
)


# Emscripten-specific link flags
# These will be extended by the build presets or additional configuration
set(EMSCRIPTEN_LINK_FLAGS
    "-sUSE_SDL=3"                     # SDL3 support (Emscripten includes prebuilt SDL3)
    "-sUSE_LIBPNG=1"                  # PNG support via Emscripten port
    "-sUSE_ZLIB=1"                    # ZLIB support via Emscripten port
    "-sALLOW_MEMORY_GROWTH=1"         # Dynamic memory growth
    "-sASYNCIFY"                      # Enable asyncify for synchronous file I/O
    "-sFORCE_FILESYSTEM=1"            # Force filesystem support
    "-sINITIAL_MEMORY=67108864"       # 64MB initial memory
    "-sSTACK_SIZE=5242880"            # 5MB stack size
    "-sALLOW_TABLE_GROWTH=1"          # Allow table growth for indirect calls
    "-fexceptions"                    # Enable exception catching
    "-sSAFE_HEAP=0"                   # Disable safe heap for performance (enable for debugging)
    "-sEXPORTED_RUNTIME_METHODS=['cwrap','ccall']" # Export runtime methods
    "-sALLOW_UNIMPLEMENTED_SYSCALLS=1" # Allow unimplemented syscalls (some may be needed)
    "-sFULL_ES3=1"                    # Full OpenGL ES 3.0 emulation via WebGL
    "-sMAX_WEBGL_VERSION=2"           # Use WebGL 2
    "-sMIN_WEBGL_VERSION=2"           # Minimum WebGL version
)

# Add preload files for assets (will be configured during build)
# This is done via CMAKE_EXE_LINKER_FLAGS or target properties

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Debug-specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND EMSCRIPTEN_LINK_FLAGS
        "-s ASSERTIONS=1"
        "-s SAFE_HEAP=1"
        "-g"
    )
endif()

# Convert lists to strings for cache variables
string(REPLACE ";" " " EMSCRIPTEN_CXX_FLAGS "${EMSCRIPTEN_COMPILE_FLAGS}")
string(REPLACE ";" " " EMSCRIPTEN_EXE_LINKER_FLAGS "${EMSCRIPTEN_LINK_FLAGS}")

# Set the flags in cache
set(CMAKE_CXX_FLAGS "${EMSCRIPTEN_CXX_FLAGS}" CACHE STRING "Emscripten C++ flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${EMSCRIPTEN_EXE_LINKER_FLAGS}" CACHE STRING "Emscripten linker flags" FORCE)


message(STATUS "Configuring OpenLoco for Emscripten/WebAssembly")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Memory: 64MB initial, auto-growing")
message(STATUS "Audio: Web Audio API")
message(STATUS "Graphics: SDL3 (WebGL)")
