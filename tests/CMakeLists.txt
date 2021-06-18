include_directories("${PATH_SOURCE}")
link_directories("${LIBRARY_OUTPUT_PATH}")

add_executable(test_directly test_directly.cpp)
if(MSVC)

else()
	target_link_libraries(test_directly pthread stdc++fs)
endif()