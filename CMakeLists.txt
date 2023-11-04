cmake_minimum_required(VERSION 3.20)

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	project(ThunderByteScan)
endif()

add_library(ThunderByteScan INTERFACE)

target_include_directories(ThunderByteScan INTERFACE include)

add_subdirectory(tests)