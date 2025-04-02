#pragma once

#include <postgresql/libpq-fe.h>
#include <string>

/**
 * @brief Класс прокси-сервера для СУБД Postgresql с возможностью
 *  логирования всех SQL запросов проходящих через него
 */
class ProxyServer {
public:
  ProxyServer() = default;
  ~ProxyServer();

  /**
  * @brief start запуск сервера
  * @return успешно/не успешно
  */
  bool start();

  /**
  * @brief stop останов сервера
  */
  void stop();

private:
  // Функция для подключения к базе данных PostgreSQL с опцией отключения шифрования SSL
  PGconn* connectToDatabase() const;
  // Функция для отключения от базы данных PostgreSQL
  void disconnectFromDatabase();

  // Обработка запроса от клиента к базе данных
  void handleRequest(const std::string& request, int socket);
  // Логирование запроса к базе данных в лог-файл
  void logOutQuery(const std::string& request) const;
  // Выполнение запроса к базе данных и обработка ответа
  void execQueryAndSendResponse(const std::string& request, int socket);

  PGconn* m_pgConn = nullptr;
};
