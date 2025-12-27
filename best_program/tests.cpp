#include <gtest/gtest.h>
#include "collector.hpp"
#include <vector>
#include <cmath>

// 1. Тест расчета контрольной суммы
TEST(UtilsTest, ChecksumCalculation) {
    std::vector<uint8_t> data = {10, 20, 30}; // Sum = 60
    EXPECT_EQ(calculate_checksum(data.data(), data.size()), 60);

    std::vector<uint8_t> overflow_data = {200, 100}; // Sum = 300 -> 44 (mod 256)
    EXPECT_EQ(calculate_checksum(overflow_data.data(), overflow_data.size()), 44);
}

// 2. Тест конвертации Float (Big Endian -> Host)
TEST(UtilsTest, FloatConversion) {
    float original = 123.456f;
    uint32_t net_representation;
    std::memcpy(&net_representation, &original, 4);
    net_representation = htobe32(net_representation); // Имитируем данные из сети
    
    float net_float_val;
    std::memcpy(&net_float_val, &net_representation, 4);

    float result = network_to_host_float(net_float_val);
    EXPECT_FLOAT_EQ(result, original);
}

// 3. Тест форматирования времени
TEST(UtilsTest, TimeFormatting) {
    // 1672531200 секунд = 2023-01-01 00:00:00 UTC
    int64_t timestamp_us = 1672531200LL * 1000000LL;
    EXPECT_EQ(format_time(timestamp_us), "2023-01-01 00:00:00");
}

// 4. Тест парсера (Скользящее окно) для SensorData1
TEST(ParserTest, ParseSensorData1_Valid) {
    std::vector<uint8_t> buffer;
    
    // Создаем фейковый пакет
    SensorData1 pkt;
    pkt.timestamp_us = htobe64(1000000); // 1970-01-01 00:00:01
    float temp_val = 25.5f;
    uint32_t temp_raw;
    std::memcpy(&temp_raw, &temp_val, 4);
    temp_raw = htobe32(temp_raw);
    std::memcpy(&pkt.temp, &temp_raw, 4);
    pkt.pressure = htons(750);
    
    // Считаем CRC
    pkt.checksum = calculate_checksum(reinterpret_cast<uint8_t*>(&pkt), sizeof(SensorData1) - 1);

    // Записываем в буфер байты структуры
    uint8_t* raw = reinterpret_cast<uint8_t*>(&pkt);
    buffer.insert(buffer.end(), raw, raw + sizeof(SensorData1));

    std::string output;
    bool success = try_parse_packet<SensorData1>(buffer, 5123, output);

    EXPECT_TRUE(success);
    EXPECT_TRUE(output.find("Temp: 25.50") != std::string::npos);
    EXPECT_TRUE(output.find("Pressure: 750") != std::string::npos);
    EXPECT_TRUE(buffer.empty()); // Буфер должен очиститься
}

// 5. Тест парсера с мусором (Sliding Window logic)
TEST(ParserTest, ParseWithGarbage) {
    std::vector<uint8_t> buffer;
    
    // Добавляем мусор в начало
    buffer.push_back(0xAA);
    buffer.push_back(0xBB);
    buffer.push_back(0xCC);

    // Создаем валидный пакет
    SensorData2 pkt;
    pkt.timestamp_us = 0;
    pkt.x = htonl(10);
    pkt.y = htonl(20);
    pkt.z = htonl(30);
    pkt.checksum = calculate_checksum(reinterpret_cast<uint8_t*>(&pkt), sizeof(SensorData2) - 1);

    uint8_t* raw = reinterpret_cast<uint8_t*>(&pkt);
    buffer.insert(buffer.end(), raw, raw + sizeof(SensorData2));

    std::string output;
    
    // Первый вызов не должен найти пакет (из-за мусора), но должен удалить мусор
    // В реальности try_parse_packet вызывается в цикле while.
    // Эмулируем цикл:
    
    bool found = false;
    // Ожидаем, что парсер "съест" 3 байта мусора, а на 4-й раз найдет пакет
    for(int i=0; i<10; ++i) {
        if (try_parse_packet<SensorData2>(buffer, 5124, output)) {
            found = true;
            break;
        }
        if(buffer.empty()) break;
    }

    EXPECT_TRUE(found);
    EXPECT_TRUE(output.find("X: 10") != std::string::npos);
}

// 6. Тест очереди
TEST(QueueTest, PushPop) {
    AsyncLogQueue q;
    q.push("test message");
    
    std::string msg;
    bool res = q.pop(msg);
    EXPECT_TRUE(res);
    EXPECT_EQ(msg, "test message");
    EXPECT_TRUE(q.empty());
}