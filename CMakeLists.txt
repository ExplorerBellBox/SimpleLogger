cmake_minimum_required(VERSION 3.16)

project(SimpleLogger)

set(CMAKE_CXX_STANDARD 17)

# ******************************** Check Build Type ********************************
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_definitions("-DDEBUG" "-D_DEBUG" "-D_DEBUG_")
else()
	set(CMAKE_BUILD_TYPE "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s -Wl,-x")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS}")
# ******************************** Check Build Type ********************************

# ******************************** Output Path ********************************
set(PATH_OUTPUT "${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}")
set(LIBRARY_OUTPUT_PATH "${PATH_OUTPUT}")
set(EXECUTABLE_OUTPUT_PATH "${PATH_OUTPUT}")
message(STATUS "Binary output path: ${PATH_OUTPUT}")
# ******************************** Output Path ********************************

set(PATH_SOURCE "${PROJECT_SOURCE_DIR}/source")
set(PATH_TESTS "${PROJECT_SOURCE_DIR}/tests")
set(BINARY_PREFIX "simple_")

add_subdirectory("./source")
add_subdirectory("./tests")