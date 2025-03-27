#include "client.h"

#include <iostream>

int main() {
    Client client;
    bool success = client.connectToServer("127.0.0.1", 5433);
    if (!success) {
        std::cerr << "Не удалось подключиться к серверу" << std::endl;
        return EXIT_FAILURE;
    }
    client.run();

    return EXIT_SUCCESS;
}
