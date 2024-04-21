cmake_minimum_required(VERSION 3.15)

set(MAIN_PROJECT OFF)

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	project(TBS)
	set(MAIN_PROJECT ON)
endif()

option(TBS_TESTS "TBS with Tests" ${MAIN_PROJECT})
option(TBS_MT "Enable multithreading in TBS" ON)
option(TBS_USE_SSE2 "Enable SSE2 in TBS" ON)
option(TBS_USE_AVX "Enable AVX in TBS" ON)
option(TBS_USE_ARCH_WORD_SIMD "Enable SIMD using Arch Word in TBS" ON)
option(TBS_USE_ETL "Enable Embedded Template Library in TBS" OFF)
option(TBS_NO_STL "Disable STL Usage in TBS" OFF)

if(TBS_NO_STL)
    set(TBS_USE_ETL ON) # Using Aux ETL
    set(TBS_MT OFF) # Disabling Multi-Threading
endif()

add_subdirectory(vendor)

add_library(TBS INTERFACE)

if(MSVC)
    # Set the minimum required version for SSE2
    if(TBS_USE_SSE2)
        target_compile_options(TBS INTERFACE /arch:SSE2)
        target_compile_definitions(TBS INTERFACE TBS_USE_SSE2)
        message(STATUS "SSE2 support enabled.")
    endif()
    if(TBS_USE_AVX)
        target_compile_options(TBS INTERFACE /arch:AVX)
        target_compile_definitions(TBS INTERFACE TBS_USE_AVX)
        message(STATUS "AVX support enabled.")
    endif()
endif()

if(TBS_MT)
    target_compile_definitions(TBS INTERFACE TBS_MT)
endif()

if(TBS_USE_ETL)
    target_compile_definitions(TBS INTERFACE TBS_USE_ETL)
    target_link_libraries(TBS INTERFACE etl)
endif()

if(TBS_USE_ARCH_WORD_SIMD)
    target_compile_definitions(TBS INTERFACE TBS_USE_ARCH_WORD_SIMD)
endif()
    

target_include_directories(TBS INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(TBS_TESTS)
	add_subdirectory(tests)
endif()
