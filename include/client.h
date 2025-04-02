#pragma once

#include <arpa/inet.h>
#include <string>

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
    // Проверка является ли строка языком SQL
    bool isSQL(const std::string& data) const;
    // Формируем PostgresSQL-пакет: 'Q' + SQL-запрос + '\0'
    std::string makeSQLPacketFromString(const std::string& data) const;

    int m_sock = 0;
    struct sockaddr_in m_server_addr;
};

