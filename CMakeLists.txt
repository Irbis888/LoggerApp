cmake_minimum_required(VERSION 3.10)
project(LogProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции для сборки логгера в виде shared/static
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Собираем библиотеку
add_subdirectory(logger)

# Приложения
add_subdirectory(app)
add_subdirectory(stats)
add_subdirectory(tests)
