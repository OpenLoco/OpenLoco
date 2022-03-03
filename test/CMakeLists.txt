cmake_minimum_required(VERSION 3.9)
cmake_policy(VERSION 3.9)

option(SYSTEM_GTEST "Use the googletest library provided by the system.")

if (SYSTEM_GTEST)
    find_package(GTest REQUIRED)

    set(GTEST_LIBRARIES ${GTEST_BOTH_LIBRARIES})

    message(WARNING "Gtest strongly advices against using a system installation, see https://github.com/google/googletest/blob/master/googletest/docs/FAQ.md#why-is-it-not-recommended-to-install-a-pre-compiled-copy-of-google-test-for-example-into-usrlocal for detailed information. If errors occur please double-check without the SYSTEM_GTEST flag.")
else (SYSTEM_GTEST)

        # Bootstrap GoogleTest
        INCLUDE(ExternalProject)

        # Use googletest with https://github.com/google/googletest/pull/1045 applied
        ExternalProject_Add(
                googletest-distribution
                URL https://github.com/google/googletest/archive/ad49eaa0466b98f21449efaaaae817683daa8b99.tar.gz
                URL_HASH SHA1=e468bfdffe7c46c14ce06bfb48a29b6201908504
                TIMEOUT 10
                CONFIGURE_COMMAND ""
                BUILD_COMMAND ""
                INSTALL_COMMAND ""
        )

        # Specify include dir
        ExternalProject_Get_Property(googletest-distribution SOURCE_DIR)
        set(GOOGLETEST_DISTRIB_SOURCE_DIR "${SOURCE_DIR}")

        ExternalProject_Add(
                googletest
                DEPENDS googletest-distribution
                DOWNLOAD_COMMAND ""
                SOURCE_DIR "${GOOGLETEST_DISTRIB_SOURCE_DIR}/googletest"
                CMAKE_ARGS "-DCMAKE_CXX_FLAGS=${TARGET_M}"
                BUILD_BYPRODUCTS "googletest-prefix/src/googletest-build/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
                BUILD_BYPRODUCTS "googletest-prefix/src/googletest-build/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
                # Disable install step
                INSTALL_COMMAND ""
                # Wrap download, configure and build steps in a script to log output
                LOG_DOWNLOAD ON
                LOG_CONFIGURE ON
                LOG_BUILD ON)


        # Specify include dir
        set(GTEST_INCLUDE_DIRS ${GOOGLETEST_DISTRIB_SOURCE_DIR}/googletest/include)

        # Library
        ExternalProject_Get_Property(googletest BINARY_DIR)
        set(GOOGLETEST_BINARY_DIR "${BINARY_DIR}")
        set(GTEST_LIBRARY_PATH ${GOOGLETEST_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
        set(GTEST_MAIN_LIBRARY_PATH ${GOOGLETEST_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
        set(GTEST_LIBRARY gtest)
        set(GTEST_MAIN_LIBRARY gtest_main)
        add_library(${GTEST_LIBRARY} STATIC IMPORTED)
        add_library(${GTEST_MAIN_LIBRARY} STATIC IMPORTED)
        set_property(TARGET ${GTEST_LIBRARY} PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH})
        set_property(TARGET ${GTEST_MAIN_LIBRARY} PROPERTY IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY_PATH})
        add_dependencies(${GTEST_LIBRARY} googletest)
        add_dependencies(${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY})

        set(GTEST_LIBRARIES gtest gtest_main pthread)

endif (SYSTEM_GTEST)

include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
include_directories("${ROOT_DIR}/src")
include_directories(SYSTEM ${SDL2_INCLUDE_DIRS})

# Some most common files required in tests
#set(COMMON_TEST_SOURCES
#    "${ROOT_DIR}/src/openrct2/core/Console.cpp"
#    )

# Create a re-usable library to save some compilation time
# add_library(test-common STATIC ${COMMON_TEST_SOURCES})

# Start of our tests

# sawyercoding test

set(SAWYERCODING_TEST_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test.cpp"
        )
add_executable(test_sawyercoding ${SAWYERCODING_TEST_SOURCES})
target_link_libraries(test_sawyercoding ${GTEST_LIBRARIES})
target_link_libraries(test_sawyercoding)
add_test(NAME sawyercoding COMMAND test_sawyercoding)


