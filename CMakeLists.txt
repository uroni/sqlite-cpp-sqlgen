cmake_minimum_required(VERSION 3.10)

project(sqlite-cpp-sqlgen VERSION 0.2)

include(GNUInstallDirs)

configure_file(sqlgen_config.h.in sqlgen_config.h)

add_executable(sqlite-cpp-sqlgen
    main.cpp
    sqlgen.cpp
    DatabaseLogger.cpp
    Database.cpp
    DatabaseCursor.cpp
    stringtools.cpp
    DatabaseQuery.cpp
    sqlite/sqlite3.c
    test.cpp
    sample/SampleGen.cpp)

target_include_directories(sqlite-cpp-sqlgen PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )

add_library(SqliteCppGen Database.cpp
                         DatabaseCursor.cpp
                         DatabaseLogger.cpp
                         DatabaseQuery.cpp
                         stringtools.cpp
                         sqlite/sqlite3.c)

target_include_directories (SqliteCppGen PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

install(TARGETS sqlite-cpp-sqlgen
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(TARGETS SqliteCppGen
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "${PROJECT_BINARY_DIR}/sqlgen_config.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sqlite-cpp-sqlgen)

install(FILES Database.h DatabaseCursor.h DatabaseLogger.h DatabaseQuery.h sqlite/sqlite3.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sqlite-cpp-sqlgen)

install(FILES "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION ${CMAKE_INSTALL_DATADIR}/sqlite-cpp-sqlgen RENAME "copyright")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sqlite-cpp-sqlgen-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite-cpp-sqlgen-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/sqlite-cpp-sqlgen-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/sqlite-cpp-sqlgen/cmake
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/sqlite-cpp-sqlgen-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/sqlite-cpp-sqlgen-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/sqlite-cpp-sqlgen/cmake
)
