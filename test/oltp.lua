-- Подключение к PostgreSQL
local pg = require "pg"

function prepare()
    -- Устанавливаем соединение с PostgreSQL
    local conn = pg.connect("host=localhost user=my_user password=my_password dbname=my_database")
    if not conn then
        error("Ошибка подключения к базе данных")
    end

    -- Создаем таблицу
    local create_table_query = [[
        DROP TABLE IF EXISTS test;
        CREATE TABLE test (
            id SERIAL PRIMARY KEY,
            value VARCHAR(255)
        );
    ]]
    local res, err = conn.query(create_table_query)
    if not res then
        error("Ошибка создания таблицы: "..err)
    end

    -- Вставляем начальные данные
    local insert_data_query = "INSERT INTO test (value) VALUES ($1)"
    local values = {"value"}
    local res, err = conn.query(insert_data_query, values)
    if not res then
        error("Ошибка вставки данных: "..err)
    end

    -- Закрываем соединение
    conn:close()
end

function bench_run(thread_count)
    local conn = pg.connect("host=localhost user=my_user password=my_password dbname=my_database")
    if not conn then
        error("Ошибка подключения к базе данных")
    end

    -- Рабочая нить
    local function worker()
        local stmt = conn.prepare("INSERT INTO test (value) VALUES ($1)")
        for i = 1, 10000 do
            local res, err = stmt:execute(i)
            if not res then
                error("Ошибка выполнения запроса: "..err)
            end
        end
        stmt:finish()
    end

    -- Запуск рабочих нитей
    local threads = {}
    for i = 1, thread_count do
        threads[i] = assert(pcall(spawn(worker)))
    end

    -- Ожидание завершения всех нитей
    for _, th in ipairs(threads) do
        assert(thwait(th))
    end

    -- Закрываем соединение
    conn:close()
end

function cleanup()
    -- Подключаемся к базе данных
    local conn = pg.connect("host=localhost user=my_user password=my_password dbname=my_database")
    if not conn then
        error("Ошибка подключения к базе данных")
    end

    -- Удаляем таблицу
    local drop_table_query = "DROP TABLE IF EXISTS test"
    local res, err = conn.query(drop_table_query)
    if not res then
        error("Ошибка удаления таблицы: "..err)
    end

    -- Закрываем соединение
    conn:close()
end