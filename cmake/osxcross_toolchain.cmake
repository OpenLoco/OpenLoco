message("HELLO FROM TOOLCHAIN")
message("TOOLCHAIN2: $ENV{TOOLCHAIN2}")

macro(osxcross_getconf VAR)
    if(NOT ${VAR})
        set(${VAR} "$ENV{${VAR}}")
        if(${VAR})
            set(${VAR} "${${VAR}}" CACHE STRING "${VAR}")
            message(STATUS "Found ${VAR}: ${${VAR}}")
        else()
            message(FATAL_ERROR "Cannot determine \"${VAR}\"")
        endif()
    endif()
endmacro()

osxcross_getconf(OSXCROSS_HOST)
osxcross_getconf(OSXCROSS_TARGET_DIR)
osxcross_getconf(OSXCROSS_TARGET)
osxcross_getconf(OSXCROSS_SDK)

set(CMAKE_SYSTEM_NAME "Darwin")
string(REGEX REPLACE "-.*" "" CMAKE_SYSTEM_PROCESSOR "${OSXCROSS_HOST}")

# specify the cross compiler
set(CMAKE_C_COMPILER "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-clang")
set(CMAKE_CXX_COMPILER "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-clang++")

set(CMAKE_AR "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-ar" CACHE FILEPATH "ar")
set(CMAKE_RANLIB "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-ranlib" CACHE FILEPATH "ranlib")
set(CMAKE_INSTALL_NAME_TOOL "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-install_name_tool" CACHE FILEPATH "install_name_tool")

set(ENV{PKG_CONFIG_LIBDIR} "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local/lib/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${OSXCROSS_TARGET_DIR}/macports/pkgs")

set(TOOLCHAIN2 "$ENV{TOOLCHAIN2}")
include("${TOOLCHAIN2}")
message("CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")
