enable_testing()

add_executable(TBSTest "TBSTests.cpp" "TestInclution.cpp")

if(TBS_USE_ETL AND MSVC)
    target_link_options(TBSTest PRIVATE /STACK:32000000)
endif()

target_link_libraries(TBSTest TBS doctest_with_main)