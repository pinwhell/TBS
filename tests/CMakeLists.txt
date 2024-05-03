enable_testing()

file(GLOB ALL_TST_SRC *.cpp)

foreach(TST_SRC ${ALL_TST_SRC})
    get_filename_component(TST_SRC_NAME ${TST_SRC} NAME_WE)

    string(FIND "${TST_SRC_NAME}" "SIMD" SIMD_IDX)
    if(SIMD_IDX EQUAL -1 OR TBS_USE_AVX OR TBS_USE_SSE2)
    	add_executable(${TST_SRC_NAME} ${TST_SRC})

    	if(TBS_USE_ETL AND MSVC)
        	target_link_options(${TST_SRC_NAME} PRIVATE /STACK:32000000)
    	endif()

	target_link_libraries(${TST_SRC_NAME} TBS doctest_with_main)
    endif()
endforeach()
