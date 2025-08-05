#include "logger.hpp"

#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
void sleep_ms(int ms) { Sleep(ms); }
#else
#include <chrono>
void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif

// Функция: удаляет пробелы в начале и в конце
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return ""; // строка только из пробелов или пустая
    }
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

// Функция: делает все буквы строчными (в нижнем регистре)
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::atomic<bool> isRunning(true);

/*void backgroundLogger(Logger& logger) {
    int counter = 0;
    while (isRunning) {
        logger.log("Background log entry #" + std::to_string(counter++), LogLevel::Info);
        sleep_ms(5000);  // пишем раз в 5 секунд
    }
}*/

std::pair<std::string, LogLevel> getMessage (std::string& s){
    std::string temp;
    size_t start = 0;
    size_t pos = s.find_first_of('!');
    if (pos == std::string::npos){
        return std::make_pair(s, LogLevel::Info);
    }
    else{
        LogLevel ll;
        temp = s.substr(0, pos);
        temp = trim(toLower(temp));
        if (temp == "info") ll = LogLevel::Info;
        else if (temp == "debug") ll = LogLevel::Debug;
        else if (temp == "error") ll = LogLevel::Error;
        else if (temp == "warning") ll = LogLevel::Warning;
        else {
            ll = LogLevel::Info;
            std::cout << "App: level " << temp << " is not defined" << std::endl;
        }
        //std::cout << pos;
        return std::make_pair(trim(s.substr(pos+1, std::string::npos)), ll);
    }
}

void displayMessage(std::string msg, std::queue<std::string>& target){
    target.push(msg);
}

// --- userInputHandler ---

void userInputHandler(Logger& logger) {
    // Очередь сообщений от пользователя
    std::queue<std::pair<std::string, LogLevel>> messageQueue;
    std::mutex queueMutex;
    std::condition_variable hasData;
    std::queue<std::string> to_display;

    // Лямбда: поток ввода
    auto inputThread = [&]() {
        std::string input;
        std::pair<std::string, LogLevel> output;
        while (isRunning) {
            while (!to_display.empty())
            {
                std::cout << to_display.front() << std::endl;
                to_display.pop();
            }
            
            std::cout << "> ";
            std::getline(std::cin, input);
            output = getMessage(input);
            if (!input.empty()) {
                std::lock_guard<std::mutex> lock(queueMutex);
                messageQueue.push(output);
                hasData.notify_one();
            }
        }
        hasData.notify_all(); // разбудить логгер, чтобы он завершился
    };

    // Лямбда: поток записи в лог
    auto loggingThread = [&]() {
        while (isRunning) {
            std::unique_lock<std::mutex> lock(queueMutex);
            hasData.wait(lock, [&]() { return !messageQueue.empty() || !isRunning; });

            if (!messageQueue.empty()) {
                std::pair<std::string, LogLevel> msg = messageQueue.front();
                messageQueue.pop();
                lock.unlock(); // освобождаем мьютекс перед вызовом log()
                if (msg.first == "exit") isRunning = false;
                else if (msg.first == "chlevel"){ 
                    logger.setLevel(msg.second);
                    displayMessage("Level changed to " + logger.levelToString(msg.second), to_display);
                }
                else logger.log(msg.first, msg.second);  
                
            }
        }
    };

    // Запускаем оба потока
    std::thread inputT(inputThread);
    std::thread loggingT(loggingThread);

    // Ждём их завершения
    inputT.join();
    loggingT.join();
}

int main() {
    Logger logger("interactive_log.txt", LogLevel::Debug);

    std::thread input(userInputHandler, std::ref(logger));

    input.join();

    std::cout << "App terminated.\n";
    return 0;
}