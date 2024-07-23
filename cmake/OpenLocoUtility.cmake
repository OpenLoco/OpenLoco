function(loco_thirdparty_target_compile_link_flags TARGET)
    # Set some compiler options

    # MSVC
    set(COMMON_COMPILE_OPTIONS_MSVC
        /MP                                 # Multithreaded compilation
        $<$<CONFIG:Debug>:/ZI>              # Debug Edit and Continue (Hot reload)
        $<$<CONFIG:Release>:/Zi>            # Debug information in release
        $<$<CONFIG:Release>:/Oi>            # Intrinsics
        $<$<CONFIG:RelWithDebInfo>:/Oi>     # Intrinsics
        /Zc:char8_t-                        # Enable char8_t<->char conversion :(
    )

    # GNU/CLANG
    set(COMMON_COMPILE_OPTIONS_GNU
        -fno-char8_t             # Enable char8_t<->char conversion :(
    )

    set(COMMON_COMPILE_OPTIONS
        $<$<CXX_COMPILER_ID:MSVC>:${COMMON_COMPILE_OPTIONS_MSVC}>
        $<$<CXX_COMPILER_ID:GNU>:${COMMON_COMPILE_OPTIONS_GNU}>
        $<$<CXX_COMPILER_ID:Clang>:${COMMON_COMPILE_OPTIONS_GNU}>
    )

    # Set common link options

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

    # GNU/CLANG
    set(COMMON_LINK_OPTIONS_GNU
    )

    set(COMMON_LINK_OPTIONS
        $<$<CXX_COMPILER_ID:MSVC>:${COMMON_LINK_OPTIONS_MSVC}>
        $<$<CXX_COMPILER_ID:GNU>:${COMMON_LINK_OPTIONS_GNU}>
        $<$<CXX_COMPILER_ID:Clang>:${COMMON_LINK_OPTIONS_GNU}>
    )

    target_compile_options(${TARGET} PUBLIC ${COMMON_COMPILE_OPTIONS})
    target_link_options(${TARGET} PUBLIC ${COMMON_LINK_OPTIONS})
    target_compile_features(${TARGET} PUBLIC cxx_std_${CMAKE_CXX_STANDARD})
    set_property(TARGET ${TARGET} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Statically link the MSVC++ Runtime
    set_property(TARGET ${TARGET} PROPERTY POSITION_INDEPENDENT_CODE OFF) # Due to the way the linking works we must have no pie (remove when fully implemented)
endfunction()

function(loco_target_compile_link_flags TARGET)
    # Set some compiler options

    # MSVC
    set(COMMON_COMPILE_OPTIONS_MSVC
        /MP                      # Multithreaded compilation
        $<$<CONFIG:Debug>:/ZI>   # Debug Edit and Continue (Hot reload)
        $<$<CONFIG:Release>:/Zi> # Debug information in release

        $<$<BOOL:${STRICT}>:/WX> # Warnings are errors (STRICT ONLY)
        /W4                      # Warning level 4
                                 # Poke holes in W4 due to our interop code
        /wd4068                  #   4068: unknown pragma
        /wd4200                  #   4200: nonstandard extension used : zero-sized array in struct/union
        /wd4201                  #   4201: nonstandard extension used : nameless struct/union
        /wd4244                  #   4244: 'argument' : conversion from 'type1' to 'type2', possible loss of data
        /Zc:char8_t-             # Enable char8_t<->char conversion :(
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

    set(COMMON_COMPILE_OPTIONS
        $<$<CXX_COMPILER_ID:MSVC>:${COMMON_COMPILE_OPTIONS_MSVC}>
        $<$<CXX_COMPILER_ID:GNU>:${COMMON_COMPILE_OPTIONS_GNU}>
        $<$<CXX_COMPILER_ID:Clang>:${COMMON_COMPILE_OPTIONS_GNU}>
    )

    # Set common link options

    # MSVC
    set(COMMON_LINK_OPTIONS_MSVC
        $<$<CONFIG:Release>:/DEBUG>         # Generate debug symbols even in release
        $<$<CONFIG:Debug>:/INCREMENTAL>     # Incremental linking required for hot reload
        $<$<CONFIG:Debug>:/SAFESEH:NO>      # No safeseh linking required for hot reload
    )

    # GNU/CLANG
    set(COMMON_LINK_OPTIONS_GNU
    )

    set(COMMON_LINK_OPTIONS
        $<$<CXX_COMPILER_ID:MSVC>:${COMMON_LINK_OPTIONS_MSVC}>
        $<$<CXX_COMPILER_ID:GNU>:${COMMON_LINK_OPTIONS_GNU}>
        $<$<CXX_COMPILER_ID:Clang>:${COMMON_LINK_OPTIONS_GNU}>
    )

    target_compile_options(${TARGET} PUBLIC ${COMMON_COMPILE_OPTIONS})
    target_link_options(${TARGET} PUBLIC ${COMMON_LINK_OPTIONS})
    target_compile_features(${TARGET} PUBLIC cxx_std_${CMAKE_CXX_STANDARD})
    set_property(TARGET ${TARGET} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Statically link the MSVC++ Runtime
    set_property(TARGET ${TARGET} PROPERTY POSITION_INDEPENDENT_CODE OFF) # Due to the way the linking works we must have no pie (remove when fully implemented)
endfunction()

function(_loco_add_target TARGET TYPE)
    cmake_parse_arguments("" "LIBRARY;EXECUTABLE;INTERFACE" "" "PRIVATE_FILES;PUBLIC_FILES;TEST_FILES;" ${ARGN})

    if (${TYPE} STREQUAL "INTERFACE")
        set(_LIBRARY NO)
        set(_INTERFACE YES)
    endif()
    # Add public files to target so that source_group works
    # (nice IDE layout)
    if (_LIBRARY)
        add_library(${TARGET} ${TYPE} 
            ${_PRIVATE_FILES}
            ${_PUBLIC_FILES})
        add_library(OpenLoco::${TARGET} ALIAS ${TARGET})

        # We need to include both include and src as src may have private headers
        # Note: Generator expresions for this were not working!
        if (DEFINED _PUBLIC_FILES)
            target_include_directories(${TARGET}
                PUBLIC
                    "${CMAKE_CURRENT_SOURCE_DIR}/include")
        endif()
        if (DEFINED _PRIVATE_FILES)
            target_include_directories(${TARGET}
                PRIVATE
                    "${CMAKE_CURRENT_SOURCE_DIR}/src")
        endif()

        # TODO Maybe pass an additional Component variable to the function instead of repeat TARGET
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/${TARGET}")
            target_include_directories(${TARGET}
                PRIVATE
                    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/${TARGET}")
        endif()
        loco_target_compile_link_flags(${TARGET})
        set_property(TARGET ${TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    elseif(_EXECUTABLE)
        add_executable(${TARGET}
            ${_PRIVATE_FILES}
            ${_PUBLIC_FILES})
        add_executable(OpenLoco::${TARGET} ALIAS ${TARGET})

        target_include_directories(${TARGET}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src)

        loco_target_compile_link_flags(${TARGET})
        set_property(TARGET ${TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    elseif(_INTERFACE)
        # We want to add the headers to the interface library so that it displays
        # nicely wihtin IDEs
        add_library(${TARGET} ${TYPE}
            ${_PUBLIC_FILES})
        add_library(OpenLoco::${TARGET} ALIAS ${TARGET})

        target_include_directories(${TARGET}
            INTERFACE
                "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif()

    if (_PUBLIC_FILES AND OPENLOCO_HEADER_CHECK)
        # Creates a source file for every public header and includes only that one header.
        # This ensures that each header file is standalone.
        # We also link the public interfaces of the target ensuring that they are available for the public headers.
        foreach(hdr ${_PUBLIC_FILES})
            cmake_path(RELATIVE_PATH hdr BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" OUTPUT_VARIABLE hdr_rel)
            cmake_path(GET hdr STEM hdr_base)
            set(hdr_src "${CMAKE_CURRENT_BINARY_DIR}/${hdr_base}.cpp")
            file(WRITE ${hdr_src} "#include <${hdr_rel}>")
            list(APPEND hdr_srcs "${hdr_src}")
        endforeach()
        add_library(${TARGET}HdrCheck STATIC
                ${hdr_srcs}
                ${_PUBLIC_FILES})

        target_link_libraries(${TARGET}HdrCheck PRIVATE ${TARGET})
        # Group the targets nicely within IDEs
        set_target_properties(${TARGET} ${TARGET}HdrCheck PROPERTIES FOLDER ${TARGET})
    endif()

    # Group the files nicely in IDEs into a tree view
    if (_PUBLIC_FILES)
        source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include" PREFIX "include" FILES ${_PUBLIC_FILES})
    endif()
    if (_PRIVATE_FILES)
        source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${_PRIVATE_FILES})
    endif()
    if (_TEST_FILES AND ${OPENLOCO_BUILD_TESTS})
        # Tests will be under the libraryNameTests.exe
        set(TEST_TARGET ${TARGET}Tests)
        add_executable(${TEST_TARGET} ${_TEST_FILES})
        add_executable(OpenLoco::${TEST_TARGET} ALIAS ${TEST_TARGET})

        target_link_libraries(${TEST_TARGET}
            $<$<BOOL:${_LIBRARY}>:${TARGET}>
            GTest::gtest_main)

        include(GoogleTest)

        gtest_discover_tests(${TEST_TARGET})

        # Now that we have tests group the two targets as one in IDEs
        set_target_properties(${TARGET} ${TEST_TARGET} PROPERTIES FOLDER ${TARGET})
        # Group files nicely in IDEs
        source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" PREFIX "tests" FILES ${_TEST_FILES})
        
        # Tell each target about the project directory.
        target_compile_definitions(${TEST_TARGET} PRIVATE OPENLOCO_PROJECT_PATH="${OPENLOCO_PROJECT_PATH}")

        loco_target_compile_link_flags(${TEST_TARGET})
    endif()
    
    # Tell each target about the project directory.
    target_compile_definitions(${TARGET} PRIVATE OPENLOCO_PROJECT_PATH="${OPENLOCO_PROJECT_PATH}")
endfunction()

function(loco_add_library TARGET TYPE)
    _loco_add_target(${TARGET} ${TYPE} ${ARGN} LIBRARY)
endfunction()

function(loco_add_executable TARGET)
    _loco_add_target(${TARGET} NULL ${ARGN} EXECUTABLE)
endfunction()
