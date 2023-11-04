cmake_minimum_required(VERSION 3.20)

set(MAIN_PROJECT OFF)

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	project(ThunderByteScan)
	set(MAIN_PROJECT ON)
endif()

option(TBS_TESTS "Thunder Byte Scan with Tests" ${MAIN_PROJECT})

add_library(ThunderByteScan INTERFACE)

target_include_directories(ThunderByteScan INTERFACE include)

if(TBS_TESTS)
	add_subdirectory(tests)
endif()