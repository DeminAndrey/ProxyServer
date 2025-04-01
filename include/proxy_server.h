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
  // Функция для подключения к базе данных PostgreSQL, отключает шифрование SSL
  PGconn* connectToDatabase() const;
  // Функция для отключения от базы данных PostgreSQL
  void disconnectFromDatabase();

  // Проверка является ли строка языком SQL
  bool isSQL(const std::string& data) const;
  // Обработка запроса от клиента к базе данных
  void handleRequest(const std::string& request, int socket);
  // Парсинг и логирование запросов к базе данных
  void parseAndLogRequest(const std::string& request) const;
  // Выполнение запроса к базе данных и обработка ответа
  void execRequestAndSendResponse(const std::string& request, int socket);

  PGconn* m_pgConn = nullptr;
};
