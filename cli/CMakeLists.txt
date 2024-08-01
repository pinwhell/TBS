set(CMAKE_CXX_STANDARD 17) 

find_package(cxxopts REQUIRED)

add_executable(TBSCLI main.cpp TBSCLI.cpp)
target_link_libraries(TBSCLI TBS cxxopts::cxxopts)

install(TARGETS TBSCLI
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)