#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <csignal>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "collector.hpp" // Подключаем логику

// КОНФИГУРАЦИЯ
const std::string SERVER_IP = "95.163.237.76";
const int PORT_1 = 5123;
const int PORT_2 = 5124;
const std::string AUTH_KEY = "isu_pt";
const std::string GET_CMD = "get";
const std::string OUTPUT_FILE = "sensor_data.txt";
const int SOCKET_TIMEOUT_SEC = 5;
const int DELAY_AFTER_AUTH_MS = 300;

std::atomic<bool> g_running(true);
AsyncLogQueue g_logQueue; // Экземпляр очереди

// СЕТЕВОЙ КЛИЕНТ
class TCPClient
{
    std::string ip_;
    int port_;
    int sockfd_ = -1;

public:
    TCPClient(std::string ip, int port) : ip_(std::move(ip)), port_(port) {}
    ~TCPClient() { close_socket(); }

    void close_socket()
    {
        if (sockfd_ != -1)
        {
            close(sockfd_);
            sockfd_ = -1;
        }
    }

    bool connect_and_auth()
    {
        close_socket();
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0)
            return false;

        int flag = 1;
        setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

        struct timeval tv;
        tv.tv_sec = SOCKET_TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
        setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof tv);

        struct sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port_);
        if (inet_pton(AF_INET, ip_.c_str(), &serv_addr.sin_addr) <= 0)
            return false;

        if (connect(sockfd_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            return false;
        if (send(sockfd_, AUTH_KEY.c_str(), AUTH_KEY.length(), 0) != (ssize_t)AUTH_KEY.length())
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_AFTER_AUTH_MS));
        char trash[256];
        while (recv(sockfd_, trash, sizeof(trash), MSG_DONTWAIT) > 0)
            ;
        return true;
    }

    template <typename T>
    void run_loop()
    {
        std::vector<uint8_t> accumulator;
        accumulator.reserve(1024);
        char temp_buf[256];

        while (g_running)
        {
            if (!connect_and_auth())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            std::cout << "[Port " << port_ << "] Connected." << std::endl;
            accumulator.clear();

            while (g_running)
            {
                if (send(sockfd_, GET_CMD.c_str(), GET_CMD.length(), 0) != (ssize_t)GET_CMD.length())
                    break;

                // Чтение данных
                ssize_t n = recv(sockfd_, temp_buf, sizeof(temp_buf), 0);
                if (n <= 0)
                    break; // Разрыв или ошибка

                accumulator.insert(accumulator.end(), temp_buf, temp_buf + n);

                // Попытка парсинга с помощью функции из collector.hpp
                std::string msg;
                while (try_parse_packet<T>(accumulator, port_, msg))
                {
                    g_logQueue.push(msg);
                }

                if (accumulator.size() > 512)
                    accumulator.clear(); // Защита от переполнения
            }
            close_socket();
        }
    }
};

void file_writer_thread()
{
    std::ofstream outfile(OUTPUT_FILE, std::ios::app);
    if (!outfile.is_open())
        return;
    std::string msg;
    while (g_logQueue.pop(msg))
    {
        outfile << msg;
        outfile.flush();
    }
}

void signal_handler(int)
{
    g_running = false;
    g_logQueue.stop();
}

int main()
{
    std::signal(SIGINT, signal_handler);
    std::cout << "Starting collector..." << std::endl;

    std::thread writer(file_writer_thread);
    TCPClient client1(SERVER_IP, PORT_1);
    TCPClient client2(SERVER_IP, PORT_2);

    std::thread t1(&TCPClient::run_loop<SensorData1>, &client1);
    std::thread t2(&TCPClient::run_loop<SensorData2>, &client2);

    t1.join();
    t2.join();
    g_logQueue.stop();
    writer.join();
    return 0;
}