set(public_files "${CMAKE_CURRENT_SOURCE_DIR}/include/example.h")
set(private_files "${CMAKE_CURRENT_SOURCE_DIR}/src/example.cpp")
set(test_files "${CMAKE_CURRENT_SOURCE_DIR}/tests/example.cpp")
loco_add_library(Example STATIC
	PUBLIC_FILES ${public_files}
	PRIVATE_FILES ${private_files}
	TEST_FILES ${test_files})

