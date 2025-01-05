cmake_minimum_required(VERSION 3.15)

project(TBS)

option(TBS_BUILD_TESTS "TBS Tests" OFF)
option(TBS_INSTALL "TBS Install" ON)
if(NOT TBS_INSTALL)
set(CBUILDKIT_NOINSTALL ON)
endif()

find_package(CBuildKit REQUIRED)

option(TBS_MT "Enable multithreading in TBS" OFF)
option(TBS_USE_SSE2 "Enable SSE2 in TBS" OFF)
option(TBS_USE_AVX "Enable AVX in TBS" OFF)
option(TBS_USE_ARCH_WORD_SIMD "Enable SIMD using Arch Word in TBS" ON)
option(TBS_USE_ETL "Enable Embedded Template Library in TBS" OFF)
option(TBS_NO_STL "Disable STL Usage in TBS" OFF)

if(TBS_NO_STL)
    set(TBS_USE_ETL ON) # Using Aux ETL
    set(TBS_MT OFF) # Disabling Multi-Threading
endif()

add_subdirectory(vendor)

add_library_ns(tbs tbs INTERFACE)

if(MSVC)
    # Set the minimum required version for SSE2
    if(TBS_USE_SSE2)
        # target_compile_options(tbs-tbs INTERFACE /arch:SSE2)
        target_compile_definitions(tbs-tbs INTERFACE TBS_USE_SSE2)
        message(STATUS "SSE2 support enabled.")
    endif()
    if(TBS_USE_AVX)
        # target_compile_options(tbs-tbs INTERFACE /arch:AVX)
        target_compile_definitions(tbs-tbs INTERFACE TBS_USE_AVX)
        message(STATUS "AVX support enabled.")
    endif()
endif()

if(TBS_MT)
    target_compile_definitions(tbs-tbs INTERFACE TBS_MT)
endif()

if(TBS_USE_ETL)
    target_compile_definitions(tbs-tbs INTERFACE TBS_USE_ETL)
    target_link_libraries(tbs-tbs INTERFACE etl)
endif()

if(TBS_USE_ARCH_WORD_SIMD)
    target_compile_definitions(tbs-tbs INTERFACE TBS_USE_ARCH_WORD_SIMD)
endif()

add_subdirectory(cli)

if(TBS_BUILD_TESTS)
	add_subdirectory(tests)
endif()

target_include_dir_iface(tbs-tbs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include include)

if(TBS_INSTALL)
install_target_and_headers(tbs tbs)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include)

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TBSConfig.cmake
        DESTINATION lib/cmake/tbs)
endif()