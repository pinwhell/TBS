set(CMAKE_CXX_STANDARD 17) 

find_package(cxxopts REQUIRED)

add_executable(tbs-cli main.cpp TBSCLI.cpp)
target_link_libraries(tbs-cli tbs::tbs cxxopts::cxxopts)
set_target_properties(tbs-cli PROPERTIES OUTPUT_NAME "TBSCLI")
install_target_and_headers(tbs cli)