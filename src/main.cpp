#include "proxy_server.h"

#include <iostream>

int main() {
    ProxyServer proxyServer;

    // Запуск прокси-сервера
    if (!proxyServer.start()) {
        std::cerr << "Ошибка запуска прокси-сервера" << std::endl;
        return EXIT_FAILURE;
    }

    // Ждем завершения работы
    std::cout << "Прокси-сервер запущен. Нажмите Enter для завершения." << std::endl;
    std::getchar();

    return EXIT_SUCCESS;
}
