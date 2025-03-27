#include "proxy_server.h"

#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

constexpr int BACKLOG_SIZE = 5;
constexpr int BUF_SIZE = 1024;
constexpr int MAX_CONNECTIONS = 1000;
constexpr int PORT = 5433;
const char* LOG_FILE = "queries.log";

namespace {
struct DataBaseConnectionInfo {
    const char* host = "localhost";
    const char* port = "5432";
    const char* user = "my_user";
    const char* password = "my_password";
    const char* dbname = "my_database";
};
}

ProxyServer::~ProxyServer() {
    disconnectFromDatabase();
}

void ProxyServer::stop() {
    disconnectFromDatabase();
}

bool ProxyServer::start() {
    m_pgConn = connectToDatabase();
    if (!m_pgConn) {
        std::cerr << "Не удалось подключиться к базе данных" << std::endl;
        return false;
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("socket");
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("bind");
        close(serverSocket);
        return false;
    }

    if (listen(serverSocket, BACKLOG_SIZE) != 0) {
        perror("listen");
        close(serverSocket);
        return false;
    }

    std::cout << "Proxy server started on port " << PORT << "\n";

    while (true) {
        struct pollfd clients[MAX_CONNECTIONS + 1];
        int conns = 0;
        clients[conns].fd = serverSocket;
        clients[conns].events = POLLIN;
        conns++;

        while (true) {
            int pollResult = poll(clients, conns, -1);
            if (pollResult == -1) {
                perror("poll");
                break;
            } else if (pollResult > 0) {
                for (int i = 0; i < conns; ++i) {
                    if (clients[i].revents & POLLIN) {
                        if (clients[i].fd == serverSocket) {
                            struct sockaddr_storage server_addr;
                            socklen_t addr_size = sizeof(server_addr);
                            int newSocket = accept(serverSocket, (struct sockaddr *)&server_addr, &addr_size);
                            if (newSocket == -1) {
                                perror("accept");
                                continue;
                            }

                            std::cout << "New connection accepted\n";

                            clients[conns].fd = newSocket;
                            clients[conns].events = POLLIN;
                            conns++;

                            if (conns >= MAX_CONNECTIONS) {
                                std::cerr << "Превышен допустимый предел соединений\n";
                                break;
                            }
                        }
                        else {
                            char buffer[BUF_SIZE];
                            ssize_t bytesRead = recv(clients[i].fd, buffer, sizeof(buffer), 0);
                            if (bytesRead <= 0) {
                                if (bytesRead == 0) {
                                    std::cout << "Соединение закрыто клиентом\n";
                                } else {
                                    perror("recv");
                                }
                                close(clients[i].fd);
                                clients[i] = clients[conns - 1];
                                conns--;
                            } else {
                                handleRequest(std::string(buffer, bytesRead), clients[i].fd);
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

void ProxyServer::handleRequest(const std::string& request, int socket) {
    parseAndLogRequest(request);
    if (!socket) {
        std::cerr << "Получен недействительный или нулевой дескриптор сокета" << std::endl;
    }
    sendToDatabase(request, socket);
}

PGconn* ProxyServer::connectToDatabase() const {
    DataBaseConnectionInfo conf;
    char conninfo[256];
    snprintf(conninfo, sizeof(conninfo),
             "host='%s' port='%s' user='%s' password='%s' dbname='%s'",
             conf.host, conf.port, conf.user, conf.password, conf.dbname);
    PGconn* conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Ошибка подключения к базе данных: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return nullptr;
    }

    return conn;
}

void ProxyServer::disconnectFromDatabase() {
    if (m_pgConn) {
        PQfinish(m_pgConn);
        m_pgConn = nullptr;
    }
}

void ProxyServer::parseAndLogRequest(const std::string& request) const {
    static std::ofstream log(LOG_FILE, std::ios::app);
    if (!log.is_open()) {
        std::cerr << "Ошибка открытия файла: " << LOG_FILE << std::endl;
        return;
    }
    log << request << std::endl;
}

void ProxyServer::sendToDatabase(const std::string& request, int socket) {
    if (PQstatus(m_pgConn) != CONNECTION_OK) {
        std::cerr << "Не удалось подключиться к базе данных: " << PQerrorMessage(m_pgConn) << std::endl;
        return;
    }

    PGresult* result = PQexec(m_pgConn, request.c_str());
    if (PQresultStatus(result) != PGRES_TUPLES_OK && PQresultStatus(result) != PGRES_COMMAND_OK) {
        std::string errorMessage = PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY);
        std::cerr << "Ошибка запроса: " << errorMessage << std::endl;
        ssize_t send_bytes = send(socket, errorMessage.c_str(), errorMessage.size(), 0);
        if (send_bytes < 0) {
            std::cerr << "Ошибка отправки данных: "
                      << std::system_category().message(errno) << std::endl;
        }
        PQclear(result);
    }

    int numTuples = PQntuples(result);
    int numFields = PQnfields(result);

    // Проверка на наличие данных
    if (numTuples == 0) {
        send(socket, "No results returned.\n", 22, 0);
        return;
    }

    // Вывод заголовков столбцов
    for (int i = 0; i < numFields; ++i) {
        ssize_t send_bytes = send(socket, PQfname(result, i), strlen(PQfname(result, i)), 0);
            if (send_bytes < 0) {
                std::cerr << "Ошибка отправки данных: "
                          << std::system_category().message(errno) << std::endl;
                break;
            }
            else if (send_bytes == 0) {
                std::cerr << "Соединение закрыто клиентом" << std::endl;
                break;
        }
        if (i < numFields - 1) {
            send(socket, " ", 1, 0);
        }
    }
    send(socket, "\n", 1, 0);

    // Вывод содержимого каждой строки
    for (int i = 0; i < numTuples; ++i) {
        for (int j = 0; j < numFields; ++j) {
            ssize_t send_bytes = send(socket, PQgetvalue(result, i, j), strlen(PQgetvalue(result, i, j)), 0);
            if (send_bytes < 0) {
                std::cerr << "Ошибка отправки данных: "
                          << std::system_category().message(errno) << std::endl;
                break;
            }
            else if (send_bytes == 0) {
                std::cerr << "Соединение закрыто клиентом" << std::endl;
                break;
            }
            if (j < numFields - 1) {
                send(socket, " ", 1, 0);
            }
        }
        send(socket, "\n", 1, 0);
    }

    if (result)
        PQclear(result);
}
