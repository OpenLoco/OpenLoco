function(_loco_add_target TARGET TYPE)
    cmake_parse_arguments("" "LIBRARY;EXECUTABLE;INTERFACE" "" "PRIVATE_FILES;PUBLIC_FILES;TEST_FILES;PUBLIC_LINK_LIBRARIES;PRIVATE_LINK_LIBRARIES;PUBLIC_COMPILE_DEFINITIONS;PRIVATE_COMPILE_DEFINITIONS" ${ARGN})

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
        
        # Link to common interface
        target_link_libraries(${TARGET} PUBLIC OpenLocoCommonInterface)
        
        # Set target-specific properties that can't be in INTERFACE library
        set_property(TARGET ${TARGET} PROPERTY POSITION_INDEPENDENT_CODE OFF) # Due to the way the linking works we must have no pie (remove when fully implemented)
        
        # Link libraries
        if (DEFINED _PUBLIC_LINK_LIBRARIES)
            target_link_libraries(${TARGET} PUBLIC ${_PUBLIC_LINK_LIBRARIES})
        endif()
        if (DEFINED _PRIVATE_LINK_LIBRARIES)
            target_link_libraries(${TARGET} PRIVATE ${_PRIVATE_LINK_LIBRARIES})
        endif()
        
        # Compile definitions
        if (DEFINED _PUBLIC_COMPILE_DEFINITIONS)
            target_compile_definitions(${TARGET} PUBLIC ${_PUBLIC_COMPILE_DEFINITIONS})
        endif()
        if (DEFINED _PRIVATE_COMPILE_DEFINITIONS)
            target_compile_definitions(${TARGET} PRIVATE ${_PRIVATE_COMPILE_DEFINITIONS})
        endif()
        
        set_property(TARGET ${TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    elseif(_EXECUTABLE)
        add_executable(${TARGET}
            ${_PRIVATE_FILES}
            ${_PUBLIC_FILES})
        add_executable(OpenLoco::${TARGET} ALIAS ${TARGET})

        target_include_directories(${TARGET}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src)

        # Link to common interface
        target_link_libraries(${TARGET} PUBLIC OpenLocoCommonInterface)
        
        # Set target-specific properties that can't be in INTERFACE library
        set_property(TARGET ${TARGET} PROPERTY POSITION_INDEPENDENT_CODE OFF) # Due to the way the linking works we must have no pie (remove when fully implemented)
        
        # Link libraries
        if (DEFINED _PUBLIC_LINK_LIBRARIES)
            target_link_libraries(${TARGET} PUBLIC ${_PUBLIC_LINK_LIBRARIES})
        endif()
        if (DEFINED _PRIVATE_LINK_LIBRARIES)
            target_link_libraries(${TARGET} PRIVATE ${_PRIVATE_LINK_LIBRARIES})
        endif()
        
        # Compile definitions
        if (DEFINED _PUBLIC_COMPILE_DEFINITIONS)
            target_compile_definitions(${TARGET} PUBLIC ${_PUBLIC_COMPILE_DEFINITIONS})
        endif()
        if (DEFINED _PRIVATE_COMPILE_DEFINITIONS)
            target_compile_definitions(${TARGET} PRIVATE ${_PRIVATE_COMPILE_DEFINITIONS})
        endif()
        
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

    # Defer header check to after configure time
    # This ensures all target properties and dependencies are fully set up
    if (OPENLOCO_HEADER_CHECK)
        cmake_language(EVAL CODE "cmake_language(DEFER CALL _loco_add_headers_check \"${TARGET}\")")
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
            PRIVATE
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

        # Link to common interface
        target_link_libraries(${TEST_TARGET} PUBLIC OpenLocoCommonInterface)
        
        # Set target-specific properties that can't be in INTERFACE library
        set_property(TARGET ${TEST_TARGET} PROPERTY POSITION_INDEPENDENT_CODE OFF) # Due to the way the linking works we must have no pie (remove when fully implemented)
    endif()
    
    # Tell each target about the project directory.
    # INTERFACE libraries can only have INTERFACE properties
    if (NOT _INTERFACE)
        target_compile_definitions(${TARGET} PRIVATE OPENLOCO_PROJECT_PATH="${OPENLOCO_PROJECT_PATH}")
    endif()
endfunction()

function(loco_add_library TARGET TYPE)
    _loco_add_target(${TARGET} ${TYPE} ${ARGN} LIBRARY)
endfunction()

function(loco_add_executable TARGET)
    _loco_add_target(${TARGET} NULL ${ARGN} EXECUTABLE)
endfunction()

function(_loco_add_headers_check TARGET)
    if (NOT TARGET all-headers-check)
        add_custom_target(all-headers-check)
    endif()
    
    if (NOT OPENLOCO_HEADER_CHECK OR NOT TARGET ${TARGET})
        return()
    endif()
    
    get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
    if (NOT TARGET_SOURCES OR TARGET_SOURCES STREQUAL "TARGET_SOURCES-NOTFOUND")
        return()
    endif()
    
    get_target_property(TARGET_INCLUDE_DIRS ${TARGET} INCLUDE_DIRECTORIES)
    get_target_property(TARGET_INTERFACE_INCLUDE_DIRS ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    
    # Separate headers into public/private
    set(PUBLIC_HEADER_FILES)
    set(PRIVATE_HEADER_FILES)
    foreach(source_file ${TARGET_SOURCES})
        if (source_file MATCHES "\\.(h|hpp|hxx)$")
            get_filename_component(header_abs ${source_file} ABSOLUTE)
            set(IS_PUBLIC NO)
            if (TARGET_INTERFACE_INCLUDE_DIRS AND NOT TARGET_INTERFACE_INCLUDE_DIRS STREQUAL "TARGET_INTERFACE_INCLUDE_DIRS-NOTFOUND")
                foreach(public_dir ${TARGET_INTERFACE_INCLUDE_DIRS})
                    file(RELATIVE_PATH rel_path ${public_dir} ${header_abs})
                    if (NOT rel_path MATCHES "^\\.\\.")
                        set(IS_PUBLIC YES)
                        break()
                    endif()
                endforeach()
            endif()
            if (IS_PUBLIC)
                list(APPEND PUBLIC_HEADER_FILES ${header_abs})
            else()
                list(APPEND PRIVATE_HEADER_FILES ${header_abs})
            endif()
        endif()
    endforeach()

    list(LENGTH PUBLIC_HEADER_FILES PUBLIC_COUNT)
    list(LENGTH PRIVATE_HEADER_FILES PRIVATE_COUNT)
    if (PUBLIC_COUNT EQUAL 0 AND PRIVATE_COUNT EQUAL 0)
        return()
    endif()
    
    message(STATUS "Header check for ${TARGET}: ${PUBLIC_COUNT} public, ${PRIVATE_COUNT} private headers")

    # Create wrapper files for public headers
    if (PUBLIC_COUNT GREATER 0)
        set(WRAPPER_DIR "${CMAKE_BINARY_DIR}/header-check/${TARGET}/public")
        set(WRAPPER_FILES)
        foreach(header_file ${PUBLIC_HEADER_FILES})
            set(rel_path)
            if (TARGET_INTERFACE_INCLUDE_DIRS AND NOT TARGET_INTERFACE_INCLUDE_DIRS STREQUAL "TARGET_INTERFACE_INCLUDE_DIRS-NOTFOUND")
                foreach(dir ${TARGET_INTERFACE_INCLUDE_DIRS})
                    file(RELATIVE_PATH temp_rel ${dir} ${header_file})
                    if (NOT temp_rel MATCHES "^\\.\\.")
                        set(rel_path ${temp_rel})
                        break()
                    endif()
                endforeach()
            endif()
            if (NOT rel_path)
                get_filename_component(rel_path ${header_file} NAME)
            endif()
            string(REGEX REPLACE "\\.[^.]*$" ".cpp" wrapper_rel ${rel_path})
            set(wrapper_file "${WRAPPER_DIR}/${wrapper_rel}")
            get_filename_component(wrapper_dir ${wrapper_file} DIRECTORY)
            file(MAKE_DIRECTORY ${wrapper_dir})
            file(WRITE ${wrapper_file} "#include \"${header_file}\"\n")
            list(APPEND WRAPPER_FILES ${wrapper_file})
        endforeach()
        
        add_library(${TARGET}-public-headers-check OBJECT ${WRAPPER_FILES})
        set_target_properties(${TARGET}-public-headers-check PROPERTIES LINKER_LANGUAGE CXX)
        target_include_directories(${TARGET}-public-headers-check PUBLIC
            $<TARGET_PROPERTY:${TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
        target_link_libraries(${TARGET}-public-headers-check PUBLIC
            $<TARGET_PROPERTY:${TARGET},INTERFACE_LINK_LIBRARIES>)
        add_dependencies(all-headers-check ${TARGET}-public-headers-check)
    endif()
    
    # Create wrapper files for private headers
    if (PRIVATE_COUNT GREATER 0)
        set(WRAPPER_DIR "${CMAKE_BINARY_DIR}/header-check/${TARGET}/private")
        set(WRAPPER_FILES)
        foreach(header_file ${PRIVATE_HEADER_FILES})
            set(rel_path)
            if (TARGET_INCLUDE_DIRS AND NOT TARGET_INCLUDE_DIRS STREQUAL "TARGET_INCLUDE_DIRS-NOTFOUND")
                foreach(dir ${TARGET_INCLUDE_DIRS})
                    file(RELATIVE_PATH temp_rel ${dir} ${header_file})
                    if (NOT temp_rel MATCHES "^\\.\\.")
                        set(rel_path ${temp_rel})
                        break()
                    endif()
                endforeach()
            endif()
            if (NOT rel_path)
                get_filename_component(rel_path ${header_file} NAME)
            endif()
            string(REGEX REPLACE "\\.[^.]*$" ".cpp" wrapper_rel ${rel_path})
            set(wrapper_file "${WRAPPER_DIR}/${wrapper_rel}")
            get_filename_component(wrapper_dir ${wrapper_file} DIRECTORY)
            file(MAKE_DIRECTORY ${wrapper_dir})
            file(WRITE ${wrapper_file} "#include \"${header_file}\"\n")
            list(APPEND WRAPPER_FILES ${wrapper_file})
        endforeach()
        
        add_library(${TARGET}-private-headers-check OBJECT ${WRAPPER_FILES})
        set_target_properties(${TARGET}-private-headers-check PROPERTIES LINKER_LANGUAGE CXX)
        target_include_directories(${TARGET}-private-headers-check PUBLIC
            $<TARGET_PROPERTY:${TARGET},INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:${TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
        target_link_libraries(${TARGET}-private-headers-check PUBLIC
            $<TARGET_PROPERTY:${TARGET},LINK_LIBRARIES>
            $<TARGET_PROPERTY:${TARGET},INTERFACE_LINK_LIBRARIES>)
        add_dependencies(all-headers-check ${TARGET}-private-headers-check)
    endif()
endfunction()
