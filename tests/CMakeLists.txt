enable_testing()

include (FetchContent)

FetchContent_Declare(
	doctest
	GIT_REPOSITORY https://github.com/doctest/doctest.git
	GIT_TAG master
)

FetchContent_MakeAvailable(doctest)

add_executable(TBSTest "TBSTests.cpp")

target_link_libraries(TBSTest TBS doctest_with_main)