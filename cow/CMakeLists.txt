cmake_minimum_required(VERSION 3.10)
project(CowInterpreter)

set(CMAKE_CXX_STANDARD 17)

# Находим Google Test
find_package(GTest REQUIRED)

# Добавляем исполняемый файл
add_executable(cow_tests tests.cpp)

# Линкуем с GTest
target_link_libraries(cow_tests GTest::GTest GTest::Main)

# Основная утилита
add_executable(cow main.cpp)
