#pragma once

#include <arpa/inet.h>

/**
 * @brief Класс клиента для отправки запросов в базу данных
 */
class Client {
public:
    Client() = default;
    ~Client();

    /**
     * @brief connectToServer подключение к серверу
     * @param host хост
     * @param port порт
     * @return успешно/не успешно
     */
    bool connectToServer(const char* host, uint16_t port);

    /**
     * @brief run запуск клиента
     */
    void run();

private:
    int m_sock = 0;
    struct sockaddr_in m_server_addr;
};

