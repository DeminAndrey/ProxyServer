#!/bin/bash

# Настройки
NUM_QUERIES=1000      # Количество запросов
TABLE_NAME="users"
FIELD_COUNT=3        # Количество столбцов в таблице

# Инициализация таблицы
echo "Creating table $TABLE_NAME..."
psql -h localhost -U my_user -d my_database -c "DROP TABLE IF EXISTS $TABLE_NAME;"
psql -h localhost -U my_user -d my_database -c "CREATE TABLE $TABLE_NAME (id SERIAL PRIMARY KEY, name TEXT, email TEXT);"

# Генерация данных
for ((i=1; i<=NUM_QUERIES; i++)); do
    echo "Inserting data into $TABLE_NAME..."
    psql -h localhost -U my_user -d my_database -c "INSERT INTO $TABLE_NAME (name, email) VALUES ('User$RANDOM', 'user$RANDOM@example.com');"
done

# Генерация нагрузки
echo "Starting load test with sysbench..."
sysbench oltp.lua prepare --proxy-host=localhost --proxy-port=5433

# Запуск теста
sysbench oltp.lua run --proxy-host=localhost --proxy-port=5433
