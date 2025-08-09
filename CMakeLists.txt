cmake_minimum_required(VERSION 3.10)
project(LogProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опция для выбора типа сборки библиотеки
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
# Дополнительная опция для явного управления экспортом символов
option(LOGGER_EXPORT_SYMBOLS "Export symbols for shared library" ${BUILD_SHARED_LIBS})

# Настройка экспорта символов для shared библиотеки
if(LOGGER_EXPORT_SYMBOLS)
    add_definitions(-DLOGGER_DLL_EXPORT)
endif()

# Собираем библиотеку - будет собрана как STATIC или SHARED в зависимости от BUILD_SHARED_LIBS
add_subdirectory(logger)

# Альтернативная сборка - и static и shared одновременно
option(BUILD_BOTH_LIB_TYPES "Build both static and shared libraries" OFF)
if(BUILD_BOTH_LIB_TYPES)
    # Сохраняем текущее значение
    set(BUILD_SHARED_LIBS_BAK ${BUILD_SHARED_LIBS})
    
    # Сначала собираем static
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(logger)
    set_target_properties(logger PROPERTIES OUTPUT_NAME "logger_static")
    
    # Затем собираем shared
    set(BUILD_SHARED_LIBS ON)
    add_subdirectory(logger)
    set_target_properties(logger PROPERTIES OUTPUT_NAME "logger_shared")
    
    # Восстанавливаем значение
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_BAK})
endif()

# Приложения
add_subdirectory(app)
add_subdirectory(stats)
add_subdirectory(tests)
add_subdirectory(server)