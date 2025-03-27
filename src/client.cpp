#include "client.h"
#include <cstring>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
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

void Client::run() {
    std::ifstream in(FILE_NAME, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Ошибка открытия файла запросов" << std::endl;
        return;
    }

    std::string request;

    while (std::getline(in, request)) {
        // Убираем символ перевода строки, если он есть
        if (!request.empty() && request.back() == '\n') {
            request.pop_back();
        }

        // Проверяем, не пустой ли запрос
        if (request.empty()) {
            std::cout << "Запрос не может быть пустым.\n";
            continue;
        }

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
