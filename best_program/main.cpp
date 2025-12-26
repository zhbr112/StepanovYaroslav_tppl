#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <csignal>
#include <poll.h>

// Сетевые библиотеки и работа с байтами
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <endian.h>

// КОНФИГУРАЦИЯ
const std::string SERVER_IP = "95.163.237.76";
const int PORT_1 = 5123;
const int PORT_2 = 5124;
const std::string AUTH_KEY = "isu_pt";
const std::string GET_CMD = "get";
const std::string OUTPUT_FILE = "sensor_data.txt";

// Настройки тайм-аутов
const int SOCKET_TIMEOUT_SEC = 5;      // Тайм-аут на чтение
const int DELAY_AFTER_AUTH_MS = 200;   // Пауза после отправки ключа для стабилизации
const int LOOP_DELAY_MS = 0;

std::atomic<bool> g_running(true);

// Используем выравнивание 1 байт, чтобы структура соответствовала потоку данных
#pragma pack(push, 1)

// Данные с порта 5123 (15 байт)
struct SensorData1 {
    int64_t timestamp_us;
    float temp;
    int16_t pressure;
    uint8_t checksum;
};

// Данные с порта 5124 (21 байт)
struct SensorData2 {
    int64_t timestamp_us;
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t checksum;
};
#pragma pack(pop)

// Потокобезопасная очередь
class AsyncLogQueue {
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool finished_ = false;
public:
    void push(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(msg);
        cv_.notify_one();
    }
    
    bool pop(std::string& msg) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || finished_; });
        
        if (queue_.empty() && finished_) return false;
        
        msg = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;
        cv_.notify_all();
    }
};

AsyncLogQueue g_logQueue;

// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

// Функция для переворота байт float
float network_to_host_float(float net_float) {
    uint32_t temp;
    std::memcpy(&temp, &net_float, sizeof(float));
    temp = be32toh(temp);
    float result;
    std::memcpy(&result, &temp, sizeof(float));
    return result;
}

// Форматирование времени в YYYY-MM-DD HH:MM:SS
std::string format_time(int64_t timestamp_us) {
    time_t seconds = timestamp_us / 1000000;
    std::tm tm_buf{};
    gmtime_r(&seconds, &tm_buf);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buffer);
}

// Подсчет контрольной суммы
uint8_t calculate_checksum(const uint8_t* data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += data[i];
    }
    return sum;
}

// Очистка входного буфера сокета
void flush_socket_buffer(int sockfd) {
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    char dummy[256];
    while (poll(&pfd, 1, 10) > 0 && (pfd.revents & POLLIN)) {
        if (recv(sockfd, dummy, sizeof(dummy), MSG_DONTWAIT) <= 0) break;
    }
}

// Гарантированное чтение N байт
bool read_n_bytes(int sockfd, void* buffer, size_t n) {
    size_t total_read = 0;
    char* ptr = static_cast<char*>(buffer);
    while (total_read < n) {
        ssize_t bytes = recv(sockfd, ptr + total_read, n - total_read, 0);
        if (bytes <= 0) return false;
        total_read += bytes;
    }
    return true;
}

// КЛИЕНТ
class TCPClient {
    std::string ip_;
    int port_;
    int sockfd_ = -1;

public:
    TCPClient(std::string ip, int port) : ip_(std::move(ip)), port_(port) {}

    ~TCPClient() { close_socket(); }

    void close_socket() {
        if (sockfd_ != -1) {
            close(sockfd_);
            sockfd_ = -1;
        }
    }

    // Подключение и авторизация
    bool connect_and_auth() {
        close_socket();
        
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) return false;

        // Установка тайм-аутов
        struct timeval tv;
        tv.tv_sec = SOCKET_TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        struct sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port_);
        if (inet_pton(AF_INET, ip_.c_str(), &serv_addr.sin_addr) <= 0) return false;

        if (connect(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) return false;

        // Отправка ключа авторизации
        if (send(sockfd_, AUTH_KEY.c_str(), AUTH_KEY.length(), 0) != (ssize_t)AUTH_KEY.length()) {
            return false;
        }

        // Даем серверу время на обработку ключа
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_AFTER_AUTH_MS));
        
        flush_socket_buffer(sockfd_);

        return true;
    }

    // Обработчик для Порта 5123
    void run_worker_1() {
        std::vector<uint8_t> buffer(sizeof(SensorData1));
        
        while (g_running) {
            if (!connect_and_auth()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            while (g_running) {
                // 1. Отправляем команду
                if (send(sockfd_, GET_CMD.c_str(), GET_CMD.length(), 0) != (ssize_t)GET_CMD.length()) break;
                
                // 2. Читаем структуру целиком
                if (!read_n_bytes(sockfd_, buffer.data(), sizeof(SensorData1))) break;

                SensorData1* pkt = reinterpret_cast<SensorData1*>(buffer.data());
                
                // 3. Проверяем контрольную сумму (на сырых данных!)
                uint8_t calced_crc = calculate_checksum(buffer.data(), sizeof(SensorData1) - 1);
                
                if (calced_crc == pkt->checksum) {
                    // 4. Конвертируем Big Endian -> Little Endian (Host)
                    int64_t ts = be64toh(pkt->timestamp_us);
                    float temp = network_to_host_float(pkt->temp);
                    int16_t pressure = (int16_t)ntohs(pkt->pressure);

                    // 5. Формируем строку
                    std::ostringstream ss;
                    ss << format_time(ts) 
                       << " | Source: " << port_ 
                       << " | Temp: " << std::fixed << std::setprecision(2) << temp 
                       << " | Pressure: " << pressure << "\n";
                    g_logQueue.push(ss.str());
                } else {
                    break; 
                }
                
                if (LOOP_DELAY_MS > 0) std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_DELAY_MS));
            }
            close_socket();
        }
    }

    // Обработчик для Порта 5124
    void run_worker_2() {
        std::vector<uint8_t> buffer(sizeof(SensorData2));
        
        while (g_running) {
            if (!connect_and_auth()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            while (g_running) {
                if (send(sockfd_, GET_CMD.c_str(), GET_CMD.length(), 0) != (ssize_t)GET_CMD.length()) break;

                if (!read_n_bytes(sockfd_, buffer.data(), sizeof(SensorData2))) break;

                SensorData2* pkt = reinterpret_cast<SensorData2*>(buffer.data());
                
                uint8_t calced_crc = calculate_checksum(buffer.data(), sizeof(SensorData2) - 1);

                if (calced_crc == pkt->checksum) {
                    int64_t ts = be64toh(pkt->timestamp_us);
                    int32_t x = (int32_t)ntohl(pkt->x);
                    int32_t y = (int32_t)ntohl(pkt->y);
                    int32_t z = (int32_t)ntohl(pkt->z);

                    std::ostringstream ss;
                    ss << format_time(ts) 
                       << " | Source: " << port_ 
                       << " | X: " << x 
                       << " | Y: " << y 
                       << " | Z: " << z << "\n";
                    g_logQueue.push(ss.str());
                } else {
                     break; 
                }

                if (LOOP_DELAY_MS > 0) std::this_thread::sleep_for(std::chrono::milliseconds(LOOP_DELAY_MS));
            }
            close_socket();
        }
    }
};

// ПОТОК ЗАПИСИ В ФАЙЛ
void file_writer_thread() {
    std::ofstream outfile(OUTPUT_FILE, std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot open " << OUTPUT_FILE << std::endl;
        return;
    }

    std::string msg;
    while (g_logQueue.pop(msg)) {
        outfile << msg;
        // Принудительный сброс буфера, чтобы данные сразу появлялись в файле
        outfile.flush(); 
    }
}

// Обработка Ctrl+C
void signal_handler(int signum) {
    std::cout << "\nStopping..." << std::endl;
    g_running = false;
    g_logQueue.stop();
}

int main() {
    // Регистрация сигнала завершения
    std::signal(SIGINT, signal_handler);

    std::cout << "Starting data collector" << std::endl;
    std::cout << "Data will be saved to: " << OUTPUT_FILE << std::endl;

    // Запуск писателя
    std::thread writer(file_writer_thread);

    // Запуск клиентов
    TCPClient client1(SERVER_IP, PORT_1);
    TCPClient client2(SERVER_IP, PORT_2);

    std::thread t1(&TCPClient::run_worker_1, &client1);
    std::thread t2(&TCPClient::run_worker_2, &client2);

    // Ожидание завершения
    t1.join();
    t2.join();
    
    // Завершаем запись
    g_logQueue.stop();
    writer.join();

    std::cout << "Done." << std::endl;
    return 0;
}