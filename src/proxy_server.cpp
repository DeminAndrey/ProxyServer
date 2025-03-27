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

constexpr int MAX_CONNECTIONS = 1000;
constexpr int BUF_SIZE = 1024;
constexpr int PORT = 5433;
const char* LOG_FILE_PATH = "queries.log";

namespace {
struct DataBaseConnectionConfig {
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

    if (listen(serverSocket, 5) != 0) {
        perror("listen");
        close(serverSocket);
        return false;
    }

    std::cout << "Proxy server started on port " << PORT << "\n";

    while (true) {
        struct pollfd fds[MAX_CONNECTIONS + 1];
        int nfds = 0;
        fds[nfds].fd = serverSocket;
        fds[nfds].events = POLLIN;
        nfds++;

        for (;;) {
            int pollResult = poll(fds, nfds, -1);
            if (pollResult == -1) {
                perror("poll");
                break;
            } else if (pollResult > 0) {
                for (int i = 0; i < nfds; ++i) {
                    if (fds[i].revents & POLLIN) {
                        if (fds[i].fd == serverSocket) {
                            struct sockaddr_storage their_addr;
                            socklen_t addr_size = sizeof(their_addr);
                            int newSocket = accept(serverSocket, (struct sockaddr *)&their_addr, &addr_size);
                            if (newSocket == -1) {
                                perror("accept");
                                continue;
                            }

                            std::cout << "New connection accepted\n";

                            fds[nfds].fd = newSocket;
                            fds[nfds].events = POLLIN;
                            nfds++;

                            if (nfds >= MAX_CONNECTIONS) {
                                std::cerr << "Too many connections\n";
                                break;
                            }
                        }
                        else {
                            char buffer[BUF_SIZE];
                            ssize_t bytesRead = recv(fds[i].fd, buffer, sizeof(buffer), 0);
                            if (bytesRead <= 0) {
                                if (bytesRead == 0) {
                                    std::cout << "Connection closed by client\n";
                                } else {
                                    perror("recv");
                                }
                                close(fds[i].fd);
                                fds[i] = fds[nfds-1];
                                nfds--;
                            } else {
                                std::cout << fds[nfds].fd << std::endl;
                                handleRequest(std::string(buffer, bytesRead), fds[nfds].fd);
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
    DataBaseConnectionConfig conf;
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
    static std::ofstream log(LOG_FILE_PATH, std::ios::app);
    if (!log.is_open()) {
        std::cerr << "Failed to open log file\n";
        return;
    }
    log << request << std::endl;
}

void ProxyServer::sendToDatabase(const std::string& request, int socket) {
    if (PQstatus(m_pgConn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(m_pgConn) << std::endl;
        return;
    }

    PGresult* result = PQexec(m_pgConn, request.c_str());
    if (PQresultStatus(result) != PGRES_TUPLES_OK && PQresultStatus(result) != PGRES_COMMAND_OK) {
        std::string errorMessage = PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY);
        send(socket, errorMessage.c_str(), errorMessage.size(), 0);
        std::cerr << "Query failed: " << errorMessage << std::endl;
        PQclear(result);
    }

    int numTuples = PQntuples(result);
    int numFields = PQnfields(result);

    if (PQntuples(result) > 0) {
        for (int i = 0; i < numTuples; ++i) {
            for (int j = 0; numFields; ++j) {
                send(socket, PQgetvalue(result, i, j), strlen(PQgetvalue(result, i, j)), 0);
                send(socket, " ", 1, 0);
            }
            std::cout << std::endl;
        }
    }
    else {
        send(socket, "No result!\n", 16, 0);
    }

    if (result)
        PQclear(result);
}
