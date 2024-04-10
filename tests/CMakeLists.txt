enable_testing()

add_executable(TBSTest "TBSTests.cpp")

target_link_libraries(TBSTest TBS doctest_with_main)