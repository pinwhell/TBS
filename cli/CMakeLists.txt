set(CMAKE_CXX_STANDARD 17)

find_package(cxxopts QUIET)
if(NOT cxxopts_FOUND)
    include(FetchContent)
    FetchContent_Declare(cxxopts
        GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
        GIT_TAG v3.3.1
    )
    FetchContent_MakeAvailable(cxxopts)
endif()

add_executable(tbs-cli main.cpp TBSCLI.cpp)
target_link_libraries(tbs-cli tbs::tbs cxxopts::cxxopts)
set_target_properties(tbs-cli PROPERTIES OUTPUT_NAME "TBSCLI")
install_target_and_headers(tbs cli)