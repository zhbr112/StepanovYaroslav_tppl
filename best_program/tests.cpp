#include <gtest/gtest.h>
#include "collector.hpp"
#include <vector>
#include <cstring>

// --- 1. Тесты утилит ---
TEST(UtilsTest, ChecksumCalculation)
{
    std::vector<uint8_t> data = {1, 2, 3};
    EXPECT_EQ(calculate_checksum(data.data(), data.size()), 6);

    std::vector<uint8_t> overflow = {250, 10};
    EXPECT_EQ(calculate_checksum(overflow.data(), overflow.size()), 4);
    EXPECT_EQ(calculate_checksum(nullptr, 0), 0);
}

TEST(UtilsTest, FloatConversion)
{
    float original = 123.456f;
    uint32_t net_representation;
    std::memcpy(&net_representation, &original, 4);
    net_representation = htobe32(net_representation);

    float net_float_val;
    std::memcpy(&net_float_val, &net_representation, 4);

    float result = network_to_host_float(net_float_val);
    EXPECT_FLOAT_EQ(result, original);
}

TEST(UtilsTest, TimeFormatting)
{
    int64_t timestamp_us = 1672531200LL * 1000000LL;
    EXPECT_EQ(format_time(timestamp_us), "2023-01-01 00:00:00");
}

// --- 2. Тесты Парсера ---
const int64_t TEST_TIMESTAMP = 1672531200000000LL;

TEST(ParserTest, ParseSensorData1_Valid)
{
    std::vector<uint8_t> buffer;
    SensorData1 pkt;
    pkt.timestamp_us = htobe64(TEST_TIMESTAMP);

    float temp_val = 25.5f;
    uint32_t temp_raw;
    std::memcpy(&temp_raw, &temp_val, 4);
    temp_raw = htobe32(temp_raw);
    std::memcpy(&pkt.temp, &temp_raw, 4);
    pkt.pressure = htons(750);
    pkt.checksum = calculate_checksum(reinterpret_cast<uint8_t *>(&pkt), sizeof(SensorData1) - 1);

    uint8_t *raw = reinterpret_cast<uint8_t *>(&pkt);
    buffer.insert(buffer.end(), raw, raw + sizeof(SensorData1));

    std::string output;
    bool success = try_parse_packet<SensorData1>(buffer, 5123, output);

    EXPECT_TRUE(success);
    EXPECT_TRUE(output.find("Temp: 25.50") != std::string::npos);
    EXPECT_TRUE(buffer.empty());
}

TEST(ParserTest, ParseSensorData2_Valid)
{
    std::vector<uint8_t> buffer;
    SensorData2 pkt;
    pkt.timestamp_us = htobe64(TEST_TIMESTAMP);

    pkt.x = htonl(10);
    pkt.y = htonl(-20);
    pkt.z = htonl(30);
    pkt.checksum = calculate_checksum(reinterpret_cast<uint8_t *>(&pkt), sizeof(SensorData2) - 1);

    uint8_t *raw = reinterpret_cast<uint8_t *>(&pkt);
    buffer.insert(buffer.end(), raw, raw + sizeof(SensorData2));

    std::string output;
    bool success = try_parse_packet<SensorData2>(buffer, 5124, output);

    EXPECT_TRUE(success);
    EXPECT_TRUE(output.find("Y: -20") != std::string::npos);
    EXPECT_TRUE(buffer.empty());
}

TEST(ParserTest, InsufficientData)
{
    std::vector<uint8_t> buffer = {0x01, 0x02};
    std::string output;
    bool success = try_parse_packet<SensorData1>(buffer, 5123, output);
    EXPECT_FALSE(success);
    EXPECT_EQ(buffer.size(), 2);
}

TEST(ParserTest, GarbageHandling_AutoRecover)
{
    std::vector<uint8_t> buffer;
    buffer.push_back(0xFF);

    SensorData1 pkt;
    pkt.timestamp_us = htobe64(TEST_TIMESTAMP);
    pkt.temp = 10;
    pkt.pressure = 10;
    pkt.checksum = calculate_checksum(reinterpret_cast<uint8_t *>(&pkt), sizeof(SensorData1) - 1);

    uint8_t *raw = reinterpret_cast<uint8_t *>(&pkt);
    buffer.insert(buffer.end(), raw, raw + sizeof(SensorData1));

    std::string output;
    bool success = try_parse_packet<SensorData1>(buffer, 5123, output);

    EXPECT_TRUE(success);
    EXPECT_TRUE(buffer.empty());
}

// Тест проверки на мусорные значения
TEST(ParserTest, RejectInsaneValues_WithValidCRC)
{
    std::vector<uint8_t> buffer;
    SensorData1 pkt;

    // 1. Ставим невозможный год
    pkt.timestamp_us = htobe64(99999999999999999LL);

    // Делаем валидную CRC для этого мусора
    pkt.temp = 10;
    pkt.pressure = 10;
    pkt.checksum = calculate_checksum(reinterpret_cast<uint8_t *>(&pkt), sizeof(SensorData1) - 1);

    uint8_t *raw = reinterpret_cast<uint8_t *>(&pkt);
    buffer.insert(buffer.end(), raw, raw + sizeof(SensorData1));

    std::string output;

    // Попытка парсинга
    bool success = try_parse_packet<SensorData1>(buffer, 5123, output);

    // Ожидаем FALSE, так как год невалидный
    EXPECT_FALSE(success);

    // Буфер должен уменьшиться на 1 байт (сдвиг окна), а не быть удаленным целиком
    EXPECT_EQ(buffer.size(), sizeof(SensorData1) - 1);
}

// --- 3. Тесты Очереди ---
TEST(QueueTest, PushPop)
{
    AsyncLogQueue q;
    q.push("msg1");
    std::string val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, "msg1");
}

TEST(QueueTest, StopMechanism)
{
    AsyncLogQueue q;
    q.push("last_msg");
    q.stop();
    std::string val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, "last_msg");
    EXPECT_FALSE(q.pop(val));
}