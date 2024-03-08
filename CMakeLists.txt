cmake_minimum_required(VERSION 3.20)

set(MAIN_PROJECT OFF)

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	project(ThunderByteScan)
	set(MAIN_PROJECT ON)
endif()

option(TBS_TESTS "Thunder Byte Scan with Tests" ${MAIN_PROJECT})
option(TBS_NO_MT "Disable multithreading in Thunder Byte Scan" OFF)

add_library(ThunderByteScan INTERFACE)

target_include_directories(ThunderByteScan INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(TBS_NO_MT)
    target_compile_definitions(ThunderByteScan INTERFACE TBS_NO_MT)
endif()

if(TBS_TESTS)
	add_subdirectory(tests)
endif()
