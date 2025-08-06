// stats_collector.cpp
#include <iostream>
#include <string>
#include <map>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>

// Глобальная статистика
std::map<std::string, int> logStats;
std::mutex statsMutex;
std::atomic<bool> running(true);

// Функция: обновить статистику по строке лога
void updateStats(const std::string& line) {
    // Ищем уровень: "[Info]", "[Error]" и т.д.
    std::regex levelRegex(R"(\[(Debug|Info|Warning|Error)\])");
    std::smatch match;

    if (std::regex_search(line, match, levelRegex)) {
        std::string level = match[1].str();

        std::lock_guard<std::mutex> lock(statsMutex);
        logStats[level]++;
    }
}

// Функция: вывод статистики раз в 5 секунд
void printStats() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        std::lock_guard<std::mutex> lock(statsMutex);
        std::cout << "\n--- Статистика логов ---\n";
        for (const auto& [level, count] : logStats) {
            std::cout << level << ": " << count << "\n";
        }
        std::cout << "------------------------\n\n";
    }
}

int main() {
    const std::string host = "127.0.0.1";
    const int port = 9999;

    logStats["Info"] = 0;
    logStats["Debug"] = 0;
    logStats["Warning"] = 0;
    logStats["Error"] = 0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Не удалось создать сокет\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    std::cout << "Подключение к " << host << ":" << port << "...\n";
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Не удалось подключиться к серверу. Убедитесь, что сервер запущен и принимает несколько клиентов.\n";
        close(sock);
        return 1;
    }

    std::cout << "Подключено. Начинаем получать логи...\n\n";

    // Запускаем поток для вывода статистики
    std::thread statsThread(printStats);

    // Буфер для чтения
    char buffer[4096];
    std::string leftover;  // на случай, если сообщение разорвано

    while (running) {
        ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            std::cerr << "Соединение с сервером потеряно.\n";
            break;
        }

        buffer[n] = '\0';
        std::string data = leftover + buffer;

        // Разбиваем на строки
        size_t pos = 0;
        std::string line;
        while ((pos = data.find('\n')) != std::string::npos) {
            line = data.substr(0, pos);
            std::cout << line << std::endl;
            if (!line.empty()) {
                updateStats(line);
            }
            data.erase(0, pos + 1);
        }
        leftover = data;  // остаток без \n
    }

    running = false;
    if (statsThread.joinable()) {
        statsThread.join();
    }

    close(sock);
    std::cout << "Сбор статистики завершён.\n";
    return 0;
}