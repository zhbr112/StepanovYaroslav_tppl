#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <cstring>
#include <netinet/in.h>
#include <endian.h>

// СТРУКТУРЫ
#pragma pack(push, 1)
struct SensorData1
{
    int64_t timestamp_us;
    float temp;
    int16_t pressure;
    uint8_t checksum;
};
struct SensorData2
{
    int64_t timestamp_us;
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t checksum;
};
#pragma pack(pop)

// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
inline float network_to_host_float(float net_float)
{
    uint32_t temp;
    std::memcpy(&temp, &net_float, sizeof(float));
    temp = be32toh(temp);
    float result;
    std::memcpy(&result, &temp, sizeof(float));
    return result;
}

inline std::string format_time(int64_t timestamp_us)
{
    time_t seconds = timestamp_us / 1000000;
    std::tm tm_buf{};
    gmtime_r(&seconds, &tm_buf);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buffer);
}

inline uint8_t calculate_checksum(const uint8_t *data, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i)
        sum += data[i];
    return sum;
}

// Константы для проверки валидности (Sanity Check)
// 2020-01-01 00:00:00 UTC
const int64_t MIN_VALID_TIMESTAMP = 1577836800000000LL;
// 2030-01-01 00:00:00 UTC
const int64_t MAX_VALID_TIMESTAMP = 1893456000000000LL;

// ЛОГИКА ПАРСИНГА
// Эта функция пытается найти валидный пакет в буфере.
// Если находит - возвращает форматированную строку и удаляет пакет из буфера.
// Если находит мусор - удаляет мусор.
// Возвращает true, если пакет найден.
template <typename T>
bool try_parse_packet(std::vector<uint8_t> &accumulator, int port, std::string &out_msg)
{
    size_t packet_size = sizeof(T);

    while (accumulator.size() >= packet_size)
    {
        uint8_t *ptr = accumulator.data();
        uint8_t calced = calculate_checksum(ptr, packet_size - 1);
        uint8_t received_crc = ptr[packet_size - 1];

        bool is_valid = false;

        // 1. Проверка CRC
        if (calced == received_crc)
        {
            T *pkt = reinterpret_cast<T *>(ptr);
            int64_t ts = be64toh(pkt->timestamp_us);

            // 2. Проверка времени
            if (ts >= MIN_VALID_TIMESTAMP && ts <= MAX_VALID_TIMESTAMP)
            {
                // 3. Проверка значений
                if constexpr (std::is_same_v<T, SensorData1>)
                {
                    float temp = network_to_host_float(pkt->temp);
                    int16_t pressure = (int16_t)ntohs(pkt->pressure);
                    // Температура от -100 до +100, Давление > 0
                    if (temp > -273.0f && temp < 200.0f && pressure > 0)
                    {

                        std::ostringstream ss;
                        ss << format_time(ts) << " | Source: " << port
                           << " | Temp: " << std::fixed << std::setprecision(2) << temp
                           << " | Pressure: " << pressure << "\n";
                        out_msg = ss.str();
                        is_valid = true;
                    }
                }
                else
                {
                    int32_t x = (int32_t)ntohl(pkt->x);
                    int32_t y = (int32_t)ntohl(pkt->y);
                    int32_t z = (int32_t)ntohl(pkt->z);

                    std::ostringstream ss;
                    ss << format_time(ts) << " | Source: " << port
                       << " | X: " << x << " | Y: " << y << " | Z: " << z << "\n";
                    out_msg = ss.str();
                    is_valid = true;
                }
            }
        }

        if (is_valid)
        {
            // Пакет настоящий - удаляем и выходим
            accumulator.erase(accumulator.begin(), accumulator.begin() + packet_size);
            return true;
        }
        else
        {
            // CRC не совпал или данные мусор.
            // Сдвигаем окно на 1 байт и ищем дальше.
            accumulator.erase(accumulator.begin());
        }
    }
    return false;
}

// ОЧЕРЕДЬ
class AsyncLogQueue
{
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool finished_ = false;

public:
    void push(const std::string &msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(msg);
        cv_.notify_one();
    }
    
    bool pop(std::string &msg)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]
                 { return !queue_.empty() || finished_; });
        if (queue_.empty() && finished_)
            return false;
        msg = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;
        cv_.notify_all();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
};