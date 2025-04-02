#include "client.h"
#include <cstring>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
#include <regex>
#include <sys/socket.h>
#include <unistd.h>

constexpr int BUF_SIZE = 1024;
const std::string FILE_NAME = "requests.txt";

Client::~Client() {
    close(m_sock);
}

bool Client::connectToServer(const char* host, uint16_t port) {
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return false;
    }

    m_server_addr.sin_family = AF_INET;
    m_server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &m_server_addr.sin_addr);

    if (connect(m_sock, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr)) < 0) {
        std::cerr << "Ошибка установки соединения" << std::endl;
        close(m_sock);
        return false;
    }

    return true;
}

bool Client::isSQL(const std::string& data) const {
    try {
        // Регулярное выражение для поиска SQL-команд, без учета регистра
        std::regex sqlRegex(R"(\b(SELECT|INSERT|UPDATE|DELETE)\b)", std::regex_constants::icase);
        return std::regex_search(data, sqlRegex);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error in isSQL: " << e.what() << std::endl;
        return false;
    }
}

std::string Client::makeSQLPacketFromString(const std::string& data) const {
    return std::string("Q" + data + "\0");
}

void Client::run() {
    std::ifstream in(FILE_NAME, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Ошибка открытия файла запросов" << std::endl;
        return;
    }

    std::string str;

    while (std::getline(in, str)) {
        // Убираем символ перевода строки, если он есть
        if (!str.empty() && str.back() == '\n') {
            str.pop_back();
        }

        // Проверяем, не пустой ли запрос
        if (str.empty()) {
            std::cout << "Запрос не может быть пустым.\n";
            continue;
        }

        // Проверяем является ли прочитанная строка языком SQL
        if (!isSQL(str)) {
            std::cout << "Запрос не соответсвует языку SQL.\n";
            continue;
        }

        // Формируем PostgresSQL-пакет
        std::string request = makeSQLPacketFromString(str);

        // Отправляем запрос на сервер
        ssize_t send_bytes = send(m_sock, request.c_str(), request.size(), 0);
        if (send_bytes < 0) {
            std::cerr << "Ошибка отправки запроса" << std::endl;
            break;
        }

        // Чтение ответа от сервера
        char response[BUF_SIZE];
        ssize_t received_bytes = recv(m_sock, response, sizeof(response) - 1, 0);
        if (received_bytes < 0) {
            std::cerr << "Ошибка приема данных" << std::endl;
            break;
        }

        // Завершаем строку нулями
        response[received_bytes] = '\0';

        // Выводим результат
        std::cout << "Ответ сервера:\n" << response << std::endl;
    }

    in.close();
}
