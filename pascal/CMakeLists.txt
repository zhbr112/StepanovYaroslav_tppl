cmake_minimum_required(VERSION 3.10)
project(PascalInterpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

# Включаем флаги покрытия
add_compile_options(--coverage)
add_link_options(--coverage)

# Находим Google Test
find_package(GTest REQUIRED)

# Тесты
add_executable(pascal_tests tests.cpp)
target_link_libraries(pascal_tests GTest::GTest GTest::Main)

# Основная утилита
add_executable(pascal_app main.cpp)