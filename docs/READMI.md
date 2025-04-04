### Описание работы прокси-сервера

Данный прокси-сервер предназначен для работы с СУБД PostgreSQL. Он принимает SQL-запросы от клиентов, логирует их и передает в базу данных. После выполнения запроса сервер отправляет клиенту результат.

#### **Основные функции**
1. **Запуск сервера (`start()`)**
   - Подключается к базе данных PostgreSQL.
   - Создает серверный сокет и привязывает его к порту 5433.
   - Ожидает входящих подключений от клиентов.
   - Принимает новые соединения и добавляет их в пул обработчиков (`poll`).

2. **Обработка клиентских запросов**
   - Принимает запрос от клиента.
   - Если запрос начинается с `Q`, он считается SQL-запросом.
   - Запрос логируется в файл `queries.log`.
   - Выполняется SQL-запрос в базе данных.
   - Результат запроса отправляется клиенту.

3. **Взаимодействие с базой данных**
   - Подключение происходит через `connectToDatabase()`.
   - Запрос выполняется через `execQueryAndSendResponse()`, где обрабатываются ошибки и формируется ответ клиенту.
   - После завершения работы соединение с БД разрывается.

4. **Логирование запросов**
   - SQL-запросы записываются в файл `queries.log`.

5. **Остановка сервера (`stop()`)**
   - Закрывает соединение с базой данных.

#### **Ключевые параметры**
- Порт сервера: `5433`
- Порт базы данных: `5432`
- Максимальное количество соединений: `1000`
- Размер буфера для чтения данных: `1024`
- Файл логирования: `queries.log`

Таким образом, сервер работает как посредник между клиентами и PostgreSQL, обеспечивая логирование запросов и возможность их анализа.
