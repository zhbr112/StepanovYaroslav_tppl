cmake_minimum_required(VERSION 3.10)
project(NetworkDataCollector)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

find_package(GTest REQUIRED)

add_executable(collector_tests tests.cpp)
target_link_libraries(collector_tests GTest::GTest GTest::Main Threads::Threads)

add_executable(data_collector main.cpp)
target_link_libraries(data_collector Threads::Threads)