cmake_minimum_required(VERSION 3.28)
project(yellows)

set(CMAKE_CXX_STANDARD 17)
#==========================================================================================
find_package(unofficial-brotli CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(unofficial-curlpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
#==========================================================================================
add_executable(yellows main.cpp
)

#==========================================================================================
target_link_libraries(yellows PRIVATE unofficial::brotli::brotlidec)
target_link_libraries(yellows PRIVATE unofficial::brotli::brotlienc)
target_link_libraries(yellows PRIVATE httplib::httplib)
target_link_libraries(yellows PRIVATE unofficial::curlpp::curlpp)
target_link_libraries(yellows PRIVATE nlohmann_json::nlohmann_json)
#==========================================================================================
