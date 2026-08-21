# Linux x86_64 AppImage packaging (custom target: appimage).

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "OPENLOCO_BUILD_APPIMAGE is only supported on Linux")
endif()

if (NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "OPENLOCO_BUILD_APPIMAGE is only supported on x86_64, not ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(APPIMAGE_DIR "${CMAKE_BINARY_DIR}/appimage")
set(APPIMAGE_APPDIR "${APPIMAGE_DIR}/AppDir")
set(APPIMAGE_OUTPUT "${CMAKE_BINARY_DIR}/dist/OpenLoco-${OPENLOCO_VERSION_TAG}-linux-x64.AppImage")

# Without --config, cmake --install would install every configuration of a multi-config build.
set(APPIMAGE_INSTALL_ARGS)
if (CMAKE_CONFIGURATION_TYPES)
    set(APPIMAGE_INSTALL_ARGS --config Release)
endif()

set(APPIMAGETOOL "${APPIMAGE_DIR}/appimagetool-x86_64.AppImage")
if (NOT EXISTS "${APPIMAGETOOL}")
    file(DOWNLOAD
        "https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage"
        "${APPIMAGETOOL}"
        STATUS APPIMAGETOOL_DOWNLOAD_STATUS
        LOG APPIMAGETOOL_DOWNLOAD_LOG)
    list(GET APPIMAGETOOL_DOWNLOAD_STATUS 0 APPIMAGETOOL_DOWNLOAD_CODE)
    if (NOT APPIMAGETOOL_DOWNLOAD_CODE EQUAL 0)
        file(REMOVE "${APPIMAGETOOL}")
        message(FATAL_ERROR "Failed to download appimagetool: ${APPIMAGETOOL_DOWNLOAD_STATUS}\n${APPIMAGETOOL_DOWNLOAD_LOG}")
    endif()
    file(SHA256 "${APPIMAGETOOL}" APPIMAGETOOL_HASH)
    if (NOT APPIMAGETOOL_HASH STREQUAL "ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0")
        file(REMOVE "${APPIMAGETOOL}")
        message(FATAL_ERROR "appimagetool hash mismatch: expected ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0, got ${APPIMAGETOOL_HASH}")
    endif()
    file(CHMOD "${APPIMAGETOOL}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# Running appimagetool (an AppImage itself) requires /dev/fuse. Without it, extract and run the unpacked binary.
if (EXISTS /dev/fuse)
    set(APPIMAGETOOL_EXECUTABLE "${APPIMAGETOOL}")
else()
    set(APPIMAGETOOL_EXECUTABLE "${APPIMAGE_DIR}/appimagetool-extracted/squashfs-root/AppRun")
    add_custom_command(
        OUTPUT "${APPIMAGETOOL_EXECUTABLE}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${APPIMAGE_DIR}/appimagetool-extracted"
        COMMAND "${APPIMAGETOOL}" --appimage-extract
        WORKING_DIRECTORY "${APPIMAGE_DIR}/appimagetool-extracted"
        DEPENDS "${APPIMAGETOOL}"
        COMMENT "Extracting appimagetool (no /dev/fuse)"
        VERBATIM)
endif()

# Reuse the install rules for the AppDir payload, then add AppRun plus the root desktop file/icon that appimagetool embeds.
add_custom_command(
    OUTPUT "${APPIMAGE_OUTPUT}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${APPIMAGE_APPDIR}"
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --prefix "${APPIMAGE_APPDIR}/usr" ${APPIMAGE_INSTALL_ARGS}
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/distribution/linux/appimage/AppRun" "${APPIMAGE_APPDIR}/AppRun"
    COMMAND chmod 755 "${APPIMAGE_APPDIR}/AppRun"
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/distribution/linux/openloco.desktop" "${APPIMAGE_APPDIR}/openloco.desktop"
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/Resources/src/logo/icon_x256.png" "${APPIMAGE_APPDIR}/openloco.png"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/dist"
    COMMAND "${APPIMAGETOOL_EXECUTABLE}" "${APPIMAGE_APPDIR}" "${APPIMAGE_OUTPUT}"
    DEPENDS App
        "${CMAKE_CURRENT_SOURCE_DIR}/distribution/linux/appimage/AppRun"
        "${CMAKE_CURRENT_SOURCE_DIR}/distribution/linux/openloco.desktop"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Resources/src/logo/icon_x256.png"
        "${APPIMAGETOOL_EXECUTABLE}"
    COMMENT "Building ${APPIMAGE_OUTPUT}"
    VERBATIM)

add_custom_target(appimage DEPENDS "${APPIMAGE_OUTPUT}")
