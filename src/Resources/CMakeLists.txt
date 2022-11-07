# Resource files are for Windows only for now
# Eventually there will be Mac resources as well here.
if (WIN32)
    enable_language(RC)
    set(public_files "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Resources/Resource.h")
    set(private_files "${CMAKE_CURRENT_SOURCE_DIR}/src/OpenLoco.rc"
                      "${CMAKE_CURRENT_SOURCE_DIR}/src/logo/icon.ico")

    loco_add_library(Resources OBJECT
        PUBLIC_FILES ${public_files}
        PRIVATE_FILES ${private_files})
endif()
