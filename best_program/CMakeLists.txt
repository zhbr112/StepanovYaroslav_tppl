cmake_minimum_required(VERSION 3.10)
project(NetworkDataCollector)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(--coverage)
add_link_options(--coverage)

find_package(Threads REQUIRED)

find_package(GTest REQUIRED)

add_executable(collector_tests tests.cpp)
target_link_libraries(collector_tests GTest::GTest GTest::Main Threads::Threads)

add_executable(data_collector main.cpp)
target_link_libraries(data_collector Threads::Threads)