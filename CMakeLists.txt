cmake_minimum_required(VERSION 3.10)

project(proxy_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(server
  src/main.cpp
  src/proxy_server.cpp)

find_package(PostgreSQL REQUIRED)

target_include_directories(server
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
  PUBLIC ${PostgreSQL_INCLUDE_DIR}
)

target_link_libraries(server
    PRIVATE PostgreSQL::PostgreSQL)

add_executable(client
  src/session.cpp
  src/client.cpp)

target_include_directories(client
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
  PUBLIC ${PostgreSQL_INCLUDE_DIR}
)

target_link_libraries(client
    PRIVATE PostgreSQL::PostgreSQL)

install(TARGETS client server DESTINATION bin)
