cmake_minimum_required(VERSION 3.10)
project(NetworkDataCollector)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(Threads REQUIRED)

add_executable(data_collector main.cpp)

target_link_libraries(data_collector PRIVATE Threads::Threads)