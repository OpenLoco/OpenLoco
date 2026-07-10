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
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon")

    if("${OPENLOCO_BRANCH}" STREQUAL "master")
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG}")
    else()
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG} ${OPENLOCO_BRANCH}")
    endif()

    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

    set(BUNDLE_RESOURCES "")

    find_program(ICONUTIL iconutil)
    if (ICONUTIL)
        file(COPY ${CMAKE_SOURCE_DIR}/distribution/macos/AppIcon.iconset DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        set(ICON_TARGET "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.iconset")
        set(ICON_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns")
        set(SOURCE_ICON_DIR "${CMAKE_SOURCE_DIR}/src/Resources/src/logo")
        add_custom_command(OUTPUT ${ICON_OUTPUT}
                COMMAND cp icon_x16.png ${ICON_TARGET}/icon_16x16.png
                COMMAND cp icon_x32.png ${ICON_TARGET}/icon_16x16@2x.png
                COMMAND cp icon_x32.png ${ICON_TARGET}/icon_32x32.png
                COMMAND cp icon_x64.png ${ICON_TARGET}/icon_32x32@2x.png
                COMMAND cp icon_x128.png ${ICON_TARGET}/icon_128x128.png
                COMMAND cp icon_x256.png ${ICON_TARGET}/icon_128x128@2x.png
                COMMAND cp icon_x256.png ${ICON_TARGET}/icon_256x256.png
                COMMAND cp icon_x512.png ${ICON_TARGET}/icon_256x256@2x.png
                COMMAND cp icon_x512.png ${ICON_TARGET}/icon_512x512.png
                COMMAND cp icon_x1024.png ${ICON_TARGET}/icon_512x512@2x.png
                COMMAND ${ICONUTIL} --convert icns --output ${ICON_OUTPUT} ${ICON_TARGET}
                WORKING_DIRECTORY ${SOURCE_ICON_DIR})
        list(APPEND BUNDLE_RESOURCES ${ICON_OUTPUT})
    endif ()

    list(APPEND BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/CHANGELOG.md)
    list(APPEND BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/CONTRIBUTORS.md)

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
