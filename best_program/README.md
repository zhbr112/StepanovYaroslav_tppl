# Network Data Collector (best_program)

Высокопроизводительная утилита на C++ для непрерывного сбора данных с сетевых датчиков. Программа подключается к двум TCP-портам, парсит бинарные данные и сохраняет их в читаемом формате.


## Требования
*   Linux
*   C++17 компилятор (GCC/Clang)
*   CMake 3.10 или выше

## Сборка проекта

```bash
mkdir build
cd build
cmake ..
make
```

## Запуск проекта
```bash
./data_collector
```
Данные сохраняются в файл sensor_data.txt

## Запуск тестов
```bash
./collector_tests
```

## Проверка покртытия
```bash
gcovr -r .. . --exclude '.*main\.cpp'
```

```bash
cmake .. && make
./collector_tests
lcov --capture --directory . --output-file coverage_all.info --ignore-errors inconsistent,mismatch 
lcov --extract coverage_all.info '*collector.hpp' --output-file coverage.info --ignore-errors inconsistent,mismatch
```
