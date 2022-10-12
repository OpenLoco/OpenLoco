set(private_files "${CMAKE_CURRENT_SOURCE_DIR}/src/example.cpp")
set(test_files "${CMAKE_CURRENT_SOURCE_DIR}/tests/example.cpp")
loco_add_executable(Example2
	PRIVATE_FILES ${private_files}
	TEST_FILES ${test_files})

target_link_libraries(Example2 Example)

