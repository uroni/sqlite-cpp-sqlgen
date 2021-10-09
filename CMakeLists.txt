cmake_minimum_required(VERSION 3.10)

project(sqlite-cpp-sqlgen VERSION 1.0)

configure_file(sqlgen_config.h.in sqlgen_config.h)

add_executable(sqlite-cpp-sqlgen main.cpp sqlgen.cpp DatabaseLogger.cpp Database.cpp DatabaseCursor.cpp stringtools.cpp DatabaseQuery.cpp sqlite/sqlite3.c)

target_include_directories(sqlite-cpp-sqlgen PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

