set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp"
)

set(private_link_libraries
    OpenLoco
    SDL3::SDL3
    $<$<PLATFORM_ID:Windows>:Resources>
    $<$<BOOL:${MINGW}>:version>
)

set(private_compile_definitions
    SDL_MAIN_HANDLED
)

loco_add_executable(App
    PRIVATE_FILES
        ${private_files}
    PRIVATE_LINK_LIBRARIES
        ${private_link_libraries}
    PRIVATE_COMPILE_DEFINITIONS
        ${private_compile_definitions}
)

set_target_properties(App PROPERTIES OUTPUT_NAME OpenLoco)

if (APPLE)
    target_link_options(App PRIVATE "-framework" "Cocoa")

    set_target_properties(App PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/distribution/macos/Info.plist.in)

    set(MACOSX_BUNDLE_BUNDLE_NAME "OpenLoco")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${OPENLOCO_COMMIT_SHA1_SHORT}")
    set(MACOSX_BUNDLE_COPYRIGHT "OpenLoco is licensed under the MIT License")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.openloco.OpenLoco")
    set(MACOSX_BUNDLE_ICON_FILE "OpenLoco")

    if("${OPENLOCO_BRANCH}" STREQUAL "master")
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG}")
    else()
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG} ${OPENLOCO_BRANCH}")
    endif()

    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

    set(BUNDLE_RESOURCES "")

    list(APPEND BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/CHANGELOG.md)
    list(APPEND BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/CONTRIBUTORS.md)

    # NB: icons are created Icon Composer and then exported using Xcode's `actool`
    list(APPEND BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/distribution/macos/Assets.car)
    list(APPEND BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/distribution/macos/OpenLoco.icns)

    file(GLOB BUNDLE_LANGUAGES "${CMAKE_SOURCE_DIR}/data/language/*")
    file(GLOB_RECURSE  BUNDLE_OBJECTS "${openloco_objects_SOURCE_DIR}/*")

    target_sources(App PRIVATE "${BUNDLE_RESOURCES}" "${BUNDLE_LANGUAGES}" "${BUNDLE_OBJECTS}")

    set_target_properties(App PROPERTIES RESOURCE "${BUNDLE_RESOURCES}" "${BUNDLE_LANGUAGES}" "${BUNDLE_OBJECTS}")

    set_property(
        SOURCE ${BUNDLE_LANGUAGES}
        PROPERTY MACOSX_PACKAGE_LOCATION "Resources/language"
    )

    set_property(
        SOURCE ${BUNDLE_OBJECTS}
        PROPERTY MACOSX_PACKAGE_LOCATION "Resources/objects"
    )
endif ()
